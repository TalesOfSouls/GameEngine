/**
 * Network envelope: header packing, sealing and opening of datagrams (types: NetPacket.h)
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_NETWORK_PACKET_NET_PACKET_C
#define COMS_NETWORK_PACKET_NET_PACKET_C

#include "../../stdlib/Stdlib.h"
#include "../../utils/ConstantTime.h"
#include "../../utils/BitUtils.h"
#include "../../compression/LZ4.h"
#include "../../encryption/ChaCha20Poly1305.h"
#include "NetPacket.h"

FORCE_INLINE
void net_write_u16(byte* const out, uint16 value) NO_EXCEPT
{
    out[0] = (byte) value;
    out[1] = (byte) (value >> 8);
}

FORCE_INLINE
void net_write_u32(byte* const out, uint32 value) NO_EXCEPT
{
    out[0] = (byte) value;
    out[1] = (byte) (value >> 8);
    out[2] = (byte) (value >> 16);
    out[3] = (byte) (value >> 24);
}

FORCE_INLINE
uint16 net_read_u16(const byte* const in) NO_EXCEPT
{
    return (uint16) (in[0] | (in[1] << 8));
}

FORCE_INLINE
uint32 net_read_u32(const byte* const in) NO_EXCEPT
{
    return (uint32) in[0] | ((uint32) in[1] << 8) | ((uint32) in[2] << 16) | ((uint32) in[3] << 24);
}

/**
 * Packs the header (layout see above)
 *
 * @return int32 Size of the packed header (NET_HEADER_SIZE), 0 if a field doesn't fit its bits
 */
inline
int32 net_header_write(byte* const __restrict out, const NetHeader* const __restrict header) NO_EXCEPT
{
    // A field that doesn't fit its bits would be truncated silently
    if (header->protocol_version >= (1U << 4)
        || header->channel >= (1U << 2)
        || header->payload_size >= (1U << NET_PAYLOAD_SIZE_BITS)
    ) {
        return 0;
    }

    BitWriter writer;
    bit_writer_init(&writer, out, NET_HEADER_SIZE);

    bit_write(&writer, header->protocol_version, 4);

    // Only the wire flags, NET_FLAG_RELIABLE is derived from the channel by the receiver
    bit_write(&writer, header->flags & NET_WIRE_FLAGS, 5);
    bit_write(&writer, header->channel, 2);
    bit_write(&writer, header->payload_size, NET_PAYLOAD_SIZE_BITS);
    bit_write(&writer, header->connection_id, 32);

    // Low 16 bits, the receiver reconstructs the full 32 bit values
    bit_write(&writer, header->sequence & 0xFFFF, 16);
    bit_write(&writer, header->ack & 0xFFFF, 16);
    bit_write(&writer, header->ack_bits, 32);

    return (int32) bit_writer_flush(&writer);
}

/**
 * Unpacks the header (layout see above)
 *
 * @return int32 Size of the packed header (NET_HEADER_SIZE), 0 if the data is too short or the padding isn't 0
 */
inline
int32 net_header_read(const byte* const __restrict in, int32 size, NetHeader* const __restrict header) NO_EXCEPT
{
    // Fixed layout: every header (also of the handshake) has NET_HEADER_SIZE bytes
    if (size < NET_HEADER_SIZE) {
        return 0;
    }

    BitReader reader;
    bit_reader_init(&reader, in, NET_HEADER_SIZE);

    header->protocol_version = (uint8) bit_read(&reader, 4);
    header->flags = (uint8) bit_read(&reader, 5);
    header->channel = (uint8) bit_read(&reader, 2);
    header->payload_size = (uint16) bit_read(&reader, NET_PAYLOAD_SIZE_BITS);
    header->connection_id = bit_read(&reader, 32);
    header->sequence = bit_read(&reader, 16);
    header->ack = bit_read(&reader, 16);
    header->ack_bits = bit_read(&reader, 32);
    bit_read_align(&reader);

    if (reader.overflow) {
        return 0;
    }

    // Not a wire flag, derived from the channel
    if (!(header->flags & NET_FLAG_HANDSHAKE) && header->channel == NET_CHANNEL_RELIABLE) {
        header->flags |= NET_FLAG_RELIABLE;
    }

    return NET_HEADER_SIZE;
}

/**
 * Total datagram size of a header
 */
FORCE_INLINE
int32 net_packet_size(const NetHeader* const header) NO_EXCEPT
{
    return NET_HEADER_SIZE + header->payload_size + ((header->flags & NET_FLAG_ENCRYPTED) ? NET_TAG_SIZE : 0);
}

/**
 * Full sequence with the given low 16 bits that is closest to the reference
 *
 * @param uint32 low        Low 16 bits from the header
 * @param uint32 reference  E.g. the highest received sequence
 */
inline
uint32 net_sequence_expand(uint32 low, uint32 reference) NO_EXCEPT
{
    uint32 candidate = (reference & 0xFFFF0000) | (low & 0xFFFF);

    if (candidate > reference && candidate - reference > 0x8000 && candidate >= 0x10000) {
        candidate -= 0x10000;
    } else if (candidate < reference && reference - candidate > 0x8000 && candidate <= 0xFFFFFFFF - 0x10000) {
        candidate += 0x10000;
    }

    return candidate;
}

/**
 * Reads and validates the header of a received datagram (no authentication)
 *
 * Only checks that the datagram is structurally valid, so a receiver can look up
 * the connection before doing anything expensive. sequence/ack are the low 16 bits.
 *
 * @return bool False if the datagram can't be an envelope of this protocol version
 */
