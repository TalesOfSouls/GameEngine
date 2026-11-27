/**
 * Server side of the network envelope (types: NetServer.h)
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_NETWORK_NET_SERVER_C
#define COMS_NETWORK_NET_SERVER_C

#include "../stdlib/Stdlib.h"
// Before anything that includes windows.h (winsock2.h must come first)
#include "Socket.h"
#include "../thread/Spinlock.h"
#include "../hash/Hmac.h"
#include "NetConnection.cpp"
#include "NetServer.h"

/**
 * @param NetServer*            server      Server
 * @param const NetServerMemory* memory     Connection and resend buffers (owned by the caller)
 * @param const NetSecurity*    security    Own static private key and/or pre shared key
 *
 * @return bool False if no key material is configured or no random numbers are available
 */
inline
bool net_server_init(
    NetServer* const __restrict server,
    const NetServerMemory* const __restrict memory,
    const NetSecurity* const __restrict security
) NO_EXCEPT
{
    memset(server, 0, sizeof(*server));

    if (!security || (!security->has_static_key && !security->has_psk)
        || !memory->capacity || memory->capacity > NET_SERVER_MAX_CONNECTIONS
    ) {
        return false;
    }

    if (!secure_random_bytes(server->cookie_secret, sizeof(server->cookie_secret))) {
        return false;
    }

    server->connections = memory->connections;
    server->capacity = memory->capacity;
    server->security = *security;

    if (security->has_static_key) {
        x25519_public_key(server->static_public, security->static_key);
    }

    for (uint32 i = 0; i < server->capacity; ++i) {
        NetConnection* const conn = &server->connections[i];
        memset(conn, 0, sizeof(*conn));
        conn->next_free = i + 1 < server->capacity ? (int32) i + 1 : -1;
        conn->generation = 1;
    }

    server->free_head = 0;

    net_resend_pool_init(&server->pool, memory->resend_buffers, memory->resend_next, memory->resend_count);

    server->pending_timeout_us = NET_PENDING_TIMEOUT_US;
    server->idle_timeout_us = NET_IDLE_TIMEOUT_US;

    spinlock_init(&server->lock);

    return true;
}

FORCE_INLINE
uint32 net_server_connection_id(uint32 slot, uint32 generation) NO_EXCEPT
{
    return ((generation & 0xFFF) << NET_SERVER_SLOT_BITS) | slot;
}

/**
 * Finds a connection (lock must be held)
 */
static inline
NetConnection* net_server_find_locked(NetServer* const server, uint32 connection_id) NO_EXCEPT
{
    const uint32 slot = connection_id & (NET_SERVER_MAX_CONNECTIONS - 1);
    if (slot >= server->capacity) {
        return NULL;
    }

    NetConnection* const conn = &server->connections[slot];
    if (conn->state == NET_STATE_CLOSED || conn->connection_id != connection_id) {
        return NULL;
    }

    return conn;
}

static inline
void net_server_remove_locked(NetServer* const server, NetConnection* const conn) NO_EXCEPT
{
    const uint32 slot = (uint32) (conn - server->connections);

    net_connection_release(conn, &server->pool);
    conn->connection_id = 0;

    // A new generation for the next connection in this slot (never 0 -> id never 0)
    conn->generation = (conn->generation + 1) & 0xFFF;
    if (!conn->generation) {
        conn->generation = 1;
    }

    conn->next_free = server->free_head;
    server->free_head = (int32) slot;

    --server->count;
}

/**
 * Cookie = HMAC(secret, address || time slot || client key)
 */
