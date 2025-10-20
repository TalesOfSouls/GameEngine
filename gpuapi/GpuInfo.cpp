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

#if OPENGL
    #include "opengl/GpuInfo.cpp"
#elif DIRECTX
    #include "direct3d/GpuInfo.cpp"
#elif VULKAN
    #include "vulkan/GpuInfo.cpp"
#endif

#endif