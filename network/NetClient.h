/**
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_NETWORK_NET_CLIENT_H
#define COMS_NETWORK_NET_CLIENT_H

#include "../stdlib/Stdlib.h"
// Before anything that includes windows.h (winsock2.h must come first)
#include "Socket.h"
#include "NetConnection.h"
#include "packet/NetMessage.h"

/**
 * Client side of the network envelope over a connected UDP socket
 *
 * net_client_open() creates a connected UDP socket with a short receive timeout
 * (NET_CLIENT_POLL_US), the functions use it as their polling interval for
 * re-sends.
 *
 * The socket library must be initialized by the caller (socket_init() / socket_cleanup()).
 *
 * @example
 *      NetClient* client = ...;
 *      static NetResendBuffer buffers[4];
 *      static int32 buffer_next[4];
 *      net_client_init(client, buffers, buffer_next, ARRAY_COUNT(buffers));
 *
 *      if (net_client_open(client, "127.0.0.1", 19585, &security, 3000)) {
 *          NetRequestPayload request;
 *          request.request_id = net_client_next_id(client);
 *          request.size = (uint16) net_message_write(
 *              request.data, NET_MESSAGE_REQUEST, OP, request.request_id, 0, packet, packet_size
 *          );
 *          const int32 response_size = net_client_request(client, &request, response, 5000);
 *          net_client_close(client);
 *      }
 */

// Interval of HELLO re-sends while connecting
#define NET_CLIENT_HELLO_INTERVAL_US (500 * MILLI_MICRO)

// Receive timeout of the socket = polling interval of the re-sends
#define NET_CLIENT_POLL_US (50 * MILLI_MICRO)

enum NetClientResult : int32 {
    // The connection is dead (a reliable packet was never acknowledged)
    NET_CLIENT_DEAD = -4,
};

typedef bool (*NetClientRunningFn)(const void* data);

struct NetClient {
    socketid sd;
    uint32 next_request_id;

    NetConnection conn;

    // Resend buffers of the reliable packets in flight (owned by the caller)
    NetResendPool pool;
};

/**
 * Callback for messages received while waiting for a response (e.g. notifications)
 */
typedef void (*NetClientMessageFn)(void* data, const NetMessageHeader* header, const byte* message, int32 size);

#endif
