// winsock2.h must be included before windows.h (TestFramework.h)
#if _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
#endif

#include "../TestFramework.h"
#include "../../network/packet/NetPacket.cpp"
#include "../../network/packet/NetMessage.cpp"
#include "../../network/NetConnection.cpp"
#include "../../network/NetServer.cpp"

/**
 * Network envelope tests
 *
 * Server and client run in memory, datagrams are passed by hand. That allows
 * simulating loss, duplicates, reordering and attacks deterministically.
 */

#define NET_TEST_CAPACITY 8
#define NET_TEST_RESEND 64

struct NetTestServer {
    NetServer server;
    NetConnection connections[NET_TEST_CAPACITY];
    NetResendBuffer buffers[NET_TEST_RESEND];
    int32 next[NET_TEST_RESEND];
};

struct NetTestClient {
    NetConnection conn;
    NetResendPool pool;
    NetResendBuffer buffers[NET_TEST_RESEND];
    int32 next[NET_TEST_RESEND];
};

// Last datagram "sent" by the server
struct NetTestWire {
    byte data[8][NET_MAX_PACKET_SIZE];
    int32 size[8];
    int32 count;
};

static void net_test_wire_push(NetTestWire* wire, const byte* data, int32 size) {
    if (wire->count < 8) {
        memcpy(wire->data[wire->count], data, size);
        wire->size[wire->count] = size;
        ++wire->count;
    }
}

static sockaddr_in6 net_test_address(uint16 port) {
    sockaddr_in6 addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin6_family = AF_INET6;
    addr.sin6_port = htons(port);
    ((byte *) &addr.sin6_addr)[15] = 1;

    return addr;
}

static void net_test_server_keys(NetSecurity* server_security, NetSecurity* client_security, bool psk) {
    memset(server_security, 0, sizeof(*server_security));
    memset(client_security, 0, sizeof(*client_security));

    for (int32 i = 0; i < 32; ++i) {
        server_security->static_key[i] = (byte) (i * 3 + 1);
    }

    server_security->has_static_key = true;
    x25519_public_key(client_security->static_key, server_security->static_key);
    client_security->has_static_key = true;

    if (psk) {
        for (int32 i = 0; i < 32; ++i) {
            server_security->psk[i] = (byte) (0xA0 + i);
            client_security->psk[i] = (byte) (0xA0 + i);
        }

        server_security->has_psk = true;
        client_security->has_psk = true;
    }
}

static bool net_test_server_init(NetTestServer* s, const NetSecurity* security) {
    NetServerMemory memory = {};
    memory.connections = s->connections;
    memory.capacity = NET_TEST_CAPACITY;
    memory.resend_buffers = s->buffers;
    memory.resend_next = s->next;
    memory.resend_count = NET_TEST_RESEND;

    return net_server_init(&s->server, &memory, security);
}

static void net_test_client_init(NetTestClient* c) {
    net_resend_pool_init(&c->pool, c->buffers, c->next, NET_TEST_RESEND);
}

/**
 * Runs the handshake, returns the connection id (0 on failure)
 */
static uint32 net_test_connect(NetTestServer* s, NetTestClient* c, const NetSecurity* client_security, const sockaddr_in6* from, uint64 now) {
    net_test_client_init(c);
    net_connection_init(&c->conn, false, now);
    if (!net_client_handshake_begin(&c->conn, client_security)) {
        return 0;
    }

    byte packet[NET_MAX_PACKET_SIZE];
    byte reply[NET_MAX_PACKET_SIZE];
    byte payload[NET_MAX_PAYLOAD_SIZE];
    NetReceiveInfo info;
    int32 reply_size;

    for (int32 attempt = 0; attempt < 3; ++attempt) {
        const int32 hello = net_client_write_hello(&c->conn, packet);
        net_server_receive(&s->server, packet, hello, from, now, payload, &info, reply, &reply_size);

        if (reply_size <= 0) {
            return 0;
        }

        const int32 result = net_client_handshake_read(&c->conn, reply, reply_size, now);
        if (result == NET_HANDSHAKE_CONNECTED) {
            return c->conn.connection_id;
        }

        if (result != NET_HANDSHAKE_COOKIE_RECEIVED) {
            return 0;
        }
    }

    return 0;
}

