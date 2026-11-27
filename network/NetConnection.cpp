/**
 * Connection state of the network envelope: handshake, reliability, replay protection (types: NetConnection.h)
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_NETWORK_NET_CONNECTION_C
#define COMS_NETWORK_NET_CONNECTION_C

#include "../stdlib/Stdlib.h"
#include "../utils/ConstantTime.h"
#include "../utils/SecureRandom.h"
#include "../hash/Hmac.h"
#include "../hash/Hkdf.h"
#include "../encryption/X25519.h"
#include "../encryption/ChaCha20Poly1305.h"
#include "packet/NetPacket.cpp"
#include "NetConnection.h"

/**
 * Memory of count resend buffers incl. their free list and alignment (e.g. for the size of a fixed memory block)
 */
CONSTEXPR FORCE_INLINE
size_t net_resend_memory_size(uint32 count) NO_EXCEPT
{
    return sizeof(NetResendBuffer) * count + alignof(NetResendBuffer)
        + sizeof(int32) * count + alignof(int32);
}

/**
 * Resend pool
 */
inline
void net_resend_pool_init(
    NetResendPool* const __restrict pool,
    NetResendBuffer* const __restrict buffers,
    int32* const __restrict next_free,
    uint32 count
) NO_EXCEPT
{
    pool->buffers = buffers;
    pool->next_free = next_free;
    pool->count = count;
    pool->used = 0;

    for (uint32 i = 0; i < count; ++i) {
        next_free[i] = (int32) i + 1 < (int32) count ? (int32) i + 1 : -1;
    }

    pool->free_head = count ? 0 : -1;
}

static inline
int32 net_resend_pool_get(NetResendPool* const pool) NO_EXCEPT
{
    const int32 index = pool->free_head;
    if (index < 0) {
        return -1;
    }

    pool->free_head = pool->next_free[index];
    pool->next_free[index] = -1;
    ++pool->used;

    return index;
}

static inline
void net_resend_pool_release(NetResendPool* const pool, int32 index) NO_EXCEPT
{
    if (index < 0 || (uint32) index >= pool->count) {
        return;
    }

    pool->next_free[index] = pool->free_head;
    pool->free_head = index;
    --pool->used;
}

/**
 * Replay/ack window
 */
static inline
bool net_window_has(const NetConnection* const conn, uint32 sequence) NO_EXCEPT
{
    if (!sequence || sequence > conn->remote_sequence || conn->remote_sequence - sequence >= NET_REPLAY_WINDOW) {
        return false;
    }

    const uint32 bit = sequence % NET_REPLAY_WINDOW;

    return (conn->received_bits[bit / 64] >> (bit % 64)) & 1;
}

/**
 * @return int32 0 = new, 1 = duplicate, 2 = too old (outside of the replay window)
 */
static inline
int32 net_window_check(const NetConnection* const conn, uint32 sequence) NO_EXCEPT
{
    if (sequence > conn->remote_sequence) {
        return 0;
    }

    if (conn->remote_sequence - sequence >= NET_REPLAY_WINDOW) {
        return 2;
    }

    return net_window_has(conn, sequence) ? 1 : 0;
}

static inline
void net_window_set(NetConnection* const conn, uint32 sequence) NO_EXCEPT
{
    if (sequence > conn->remote_sequence) {
        if (sequence - conn->remote_sequence >= NET_REPLAY_WINDOW) {
            memset(conn->received_bits, 0, sizeof(conn->received_bits));
        } else {
            for (uint32 s = conn->remote_sequence + 1; s < sequence; ++s) {
                const uint32 bit = s % NET_REPLAY_WINDOW;
                conn->received_bits[bit / 64] &= ~((uint64) 1 << (bit % 64));
            }
        }

        conn->remote_sequence = sequence;
    }

    const uint32 bit = sequence % NET_REPLAY_WINDOW;
    conn->received_bits[bit / 64] |= (uint64) 1 << (bit % 64);
}

