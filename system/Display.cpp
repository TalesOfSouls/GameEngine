/**
 * Display mode of a window: windowed, windowed fullscreen (borderless) or fullscreen
 *
 * The platform dependent parts are in Window.cpp (window styles) and SystemInfo.cpp (monitors/resolutions)
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_SYSTEM_DISPLAY_C
#define COMS_SYSTEM_DISPLAY_C

#include "../stdlib/Stdlib.h"
#include "Display.h"
#include "Window.h"
#include "Window.cpp"
#include "SystemInfo.cpp"
#include "../log/Log.h"

/**
 * Desktop rectangle of a monitor as window rectangle
 *
 * @param const DisplayMonitor* monitor Monitor
 *
 * @return v4_int32
 */
FORCE_INLINE
v4_int32 display_monitor_rect(const DisplayMonitor* const monitor) NO_EXCEPT
{
    return { monitor->x, monitor->y, monitor->width, monitor->height };
}

/**
 * Finds the closest entry of _display_resolutions
 *
 * @param int32 width   Width
 * @param int32 height  Height
 *
 * @return int32 Index in _display_resolutions
 */
static inline
int32 display_resolution_closest(int32 width, int32 height) NO_EXCEPT
{
    int32 best = 0;
    int32 best_distance = 0x7FFFFFFF;
    for (int32 i = 0; i < ARRAY_COUNT(_display_resolutions); ++i) {
        const int32 distance = abs(_display_resolutions[i].x - width)
            + abs(_display_resolutions[i].y - height);

        if (distance < best_distance) {
            best = i;
            best_distance = distance;
        }
    }

    return best;
}

/**
 * Restores the desktop resolution of a monitor whose resolution was changed by the fullscreen mode
 *
 * @param Window* window Window (Window::display)
 *
 * @return void
 */
static inline
void display_exclusive_end(Window* const window) NO_EXCEPT
{
    DisplayState* const display = &window->display;
    if (!display->exclusive) {
        return;
    }

    if (!display->suspended) {
        display_resolution_restore(display->device);
    }

    display->exclusive = false;
    display->suspended = false;
}

/**
 * Applies a display mode to a window
 *
 * Must be called on the thread of the window (the window messages of SetWindowPos are sent synchronously)
 *
 * @param Window*       window          Window (the fullscreen state is stored in Window::display)
 * @param DisplayMode   mode            Display mode
 * @param int32         monitor_index   Index of the monitor (same order as display_info_get), -1 = primary
 * @param int32         width           Client width (windowed) or monitor resolution (fullscreen), 0 = monitor width
 * @param int32         height          Client height (windowed) or monitor resolution (fullscreen), 0 = monitor height
 *
 * @return void
 */
static inline
void display_apply(
    Window* const window,
    DisplayMode mode,
    int32 monitor_index,
    int32 width, int32 height
) NO_EXCEPT
{
    if (!window->platform_window || !((WindowPlatform *) window->platform_window)->hwnd) {
        return;
    }

    DisplayMonitor monitor;
    if (!display_monitor_get(monitor_index, &monitor)
        && !display_monitor_get(-1, &monitor)
    ) {
        LOG_1("[ERROR] Display mode: no monitor found");

        return;
    }

    if (!width) {
        width = monitor.width;
    }

    if (!height) {
        height = monitor.height;
    }

    // A maximized window ignores the new position/size
    window_unmaximize(window);

    // The old fullscreen resolution is always reverted first (other monitor, resolution or mode)
    display_exclusive_end(window);

    DisplayState* const display = &window->display;
    switch (mode) {
        case DISPLAY_MODE_FULLSCREEN: {
            if (display_resolution_set(monitor.name, width, height)) {
                memcpy(display->device, monitor.name, sizeof(display->device));
                display->width = (uint16) width;
                display->height = (uint16) height;
                display->exclusive = true;
                display->suspended = false;

                // The monitor has the new resolution now
                DisplayMonitor changed;
                if (display_monitor_by_name(monitor.name, &changed)) {
                    monitor = changed;
                }
            } else {
                LOG_1(
                    "[WARNING] Display mode: the monitor doesn't support %d x %d, using windowed fullscreen",
                    {DATA_TYPE_INT32, (void *) &width}, {DATA_TYPE_INT32, (void *) &height}
                );
            }

            window_borderless_apply(window, display_monitor_rect(&monitor));
        } break;
        case DISPLAY_MODE_BORDERLESS: {
            window_borderless_apply(window, display_monitor_rect(&monitor));
        } break;
        default: {
            window_windowed_apply(window, display_monitor_rect(&monitor), width, height);
        } break;
    }

    // SetWindowPos sends WM_SIZE (SIZE_RESTORED) synchronously, which would be interpreted as a fullscreen toggle
    window->state_changes &= ~WINDOW_STATE_CHANGE_FULLSCREEN;
    window->state_changes |= WINDOW_STATE_CHANGE_SIZE | WINDOW_STATE_CHANGE_POS;

    window_resolution_update(window);
}

/**
 * Fullscreen mode: restores the desktop resolution if the window loses the focus (+ minimizes the window)
 * and changes the resolution again if it gets the focus back
 *
 * @param Window*       window  Window (Window::display)
 * @param bool          focused The window has the focus
 *
 * @return void
 */
static inline
void display_focus_update(Window* const window, bool focused) NO_EXCEPT
{
    DisplayState* const display = &window->display;
    if (!display->exclusive) {
        return;
    }

    if (!focused && !display->suspended) {
        display_resolution_restore(display->device);
        display->suspended = true;
        window_minimize(window);
    } else if (focused && display->suspended) {
        display->suspended = false;

        if (!display_resolution_set(display->device, display->width, display->height)) {
            display->exclusive = false;

            return;
        }

        // The window covers the monitor with the changed resolution
        DisplayMonitor monitor;
        if (display_monitor_by_name(display->device, &monitor)) {
            window_borderless_apply(window, display_monitor_rect(&monitor));

            window->state_changes &= ~WINDOW_STATE_CHANGE_FULLSCREEN;
            window->state_changes |= WINDOW_STATE_CHANGE_SIZE | WINDOW_STATE_CHANGE_POS;
        }
    }
}

#endif
