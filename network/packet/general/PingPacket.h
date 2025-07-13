#ifndef COMS_NETWORK_PACKET_GENERAL_PING_H
#define COMS_NETWORK_PACKET_GENERAL_PING_H

#include <stdio.h>

#include "../../../stdlib/Types.h"

// The default ping package is an empty package
struct PingPackage {};

#pragma pack(push, 1)
struct __attribute__ ((__packed__)) TimedPingPackage {
    uint8 msg_type;
    uint8 subtype;
    uint64 time;
};
#pragma pack(pop)

#endif