inline
bool net_packet_peek(const byte* const __restrict in, int32 size, NetHeader* const __restrict header) NO_EXCEPT
{
    if (size < NET_HEADER_SIZE || size > NET_MAX_PACKET_SIZE) {
        return false;
    }

    if (!net_header_read(in, size, header)) {
        return false;
    }

    if (header->protocol_version != NET_PROTOCOL_VERSION
        || (header->flags & NET_FLAG_FRAGMENT)
        || header->payload_size > NET_MAX_PAYLOAD_SIZE
    ) {
        return false;
    }

    return net_packet_size(header) == size;
}

/**
 * Nonce of a packet: connection id + full sequence
 *
 * Unique per key since every direction has its own key and the sequence is never reused.
 */
FORCE_INLINE
void net_nonce(byte* const nonce, uint32 connection_id, uint32 sequence) NO_EXCEPT
{
    net_write_u32(nonce, 0);
    net_write_u32(nonce + 4, connection_id);
    net_write_u32(nonce + 8, sequence);
}

/**
 * Creates a datagram
 *
 * The header fields payload_size and the flags NET_FLAG_COMPRESSED/NET_FLAG_ENCRYPTED
 * are set by this function, everything else must be filled by the caller
 * (sequence/ack = full 32 bit values).
 *
 * @param byte*         out         Output (NET_MAX_PACKET_SIZE bytes)
 * @param NetHeader*    header      Header (updated)
 * @param const byte*   payload     Payload
 * @param uint16        size        Payload size (max. NET_MAX_PAYLOAD_SIZE)
 * @param const byte*   key         Encryption key (NET_KEY_SIZE), NULL = unencrypted (handshake only)
 * @param bool          compress    Try to compress the payload
 *
 * @return int32 Datagram size, 0 on failure
 */
inline
int32 net_packet_seal(
    byte* const __restrict out,
    NetHeader* const __restrict header,
    const byte* const __restrict payload,
    uint16 size,
    const byte* const __restrict key,
    bool compress
) NO_EXCEPT
{
    if (size > NET_MAX_PAYLOAD_SIZE) {
        return 0;
    }

    header->protocol_version = NET_PROTOCOL_VERSION;
    header->flags &= (uint8) ~(NET_FLAG_COMPRESSED | NET_FLAG_ENCRYPTED | NET_FLAG_FRAGMENT);

    // The payload is created in a temp buffer, the header size is only known after packing it
    byte body[NET_MAX_PAYLOAD_SIZE];
    uint16 body_size = size;
    const byte* body_data = payload;

    if (compress && size >= NET_COMPRESSION_MIN_SIZE) {
        uint32 table[LZ4_FAST_HASH_SIZE];

        // Only worth it if it's actually smaller
        const uint32 compressed_size = lz4_encode_fast(payload, size, body, size - 1, table);
        if (compressed_size > 0 && compressed_size < size) {
            body_size = (uint16) compressed_size;
            body_data = body;
            header->flags |= NET_FLAG_COMPRESSED;
        }
    }

    header->payload_size = body_size;

    if (key) {
        header->flags |= NET_FLAG_ENCRYPTED;
    }

    const int32 header_size = net_header_write(out, header);
    if (!header_size) {
        return 0;
    }

    byte* const out_body = out + header_size;
    if (body_size) {
        memmove(out_body, body_data, body_size);
    }

    if (key) {
        byte nonce[AEAD_CHACHA20_POLY1305_NONCE_SIZE];
        net_nonce(nonce, header->connection_id, header->sequence);

        aead_chacha20_poly1305_seal(
            key, nonce,
            out, header_size,
            out_body, body_size,
            out_body,
            out_body + body_size
        );
    }

    return net_packet_size(header);
}

/**
 * Opens a datagram
 *
 * @param const byte*   in          Datagram
 * @param int32         size        Datagram size
 * @param NetHeader*    header      Output header
 * @param const byte*   key         Decryption key, NULL = the packet must be unencrypted
 * @param uint32        sequence    Full (reconstructed) sequence of the packet, used for the nonce
 * @param byte*         payload     Output payload (NET_MAX_PAYLOAD_SIZE bytes)
 *
 * @return int32 Payload size, -1 if the datagram is invalid or not authentic
 */
inline
int32 net_packet_open(
    const byte* const __restrict in,
    int32 size,
    NetHeader* const __restrict header,
    const byte* const __restrict key,
    uint32 sequence,
    byte* const __restrict payload
) NO_EXCEPT
{
    if (!net_packet_peek(in, size, header)) {
        return -1;
    }

    const bool encrypted = (header->flags & NET_FLAG_ENCRYPTED) != 0;
    if (encrypted != (key != NULL) || (sequence & 0xFFFF) != header->sequence) {
        return -1;
    }

    header->sequence = sequence;

    const byte* const body = in + NET_HEADER_SIZE;
    const uint16 body_size = header->payload_size;

    byte plain[NET_MAX_PAYLOAD_SIZE];
    const byte* data = body;

    if (encrypted) {
        byte nonce[AEAD_CHACHA20_POLY1305_NONCE_SIZE];
        net_nonce(nonce, header->connection_id, sequence);

        if (!aead_chacha20_poly1305_open(
                key, nonce,
                in, NET_HEADER_SIZE,
                body, body_size,
                body + body_size,
                plain
            )
        ) {
            return -1;
        }

        data = plain;
    }

    int32 result;
    if (header->flags & NET_FLAG_COMPRESSED) {
        result = lz4_decode_safe(data, body_size, payload, NET_MAX_PAYLOAD_SIZE);
    } else {
        if (body_size) {
            memcpy(payload, data, body_size);
        }

        result = body_size;
    }

    if (encrypted) {
        ct_memzero(plain, body_size);
    }

    return result;
}

#endif
