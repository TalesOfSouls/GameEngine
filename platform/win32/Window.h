/**
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_PLATFORM_WIN32_WINDOW_H
#define COMS_PLATFORM_WIN32_WINDOW_H

#include "../../stdlib/Stdlib.h"
#include <WinUser.h>

typedef HINSTANCE WindowInstance;

struct WindowPlatform {
    HWND hwnd;

    // This can only be used depending on the render api
    // Usually this gets invalidated in software rendering
    HDC hdc;

    HINSTANCE hInstance;
};

#endif