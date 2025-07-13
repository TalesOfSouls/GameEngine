#ifndef COMS_NETWORK_PACKET_MOB_INFO_H
#define COMS_NETWORK_PACKET_MOB_INFO_H

#include <stdio.h>

#include "../../../stdlib/Types.h"

struct MobInfoPacketSnapshot {
    byte* data;
};

struct MobInfoPacketSnapshotUnpacked {
    uint32 mob_id;
    byte mob_type;
    uint32 chunk;

    byte level; // 2^7
    byte health; // 2^7
    byte resource; // 2^7
    byte xp;

    // Data layout
    // 12223444
    //      1: scale sign
    //      2: scale factor (8)
    //      3: weight sign
    //      4: weight factor (8)
    byte custom_size;

    // Data layout
    // 1122222?
    //      1: body type (4)
    //      2: skin color (32)
    byte custom_body;

    uint64 time;
};

#endif