static inline
void net_server_cookie(
    const NetServer* const __restrict server,
    const sockaddr_in6* const __restrict from,
    uint32 time_slot,
    const byte* const __restrict client_public,
    byte* const __restrict cookie
) NO_EXCEPT
{
    byte slot[4];
    net_write_u32(slot, time_slot);

    HmacSha256Context ctx;
    hmac_sha256_init(&ctx, server->cookie_secret, sizeof(server->cookie_secret));
    hmac_sha256_update(&ctx, &from->sin6_addr, sizeof(from->sin6_addr));
    hmac_sha256_update(&ctx, &from->sin6_port, sizeof(from->sin6_port));
    hmac_sha256_update(&ctx, slot, sizeof(slot));
    hmac_sha256_update(&ctx, client_public, X25519_KEY_SIZE);

    byte digest[HMAC_SHA256_DIGEST_SIZE];
    hmac_sha256_final(&ctx, digest);

    memcpy(cookie, digest, NET_COOKIE_SIZE);
    ct_memzero(digest, sizeof(digest));
}

/**
 * Handles a HELLO
 *
 * @return int32 Size of the reply (COOKIE or WELCOME), 0 = no reply
 */
static inline
int32 net_server_handle_hello(
    NetServer* const __restrict server,
    const byte* const __restrict in,
    const NetHeader* const __restrict header,
    const sockaddr_in6* const __restrict from,
    uint64 now_us,
    byte* const __restrict reply
) NO_EXCEPT
{
    if (header->payload_size != NET_HELLO_PAYLOAD_SIZE
        || (header->flags & NET_FLAG_ENCRYPTED)
        || header->connection_id != 0
    ) {
        return 0;
    }

    const byte* const client_public = in + NET_HEADER_SIZE;
    const byte* const cookie = client_public + X25519_KEY_SIZE;

    const uint32 time_slot = (uint32) (now_us / SEC_MICRO / NET_COOKIE_SLOT_SECONDS);

    byte expected[NET_COOKIE_SIZE];
    net_server_cookie(server, from, time_slot, client_public, expected);

    bool valid = ct_memcmp(expected, cookie, NET_COOKIE_SIZE);
    if (!valid && time_slot) {
        net_server_cookie(server, from, time_slot - 1, client_public, expected);
        valid = ct_memcmp(expected, cookie, NET_COOKIE_SIZE);
    }

    if (!valid) {
        // Stateless: just tell the client the cookie for its address
        net_server_cookie(server, from, time_slot, client_public, expected);

        SpinlockGuard guard(&server->lock);
        ++server->cookies_sent;

        return net_handshake_write_plain(reply, NET_HANDSHAKE_COOKIE, 0, expected, NET_COOKIE_SIZE);
    }

    // A repeated HELLO (the WELCOME got lost) gets the same WELCOME again
    {
        SpinlockGuard guard(&server->lock);

        for (uint32 i = 0; i < server->capacity; ++i) {
            NetConnection* const conn = &server->connections[i];
            if (conn->state == NET_STATE_PENDING
                && memcmp(conn->peer_public, client_public, X25519_KEY_SIZE) == 0
            ) {
                return net_handshake_write_welcome(reply, conn->connection_id, conn->handshake_public, conn->tx_key);
            }
        }
    }

    // The expensive part runs outside of the lock
    byte server_private[X25519_KEY_SIZE];
    byte server_public[X25519_KEY_SIZE];
    if (!secure_random_bytes(server_private, sizeof(server_private))) {
        return 0;
    }

    x25519_public_key(server_public, server_private);

    byte dh_ephemeral[X25519_KEY_SIZE];
    byte dh_static[X25519_KEY_SIZE];

    bool dh_valid = x25519_shared(dh_ephemeral, server_private, client_public);
    if (dh_valid && server->security.has_static_key) {
        dh_valid = x25519_shared(dh_static, server->security.static_key, client_public);
    }

    ct_memzero(server_private, sizeof(server_private));

    if (!dh_valid) {
        ct_memzero(dh_ephemeral, sizeof(dh_ephemeral));
        ct_memzero(dh_static, sizeof(dh_static));

        return 0;
    }

    SpinlockGuard guard(&server->lock);

    if (server->free_head < 0) {
        ct_memzero(dh_ephemeral, sizeof(dh_ephemeral));
        ct_memzero(dh_static, sizeof(dh_static));

        return 0;
    }

    const uint32 slot = (uint32) server->free_head;
    NetConnection* const conn = &server->connections[slot];
    server->free_head = conn->next_free;

    net_connection_init(conn, true, now_us);
    conn->connection_id = net_server_connection_id(slot, conn->generation);
    conn->state = NET_STATE_PENDING;
    memcpy(conn->addr, from, sizeof(*from));
    memcpy(conn->peer_public, client_public, X25519_KEY_SIZE);
    memcpy(conn->handshake_public, server_public, X25519_KEY_SIZE);

    net_derive_keys(
        dh_ephemeral,
        server->security.has_static_key ? dh_static : NULL,
        server->security.has_psk ? server->security.psk : NULL,
        client_public,
        server_public,
        conn->connection_id,
        conn->rx_key,
        conn->tx_key
    );

    ct_memzero(dh_ephemeral, sizeof(dh_ephemeral));
    ct_memzero(dh_static, sizeof(dh_static));

    ++server->count;
    ++server->handshakes;

    return net_handshake_write_welcome(reply, conn->connection_id, server_public, conn->tx_key);
}