/**
 * Ack bits relative to base (bit i = base - 1 - i was received)
 */
static inline
uint32 net_window_ack_bits(const NetConnection* const conn, uint32 base) NO_EXCEPT
{
    uint32 bits = 0;
    for (uint32 i = 0; i < 32; ++i) {
        if (base <= i + 1) {
            break;
        }

        if (net_window_has(conn, base - 1 - i)) {
            bits |= 1U << i;
        }
    }

    return bits;
}

/**
 * Initializes a connection (both sides)
 */
inline
void net_connection_init(NetConnection* const conn, bool is_server, uint64 now_us) NO_EXCEPT
{
    const int32 next_free = conn->next_free;
    const uint32 generation = conn->generation;

    ct_memzero(conn, sizeof(*conn));

    conn->next_free = next_free;
    conn->generation = generation;

    conn->is_server = is_server ? 1 : 0;
    conn->next_sequence = 1;
    conn->rtt_us = NET_RTT_INITIAL_US;
    conn->created_us = now_us;
    conn->last_received_us = now_us;
    conn->last_sent_us = now_us;

    for (int32 i = 0; i < NET_RESEND_SLOTS; ++i) {
        conn->resend[i].buffer = -1;
    }
}

/**
 * Releases the resend buffers and wipes the keys
 */
inline
void net_connection_release(NetConnection* const conn, NetResendPool* const pool) NO_EXCEPT
{
    for (int32 i = 0; i < NET_RESEND_SLOTS; ++i) {
        if (conn->resend[i].buffer >= 0) {
            net_resend_pool_release(pool, conn->resend[i].buffer);
            conn->resend[i].buffer = -1;
        }
    }

    conn->resend_used = 0;
    conn->state = NET_STATE_CLOSED;

    ct_memzero(conn->tx_key, sizeof(conn->tx_key));
    ct_memzero(conn->rx_key, sizeof(conn->rx_key));
    ct_memzero(conn->handshake_private, sizeof(conn->handshake_private));
    ct_memzero(&conn->security, sizeof(conn->security));
}

/**
 * Processes ack + ack_bits of an authentic packet
 */
static inline
void net_connection_process_ack(
    NetConnection* const conn,
    NetResendPool* const pool,
    uint32 ack,
    uint32 ack_bits,
    uint64 now_us
) NO_EXCEPT
{
    if (!ack || !conn->resend_used) {
        return;
    }

    for (int32 i = 0; i < NET_RESEND_SLOTS; ++i) {
        NetResendSlot* const slot = &conn->resend[i];
        if (slot->buffer < 0 || slot->sequence > ack) {
            continue;
        }

        const uint32 distance = ack - slot->sequence;
        const bool acked = distance == 0 || (distance <= 32 && ((ack_bits >> (distance - 1)) & 1));
        if (!acked) {
            continue;
        }

        // Only unambiguous samples (not re-sent) update the round trip time
        if (slot->attempts == 1 && now_us >= slot->sent_us) {
            const uint32 sample = (uint32) oms_min(now_us - slot->sent_us, (uint64) (10 * SEC_MICRO));
            conn->rtt_us = (conn->rtt_us * 7 + sample) / 8;
        }

        net_resend_pool_release(pool, slot->buffer);
        slot->buffer = -1;
        --conn->resend_used;
    }
}

FORCE_INLINE
bool net_channel_is_reliable(uint8 channel) NO_EXCEPT
{
    return channel == NET_CHANNEL_RELIABLE;
}

/**
 * Creates a data packet
 *
 * @param NetConnection*    conn        Connection (connected)
 * @param NetResendPool*    pool        Resend buffers (reliable channels)
 * @param byte*             out         Output datagram (NET_MAX_PACKET_SIZE bytes)
 * @param uint8             channel     NetChannel
 * @param const void*       payload     Payload
 * @param uint16            size        Payload size (max. NET_MAX_PAYLOAD_SIZE)
 * @param uint64            now_us      Monotonic time in us
 *
 * @return int32 Datagram size, 0 if the packet can't be sent (not connected, too large,
 *               too many reliable packets in flight)
 */
