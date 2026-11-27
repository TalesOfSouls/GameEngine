/**
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_NETWORK_PACKET_NET_PACKET_H
#define COMS_NETWORK_PACKET_NET_PACKET_H

#include "../../stdlib/Stdlib.h"
#include "../../encryption/ChaCha20Poly1305.h"

/**
 * Network envelope
 *
 * Every UDP datagram of the engine (auth, chat, game, server to server) is one
 * envelope. The envelope provides what UDP doesn't:
 *
 *      - connection id         identifies the connection independent of the ip/port (NAT rebinding)
 *      - sequence              per connection and direction, also the encryption nonce
 *      - ack + ack_bits        piggybacked acknowledgements of the 33 latest received packets
 *      - channel               unreliable / reliable / control (reliable packets are delivered in
 *                              the order of arrival, the application orders its data if needed)
 *      - compression           LZ4, only used if it makes the payload smaller
 *      - encryption            ChaCha20-Poly1305, the header is authenticated as associated data
 *
 * The header is bit packed (15 bytes):
 *      bits  field
 *      4     protocol_version  NET_PROTOCOL_VERSION
 *      5     flags             NET_FLAG_* (wire flags only)
 *      2     channel           NetChannel (handshake packets: NetHandshakeType)
 *      11    payload_size      payload bytes on the wire (after compression, without tag)
 *      32    connection_id     0 for the handshake request
 *      16    sequence          low 16 bits, the receiver reconstructs the full 32 bit sequence
 *      16    ack               low 16 bits of a received sequence of the other side
 *      32    ack_bits          bit i = sequence (ack - 1 - i) was received
 *      ...   padding to the next byte
 *
 *      payload                 encrypted (if NET_FLAG_ENCRYPTED)
 *      tag (16 bytes)          only if NET_FLAG_ENCRYPTED
 *
 * Sequence reconstruction: the full sequence is the value with these low 16 bits
 * that is closest to the highest received sequence (the replay window is much
 * smaller than 2^15). The full sequence is used for the nonce, a wrong
 * reconstruction can't decrypt.
 *
 * Processing order:
 *      send:    payload -> compress -> encrypt (header = associated data) -> send
 *      receive: verify + decrypt -> decompress -> payload
 *
 * Only the handshake (see NetConnection.h) is sent without encryption. After the
 * handshake every packet must be encrypted, otherwise it is dropped.
 */

// 2: no reliable ordered channel (and no order field) anymore
#define NET_PROTOCOL_VERSION 1

#define NET_HEADER_BITS 118
#define NET_HEADER_SIZE ((NET_HEADER_BITS + 7) / 8)

#define NET_TAG_SIZE AEAD_CHACHA20_POLY1305_TAG_SIZE
#define NET_KEY_SIZE AEAD_CHACHA20_POLY1305_KEY_SIZE

// A datagram must stay below the path MTU (IPv6 minimum MTU is 1280, typical MTU 1500)
#define NET_MAX_PACKET_SIZE 1400
#define NET_MAX_PAYLOAD_SIZE (NET_MAX_PACKET_SIZE - NET_HEADER_SIZE - NET_TAG_SIZE)

#define NET_PAYLOAD_SIZE_BITS 11
static_assert(NET_MAX_PAYLOAD_SIZE < (1 << NET_PAYLOAD_SIZE_BITS), "payload_size doesn't fit into its header field");

// Smaller payloads are never compressed (LZ4 can't gain anything meaningful)
#define NET_COMPRESSION_MIN_SIZE 64

enum NetFlag : uint8 {
    // Flags on the wire (5 bits)
    NET_FLAG_COMPRESSED = 1 << 0,
    NET_FLAG_ENCRYPTED = 1 << 1,
    NET_FLAG_ACK_ONLY = 1 << 2,

    // Reserved for payloads > NET_MAX_PAYLOAD_SIZE, not implemented -> such packets are dropped
    NET_FLAG_FRAGMENT = 1 << 3,

    NET_FLAG_HANDSHAKE = 1 << 4,

    // Not on the wire, derived from the channel
    NET_FLAG_RELIABLE = 1 << 7,
};

#define NET_WIRE_FLAGS 0x1F

enum NetChannel : uint8 {
    // Fire and forget (e.g. position snapshots, the next one replaces it anyway)
    NET_CHANNEL_UNRELIABLE = 0,

    // Re-sent until acknowledged, delivered in the order of arrival
    NET_CHANNEL_RELIABLE = 1,

    // Connection control (disconnect), payload is one NetControlType byte
    NET_CHANNEL_CONTROL = 2,

    NET_CHANNEL_COUNT,
};

enum NetControlType : uint8 {
    NET_CONTROL_DISCONNECT = 1,
};

/**
 * Unpacked header
 *
 * sequence/ack are the full 32 bit values when writing. After reading they only
 * contain the low 16 bits until the connection reconstructs them.
 */
struct NetHeader {
    uint8 protocol_version;

    // @type NetFlag
    uint8 flags;

    // @type NetChannel
    uint8 channel;

    uint32 connection_id;

    // id of this package
    uint32 sequence;

    // last id we received
    uint32 ack;

    // bits of packages relative to ack, which we received
    uint32 ack_bits;

    uint16 payload_size;
};

#endif
