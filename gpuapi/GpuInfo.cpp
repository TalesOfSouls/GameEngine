/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_GPUAPI_GPU_INFO_C
#define COMS_GPUAPI_GPU_INFO_C

#include "CpuInfo.h"

#if defined(OPENGL)
    #include "opengl/GpuInfo.cpp"
#elif defined(DIRECTX)
    #include "direct3d/GpuInfo.cpp"
#elif defined(VULKAN)
    #include "vulkan/GpuInfo.cpp"
#elif defined(SOFTWARE)
    #include "soft/GpuInfo.cpp"
#endif

#endif