/**
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_NETWORK_NET_SERVER_H
#define COMS_NETWORK_NET_SERVER_H

#include "../stdlib/Stdlib.h"
// Before anything that includes windows.h (winsock2.h must come first)
#include "Socket.h"
#include "../thread/Spinlock.h"
#include "NetConnection.h"

/**
 * Server side connection table of the network envelope
 *
 * Thread safe: all functions lock the table for the duration of one packet
 * (lookup + crypto + bookkeeping). The application logic runs outside of the lock
 * and addresses connections by their id, a connection that was closed in the
 * meantime is simply not found anymore.
 *
 * Connection id = generation (high 12 bits, never 0) | slot (low 20 bits)
 * -> O(1) lookup without a hash map, a stale id of a reused slot doesn't match.
 *
 * Usage (per received datagram):
 *
 *      NetReceiveInfo info;
 *      int32 reply_size;
 *      const int32 size = net_server_receive(server, data, length, &from, now_us, payload, &info, reply, &reply_size);
 *      if (reply_size > 0) sendto(from, reply, reply_size);    // handshake / duplicate ack
 *      if (size >= 0) {
 *          ... handle payload, respond with net_server_send(server, info.connection_id, ...)
 *          net_server_flush_ack(server, info.connection_id, now_us, send_fn);
 *      }
 *
 * Periodically: net_server_update() re-sends reliable packets and closes timed out connections.
 */

#define NET_SERVER_SLOT_BITS 20
#define NET_SERVER_MAX_CONNECTIONS (1 << NET_SERVER_SLOT_BITS)

// A cookie is valid for the current and the previous slot
#define NET_COOKIE_SLOT_SECONDS 10

#define NET_PENDING_TIMEOUT_US (5 * SEC_MICRO)
#define NET_IDLE_TIMEOUT_US (60 * SEC_MICRO)

static_assert(sizeof(sockaddr_in6) == 28, "NetConnection.addr expects a 28 byte sockaddr_in6");

struct NetServer {
    NetConnection* connections;
    uint32 capacity;
    uint32 count;

    int32 free_head;

    NetResendPool pool;

    NetSecurity security;
    byte static_public[X25519_KEY_SIZE];

    byte cookie_secret[32];

    uint32 pending_timeout_us;
    uint32 idle_timeout_us;

    spinlock32 lock;

    // Statistics
    uint32 handshakes;
    uint32 cookies_sent;
    uint32 invalid_packets;
};

struct NetReceiveInfo {
    uint32 connection_id;
    uint8 channel;

    // The connection was created by this packet (first packet after the WELCOME)
    bool connected;

    uint16 reserved;
};

/**
 * Memory of the connection table
 */
struct NetServerMemory {
    NetConnection* connections;
    uint32 capacity;

    NetResendBuffer* resend_buffers;
    int32* resend_next;
    uint32 resend_count;
};

#endif