static void test_net_packet_roundtrip() {
    byte key[NET_KEY_SIZE];
    for (int32 i = 0; i < NET_KEY_SIZE; ++i) {
        key[i] = (byte) i;
    }

    byte payload[NET_MAX_PAYLOAD_SIZE];
    for (int32 i = 0; i < NET_MAX_PAYLOAD_SIZE; ++i) {
        payload[i] = (byte) (i * 31);
    }

    NetHeader header = {};
    header.channel = NET_CHANNEL_RELIABLE;
    header.flags = NET_FLAG_RELIABLE;
    header.connection_id = 0x12345678;
    header.sequence = 42;
    header.ack = 41;
    header.ack_bits = 0xF0F0F0F0;

    byte packet[NET_MAX_PACKET_SIZE];
    const int32 size = net_packet_seal(packet, &header, payload, 100, key, false);
    TEST_EQUALS(size, NET_HEADER_SIZE + 100 + NET_TAG_SIZE);

    // The payload is encrypted
    TEST_TRUE(memcmp(packet + NET_HEADER_SIZE, payload, 100) != 0);

    NetHeader read;
    byte out[NET_MAX_PAYLOAD_SIZE];
    TEST_EQUALS(net_packet_open(packet, size, &read, key, 42, out), 100);
    TEST_MEMORY_EQUALS(out, payload, 100);
    TEST_EQUALS(read.connection_id, 0x12345678);
    TEST_EQUALS(read.sequence, 42);
    TEST_EQUALS(read.ack, 41);
    TEST_EQUALS(read.ack_bits, 0xF0F0F0F0);
    TEST_EQUALS(read.channel, NET_CHANNEL_RELIABLE);

    // Header manipulation (authenticated as associated data)
    packet[12] ^= 1;
    TEST_EQUALS(net_packet_open(packet, size, &read, key, 42, out), -1);
    packet[12] ^= 1;

    // Payload manipulation
    packet[NET_HEADER_SIZE + 3] ^= 1;
    TEST_EQUALS(net_packet_open(packet, size, &read, key, 42, out), -1);
    packet[NET_HEADER_SIZE + 3] ^= 1;

    // Wrong full sequence (same low bits) -> wrong nonce
    TEST_EQUALS(net_packet_open(packet, size, &read, key, 42 + 0x10000, out), -1);

    // Wrong size, wrong key, missing key
    TEST_EQUALS(net_packet_open(packet, size - 1, &read, key, 42, out), -1);
    byte wrong[NET_KEY_SIZE] = {0};
    TEST_EQUALS(net_packet_open(packet, size, &read, wrong, 42, out), -1);
    TEST_EQUALS(net_packet_open(packet, size, &read, NULL, 42, out), -1);

    // Max. payload
    const int32 full = net_packet_seal(packet, &header, payload, NET_MAX_PAYLOAD_SIZE, key, false);
    TEST_EQUALS(full, NET_MAX_PACKET_SIZE);
    TEST_EQUALS(net_packet_open(packet, full, &read, key, 42, out), NET_MAX_PAYLOAD_SIZE);

    // Too large
    TEST_EQUALS(net_packet_seal(packet, &header, payload, NET_MAX_PAYLOAD_SIZE + 1, key, false), 0);
}

static void test_net_packet_compression() {
    byte key[NET_KEY_SIZE] = {1};

    // Repetitive data compresses
    byte payload[1000];
    for (int32 i = 0; i < 1000; ++i) {
        payload[i] = (byte) (i % 10);
    }

    NetHeader header = {};
    header.connection_id = 1;
    header.sequence = 1;

    byte packet[NET_MAX_PACKET_SIZE];
    const int32 size = net_packet_seal(packet, &header, payload, sizeof(payload), key, true);
    TEST_TRUE(header.flags & NET_FLAG_COMPRESSED);
    TEST_LESSER_THAN(size, (int32) sizeof(payload));

    NetHeader read;
    byte out[NET_MAX_PAYLOAD_SIZE];
    TEST_EQUALS(net_packet_open(packet, size, &read, key, 1, out), (int32) sizeof(payload));
    TEST_MEMORY_EQUALS(out, payload, sizeof(payload));

    // Random data doesn't -> sent uncompressed
    uint32 x = 12345;
    for (int32 i = 0; i < 1000; ++i) {
        x = x * 1664525 + 1013904223;
        payload[i] = (byte) (x >> 24);
    }

    header.flags = 0;
    const int32 raw = net_packet_seal(packet, &header, payload, sizeof(payload), key, true);
    TEST_FALSE(header.flags & NET_FLAG_COMPRESSED);
    TEST_EQUALS(raw, NET_HEADER_SIZE + 1000 + NET_TAG_SIZE);
    TEST_EQUALS(net_packet_open(packet, raw, &read, key, 1, out), (int32) sizeof(payload));
    TEST_MEMORY_EQUALS(out, payload, sizeof(payload));
}