inline
int32 net_connection_write(
    NetConnection* const __restrict conn,
    NetResendPool* const __restrict pool,
    byte* const __restrict out,
    uint8 channel,
    const void* const __restrict payload,
    uint16 size,
    uint64 now_us
) NO_EXCEPT
{
    if ((conn->state != NET_STATE_CONNECTED && conn->state != NET_STATE_PENDING)
        || channel >= NET_CHANNEL_COUNT
        || size > NET_MAX_PAYLOAD_SIZE
        // A wrapped sequence would reuse a nonce
        || conn->next_sequence == 0xFFFFFFFF
    ) {
        return 0;
    }

    const bool reliable = net_channel_is_reliable(channel);

    int32 slot_index = -1;
    int32 buffer = -1;

    if (reliable) {
        if (conn->resend_used >= NET_RESEND_SLOTS) {
            return 0;
        }

        for (int32 i = 0; i < NET_RESEND_SLOTS; ++i) {
            if (conn->resend[i].buffer < 0) {
                slot_index = i;
                break;
            }
        }

        buffer = net_resend_pool_get(pool);
        if (slot_index < 0 || buffer < 0) {
            if (buffer >= 0) {
                net_resend_pool_release(pool, buffer);
            }

            return 0;
        }
    }

    NetHeader header = {};
    header.flags = 0;
    header.channel = channel;
    header.connection_id = conn->connection_id;
    header.sequence = conn->next_sequence;
    header.ack = conn->remote_sequence;
    header.ack_bits = net_window_ack_bits(conn, conn->remote_sequence);

    const int32 packet_size = net_packet_seal(
        out, 
        &header, (const byte *) payload, size, 
        conn->tx_key, 
        conn->compress
    );
    if (!packet_size) {
        if (buffer >= 0) {
            net_resend_pool_release(pool, buffer);
        }

        return 0;
    }

    ++conn->next_sequence;

    if (reliable) {
        NetResendSlot* const slot = &conn->resend[slot_index];
        slot->sequence = header.sequence;
        slot->buffer = buffer;
        slot->sent_us = now_us;
        slot->attempts = 1;

        pool->buffers[buffer].size = (uint16) packet_size;
        memcpy(pool->buffers[buffer].data, out, packet_size);

        ++conn->resend_used;
    }

    // The latest received packet is acknowledged by this packet
    conn->ack_pending = false;
    if (conn->duplicate_ack && conn->remote_sequence - conn->duplicate_ack <= 32) {
        conn->duplicate_ack = 0;
    }

    conn->last_sent_us = now_us;
    ++conn->packets_sent;

    return packet_size;
}

/**
 * Creates an ack packet if an acknowledgement is outstanding
 *
 * Call it after a received packet was handled (a response already carries the ack).
 * Call it repeatedly until it returns 0.
 *
 * @return int32 Datagram size, 0 = nothing to send
 */
inline
int32 net_connection_write_ack(NetConnection* const __restrict conn, byte* const __restrict out, uint64 now_us) NO_EXCEPT
{
    if ((conn->state != NET_STATE_CONNECTED && conn->state != NET_STATE_PENDING)
        || conn->next_sequence == 0xFFFFFFFF
    ) {
        return 0;
    }

    uint32 ack;
    if (conn->ack_pending) {
        ack = conn->remote_sequence;
        conn->ack_pending = false;

        if (conn->duplicate_ack && conn->remote_sequence - conn->duplicate_ack <= 32) {
            conn->duplicate_ack = 0;
        }
    } else if (conn->duplicate_ack) {
        ack = conn->duplicate_ack;
        conn->duplicate_ack = 0;
    } else {
        return 0;
    }

    NetHeader header = {};
    header.flags = NET_FLAG_ACK_ONLY;
    header.channel = NET_CHANNEL_UNRELIABLE;
    header.connection_id = conn->connection_id;
    header.sequence = conn->next_sequence++;
    header.ack = ack;
    header.ack_bits = net_window_ack_bits(conn, ack);

    conn->last_sent_us = now_us;
    ++conn->packets_sent;

    return net_packet_seal(out, &header, NULL, 0, conn->tx_key, false);
}

