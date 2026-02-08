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

#include "../stdlib/Stdlib.h"

struct WindowState {
    // Logical dimension
    uint16 logical_width;
    uint16 logical_height;

    // Physical dimension (a monitor may have more pixels than used in software e.g. retina)
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

struct Window {
    WindowState state_current;
    WindowState state_old;

    // WindowStateChanges
    byte state_changes;

    // WindowsStateFlag
    byte state_flag;

    byte dpi;

    const char* name;

    void* platform_window;
    void* gpu_api_context;
};

#endif