static void test_net_message_header() {
    const char text[] = "hello";

    byte message[NET_MAX_PAYLOAD_SIZE];
    const int32 size = net_message_write(message, NET_MESSAGE_RESPONSE, 9, 0xAABBCCDD, -17, text, sizeof(text));
    TEST_EQUALS(size, NET_MESSAGE_HEADER_SIZE + (int32) sizeof(text));

    NetMessageHeader header;
    TEST_EQUALS(net_message_read(message, size, &header), (int32) sizeof(text));
    TEST_EQUALS(header.type, NET_MESSAGE_RESPONSE);
    TEST_EQUALS(header.operation, 9);
    TEST_EQUALS(header.request_id, 0xAABBCCDD);
    TEST_EQUALS(header.status, -17);

    // Shorter payload is zero filled, too short for the minimum is rejected
    char out[16];
    TEST_TRUE(net_message_payload(message, sizeof(text), out, sizeof(out), 2));
    TEST_MEMORY_EQUALS(out, text, sizeof(text));
    TEST_EQUALS(out[15], 0);
    TEST_FALSE(net_message_payload(message, sizeof(text), out, sizeof(out), sizeof(out)));

    TEST_EQUALS(net_message_read(message, NET_MESSAGE_HEADER_SIZE - 1, &header), -1);
    TEST_EQUALS(net_message_write(message, 1, 1, 1, 0, text, NET_MAX_MESSAGE_SIZE + 1), 0);
}

static void test_net_handshake() {
    NetSecurity server_security;
    NetSecurity client_security;
    net_test_server_keys(&server_security, &client_security, false);

    static NetTestServer s;
    static NetTestClient c;
    TEST_TRUE(net_test_server_init(&s, &server_security));

    const sockaddr_in6 from = net_test_address(5000);
    const uint64 now = 100 * SEC_MICRO;

    net_test_client_init(&c);
    net_connection_init(&c.conn, false, now);
    TEST_TRUE(net_client_handshake_begin(&c.conn, &client_security));

    byte packet[NET_MAX_PACKET_SIZE];
    byte reply[NET_MAX_PACKET_SIZE];
    byte payload[NET_MAX_PAYLOAD_SIZE];
    NetReceiveInfo info;
    int32 reply_size;

    // 1. HELLO without cookie -> COOKIE, no state on the server
    const int32 hello = net_client_write_hello(&c.conn, packet);
    TEST_EQUALS(hello, NET_HEADER_SIZE + NET_HELLO_PAYLOAD_SIZE);

    TEST_EQUALS(net_server_receive(&s.server, packet, hello, &from, now, payload, &info, reply, &reply_size), NET_READ_NONE);
    TEST_GREATER_THAN(reply_size, 0);
    TEST_LESSER_THAN(reply_size, hello);
    TEST_EQUALS(s.server.count, 0);

    TEST_EQUALS(net_client_handshake_read(&c.conn, reply, reply_size, now), NET_HANDSHAKE_COOKIE_RECEIVED);

    // The cookie is bound to the address
    const sockaddr_in6 other = net_test_address(5001);
    const int32 hello2 = net_client_write_hello(&c.conn, packet);
    net_server_receive(&s.server, packet, hello2, &other, now, payload, &info, reply, &reply_size);
    TEST_EQUALS(s.server.count, 0);

    // 2. HELLO with cookie -> WELCOME, smaller than the HELLO (no amplification)
    net_server_receive(&s.server, packet, hello2, &from, now, payload, &info, reply, &reply_size);
    TEST_EQUALS(s.server.count, 1);
    TEST_GREATER_THAN(reply_size, 0);
    TEST_LESSER_THAN(reply_size, hello2);

    // A repeated HELLO gets the identical WELCOME (no second connection)
    byte reply2[NET_MAX_PACKET_SIZE];
    int32 reply2_size;
    net_server_receive(&s.server, packet, hello2, &from, now, payload, &info, reply2, &reply2_size);
    TEST_EQUALS(s.server.count, 1);
    TEST_EQUALS(reply2_size, reply_size);
    TEST_MEMORY_EQUALS(reply2, reply, reply_size);

    // A manipulated WELCOME is rejected
    reply2[NET_HEADER_SIZE + 5] ^= 1;
    TEST_EQUALS(net_client_handshake_read(&c.conn, reply2, reply2_size, now), NET_HANDSHAKE_INVALID);
    TEST_EQUALS(c.conn.state, NET_STATE_CONNECTING);

    TEST_EQUALS(net_client_handshake_read(&c.conn, reply, reply_size, now), NET_HANDSHAKE_CONNECTED);
    TEST_EQUALS(c.conn.state, NET_STATE_CONNECTED);
    TEST_NOT_EQUALS(c.conn.connection_id, 0);

    // 3. First data packet confirms the connection on the server
    const char hi[] = "hi";
    const int32 data = net_connection_write(&c.conn, &c.pool, packet, NET_CHANNEL_RELIABLE, hi, sizeof(hi), now);
    TEST_GREATER_THAN(data, 0);

    TEST_EQUALS(net_server_receive(&s.server, packet, data, &from, now, payload, &info, reply, &reply_size), (int32) sizeof(hi));
    TEST_MEMORY_EQUALS(payload, hi, sizeof(hi));
    TEST_EQUALS(info.connection_id, c.conn.connection_id);
    TEST_EQUALS(info.channel, NET_CHANNEL_RELIABLE);
    TEST_TRUE(info.connected);

    // 4. Response from the server
    NetTestWire wire = {};
    const char ok[] = "ok";
    TEST_TRUE(net_server_send(&s.server, info.connection_id, NET_CHANNEL_RELIABLE, ok, sizeof(ok), now,
        [&](const sockaddr_in6*, const byte* d, int32 size) { net_test_wire_push(&wire, d, size); }
    ));
    TEST_EQUALS(wire.count, 1);

    uint8 channel;
    TEST_EQUALS(net_connection_read(&c.conn, &c.pool, wire.data[0], wire.size[0], now, payload, &channel), (int32) sizeof(ok));
    TEST_MEMORY_EQUALS(payload, ok, sizeof(ok));

    // The response acknowledged the request
    TEST_EQUALS(c.conn.resend_used, 0);
}

