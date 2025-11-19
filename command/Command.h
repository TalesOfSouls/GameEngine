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
    CMD_INTERNAL_FONT_CREATE,
    CMD_TEXTURE_LOAD,
    CMD_INTERNAL_TEXTURE_CREATE,
    CMD_AUDIO_PLAY,
    CMD_INTERNAL_AUDIO_ENQUEUE,
    CMD_SHADER_LOAD,
    CMD_UI_LOAD,
};

typedef void* (*CommandFunction)(void* data);

// Another name for this concept is event queue and the command below is a generic event
struct Command {
    CommandType type;
    CommandFunction callback;

    // @question Maybe make this a union for different commands e.g.
    // union data { shader_load_data, audio_enqueue_data, .... }
    // This way we would be more specific and the data size would be automatically calculated correctly at compile time
    byte data[256]; // @todo to be adjusted
};

#endif