/**
 * Handles a received datagram
 *
 * @param NetServer*            server      Server
 * @param const byte*           in          Datagram
 * @param int32                 size        Datagram size
 * @param const sockaddr_in6*   from        Sender
 * @param uint64                now_us      Monotonic time in us
 * @param byte*                 payload     Output payload (NET_MAX_PAYLOAD_SIZE bytes)
 * @param NetReceiveInfo*       info        Output: connection + channel of the payload
 * @param byte*                 reply       Output: datagram for the sender (NET_MAX_PACKET_SIZE bytes)
 * @param int32*                reply_size  Output: reply size, 0 = no reply
 *
 * @return int32 Payload size (>= 0) or NetReadResult
 */
inline
int32 net_server_receive(
    NetServer* const __restrict server,
    const byte* const __restrict in,
    int32 size,
    const sockaddr_in6* const __restrict from,
    uint64 now_us,
    byte* const __restrict payload,
    NetReceiveInfo* const __restrict info,
    byte* const __restrict reply,
    int32* const __restrict reply_size
) NO_EXCEPT
{
    memset(info, 0, sizeof(*info));
    *reply_size = 0;

    NetHeader header;
    if (!net_packet_peek(in, size, &header)) {
        return NET_READ_INVALID;
    }

    if (header.flags & NET_FLAG_HANDSHAKE) {
        if (header.channel == NET_HANDSHAKE_HELLO) {
            *reply_size = net_server_handle_hello(server, in, &header, from, now_us, reply);
        }

        return NET_READ_NONE;
    }

    SpinlockGuard guard(&server->lock);

    NetConnection* const conn = net_server_find_locked(server, header.connection_id);
    if (!conn) {
        ++server->invalid_packets;

        return NET_READ_INVALID;
    }

    const bool was_pending = conn->state == NET_STATE_PENDING;

    uint8 channel = 0;
    const int32 result = net_connection_read(conn, &server->pool, in, size, now_us, payload, &channel);

    info->connection_id = header.connection_id;

    if (result == NET_READ_INVALID) {
        ++server->invalid_packets;

        return result;
    }

    if (result == NET_READ_DISCONNECT) {
        net_server_remove_locked(server, conn);

        return result;
    }

    // Authentic packet from a new address (NAT rebinding) -> answer there
    if (result != NET_READ_NONE || conn->duplicate_ack || conn->ack_pending) {
        memcpy(conn->addr, from, sizeof(*from));
    }

    info->connected = was_pending && conn->state == NET_STATE_CONNECTED;

    // Duplicates are acked right away (nothing gets delivered)
    if (result == NET_READ_NONE && conn->duplicate_ack) {
        *reply_size = net_connection_write_ack(conn, reply, now_us);
    }

    if (result >= 0) {
        info->channel = channel;
    }

    return result;
}

