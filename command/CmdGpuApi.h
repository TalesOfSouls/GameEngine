/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_APP_COMMAND_BUFFER_GPU_API_H
#define COMS_APP_COMMAND_BUFFER_GPU_API_H

#include "AppCmdBuffer.h"
#include "AppCommand.h"

#if defined(OPENGL)
    #include "../gpuapi/opengl/AppCmdBuffer.h"
#elif defined(VULKAN)
    #include "../gpuapi/vulkan/AppCmdBuffer.h"
#elif defined(DIRECTX)
    #include "../gpuapi/direct3d/AppCmdBuffer.h"
#elif defined(SOFTWARE)
    #include "../gpuapi/software/AppCmdBuffer.h"
#else
    inline void* cmd_shader_load(AppCmdBuffer*, AppCommand*) NO_EXCEPT { return NULL; }
    inline void* cmd_shader_load_sync(AppCmdBuffer*, void*, const int32*, ...) NO_EXCEPT { return NULL; }
#endif

#endif