/**
 * Creates a disconnect packet (unreliable, send it 1-3 times)
 *
 * The caller releases the connection afterwards (net_connection_release())
 *
 * @return int32 Datagram size, 0 if not connected
 */
inline
int32 net_connection_write_disconnect(
    NetConnection* const __restrict conn,
    NetResendPool* const __restrict pool,
    byte* const __restrict out,
    uint64 now_us
) NO_EXCEPT
{
    const byte control = NET_CONTROL_DISCONNECT;
    return net_connection_write(conn, pool, out, NET_CHANNEL_CONTROL, &control, 1, now_us);
}

/**
 * Handles a received data packet
 *
 * @param NetConnection*    conn        Connection
 * @param NetResendPool*    pool        Resend buffers
 * @param const byte*       in          Datagram
 * @param int32             size        Datagram size
 * @param uint64            now_us      Monotonic time in us
 * @param byte*             payload     Output payload (NET_MAX_PAYLOAD_SIZE bytes)
 * @param uint8*            channel     Output channel of the payload
 *
 * @return int32 Payload size (>= 0) or NetReadResult
 */
inline
int32 net_connection_read(
    NetConnection* const __restrict conn,
    NetResendPool* const __restrict pool,
    const byte* const __restrict in,
    int32 size,
    uint64 now_us,
    byte* const __restrict payload,
    uint8* const __restrict channel
) NO_EXCEPT
{
    if (conn->state != NET_STATE_CONNECTED && conn->state != NET_STATE_PENDING) {
        return NET_READ_INVALID;
    }

    NetHeader header;
    if (!net_packet_peek(in, size, &header)
        || (header.flags & NET_FLAG_HANDSHAKE)
        || header.connection_id != conn->connection_id
        || header.channel >= NET_CHANNEL_COUNT
    ) {
        ++conn->packets_dropped;

        return NET_READ_INVALID;
    }

    // The header only has the low 16 bits of the sequence/ack
    const uint32 sequence = net_sequence_expand(header.sequence, conn->remote_sequence);
    if (sequence == 0) {
        ++conn->packets_dropped;

        return NET_READ_INVALID;
    }

    // Cheap replay check before the decryption
    const int32 window = net_window_check(conn, sequence);
    if (window == 2) {
        ++conn->packets_dropped;

        return NET_READ_NONE;
    }

    const int32 payload_size = net_packet_open(in, size, &header, conn->rx_key, sequence, payload);
    if (payload_size < 0) {
        ++conn->packets_dropped;

        return NET_READ_INVALID;
    }

    // From here on the packet is authentic
    if (window == 1) {
        // The sender didn't get our ack, acknowledge it again
        if (header.flags & NET_FLAG_RELIABLE) {
            conn->duplicate_ack = header.sequence;
        }

        return NET_READ_NONE;
    }

    conn->last_received_us = now_us;
    ++conn->packets_received;

    if (conn->state == NET_STATE_PENDING) {
        conn->state = NET_STATE_CONNECTED;
    }

    // Acks refer to our own sequences (max. the latest sent one)
    net_connection_process_ack(
        conn, pool,
        net_sequence_expand(header.ack, conn->next_sequence - 1), header.ack_bits,
        now_us
    );

    net_window_set(conn, header.sequence);

    if (header.flags & NET_FLAG_RELIABLE) {
        conn->ack_pending = true;
    }

    if (header.flags & NET_FLAG_ACK_ONLY) {
        return NET_READ_NONE;
    }

    if (header.channel == NET_CHANNEL_CONTROL) {
        // The caller releases the connection (net_connection_release())
        if (payload_size >= 1 && payload[0] == NET_CONTROL_DISCONNECT) {
            return NET_READ_DISCONNECT;
        }

        return NET_READ_NONE;
    }

    *channel = header.channel;

    return payload_size;
}

