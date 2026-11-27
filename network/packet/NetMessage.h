/**
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_NETWORK_PACKET_NET_MESSAGE_H
#define COMS_NETWORK_PACKET_NET_MESSAGE_H

#include "../../stdlib/Stdlib.h"
#include "NetPacket.h"

/**
 * Application message header
 *
 * First bytes of every envelope payload (see NetPacket.h) of the request/response
 * protocols (auth, chat, server to server). The envelope already identifies the
 * connection, so the message header only describes the message itself.
 *
 * Layout (8 bytes, little endian):
 *      offset  size  field
 *      0       1     type              NetMessageType
 *      1       1     operation         application specific operation
 *      2       4     request_id        echoed by the response, 0 for notifications
 *      6       2     status            application specific status (responses only)
 *      8       ...   message payload (packet specific flags are part of the packet)
 *
 * The application protocol version is not part of every message, it is checked once when
 * the session is created (e.g. login challenge, chat connect).
 */

#define NET_MESSAGE_HEADER_SIZE 8
#define NET_MAX_MESSAGE_SIZE (NET_MAX_PAYLOAD_SIZE - NET_MESSAGE_HEADER_SIZE)

enum NetMessageType : uint8 {
    NET_MESSAGE_REQUEST = 1,
    NET_MESSAGE_RESPONSE = 2,

    // Server initiated, no request id
    NET_MESSAGE_NOTIFY = 3,
};

// The wire layout is written explicitly (net_message_header_write()), the struct has padding
struct NetMessageHeader {
    uint8 type;
    uint8 operation;

    uint32 request_id;

    // @question We may move this to the payload since it might not be usefull for every packet
    int16 status;
};

/**
 * A complete request message and its request id
 *
 * Written with net_message_write() into data (type NET_MESSAGE_REQUEST, size = its result),
 * sent with net_client_request() (NetClient.h).
 */
struct NetRequestPayload {
    // Set by the caller (net_client_next_id()) and written into the header, never 0
    uint32 request_id;

    // Size of the message in data
    uint16 size;

    // NetMessageHeader + binary request payload
    byte data[NET_MAX_PAYLOAD_SIZE];
};

#endif
