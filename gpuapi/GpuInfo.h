/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_GPUAPI_GPU_INFO_H
#define COMS_GPUAPI_GPU_INFO_H

#include "../stdlib/Stdlib.h"

// @todo We don't care about a lot of features, consider to remove them if we can get either below 32bit
// or if we eventually overflow 64 bit
enum GPUFeature : uint64 {
    GPU_FEATURE_BINDLESS_TEXTURE = 1ULL << 0,
    GPU_FEATURE_SPARSE_TEXTURE = 1ULL << 1,
    GPU_FEATURE_PERSISTENT_BUFFERS = 1ULL << 2,
    GPU_FEATURE_SHADER_STORAGE_BUFFER_OBJECTS = 1ULL << 3,
    GPU_FEATURE_RAY_TRACING_DXR = 1ULL << 4,
    GPU_FEATURE_RAY_TRACING_VULKANRT = 1ULL << 5,
    GPU_FEATURE_RAY_TRACING_OPTIX = 1ULL << 6,
    GPU_FEATURE_VARIABLE_RATE_SHADING = 1ULL << 7, // VRS
    GPU_FEATURE_COMPRESSION_BC = 1ULL << 8,
    GPU_FEATURE_COMPRESSION_ASTC = 1ULL << 9,
    GPU_FEATURE_COMPRESSION_ETC = 1ULL << 10,
    GPU_FEATURE_ANISOTROPIC_FILTERING = 1ULL << 11,
    GPU_FEATURE_TEXTURE_GATHER = 1ULL << 12,
    GPU_FEATURE_TEXTURE_ARRAYS = 1ULL << 13, // 3D textures, cube map textures
    GPU_FEATURE_INDIRECT_DRAW = 1ULL << 14,
    GPU_FEATURE_OCCLUSION_RENDERING = 1ULL << 15,
    GPU_FEATURE_TRANSFORM_FEEDBACK = 1ULL << 16, // stream output
    GPU_FEATURE_TIMELINE_SEMAPHORES = 1ULL << 17,
    GPU_FEATURE_CROSS_GPU_SHARED_RESOURCES = 1ULL << 18,
    GPU_FEATURE_ASYNC_COMPUTE = 1ULL << 19,
    GPU_FEATURE_RAY_TRACING_ACCEL_BLAS = 1ULL << 20,
    GPU_FEATURE_RAY_TRACING_ACCEL_TLAS = 1ULL << 21,
    GPU_FEATURE_MASH_SHADERS = 1ULL << 22,

    // Types
    GPU_FEATURE_FP16 = 1ULL << 23,
    GPU_FEATURE_FP64 = 1ULL << 24,
    GPU_FEATURE_SHADER_INT16 = 1ULL << 25,
    GPU_FEATURE_SHADER_INT64 = 1ULL << 26,
    GPU_FEATURE_SHADER_BITFIELD = 1ULL << 27,
    GPU_FEATURE_VARIABLE_PRECISION = 1ULL << 28,
};

struct GpuInfo {
    char vendor[16];
    char name[32];
    char api_version[16];
    char shader_version[16];

    // In Mb: VRAM
    uint32 vram;

    // Kinda similar to the cache line size on the cpu
    uint32 alignment;

    uint64 features;
};

#endif