/**
 * Re-sends unacknowledged reliable packets
 *
 * @param NetConnection*    conn        Connection
 * @param uint64            now_us      Monotonic time in us
 * @param send              Send function (datagram to the connection address)
 *
 * @return int32 Amount of re-sent packets, -1 if a packet exceeded NET_RESEND_MAX_ATTEMPTS
 *               (the connection is dead and should be closed)
 */
template <typename SendFn>
inline
int32 net_connection_resend(NetConnection* const conn, NetResendPool* const pool, uint64 now_us, SendFn send) NO_EXCEPT
{
    if (!conn->resend_used) {
        return 0;
    }

    const uint32 base_interval = oms_max(
        (uint32) NET_RESEND_MIN_US,
        oms_min(conn->rtt_us * 2 + 20 * MILLI_MICRO, (uint32) NET_RESEND_MAX_US)
    );

    int32 resent = 0;

    for (int32 i = 0; i < NET_RESEND_SLOTS; ++i) {
        NetResendSlot* const slot = &conn->resend[i];
        if (slot->buffer < 0) {
            continue;
        }

        // Exponential back off (max. 4x)
        const uint32 shift = oms_min((uint32) slot->attempts - 1, (uint32) 2);
        const uint64 interval = (uint64) base_interval << shift;

        if (now_us < slot->sent_us + interval) {
            continue;
        }

        if (slot->attempts >= NET_RESEND_MAX_ATTEMPTS) {
            return -1;
        }

        const NetResendBuffer* const buffer = &pool->buffers[slot->buffer];
        send(buffer->data, (int32) buffer->size);

        slot->sent_us = now_us;
        ++slot->attempts;
        ++resent;
    }

    conn->packets_resent += resent;
    if (resent) {
        conn->last_sent_us = now_us;
    }

    return resent;
}

/**
 * Key derivation of the handshake
 *
 * @param const byte*   dh_ephemeral    X25519(eph_c, eph_s)
 * @param const byte*   dh_static       X25519(eph_c, static_s) or NULL
 * @param const byte*   psk             Pre shared key or NULL
 * @param const byte*   client_public   Ephemeral key of the client
 * @param const byte*   server_public   Ephemeral key of the server
 * @param uint32        connection_id   Connection id
 * @param byte*         client_to_server Output key
 * @param byte*         server_to_client Output key
 */
inline
void net_derive_keys(
    const byte* const dh_ephemeral,
    const byte* const dh_static,
    const byte* const psk,
    const byte* const client_public,
    const byte* const server_public,
    uint32 connection_id,
    byte* const client_to_server,
    byte* const server_to_client
) NO_EXCEPT
{
    static const char label[] = "TalesOfSouls net v1";

    byte ikm[2 * X25519_KEY_SIZE];
    size_t ikm_size = X25519_KEY_SIZE;
    memcpy(ikm, dh_ephemeral, X25519_KEY_SIZE);

    if (dh_static) {
        memcpy(ikm + X25519_KEY_SIZE, dh_static, X25519_KEY_SIZE);
        ikm_size += X25519_KEY_SIZE;
    }

    byte info[sizeof(label) - 1 + 2 * X25519_KEY_SIZE + 4];
    memcpy(info, label, sizeof(label) - 1);
    memcpy(info + sizeof(label) - 1, client_public, X25519_KEY_SIZE);
    memcpy(info + sizeof(label) - 1 + X25519_KEY_SIZE, server_public, X25519_KEY_SIZE);
    net_write_u32(info + sizeof(label) - 1 + 2 * X25519_KEY_SIZE, connection_id);

    byte okm[2 * NET_KEY_SIZE];
    if (psk) {
        hkdf_sha256(psk, NET_KEY_SIZE, ikm, ikm_size, info, sizeof(info), okm, sizeof(okm));
    } else {
        hkdf_sha256((const byte *) label, sizeof(label) - 1, ikm, ikm_size, info, sizeof(info), okm, sizeof(okm));
    }

    memcpy(client_to_server, okm, NET_KEY_SIZE);
    memcpy(server_to_client, okm + NET_KEY_SIZE, NET_KEY_SIZE);

    ct_memzero(ikm, sizeof(ikm));
    ct_memzero(okm, sizeof(okm));
}

