/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef TOS_UTILS_WINDOW_H
#define TOS_UTILS_WINDOW_H

#include <windows.h>
#include "../../stdlib/Types.h"

struct WindowState {
    uint64 style;
    int32 width;
    int32 height;

    int32 x;
    int32 y;
};

struct Window {
    bool is_fullscreen;
    int32 width;
    int32 height;

    int32 x;
    int32 y;

    bool mouse_captured;

    HWND hwnd;
    HDC hdc;

    char name[32];
    WindowState state_old;
};

inline
void window_backup_state(Window* __restrict w)
{
    w->state_old.style = GetWindowLongPtr(w->hwnd, GWL_STYLE);
    w->state_old.width = w->width;
    w->state_old.height = w->height;
    w->state_old.x = w->x;
    w->state_old.y = w->y;
}

inline
void window_restore_state(Window* __restrict w)
{
    w->width = w->state_old.width;
    w->height = w->state_old.height;
    w->x = w->state_old.x;
    w->y = w->state_old.y;
}

#endif