#ifndef COMS_NETWORK_PACKET_CHAT_MESSAGE_H
#define COMS_NETWORK_PACKET_CHAT_MESSAGE_H

#include <stdio.h>

#include "../../../stdlib/Types.h"

struct ChatMessagePacket {
    byte* data; // fixed 8+2+?
};

struct ChatMessagePacketUnpacked {
    uint32 from;
    uint32 to;

    byte type; // 2^3 Global, Server, Player, Group, Guild, Local
    byte level; // 2^2 Normal, info (grey), important (yellow), critical (red)

    uint16 length; // 2^9
    char* message; // Max Length: MAX_MESSAGE_LENGTH
};

#endif