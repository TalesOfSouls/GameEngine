/**
 * Client side of the network envelope (types: NetClient.h)
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_NETWORK_NET_CLIENT_C
#define COMS_NETWORK_NET_CLIENT_C

#include "../stdlib/Stdlib.h"
// Before anything that includes windows.h (winsock2.h must come first)
#include "Socket.h"
#include "Socket.cpp"
#include "../utils/TimeUtils.h"
#include "../memory/BufferMemory.cpp"
#include "NetConnection.cpp"
#include "packet/NetMessage.cpp"
#include "NetClient.h"

static inline
void net_client_send_raw(const NetClient* const __restrict client, const byte* const __restrict data, int32 size) NO_EXCEPT
{
    ASSERT_TRUE(size > 0);
    send(client->sd, (const char *) data, size, 0);
}

static inline
int32 net_client_recv_raw(const NetClient* const __restrict client, byte* const __restrict data) NO_EXCEPT
{
    return (int32) recv(client->sd, (char *) data, NET_MAX_PACKET_SIZE, 0);
}

/**
 * Initializes the client (once, before the first net_client_open())
 *
 * The client can be opened and closed again any number of times afterwards.
 *
 * The resend buffers limit the reliable packets in flight, size them for the connection
 * (e.g. a request/response client only needs a few).
 *
 * @param NetClient*          client      Client
 * @param NetResendBuffer*    buffers     Resend buffers (owned by the caller)
 * @param int32*              buffer_next Free list of the resend buffers (count elements)
 * @param uint32              count       Amount of resend buffers
 */
inline
void net_client_init(
    NetClient* const __restrict client,
    NetResendBuffer* const __restrict buffers,
    int32* const __restrict buffer_next,
    uint32 count
) NO_EXCEPT
{
    ZERO_PTR(client);
    client->sd = SOCKET_INVALID;

    net_resend_pool_init(&client->pool, buffers, buffer_next, count);

    // Random start, a restarted client doesn't collide with requests the server still remembers
    uint32 request_id = 0;
    secure_random_bytes(&request_id, sizeof(request_id));
    client->next_request_id = request_id | 1;
}

/**
 * Initializes the client with resend buffers from a memory block (once, e.g. from the fixed memory at startup)
 *
 * The memory block needs net_resend_memory_size(count) bytes.
 *
 * @param NetClient*    client  Client
 * @param BufferMemory* memory  Memory of the resend buffers
 * @param uint32        count   Amount of resend buffers (compile time constant of the connection type)
 */
inline
void net_client_init(NetClient* const __restrict client, BufferMemory* const __restrict memory, uint32 count) NO_EXCEPT
{
    NetResendBuffer* const buffers = (NetResendBuffer *) memory_get(
        memory, sizeof(NetResendBuffer) * count, (int32) alignof(NetResendBuffer)
    );
    int32* const buffer_next = (int32 *) memory_get(memory, sizeof(int32) * count, (int32) alignof(int32));

    net_client_init(client, buffers, buffer_next, count);
}

/**
 * Creates the socket and connects (handshake)
 *
 * The client must be initialized (net_client_init())
 *
 * @param NetClient*            client      Client
 * @param const char*           host        Hostname or ip
 * @param uint16                port        Port
 * @param const NetSecurity*    security    Static public key of the server and/or pre shared key
 * @param uint32                timeout_us  Max. duration of the handshake
 * @param NetClientRunningFn    running     Optional abort check (e.g. application shutdown)
 * @param void*                 running_data Data of the abort check
 *
 * @return bool True if connected (otherwise the socket is closed again)
 */
