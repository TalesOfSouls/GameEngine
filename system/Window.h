/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_SYSTEM_WINDOW_H
#define COMS_SYSTEM_WINDOW_H

#include "../stdlib/Types.h"

// @question Why do I have this struct and don't use it in the window struct?
struct WindowState {
    uint16 logical_width;
    uint16 logical_height;

    uint16 physical_width;
    uint16 physical_height;

    uint16 x;
    uint16 y;
    uint64 style;
};

enum WindowStateChanges : byte {
    WINDOW_STATE_CHANGE_NONE = 0,
    WINDOW_STATE_CHANGE_SIZE = 1 << 0,
    WINDOW_STATE_CHANGE_POS = 1 << 1,
    WINDOW_STATE_CHANGE_FOCUS = 1 << 2,
    WINDOW_STATE_CHANGE_FULLSCREEN = 1 << 3,
    WINDOW_STATE_CHANGE_ALL = (1 << 4) - 1,
};

enum WindowStateFlag : byte {
    WINDOW_STATE_FLAG_FOCUSED = 1 << 0,
    WINDOW_STATE_FLAG_FULLSCREEN = 1 << 1,
    WINDOW_STATE_FLAG_DIMENSIONLESS = 1 << 2,
};

#endif