/**
 * Application message header (types: NetMessage.h)
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_NETWORK_PACKET_NET_MESSAGE_C
#define COMS_NETWORK_PACKET_NET_MESSAGE_C

#include "../../stdlib/Stdlib.h"
#include "NetPacket.cpp"
#include "NetMessage.h"

inline
void net_message_header_write(byte* const __restrict out, const NetMessageHeader* const __restrict header) NO_EXCEPT
{
    out[0] = header->type;
    out[1] = header->operation;

    net_write_u32(out + 2, header->request_id);
    net_write_u16(out + 6, (uint16) header->status);
}

inline
void net_message_header_read(const byte* const __restrict in, NetMessageHeader* const __restrict header) NO_EXCEPT
{
    header->type = in[0];
    header->operation = in[1];

    header->request_id = net_read_u32(in + 2);
    header->status = (int16) net_read_u16(in + 6);
}

/**
 * Writes a message (header + payload)
 *
 * @param byte*         out             Output (NET_MAX_PAYLOAD_SIZE bytes)
 * @param uint8         type            NetMessageType
 * @param uint8         operation       Operation
 * @param uint32        request_id      Request id (echoed by responses)
 * @param int16         status          Status (responses)
 * @param const void*   payload         Binary message payload, e.g. a packed struct (may be NULL)
 * @param uint16        size            Message payload size (max. NET_MAX_MESSAGE_SIZE)
 *
 * @return int32 Message size, 0 if the payload is too large
 */
inline
int32 net_message_write(
    byte* const __restrict out,
    uint8 type,
    uint8 operation,
    uint32 request_id,
    int16 status,
    const void* const __restrict payload,
    uint16 size
) NO_EXCEPT
{
    if (size > NET_MAX_MESSAGE_SIZE) {
        return 0;
    }

    NetMessageHeader header = {};
    header.type = type;
    header.operation = operation;
    header.request_id = request_id;
    header.status = status;

    net_message_header_write(out, &header);

    if (payload && size) {
        memcpy(out + NET_MESSAGE_HEADER_SIZE, payload, size);
    }

    return NET_MESSAGE_HEADER_SIZE + size;
}

/**
 * Reads the message header
 *
 * @param const byte*       in      Envelope payload
 * @param int32             size    Envelope payload size
 * @param NetMessageHeader* header  Output header
 *
 * @return int32 Message payload size, -1 if the message is too small
 */
inline
int32 net_message_read(const byte* const __restrict in, int32 size, NetMessageHeader* const __restrict header) NO_EXCEPT
{
    if (size < NET_MESSAGE_HEADER_SIZE) {
        return -1;
    }

    net_message_header_read(in, header);

    return size - NET_MESSAGE_HEADER_SIZE;
}

/**
 * Copies a fixed size message payload
 *
 * Shorter payloads are zero filled (an older peer may send a shorter struct),
 * longer ones are truncated.
 *
 * @return bool False if the payload is smaller than min_size
 */
inline
bool net_message_payload(
    const byte* const __restrict message,
    int32 available,
    void* const __restrict out,
    uint16 size,
    uint16 min_size
) NO_EXCEPT
{
    if (available < (int32) min_size) {
        return false;
    }

    const int32 copy = available < (int32) size ? available : (int32) size;
    memset(out, 0, size);
    memcpy(out, message + NET_MESSAGE_HEADER_SIZE, copy);

    return true;
}

#endif