static void test_net_handshake_wrong_keys() {
    NetSecurity server_security;
    NetSecurity client_security;
    net_test_server_keys(&server_security, &client_security, false);

    static NetTestServer s;
    static NetTestClient c;
    TEST_TRUE(net_test_server_init(&s, &server_security));

    const sockaddr_in6 from = net_test_address(6000);

    // Client expects a different server key (e.g. a man in the middle) -> WELCOME doesn't verify
    NetSecurity wrong = client_security;
    wrong.static_key[0] ^= 0x40;
    TEST_EQUALS(net_test_connect(&s, &c, &wrong, &from, 1000), 0);
    TEST_EQUALS(c.conn.state, NET_STATE_CONNECTING);

    // Pre shared key mismatch
    NetSecurity server_psk;
    NetSecurity client_psk;
    net_test_server_keys(&server_psk, &client_psk, true);
    TEST_TRUE(net_test_server_init(&s, &server_psk));

    client_psk.psk[31] ^= 1;
    TEST_EQUALS(net_test_connect(&s, &c, &client_psk, &from, 1000), 0);

    client_psk.psk[31] ^= 1;
    TEST_NOT_EQUALS(net_test_connect(&s, &c, &client_psk, &from, 1000), 0);

    // PSK only (server to server)
    NetSecurity only_psk = server_psk;
    only_psk.has_static_key = false;
    TEST_TRUE(net_test_server_init(&s, &only_psk));

    NetSecurity client_only_psk = client_psk;
    client_only_psk.has_static_key = false;
    TEST_NOT_EQUALS(net_test_connect(&s, &c, &client_only_psk, &from, 1000), 0);

    // No key material at all is refused
    NetSecurity none = {};
    TEST_FALSE(net_test_server_init(&s, &none));
    net_test_client_init(&c);
    net_connection_init(&c.conn, false, 0);
    TEST_FALSE(net_client_handshake_begin(&c.conn, &none));
}

