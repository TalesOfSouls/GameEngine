/**
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_NETWORK_PACKET_NET_BATCH_H
#define COMS_NETWORK_PACKET_NET_BATCH_H

#include "../../stdlib/Stdlib.h"
#include "../../utils/BitUtils.h"
#include "NetPacket.h"

/**
 * Batch of bit packed game messages in one envelope payload
 *
 * High frequency game data (mob states, deltas, ...) consists of many small
 * messages. Every datagram costs the envelope header, the tag and the UDP/IP
 * header, so as many messages as possible are packed into one datagram.
 *
 * Layout (bit stream, see utils/BitUtils.h):
 *      32 bits     tick        simulation tick/time of all messages in the batch
 *      per message:
 *          1 bit   more        1 = a message follows
 *          8 bits  type        message type (e.g. PACKET_TYPE_* of packet_types.h)
 *          ...     body        net_packet_serialize() of the message
 *      1 bit       end         0
 *
 * The messages have no length, the receiver must know every type (both sides
 * have the same version). An unknown type or an invalid body stops the parsing of
 * the batch.
 *
 * @example
 *      NetBatchWriter batch;
 *      net_batch_begin(&batch, payload, NET_MAX_PAYLOAD_SIZE, tick);
 *      for (...) {
 *          if (!net_batch_add(&batch, PACKET_TYPE_MOB_STATE_SNAPSHOT, &snapshot)) {
 *              send(payload, net_batch_end(&batch));          // full -> send and start a new batch
 *              net_batch_begin(&batch, payload, NET_MAX_PAYLOAD_SIZE, tick);
 *              net_batch_add(&batch, PACKET_TYPE_MOB_STATE_SNAPSHOT, &snapshot);
 *          }
 *      }
 *      send(payload, net_batch_end(&batch));
 *
 *      NetBatchReader reader;
 *      net_batch_read_begin(&reader, payload, size);
 *      uint8 type;
 *      while (net_batch_next(&reader, &type)) {
 *          switch (type) {
 *              case PACKET_TYPE_MOB_STATE_SNAPSHOT: {
 *                  MobStateSnapshot snapshot;
 *                  if (!net_batch_read(&reader, &snapshot)) { break; }
 *                  ...
 *              } break;
 *              default: net_batch_stop(&reader);
 *          }
 *      }
 */

#define NET_BATCH_TYPE_BITS 8

struct NetBatchWriter {
    BitWriter bits;
    uint32 count;
};

struct NetBatchReader {
    BitReader bits;
    uint32 tick;
    uint32 count;
    bool done;
};

#endif
