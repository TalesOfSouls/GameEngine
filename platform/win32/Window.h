/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_PLATFORM_WIN32_WINDOW_H
#define COMS_PLATFORM_WIN32_WINDOW_H

#include <windows.h>
#include "../../stdlib/Types.h"
#include "../../compiler/CompilerUtils.h"
#include "../../system/Window.h"

typedef HINSTANCE WindowInstance;

struct Window {
    // @question Should we implement a virtual width/height
    //  e.g. I think apple has that where physical resolution < virtual resolution?!
    uint16 width;
    uint16 height;

    uint16 x;
    uint16 y;

    // WindowStateChanges
    byte state_changes;

    // WindowsStateFlag
    byte state_flag;

    HWND hwnd;
    HDC hdc;
    HINSTANCE hInstance;

    // @bug This should only be available on opengl.
    // The problem is the main program doesn't know which gpuapi we are using, so maybe a void pointer?
    HGLRC openGLRC;

    WindowState state_old;

    const char* name;
};

FORCE_INLINE
void window_backup_state(Window* w) NO_EXCEPT
{
    w->state_old.style = GetWindowLongPtr(w->hwnd, GWL_STYLE);
    w->state_old.width = w->width;
    w->state_old.height = w->height;
    w->state_old.x = w->x;
    w->state_old.y = w->y;
}

FORCE_INLINE
void window_restore_state(Window* w) NO_EXCEPT
{
    w->width = w->state_old.width;
    w->height = w->state_old.height;
    w->x = w->state_old.x;
    w->y = w->state_old.y;
}

#endif