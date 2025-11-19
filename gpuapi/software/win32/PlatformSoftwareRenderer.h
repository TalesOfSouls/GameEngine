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

#include "../../../stdlib/Types.h"
#include <windows.h>

struct PlatformSoftwareRenderer {
    HDC window_hdc;
    HDC hdc;

    HWND hwnd;
    HBITMAP dib;
    BITMAPINFO bmi;
};

#endif