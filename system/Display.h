/**
 * Display mode of a window: windowed, windowed fullscreen (borderless) or fullscreen
 *
 * Fullscreen = borderless window + the resolution of the monitor is changed.
 * The desktop resolution is restored when the window loses the focus (the window is minimized) and the fullscreen
 * resolution is set again when it gets the focus back.
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_SYSTEM_DISPLAY_H
#define COMS_SYSTEM_DISPLAY_H

#include "../stdlib/Stdlib.h"

enum DisplayMode : byte {
    // Normal window with borders
    DISPLAY_MODE_WINDOWED = 1,

    // Borderless window that covers the monitor (native resolution of the monitor)
    DISPLAY_MODE_BORDERLESS = 2,

    // Borderless window + the monitor resolution is changed (restored when the window loses the focus)
    DISPLAY_MODE_FULLSCREEN = 3,
};

// Common monitor resolutions, sorted by pixel count (e.g. resolution slider of the settings)
static const v2_int32 _display_resolutions[] = {
    {1280, 720},    // HD
    {1280, 800},    // WXGA 16:10
    {1366, 768},    // WXGA
    {1600, 900},    // HD+
    {1920, 1080},   // Full HD
    {1920, 1200},   // WUXGA 16:10
    {2560, 1080},   // UW-FHD (21:9)
    {2560, 1440},   // QHD
    {2560, 1600},   // WQXGA 16:10
    {3440, 1440},   // UWQHD (21:9)
    {3840, 2160},   // 4K UHD
    {5120, 1440},   // DQHD (32:9)
    {5120, 2160},   // 5K2K (21:9)
    {5120, 2880},   // 5K
    {7680, 2160},   // DUHD (32:9)
    {6016, 3384},   // 6K
    {7680, 4320},   // 8K UHD
};

struct DisplayState {
    // Name of the monitor whose resolution was changed (name)
    // only valid if exclusive
    char device[48];

    // Resolution of the exclusive fullscreen mode
    uint16 width;
    uint16 height;

    // The resolution of the monitor is changed (DISPLAY_MODE_FULLSCREEN)
    bool exclusive;

    // The exclusive mode is temporarily reverted because the window lost the focus
    bool suspended;
};

#endif
