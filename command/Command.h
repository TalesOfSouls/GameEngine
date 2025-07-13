/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_COMMAND_H
#define COMS_COMMAND_H

#include "../stdlib/Types.h"

// @question Consider to rename internal enum values to contain the word INTERNAL
enum CommandType {
    CMD_FUNC_RUN,
    CMD_ASSET_ENQUEUE,
    CMD_ASSET_LOAD,
    CMD_FILE_LOAD,
    CMD_FONT_LOAD,
    CMD_FONT_CREATE,
    CMD_TEXTURE_LOAD,
    CMD_TEXTURE_CREATE, // Only for internal use
    CMD_AUDIO_PLAY,
    CMD_AUDIO_ENQUEUE, // Only for internal use
    CMD_SHADER_LOAD,
    CMD_UI_LOAD,
};

typedef void* (*CommandFunction)(void* data);

struct Command {
    CommandType type;
    CommandFunction callback;
    byte data[256]; // @todo to be adjusted
};

#endif