/**
 * Creates a handshake packet without encryption (HELLO, COOKIE)
 */
static inline
int32 net_handshake_write_plain(
    byte* const __restrict out,
    uint8 type,
    uint32 connection_id,
    const byte* const __restrict payload,
    uint16 size
) NO_EXCEPT
{
    NetHeader header = {};
    header.flags = NET_FLAG_HANDSHAKE;
    header.channel = type;
    header.connection_id = connection_id;

    return net_packet_seal(out, &header, payload, size, NULL, false);
}

/**
 * WELCOME: header + server ephemeral key (plain, the client needs it for the key
 * derivation) + tag over both with the server->client key (proves the keys)
 */
inline
int32 net_handshake_write_welcome(
    byte* const __restrict out,
    uint32 connection_id,
    const byte* const __restrict server_public,
    const byte* const __restrict server_to_client
) NO_EXCEPT
{
    NetHeader header = {};
    header.protocol_version = NET_PROTOCOL_VERSION;
    header.flags = NET_FLAG_HANDSHAKE | NET_FLAG_ENCRYPTED;
    header.channel = NET_HANDSHAKE_WELCOME;
    header.connection_id = connection_id;
    header.payload_size = NET_WELCOME_PAYLOAD_SIZE;

    const int32 header_size = net_header_write(out, &header);
    memcpy(out + header_size, server_public, X25519_KEY_SIZE);

    byte nonce[AEAD_CHACHA20_POLY1305_NONCE_SIZE];
    net_nonce(nonce, connection_id, 0);

    aead_chacha20_poly1305_seal(
        server_to_client, nonce,
        out, header_size + NET_WELCOME_PAYLOAD_SIZE,
        NULL, 0,
        NULL,
        out + header_size + NET_WELCOME_PAYLOAD_SIZE
    );

    return header_size + NET_WELCOME_PAYLOAD_SIZE + NET_TAG_SIZE;
}

/**
 * Client: starts the handshake
 *
 * The connection must be initialized (net_connection_init()), the caller releases it
 * when the handshake fails or the connection ends (net_connection_release())
 *
 * @param NetConnection*        conn        Connection
 * @param const NetSecurity*    security    Server static public key and/or pre shared key
 *
 * @return bool False if no key material is configured or no random numbers are available
 */
inline
bool net_client_handshake_begin(
    NetConnection* const __restrict conn,
    const NetSecurity* const __restrict security
) NO_EXCEPT
{
    if (!security || (!security->has_static_key && !security->has_psk)) {
        return false;
    }

    if (!secure_random_bytes(conn->handshake_private, sizeof(conn->handshake_private))) {
        return false;
    }

    x25519_public_key(conn->handshake_public, conn->handshake_private);
    conn->security = *security;
    conn->state = NET_STATE_CONNECTING;

    return true;
}

/**
 * Client: creates the HELLO packet (initially and for every retry)
 */
inline
int32 net_client_write_hello(const NetConnection* const __restrict conn, byte* const __restrict out) NO_EXCEPT
{
    if (conn->state != NET_STATE_CONNECTING) {
        return 0;
    }

    byte payload[NET_HELLO_PAYLOAD_SIZE] = {0};
    memcpy(payload, conn->handshake_public, X25519_KEY_SIZE);
    memcpy(payload + X25519_KEY_SIZE, conn->cookie, NET_COOKIE_SIZE);

    return net_handshake_write_plain(out, NET_HANDSHAKE_HELLO, 0, payload, sizeof(payload));
}

