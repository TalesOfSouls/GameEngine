/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_MODELS_CHAT_TYPE_H
#define COMS_MODELS_CHAT_TYPE_H

#include "../../stdlib/Types.h"

// NOT the message type, just where the message is sent to/from
enum ChatType : uint16 {
    CHAT_TYPE_MIXED = 1 << 0,
    CHAT_TYPE_LOCAL = 1 << 1,
    CHAT_TYPE_GLOBAL = 1 << 2,
    CHAT_TYPE_SERVER = 1 << 3,
    CHAT_TYPE_PLAYER = 1 << 4,
    CHAT_TYPE_GROUP = 1 << 5,
    CHAT_TYPE_GUILD = 1 << 6,
    CHAT_TYPE_AUCTION_HOUSE = 1 << 7,
    CHAT_TYPE_TRADE = 1 << 8, // When trading with someone
    CHAT_TYPE_PRIVATE = 1 << 9, // When chatting with someone in private (DM)
    CHAT_TYPE_FRIENDS = 1 << 10,
    CHAT_TYPE_CHAT_ROOM = 1 << 11, // Specific chat channels
    CHAT_TYPE_CHAT_LFG = 1 << 12, // LFG finder
    CHAT_TYPE_PVP = 1 << 13, // Also Guild vs. Guild
};

#endif