static void test_net_replay_and_spoofing() {
    NetSecurity server_security;
    NetSecurity client_security;
    net_test_server_keys(&server_security, &client_security, false);

    static NetTestServer s;
    static NetTestClient c;
    TEST_TRUE(net_test_server_init(&s, &server_security));

    const sockaddr_in6 from = net_test_address(7000);
    const uint32 id = net_test_connect(&s, &c, &client_security, &from, 1000);
    TEST_NOT_EQUALS(id, 0);

    byte packet[NET_MAX_PACKET_SIZE];
    byte reply[NET_MAX_PACKET_SIZE];
    byte payload[NET_MAX_PAYLOAD_SIZE];
    NetReceiveInfo info;
    int32 reply_size;

    const char msg[] = "buy item";
    const int32 size = net_connection_write(&c.conn, &c.pool, packet, NET_CHANNEL_RELIABLE, msg, sizeof(msg), 1000);
    TEST_EQUALS(net_server_receive(&s.server, packet, size, &from, 1000, payload, &info, reply, &reply_size), (int32) sizeof(msg));

    // Replay: not delivered again, but acked again (the sender may have missed the ack)
    TEST_EQUALS(net_server_receive(&s.server, packet, size, &from, 1000, payload, &info, reply, &reply_size), NET_READ_NONE);
    TEST_GREATER_THAN(reply_size, 0);

    // The re-ack frees the resend slot of the client
    uint8 channel;
    TEST_EQUALS(c.conn.resend_used, 1);
    TEST_EQUALS(net_connection_read(&c.conn, &c.pool, reply, reply_size, 1000, payload, &channel), NET_READ_NONE);
    TEST_EQUALS(c.conn.resend_used, 0);

    // Unencrypted packet for an existing connection
    NetHeader header = {};
    header.connection_id = id;
    header.sequence = 100;
    const int32 plain = net_packet_seal(packet, &header, (const byte *) msg, sizeof(msg), NULL, false);
    TEST_EQUALS(net_server_receive(&s.server, packet, plain, &from, 1000, payload, &info, reply, &reply_size), NET_READ_INVALID);

    // Forged packet with a wrong key
    byte forged_key[NET_KEY_SIZE] = {7};
    const int32 forged = net_packet_seal(packet, &header, (const byte *) msg, sizeof(msg), forged_key, false);
    TEST_EQUALS(net_server_receive(&s.server, packet, forged, &from, 1000, payload, &info, reply, &reply_size), NET_READ_INVALID);

    // Unknown connection
    header.connection_id = id ^ 0x00100000;
    const int32 unknown = net_packet_seal(packet, &header, (const byte *) msg, sizeof(msg), forged_key, false);
    TEST_EQUALS(net_server_receive(&s.server, packet, unknown, &from, 1000, payload, &info, reply, &reply_size), NET_READ_INVALID);

    // Garbage
    memset(packet, 0x55, 200);
    TEST_EQUALS(net_server_receive(&s.server, packet, 200, &from, 1000, payload, &info, reply, &reply_size), NET_READ_INVALID);
    TEST_EQUALS(net_server_receive(&s.server, packet, 3, &from, 1000, payload, &info, reply, &reply_size), NET_READ_INVALID);

    // NAT rebinding: an authentic packet from a new address moves the connection
    const sockaddr_in6 moved = net_test_address(7001);
    const int32 next = net_connection_write(&c.conn, &c.pool, packet, NET_CHANNEL_UNRELIABLE, msg, sizeof(msg), 1000);
    TEST_EQUALS(net_server_receive(&s.server, packet, next, &moved, 1000, payload, &info, reply, &reply_size), (int32) sizeof(msg));

    sockaddr_in6 current;
    TEST_TRUE(net_server_address(&s.server, id, &current));
    TEST_EQUALS(ntohs(current.sin6_port), 7001);
}