/**
 * Client: handles a datagram while connecting
 *
 * @return int32 NetClientHandshakeResult
 */
inline
int32 net_client_handshake_read(
    NetConnection* const __restrict conn,
    const byte* const __restrict in,
    int32 size,
    uint64 now_us
) NO_EXCEPT
{
    if (conn->state != NET_STATE_CONNECTING) {
        return NET_HANDSHAKE_IGNORED;
    }

    NetHeader header;
    if (!net_packet_peek(in, size, &header) || !(header.flags & NET_FLAG_HANDSHAKE)) {
        return NET_HANDSHAKE_IGNORED;
    }

    if (header.channel == NET_HANDSHAKE_COOKIE) {
        if ((header.flags & NET_FLAG_ENCRYPTED) || header.payload_size != NET_COOKIE_SIZE) {
            return NET_HANDSHAKE_INVALID;
        }

        memcpy(conn->cookie, in + NET_HEADER_SIZE, NET_COOKIE_SIZE);

        return NET_HANDSHAKE_COOKIE_RECEIVED;
    }

    if (header.channel != NET_HANDSHAKE_WELCOME
        || !(header.flags & NET_FLAG_ENCRYPTED)
        || header.payload_size != NET_WELCOME_PAYLOAD_SIZE
        || header.connection_id == 0
        || header.sequence != 0
    ) {
        return NET_HANDSHAKE_INVALID;
    }

    const byte* const server_public = in + NET_HEADER_SIZE;

    byte dh_ephemeral[X25519_KEY_SIZE];
    byte dh_static[X25519_KEY_SIZE];

    if (!x25519_shared(dh_ephemeral, conn->handshake_private, server_public)) {
        return NET_HANDSHAKE_INVALID;
    }

    if (conn->security.has_static_key
        && !x25519_shared(dh_static, conn->handshake_private, conn->security.static_key)
    ) {
        ct_memzero(dh_ephemeral, sizeof(dh_ephemeral));

        return NET_HANDSHAKE_INVALID;
    }

    byte client_to_server[NET_KEY_SIZE];
    byte server_to_client[NET_KEY_SIZE];

    net_derive_keys(
        dh_ephemeral,
        conn->security.has_static_key ? dh_static : NULL,
        conn->security.has_psk ? conn->security.psk : NULL,
        conn->handshake_public,
        server_public,
        header.connection_id,
        client_to_server,
        server_to_client
    );

    ct_memzero(dh_ephemeral, sizeof(dh_ephemeral));
    ct_memzero(dh_static, sizeof(dh_static));

    byte nonce[AEAD_CHACHA20_POLY1305_NONCE_SIZE];
    net_nonce(nonce, header.connection_id, 0);

    const bool valid = aead_chacha20_poly1305_open(
        server_to_client, nonce,
        in, NET_HEADER_SIZE + NET_WELCOME_PAYLOAD_SIZE,
        NULL, 0,
        in + NET_HEADER_SIZE + NET_WELCOME_PAYLOAD_SIZE,
        NULL
    );

    if (!valid) {
        ct_memzero(client_to_server, sizeof(client_to_server));
        ct_memzero(server_to_client, sizeof(server_to_client));

        // Not from the server (or the server has a different key)
        return NET_HANDSHAKE_INVALID;
    }

    memcpy(conn->tx_key, client_to_server, NET_KEY_SIZE);
    memcpy(conn->rx_key, server_to_client, NET_KEY_SIZE);
    memcpy(conn->peer_public, server_public, X25519_KEY_SIZE);

    ct_memzero(client_to_server, sizeof(client_to_server));
    ct_memzero(server_to_client, sizeof(server_to_client));
    ct_memzero(conn->handshake_private, sizeof(conn->handshake_private));
    ct_memzero(&conn->security, sizeof(conn->security));

    conn->connection_id = header.connection_id;
    conn->state = NET_STATE_CONNECTED;
    conn->last_received_us = now_us;

    return NET_HANDSHAKE_CONNECTED;
}

#endif