inline
bool net_client_open(
    NetClient* const __restrict client,
    const char* const __restrict host,
    uint16 port,
    const NetSecurity* const __restrict security,
    uint32 timeout_us,
    NetClientRunningFn running = NULL,
    const void* running_data = NULL
) NO_EXCEPT
{
    client->sd = socket_udp_connect(host, port);
    if (client->sd == SOCKET_INVALID) {
        return false;
    }

    socket_receive_timeout_set(client->sd, NET_CLIENT_POLL_US / MILLI_MICRO);

    // A server that isn't running mustn't abort the receive (only Windows: ICMP port unreachable)
    socket_udp_connreset_ignore(client->sd);

    const uint64 start = time_mu();
    net_connection_init(&client->conn, false, start);

    if (!net_client_handshake_begin(&client->conn, security)) {
        net_connection_release(&client->conn, &client->pool);
        socket_close(&client->sd);

        return false;
    }

    byte packet[NET_MAX_PACKET_SIZE];
    uint64 last_hello = 0;
    bool send_hello = true;

    while (true) {
        const uint64 now = time_mu();
        if (now >= start + timeout_us || (running && !running(running_data))) {
            break;
        }

        if (send_hello || now >= last_hello + NET_CLIENT_HELLO_INTERVAL_US) {
            net_client_send_raw(client, packet, net_client_write_hello(&client->conn, packet));
            last_hello = now;
            send_hello = false;
        }

        const int32 received = net_client_recv_raw(client, packet);
        if (received <= 0) {
            continue;
        }

        const int32 result = net_client_handshake_read(&client->conn, packet, received, time_mu());
        if (result == NET_HANDSHAKE_CONNECTED) {
            return true;
        }

        if (result == NET_HANDSHAKE_COOKIE_RECEIVED) {
            send_hello = true;
        }
    }

    net_connection_release(&client->conn, &client->pool);
    socket_close(&client->sd);

    return false;
}

FORCE_INLINE
bool net_client_is_connected(const NetClient* const client) NO_EXCEPT
{
    return client->conn.state == NET_STATE_CONNECTED;
}

/**
 * Sends a payload
 *
 * @return bool False if not connected or too many reliable packets are in flight
 */
inline
bool net_client_send(
    NetClient* const __restrict client,
    uint8 channel,
    const void* const __restrict payload,
    uint16 size
) NO_EXCEPT
{
    byte packet[NET_MAX_PACKET_SIZE];
    const int32 packet_size = net_connection_write(
        &client->conn, &client->pool, packet, channel, payload, size, time_mu()
    );

    net_client_send_raw(client, packet, packet_size);

    return packet_size > 0;
}

/**
 * Re-sends unacknowledged packets and sends outstanding acks
 *
 * @return bool False if the connection is dead
 */
inline
bool net_client_update(NetClient* const client) NO_EXCEPT
{
    const uint64 now = time_mu();

    const int32 resent = net_connection_resend(&client->conn, &client->pool, now, [&](const byte* data, int32 size) {
        net_client_send_raw(client, data, size);
    });

    if (resent < 0) {
        net_connection_release(&client->conn, &client->pool);

        return false;
    }

    byte packet[NET_MAX_PACKET_SIZE];
    int32 size;
    while ((size = net_connection_write_ack(&client->conn, packet, now)) > 0) {
        net_client_send_raw(client, packet, size);
    }

    return true;
}

/**
 * Receives the next payload
 *
 * Handles acks, duplicates and re-sends while waiting.
 *
 * @param NetClient*    client      Client
 * @param byte*         payload     Output payload (NET_MAX_PAYLOAD_SIZE bytes)
 * @param uint8*        channel     Output channel
 * @param uint32        timeout_us  Max. wait time (0 = one receive, blocks max. the socket timeout)
 *
 * @return int32 Payload size, NET_READ_NONE on timeout, NET_READ_DISCONNECT or NET_CLIENT_DEAD
 */
inline
int32 net_client_receive(
    NetClient* const __restrict client,
    byte* const __restrict payload,
    uint8* const __restrict channel,
    uint32 timeout_us
) NO_EXCEPT
{
    const uint64 start = time_mu();
    byte packet[NET_MAX_PACKET_SIZE];

    do {
        if (client->conn.state != NET_STATE_CONNECTED) {
            return NET_READ_DISCONNECT;
        }

        if (!net_client_update(client)) {
            return NET_CLIENT_DEAD;
        }

        const int32 received = net_client_recv_raw(client, packet);
        if (received <= 0) {
            continue;
        }

        const int32 result = net_connection_read(
            &client->conn, &client->pool, packet, received, time_mu(), payload, channel
        );

        if (result == NET_READ_DISCONNECT) {
            net_connection_release(&client->conn, &client->pool);

            return result;
        }

        // Acks right away, the other side stops re-sending
        if (!net_client_update(client)) {
            return NET_CLIENT_DEAD;
        }

        if (result >= 0) {
            return result;
        }
    } while (time_mu() < start + timeout_us);

    return NET_READ_NONE;
}

/**
 * Next request id of the client (part of the request message, see net_message_write())
 */
FORCE_INLINE
uint32 net_client_next_id(NetClient* const client) NO_EXCEPT
{
    // 0 marks a request without id (NetRequestPayload), skipped on the wrap around
    uint32 id = client->next_request_id++;
    if (!id) {
        id = client->next_request_id++;
    }

    return id;
}