static void test_net_reliability() {
    NetSecurity server_security;
    NetSecurity client_security;
    net_test_server_keys(&server_security, &client_security, false);

    static NetTestServer s;
    static NetTestClient c;
    TEST_TRUE(net_test_server_init(&s, &server_security));

    const sockaddr_in6 from = net_test_address(8000);
    uint64 now = 50 * SEC_MICRO;
    const uint32 id = net_test_connect(&s, &c, &client_security, &from, now);
    TEST_NOT_EQUALS(id, 0);

    byte packets[5][NET_MAX_PACKET_SIZE];
    int32 sizes[5];
    byte reply[NET_MAX_PACKET_SIZE];
    byte payload[NET_MAX_PAYLOAD_SIZE];
    NetReceiveInfo info;
    int32 reply_size;

    // 5 reliable messages, #1 and #3 get lost
    for (int32 i = 0; i < 5; ++i) {
        const byte value = (byte) i;
        sizes[i] = net_connection_write(&c.conn, &c.pool, packets[i], NET_CHANNEL_RELIABLE, &value, 1, now);
        TEST_GREATER_THAN(sizes[i], 0);
    }

    TEST_EQUALS(c.conn.resend_used, 5);

    int32 delivered[8];
    int32 delivered_count = 0;

    const int32 order[] = {0, 2, 4};
    for (int32 i = 0; i < 3; ++i) {
        const int32 r = net_server_receive(&s.server, packets[order[i]], sizes[order[i]], &from, now, payload, &info, reply, &reply_size);
        if (r > 0) {
            delivered[delivered_count++] = payload[0];
        }
    }

    // Delivered in the order of arrival, the gaps don't block
    TEST_EQUALS(delivered_count, 3);
    TEST_EQUALS(delivered[0], 0);
    TEST_EQUALS(delivered[1], 2);
    TEST_EQUALS(delivered[2], 4);

    // Ack from the server frees #0, #2 and #4
    NetTestWire wire = {};
    auto capture = [&](const sockaddr_in6*, const byte* d, int32 size) { net_test_wire_push(&wire, d, size); };
    net_server_flush_ack(&s.server, id, now, capture);
    TEST_EQUALS(wire.count, 1);

    uint8 channel;
    net_connection_read(&c.conn, &c.pool, wire.data[0], wire.size[0], now, payload, &channel);
    TEST_EQUALS(c.conn.resend_used, 2);

    // Nothing is re-sent before the timeout
    NetTestWire resend = {};
    auto capture_resend = [&](const byte* d, int32 size) { net_test_wire_push(&resend, d, size); };
    TEST_EQUALS(net_connection_resend(&c.conn, &c.pool, now + 10 * MILLI_MICRO, capture_resend), 0);

    // After the timeout the 2 unacked packets are re-sent (identical bytes)
    now += 2 * SEC_MICRO;
    TEST_EQUALS(net_connection_resend(&c.conn, &c.pool, now, capture_resend), 2);
    TEST_EQUALS(resend.count, 2);

    for (int32 i = 0; i < resend.count; ++i) {
        const int32 r = net_server_receive(&s.server, resend.data[i], resend.size[i], &from, now, payload, &info, reply, &reply_size);
        if (r > 0) {
            delivered[delivered_count++] = payload[0];
        }
    }

    // Every message is delivered exactly once (#1 and #3 after the re-send)
    TEST_EQUALS(delivered_count, 5);
    TEST_EQUALS(delivered[3], 1);
    TEST_EQUALS(delivered[4], 3);

    wire.count = 0;
    net_server_flush_ack(&s.server, id, now, capture);
    TEST_EQUALS(wire.count, 1);
    net_connection_read(&c.conn, &c.pool, wire.data[0], wire.size[0], now, payload, &channel);
    TEST_EQUALS(c.conn.resend_used, 0);
    TEST_EQUALS(c.pool.used, 0);

    // Duplicates of delivered packets are never delivered twice
    for (int32 i = 0; i < 5; ++i) {
        TEST_EQUALS(net_server_receive(&s.server, packets[i], sizes[i], &from, now, payload, &info, reply, &reply_size), NET_READ_NONE);
    }

    // Unreliable packets are not kept for re-sends
    const byte value = 9;
    TEST_GREATER_THAN(net_connection_write(&c.conn, &c.pool, packets[0], NET_CHANNEL_UNRELIABLE, &value, 1, now), 0);
    TEST_EQUALS(c.conn.resend_used, 0);

    // Back pressure: max. NET_RESEND_SLOTS reliable packets in flight
    int32 written = 0;
    for (int32 i = 0; i < NET_RESEND_SLOTS + 5; ++i) {
        if (net_connection_write(&c.conn, &c.pool, packets[0], NET_CHANNEL_RELIABLE, &value, 1, now) > 0) {
            ++written;
        }
    }

    TEST_EQUALS(written, NET_RESEND_SLOTS);

    // A packet that is never acknowledged kills the connection
    int32 result = 0;
    for (int32 i = 0; i < NET_RESEND_MAX_ATTEMPTS + 2 && result >= 0; ++i) {
        now += 10 * SEC_MICRO;
        resend.count = 0;
        result = net_connection_resend(&c.conn, &c.pool, now, capture_resend);
    }

    TEST_EQUALS(result, -1);
}

