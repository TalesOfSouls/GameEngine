/**
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_NETWORK_NET_CONNECTION_H
#define COMS_NETWORK_NET_CONNECTION_H

#include "../stdlib/Stdlib.h"
#include "../encryption/X25519.h"
#include "packet/NetPacket.h"

/**
 * Connection state of the network envelope (see packet/NetPacket.h)
 *
 * Platform and socket independent: the functions create and consume datagrams,
 * the caller sends them. This keeps the logic testable without sockets, see
 * NetServer.h and NetClient.h for the socket side.
 *
 * Handshake (X25519, keys derived with HKDF-SHA256):
 *
 *      client                                      server
 *      HELLO   (client ephemeral key, no cookie) ->
 *                                                <- COOKIE  (HMAC of address + time slot + client key)
 *      HELLO   (client ephemeral key, cookie)    ->
 *                                                <- WELCOME (connection id, server ephemeral key, tag)
 *      data    (encrypted)                       ->
 *
 *      - The cookie makes the server stateless until the client proved it can receive
 *        datagrams at its address (no state and no X25519 for spoofed requests).
 *      - HELLO is padded and larger than COOKIE and WELCOME -> the server can't be
 *        used to amplify attacks.
 *      - The server is authenticated by its static key (the client knows the public
 *        key, e.g. from its configuration) and/or a pre shared key (server to server).
 *        Without one of them nobody can derive the keys and the WELCOME tag doesn't verify.
 *      - With a pre shared key the client is authenticated as well (only clients that
 *        know the key can create valid packets). Public clients authenticate at the
 *        application level (login, token).
 *
 *      keys = HKDF(salt = psk or label,
 *                  ikm  = X25519(eph_c, eph_s) || X25519(eph_c, static_s),
 *                  info = label || client key || server key || connection id)
 *
 * Reliability:
 *      Every packet carries ack/ack_bits of the received packets. Reliable packets
 *      are kept in a resend buffer until they are acknowledged and re-sent with
 *      the identical bytes (same sequence -> same nonce, no nonce reuse with other
 *      content). The receiver drops duplicates with the replay window and acks
 *      them again. Reliable packets are delivered in the order of arrival, the
 *      application orders its data itself if it needs to (e.g. by a timestamp).
 */

#define NET_COOKIE_SIZE 16
#define NET_HELLO_PAYLOAD_SIZE 128
#define NET_WELCOME_PAYLOAD_SIZE X25519_KEY_SIZE

// Bits of the replay window (sequences older than this are dropped)
#define NET_REPLAY_WINDOW 1024

// Max. reliable packets in flight per connection
#define NET_RESEND_SLOTS 32

#define NET_RESEND_MIN_US (100 * MILLI_MICRO)
#define NET_RESEND_MAX_US (1000 * MILLI_MICRO)
#define NET_RESEND_MAX_ATTEMPTS 12

// Initial round trip time estimation
#define NET_RTT_INITIAL_US (200 * MILLI_MICRO)

enum NetHandshakeType : uint8 {
    NET_HANDSHAKE_HELLO = 1,
    NET_HANDSHAKE_COOKIE = 2,
    NET_HANDSHAKE_WELCOME = 3,
};

enum NetConnectionState : uint8 {
    NET_STATE_CLOSED = 0,

    // Client: HELLO sent, waiting for COOKIE/WELCOME
    NET_STATE_CONNECTING = 1,

    // Server: WELCOME sent, waiting for the first packet of the client
    NET_STATE_PENDING = 2,

    NET_STATE_CONNECTED = 3,
};

/**
 * Results of net_connection_read() (>= 0 = payload size)
 */
enum NetReadResult : int32 {
    // Nothing to deliver (ack, duplicate)
    NET_READ_NONE = -1,

    // Not authentic/invalid, the packet must be ignored
    NET_READ_INVALID = -2,

    // The other side closed the connection
    NET_READ_DISCONNECT = -3,
};

/**
 * Key material of a connection
 *
 * Server: static_key = own static private key
 * Client: static_key = static public key of the server
 */
struct NetSecurity {
    byte static_key[X25519_KEY_SIZE];
    byte psk[NET_KEY_SIZE];

    bool has_static_key;
    bool has_psk;
};

/**
 * Buffers of the reliable packets in flight
 *
 * Shared by all connections of a server (a fixed buffer per connection would
 * need NET_RESEND_SLOTS * NET_MAX_PACKET_SIZE for every connection).
 */
struct NetResendBuffer {
    // Actual size used in data
    uint16 size;
    byte data[NET_MAX_PACKET_SIZE];
};

struct NetResendPool {
    NetResendBuffer* buffers;
    int32* next_free;
    int32 free_head;
    uint32 count;
    uint32 used;
};

struct NetResendSlot {
    uint32 sequence;

    // -1 = slot is free
    int32 buffer;

    uint64 sent_us;
    uint16 attempts;
    uint16 reserved;
    uint32 reserved2;
};

struct NetConnection {
    uint32 connection_id;
    uint8 state;
    uint8 is_server;

    // Compress outgoing payloads (only applied if smaller)
    bool compress;

    // An ack must be sent for the latest received packet
    bool ack_pending;

    // Transmition key (for encryption)
    byte tx_key[NET_KEY_SIZE];

    // Receiving key (for decryption)
    byte rx_key[NET_KEY_SIZE];

    // Remote address (servers), clients use a connected socket
    byte addr[28];

    // Sending
    uint32 next_sequence;

    // Receiving
    uint32 remote_sequence;

    // Duplicate reliable packet that needs to be acked again (0 = none)
    uint32 duplicate_ack;

    uint64 received_bits[NET_REPLAY_WINDOW / 64];

    uint64 created_us;
    uint64 last_received_us;
    uint64 last_sent_us;

    uint32 rtt_us;
    uint32 resend_used;
    NetResendSlot resend[NET_RESEND_SLOTS];

    // Statistics
    uint32 packets_sent;
    uint32 packets_received;
    uint32 packets_resent;
    uint32 packets_dropped;

    // Handshake: own ephemeral key pair (the private key is wiped after the key derivation)
    byte handshake_private[X25519_KEY_SIZE];
    byte handshake_public[X25519_KEY_SIZE];

    // Ephemeral key of the other side
    byte peer_public[X25519_KEY_SIZE];

    // Client: cookie of the server, server: the welcome tag is created from the keys
    byte cookie[NET_COOKIE_SIZE];

    // Client: key material for the WELCOME, server: unused (the server has it)
    NetSecurity security;

    // Free list of the server connection table
    int32 next_free;
    uint32 generation;
};

enum NetClientHandshakeResult : int32 {
    NET_HANDSHAKE_INVALID = -1,
    NET_HANDSHAKE_IGNORED = 0,

    // Send the HELLO again (now with cookie)
    NET_HANDSHAKE_COOKIE_RECEIVED = 1,

    NET_HANDSHAKE_CONNECTED = 2,
};

#endif
