/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_GPUAPI_TYPE_H
#define COMS_GPUAPI_TYPE_H

enum GpuApiType : byte {
    GPU_API_TYPE_SOFTWARE,
    GPU_API_TYPE_OPENGL,
    GPU_API_TYPE_VULKAN,
    GPU_API_TYPE_DIRECTX_11,
    GPU_API_TYPE_DIRECTX_12,
};

#endif