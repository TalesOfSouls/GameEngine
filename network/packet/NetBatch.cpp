/**
 * Batches of bit packed game messages (types: NetBatch.h)
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_NETWORK_PACKET_NET_BATCH_C
#define COMS_NETWORK_PACKET_NET_BATCH_C

#include "../../stdlib/Stdlib.h"
#include "../../utils/BitUtils.h"
#include "NetPacket.cpp"
#include "NetBatch.h"

/**
 * @param byte*     buffer      Output (e.g. NET_MAX_PAYLOAD_SIZE bytes for one envelope)
 * @param uint32    capacity    Buffer size
 * @param uint32    tick        Simulation tick/time of the messages
 */
inline
void net_batch_begin(NetBatchWriter* const __restrict batch, byte* const __restrict buffer, uint32 capacity, uint32 tick) NO_EXCEPT
{
    bit_writer_init(&batch->bits, buffer, capacity);
    batch->count = 0;

    bit_write(&batch->bits, tick, 32);
}

/**
 * Adds a message
 *
 * @return bool False if the message doesn't fit anymore (the batch is unchanged)
 */
template <typename T>
inline
bool net_batch_add(NetBatchWriter* const batch, uint8 type, T* const message) NO_EXCEPT
{
    const BitWriter backup = batch->bits;

    bit_write(&batch->bits, 1, 1);
    bit_write(&batch->bits, type, NET_BATCH_TYPE_BITS);

    const bool valid = net_packet_serialize(&batch->bits, message);

    // The end bit must still fit
    if (!valid
        || batch->bits.overflow
        || bit_writer_bits(&batch->bits) + 1 > batch->bits.capacity * 8
    ) {
        batch->bits = backup;

        return false;
    }

    ++batch->count;

    return true;
}

/**
 * Finishes the batch
 *
 * @return int32 Size in bytes
 */
inline
int32 net_batch_end(NetBatchWriter* const batch) NO_EXCEPT
{
    bit_write(&batch->bits, 0, 1);

    return (int32) bit_writer_flush(&batch->bits);
}

/**
 * @return bool False if the data is too short
 */
inline
bool net_batch_read_begin(NetBatchReader* const __restrict reader, const byte* const __restrict data, uint32 size) NO_EXCEPT
{
    bit_reader_init(&reader->bits, data, size);

    reader->tick = bit_read(&reader->bits, 32);
    reader->count = 0;
    reader->done = reader->bits.overflow;

    return !reader->done;
}

/**
 * Next message type
 *
 * @return bool False at the end of the batch (or on invalid data)
 */
inline
bool net_batch_next(NetBatchReader* const __restrict reader, uint8* const __restrict type) NO_EXCEPT
{
    if (reader->done) {
        return false;
    }

    const uint32 more = bit_read(&reader->bits, 1);
    if (!more || reader->bits.overflow) {
        reader->done = true;

        return false;
    }

    *type = (uint8) bit_read(&reader->bits, NET_BATCH_TYPE_BITS);
    if (reader->bits.overflow) {
        reader->done = true;

        return false;
    }

    return true;
}

/**
 * Reads the body of the current message
 *
 * @return bool False if the body is invalid (the rest of the batch is skipped)
 */
template <typename T>
inline
bool net_batch_read(NetBatchReader* const reader, T* const message) NO_EXCEPT
{
    if (reader->done || !net_packet_serialize(&reader->bits, message)) {
        reader->done = true;

        return false;
    }

    ++reader->count;

    return true;
}

/**
 * Stops the parsing (e.g. unknown message type, the body length is unknown)
 */
FORCE_INLINE
void net_batch_stop(NetBatchReader* const reader) NO_EXCEPT
{
    reader->done = true;
}

#endif
