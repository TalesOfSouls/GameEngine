#ifndef COMS_NETWORK_PACKET_MOB_STATE_H
#define COMS_NETWORK_PACKET_MOB_STATE_H

#include <stdio.h>

#include "../../../stdlib/Types.h"

struct MobStatePacketSnapshot {
    byte* data;
};

struct MobStatePacketSnapshotUnpacked {
    uint32 mob_id;
    byte mob_type;
    uint32 chunk;

    f16 x;
    f16 y;
    f16 z;

    f16 roll;
    f16 pitch;
    f16 yaw;

    uint32 state_flag;

    uint64 time;
};

struct MobStatePacketDelta {
    byte* data;
};

struct MobStatePacketDeltaUnpacked {
    uint32 mob_id;
    byte mob_type;
    uint32 chunk;

    f16 x;
    f16 y;
    f16 z;

    f16 roll;
    f16 pitch;
    f16 yaw;

    uint32 state_flag;

    uint64 time;
};

#endif