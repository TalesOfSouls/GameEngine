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
    // @question Why am I not using WindowState here as well e.g. state_current
    //          I only use it in state_old
    // Logical dimension
    uint16 logical_width;
    uint16 logical_height;

    // Physical dimension (a monitor may have more pixels than used in software e.g. retina)
    uint16 physical_width;
    uint16 physical_height;

    uint16 x;
    uint16 y;

    // WindowStateChanges
    byte state_changes;

    // WindowsStateFlag
    byte state_flag;

    byte dpi;

    HWND hwnd;

    // This can only be used depending on the render api
    // Usually this gets invalidated in software rendering
    HDC hdc;

    HINSTANCE hInstance;

    // @bug This should only be available on opengl.
    // The problem is the main program doesn't know which gpuapi we are using, so maybe a void pointer?
    // If we do this here than we also must do SoftwareRenderer here, no?
    HGLRC openGLRC;

    WindowState state_old;

    const char* name;
};

FORCE_INLINE
void window_backup_state(Window* w) NO_EXCEPT
{
    w->state_old.style = GetWindowLongPtr(w->hwnd, GWL_STYLE);
    w->state_old.logical_width = w->logical_width;
    w->state_old.logical_height = w->logical_height;
    w->state_old.physical_width = w->physical_width;
    w->state_old.physical_height = w->physical_height;
    w->state_old.x = w->x;
    w->state_old.y = w->y;
}

FORCE_INLINE
void window_restore_state(Window* w) NO_EXCEPT
{
    w->logical_width = w->state_old.logical_width;
    w->logical_height = w->state_old.logical_height;
    w->physical_width = w->state_old.physical_width;
    w->physical_height = w->state_old.physical_height;
    w->x = w->state_old.x;
    w->y = w->state_old.y;
}

#endif