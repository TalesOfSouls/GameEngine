/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_GPUAPI_OPENGL_H
#define COMS_GPUAPI_OPENGL_H

// Some apis require a different sign for various operations (left/right)
#define GPU_API_SIGN -1

#if _WIN32
    #include "OpenglWin32.h"
#else
    #include "OpenglLinux.h"
#endif

#endif