/**
 * Sends a request message (reliable) and waits for the response message
 *
 * The caller creates the complete message (request id from net_client_next_id(), then
 * net_message_write() into request.data), the response is matched by the request id.
 *
 * @example
 *      // Packet specific step of the protocol (e.g. <packet>_write()), the network layer only sees bytes
 *      byte packet[NET_MAX_MESSAGE_SIZE];
 *      const uint16 packet_size = auth_ping_request_write(packet, &ping);
 *
 *      NetRequestPayload request;
 *      request.request_id = net_client_next_id(client);
 *      request.size = (uint16) net_message_write(
 *          request.data, NET_MESSAGE_REQUEST, OP, request.request_id, 0, packet, packet_size
 *      );
 *
 *      byte response[NET_MAX_PAYLOAD_SIZE];
 *      const int32 response_size = net_client_request(client, &request, response, 5 * SEC_MICRO);
 *
 *      NetMessageHeader header;
 *      const int32 available = net_message_read(response, response_size, &header);
 *      auth_ping_response_read(response + NET_MESSAGE_HEADER_SIZE, available, &pong);
 *
 * @param NetClient*            client          Client
 * @param NetRequestPayload*    request         Request message (request id + net_message_write())
 * @param byte*                 response        Output response message (NET_MAX_PAYLOAD_SIZE bytes)
 * @param uint32                timeout_us      Max. wait time
 * @param NetClientRunningFn    running         Optional abort check
 * @param void*                 running_data    Data of the abort check
 * @param NetClientMessageFn    on_message      Optional handler of other messages (e.g. notifications)
 * @param void*                 message_data    Data of the handler
 *
 * @return int32 Response message size (>= NET_MESSAGE_HEADER_SIZE), NET_READ_NONE on timeout,
 *               < NET_READ_NONE on failure
 */
inline
int32 net_client_request(
    NetClient* const client,
    const NetRequestPayload* const request,
    byte* const response,
    uint32 timeout_us,
    NetClientRunningFn running = NULL,
    const void* running_data = NULL,
    NetClientMessageFn on_message = NULL,
    void* message_data = NULL
) NO_EXCEPT
{
    // 0 = the caller forgot to set the request id, the response could never be matched
    ASSERT_TRUE(request->request_id);

    if (!request->size || request->size > NET_MAX_PAYLOAD_SIZE) {
        return NET_READ_INVALID;
    }

    if (!net_client_send(client, NET_CHANNEL_RELIABLE, request->data, request->size)) {
        return net_client_is_connected(client) ? NET_READ_INVALID : NET_READ_DISCONNECT;
    }

    const uint64 start = time_mu();

    while (true) {
        const uint64 now = time_mu();
        if (now >= start + timeout_us || (running && !running(running_data))) {
            return NET_READ_NONE;
        }

        uint8 channel;
        const int32 received = net_client_receive(
            client, response, &channel, (uint32) (start + timeout_us - now)
        );

        if (received == NET_READ_NONE) {
            continue;
        }

        if (received < 0) {
            return received;
        }

        NetMessageHeader header;
        const int32 available = net_message_read(response, received, &header);
        if (available < 0) {
            continue;
        }

        if (header.type == NET_MESSAGE_RESPONSE && header.request_id == request->request_id) {
            return received;
        }

        if (on_message) {
            on_message(message_data, &header, response + NET_MESSAGE_HEADER_SIZE, available);
        }
    }
}

/**
 * Closes the connection (the socket stays open, the caller closes it)
 */
inline
void net_client_disconnect(NetClient* const client) NO_EXCEPT
{
    if (client->conn.state != NET_STATE_CONNECTED) {
        net_connection_release(&client->conn, &client->pool);

        return;
    }

    byte packet[NET_MAX_PACKET_SIZE];
    const int32 size = net_connection_write_disconnect(&client->conn, &client->pool, packet, time_mu());
    net_connection_release(&client->conn, &client->pool);

    // Unreliable -> twice, a lost disconnect only means the server times the connection out
    net_client_send_raw(client, packet, size);
    net_client_send_raw(client, packet, size);
}

/**
 * Disconnects and closes the socket
 */
inline
void net_client_close(NetClient* const client) NO_EXCEPT
{
    if (client->sd != SOCKET_INVALID) {
        net_client_disconnect(client);
    }

    socket_close(&client->sd);
}

#endif
