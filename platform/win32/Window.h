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

#include "../../stdlib/Stdlib.h"
#include "../../compiler/CompilerUtils.h"
#include "../../system/Window.h"
#include <WinUser.h>

typedef HINSTANCE WindowInstance;

struct WindowPlatform {
    HWND hwnd;

    // This can only be used depending on the render api
    // Usually this gets invalidated in software rendering
    HDC hdc;

    HINSTANCE hInstance;
};

FORCE_INLINE
void window_backup_state(Window* const w) NO_EXCEPT
{
    memcpy(&w->state_old, &w->state_current, sizeof(w->state_current));
    w->state_old.style = GetWindowLongPtrW(
        ((WindowPlatform *) w->platform_window)->hwnd,
        GWL_STYLE
    );
}

FORCE_INLINE
void window_restore_state(Window* const w) NO_EXCEPT
{
    memcpy(&w->state_current, &w->state_old, sizeof(w->state_current));
}

#endif