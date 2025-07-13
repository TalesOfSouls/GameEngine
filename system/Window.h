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

struct WindowState {
    uint16 width;
    uint16 height;

    uint16 x;
    uint16 y;
    uint64 style;
};

enum WindowStateChanges : byte {
    WINDOW_STATE_CHANGE_NONE = 0,
    WINDOW_STATE_CHANGE_SIZE = 1,
    WINDOW_STATE_CHANGE_POS = 2,
    WINDOW_STATE_CHANGE_FOCUS = 4,
    WINDOW_STATE_CHANGE_FULLSCREEN = 8,
    WINDOW_STATE_CHANGE_ALL = 16,
};

#endif