/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_MODELS_CHAT_H
#define COMS_MODELS_CHAT_H

#include "../../stdlib/Types.h"
#include "ChatLevel.h"
#include "ChatType.h"

#ifndef MAX_CHAR_NAME_LENGTH
    #define MAX_CHAR_NAME_LENGTH 32
#endif

#ifndef MAX_MESSAGE_LENGTH
    #define MAX_MESSAGE_LENGTH 512
#endif

#ifndef MAX_MESSAGE_HISTORY
    #define MAX_MESSAGE_HISTORY 64
#endif

struct ChatMessage {
    uint64 id;

    unsigned int sender_id;
    unsigned int receiver_id;
    char sender_name[MAX_CHAR_NAME_LENGTH];

    time_t dt;

    char message[MAX_MESSAGE_LENGTH];
    ChatLevel level;
    ChatType type;
};

// Messages are stored in a ring
//      -> latest = the newest message
//      -> previous message = latest - 1
//      -> This makes iterating messages easy since we also know where the chat history memory block starts
struct ChatHistory {
    ChatMessage messages[MAX_MESSAGE_HISTORY];
    int latest;
};

#endif