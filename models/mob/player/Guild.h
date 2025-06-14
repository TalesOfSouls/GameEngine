/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_MODELS_GUILD_H
#define COMS_MODELS_GUILD_H

#include "../../../stdlib/Types.h"

#ifndef MAX_CHAR_NAME_LENGTH
    #define MAX_CHAR_NAME_LENGTH 32
#endif

#if SERVER
struct SGuild {
    int id = 0;
    char name[MAX_CHAR_NAME_LENGTH];

    byte rank = 0x00;
};
#endif

// @performance The client should cache the guild names -> avoid sending guild names through network
struct CGuild {
    int id = 0;
    char name[MAX_CHAR_NAME_LENGTH];

    byte rank = 0x00;
};

#endif