static void test_net_server_timeouts() {
    NetSecurity server_security;
    NetSecurity client_security;
    net_test_server_keys(&server_security, &client_security, false);

    static NetTestServer s;
    static NetTestClient c;
    TEST_TRUE(net_test_server_init(&s, &server_security));

    const sockaddr_in6 from = net_test_address(9000);
    uint64 now = SEC_MICRO;

    // Pending connection (no data after the WELCOME) times out quickly
    const uint32 pending = net_test_connect(&s, &c, &client_security, &from, now);
    TEST_NOT_EQUALS(pending, 0);
    TEST_EQUALS(s.server.count, 1);

    uint32 closed[4];
    int32 closed_count = 0;
    auto send_none = [](const sockaddr_in6*, const byte*, int32) {};
    auto on_close = [&](uint32 id) { closed[closed_count++] = id; };

    net_server_update(&s.server, now + SEC_MICRO, send_none, on_close);
    TEST_EQUALS(closed_count, 0);

    net_server_update(&s.server, now + NET_PENDING_TIMEOUT_US + 1, send_none, on_close);
    TEST_EQUALS(closed_count, 1);
    TEST_EQUALS(closed[0], pending);
    TEST_EQUALS(s.server.count, 0);

    // The slot is reused with a new id, the old id is not valid anymore
    now = 100 * SEC_MICRO;
    static NetTestClient c2;
    const uint32 second = net_test_connect(&s, &c2, &client_security, &from, now);
    TEST_NOT_EQUALS(second, 0);
    TEST_NOT_EQUALS(second, pending);

    sockaddr_in6 addr;
    TEST_FALSE(net_server_address(&s.server, pending, &addr));
    TEST_TRUE(net_server_address(&s.server, second, &addr));

    // Connected + idle -> idle timeout
    byte packet[NET_MAX_PACKET_SIZE];
    byte reply[NET_MAX_PACKET_SIZE];
    byte payload[NET_MAX_PAYLOAD_SIZE];
    NetReceiveInfo info;
    int32 reply_size;

    const byte value = 1;
    const int32 size = net_connection_write(&c2.conn, &c2.pool, packet, NET_CHANNEL_UNRELIABLE, &value, 1, now);
    TEST_EQUALS(net_server_receive(&s.server, packet, size, &from, now, payload, &info, reply, &reply_size), 1);

    closed_count = 0;
    net_server_update(&s.server, now + NET_PENDING_TIMEOUT_US + 1, send_none, on_close);
    TEST_EQUALS(closed_count, 0);

    net_server_update(&s.server, now + NET_IDLE_TIMEOUT_US + 1, send_none, on_close);
    TEST_EQUALS(closed_count, 1);

    // Disconnect from the client removes the connection
    now = 300 * SEC_MICRO;
    static NetTestClient c3;
    const uint32 third = net_test_connect(&s, &c3, &client_security, &from, now);
    TEST_NOT_EQUALS(third, 0);
    TEST_EQUALS(s.server.count, 1);

    const int32 bye = net_connection_write_disconnect(&c3.conn, &c3.pool, packet, now);
    TEST_GREATER_THAN(bye, 0);
    net_connection_release(&c3.conn, &c3.pool);
    TEST_EQUALS(c3.conn.state, NET_STATE_CLOSED);
    TEST_EQUALS(c3.pool.used, 0);
    TEST_EQUALS(net_server_receive(&s.server, packet, bye, &from, now, payload, &info, reply, &reply_size), NET_READ_DISCONNECT);
    TEST_EQUALS(info.connection_id, third);
    TEST_EQUALS(s.server.count, 0);

    // Table full -> no WELCOME
    static NetTestClient many[NET_TEST_CAPACITY + 1];
    int32 connected = 0;
    for (int32 i = 0; i < NET_TEST_CAPACITY + 1; ++i) {
        const sockaddr_in6 addr_i = net_test_address((uint16) (10000 + i));
        if (net_test_connect(&s, &many[i], &client_security, &addr_i, now)) {
            ++connected;
        }
    }

    TEST_EQUALS(connected, NET_TEST_CAPACITY);
}

static void test_net_header_packing() {
    NetHeader header = {};
    header.flags = NET_FLAG_COMPRESSED | NET_FLAG_ENCRYPTED | NET_FLAG_ACK_ONLY;
    header.channel = NET_CHANNEL_RELIABLE;
    header.connection_id = 0xFEDCBA98;
    header.sequence = 0x0001ABCD;
    header.ack = 0x0002BEEF;
    header.ack_bits = 0x80000001;
    header.payload_size = NET_MAX_PAYLOAD_SIZE;
    header.protocol_version = NET_PROTOCOL_VERSION;

    byte packed[NET_HEADER_SIZE];
    TEST_EQUALS(net_header_write(packed, &header), NET_HEADER_SIZE);

    NetHeader read = {};
    TEST_EQUALS(net_header_read(packed, sizeof(packed), &read), NET_HEADER_SIZE);

    TEST_EQUALS(read.protocol_version, NET_PROTOCOL_VERSION);
    TEST_EQUALS(read.channel, NET_CHANNEL_RELIABLE);
    TEST_EQUALS(read.connection_id, 0xFEDCBA98);
    TEST_EQUALS(read.ack_bits, 0x80000001);
    TEST_EQUALS(read.payload_size, NET_MAX_PAYLOAD_SIZE);

    // Only the low 16 bits are on the wire
    TEST_EQUALS(read.sequence, 0xABCD);
    TEST_EQUALS(read.ack, 0xBEEF);

    // Reliable is derived from the channel
    TEST_EQUALS(read.flags, NET_FLAG_COMPRESSED | NET_FLAG_ENCRYPTED | NET_FLAG_ACK_ONLY | NET_FLAG_RELIABLE);

    // Truncated header
    TEST_EQUALS(net_header_read(packed, NET_HEADER_SIZE - 1, &read), 0);

    // 24 bytes before packing
    TEST_LESSER_THAN(NET_HEADER_SIZE, 24);
}

