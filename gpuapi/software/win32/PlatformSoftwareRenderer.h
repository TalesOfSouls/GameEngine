/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_GPUAPI_SOFTWARE_WIN32_SOFTWARE_RENDERER_H
#define COMS_GPUAPI_SOFTWARE_WIN32_SOFTWARE_RENDERER_H

#include "../../../stdlib/Stdlib.h"
#include <windows.h>

struct PlatformSoftwareRenderer {
    // Cached window hdc
    // WARNING: only allowed if we don't use WM_PAINT
    HDC window_hdc;

    // Memory hdc
    HDC mem_hdc;

    HWND hwnd;
    HBITMAP dib;
    BITMAPINFO bmi;
};

#endif