/**
 * Sends a payload to a connection
 *
 * @param send Callback (const sockaddr_in6* addr, const byte* data, int32 size)
 *
 * @return bool False if the connection doesn't exist or the packet can't be sent
 *              (e.g. too many reliable packets in flight)
 */
template <typename SendFn>
inline
bool net_server_send(
    NetServer* const server,
    uint32 connection_id,
    uint8 channel,
    const void* const payload,
    uint16 size,
    uint64 now_us,
    SendFn send
) NO_EXCEPT
{
    byte packet[NET_MAX_PACKET_SIZE];
    sockaddr_in6 addr;
    int32 packet_size;

    {
        SpinlockGuard guard(&server->lock);

        NetConnection* const conn = net_server_find_locked(server, connection_id);
        if (!conn) {
            return false;
        }

        packet_size = net_connection_write(conn, &server->pool, packet, channel, payload, size, now_us);
        memcpy(&addr, conn->addr, sizeof(addr));
    }

    if (packet_size <= 0) {
        return false;
    }

    send(&addr, packet, packet_size);

    return true;
}

/**
 * Sends the outstanding ack of a connection (if the response didn't carry it)
 */
template <typename SendFn>
inline
void net_server_flush_ack(NetServer* const server, uint32 connection_id, uint64 now_us, SendFn send) NO_EXCEPT
{
    byte packet[NET_MAX_PACKET_SIZE];
    sockaddr_in6 addr;
    int32 packet_size;

    {
        SpinlockGuard guard(&server->lock);

        NetConnection* const conn = net_server_find_locked(server, connection_id);
        if (!conn) {
            return;
        }

        packet_size = net_connection_write_ack(conn, packet, now_us);
        memcpy(&addr, conn->addr, sizeof(addr));
    }

    if (packet_size > 0) {
        send(&addr, packet, packet_size);
    }
}

/**
 * Re-sends reliable packets and closes dead/timed out connections
 *
 * @param send      Callback (const sockaddr_in6* addr, const byte* data, int32 size)
 * @param on_close  Callback (uint32 connection_id), called after the connection was removed
 *
 * @return uint32 Amount of closed connections
 */
template <typename SendFn, typename CloseFn>
inline
uint32 net_server_update(NetServer* const server, uint64 now_us, SendFn send, CloseFn on_close) NO_EXCEPT
{
    uint32 closed[64];
    uint32 closed_count = 0;

    // The lock is released between the connections, sending doesn't block the workers for long
    for (uint32 i = 0; i < server->capacity; ++i) {
        SpinlockGuard guard(&server->lock);

        NetConnection* const conn = &server->connections[i];
        if (conn->state == NET_STATE_CLOSED) {
            continue;
        }

        const bool timed_out = conn->state == NET_STATE_PENDING
            ? now_us > conn->created_us + server->pending_timeout_us
            : now_us > conn->last_received_us + server->idle_timeout_us;

        sockaddr_in6 addr;
        memcpy(&addr, conn->addr, sizeof(addr));

        const bool dead = timed_out
            || net_connection_resend(conn, &server->pool, now_us, [&](const byte* data, int32 size) {
                send(&addr, data, size);
            }) < 0;

        if (!dead) {
            continue;
        }

        closed[closed_count++] = conn->connection_id;
        net_server_remove_locked(server, conn);

        // The remaining connections are checked by the next update
        if (closed_count >= ARRAY_COUNT(closed)) {
            break;
        }
    }

    for (uint32 i = 0; i < closed_count; ++i) {
        on_close(closed[i]);
    }

    return closed_count;
}

/**
 * Address of a connection (e.g. for logging or rate limiting)
 */
inline
bool net_server_address(NetServer* const __restrict server, uint32 connection_id, sockaddr_in6* const __restrict addr) NO_EXCEPT
{
    SpinlockGuard guard(&server->lock);

    const NetConnection* const conn = net_server_find_locked(server, connection_id);
    if (!conn) {
        return false;
    }

    memcpy(addr, conn->addr, sizeof(*addr));

    return true;
}

#endif