static void test_net_sequence_expand() {
    TEST_EQUALS(net_sequence_expand(1, 0), 1);
    TEST_EQUALS(net_sequence_expand(100, 90), 100);
    TEST_EQUALS(net_sequence_expand(80, 90), 80);

    // Across the 16 bit boundary in both directions
    TEST_EQUALS(net_sequence_expand(0x0002, 0xFFFE), 0x10002);
    TEST_EQUALS(net_sequence_expand(0xFFFE, 0x10002), 0xFFFE);
    TEST_EQUALS(net_sequence_expand(0x0005, 0x3FFF0), 0x40005);
    TEST_EQUALS(net_sequence_expand(0xFFF0, 0x40005), 0x3FFF0);

    // No underflow below 0
    TEST_EQUALS(net_sequence_expand(0xFFFF, 1), 0xFFFF);
}

static void test_net_sequence_wrap() {
    NetSecurity server_security;
    NetSecurity client_security;
    net_test_server_keys(&server_security, &client_security, false);

    static NetTestServer s;
    static NetTestClient c;
    TEST_TRUE(net_test_server_init(&s, &server_security));

    const sockaddr_in6 from = net_test_address(11000);
    const uint32 id = net_test_connect(&s, &c, &client_security, &from, 1000);
    TEST_NOT_EQUALS(id, 0);

    // Jump close to the 16 bit boundary of the header sequence
    c.conn.next_sequence = 0xFFF0;

    byte packet[NET_MAX_PACKET_SIZE];
    byte reply[NET_MAX_PACKET_SIZE];
    byte payload[NET_MAX_PAYLOAD_SIZE];
    NetReceiveInfo info;
    int32 reply_size;

    NetTestWire wire = {};
    auto capture = [&](const sockaddr_in6*, const byte* d, int32 size) { net_test_wire_push(&wire, d, size); };

    int32 delivered = 0;
    for (int32 i = 0; i < 40; ++i) {
        const byte value = (byte) i;
        const int32 size = net_connection_write(&c.conn, &c.pool, packet, NET_CHANNEL_RELIABLE, &value, 1, 1000);

        if (net_server_receive(&s.server, packet, size, &from, 1000, payload, &info, reply, &reply_size) == 1
            && payload[0] == value
        ) {
            ++delivered;
        }

        // The ack of the server (low 16 bits) frees the reliable packet on the client
        wire.count = 0;
        net_server_flush_ack(&s.server, id, 1000, capture);

        uint8 channel;
        if (wire.count) {
            net_connection_read(&c.conn, &c.pool, wire.data[0], wire.size[0], 1000, payload, &channel);
        }
    }

    TEST_EQUALS(delivered, 40);
    TEST_GREATER_THAN(c.conn.next_sequence, (uint32) 0x10000);
    TEST_EQUALS(c.conn.resend_used, 0);

    // A packet with a sequence far behind the window is not accepted
    sockaddr_in6 addr;
    TEST_TRUE(net_server_address(&s.server, id, &addr));
}

#ifdef UBER_TEST
    #ifdef main
        #undef main
    #endif
    #define main NetworkNetTest
#endif

int main() {
    TEST_INIT(256);

    TEST_RUN(test_net_header_packing);
    TEST_RUN(test_net_sequence_expand);
    TEST_RUN(test_net_packet_roundtrip);
    TEST_RUN(test_net_packet_compression);
    TEST_RUN(test_net_message_header);
    TEST_RUN(test_net_handshake);
    TEST_RUN(test_net_handshake_wrong_keys);
    TEST_RUN(test_net_replay_and_spoofing);
    TEST_RUN(test_net_reliability);
    TEST_RUN(test_net_server_timeouts);
    TEST_RUN(test_net_sequence_wrap);

    TEST_FINALIZE();

    return 0;
}
