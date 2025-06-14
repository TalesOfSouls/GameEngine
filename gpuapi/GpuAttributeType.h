/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_GPUAPI_ATTRIBUTE_TYPE_H
#define COMS_GPUAPI_ATTRIBUTE_TYPE_H

#include "../stdlib/Types.h"
#include "../compiler/CompilerUtils.h"

enum GpuAttributeType {
    GPU_ATTRIBUTE_TYPE_VERTEX_3D,
    GPU_ATTRIBUTE_TYPE_VERTEX_3D_NORMAL,
    GPU_ATTRIBUTE_TYPE_VERTEX_3D_COLOR,
    GPU_ATTRIBUTE_TYPE_VERTEX_3D_TEXTURE_COLOR,
    GPU_ATTRIBUTE_TYPE_VERTEX_3D_SAMPLER_TEXTURE_COLOR,
    GPU_ATTRIBUTE_TYPE_VERTEX_2D_TEXTURE
};

inline constexpr
int32 gpuapi_attribute_count(GpuAttributeType type)
{
    switch (type) {
        case GPU_ATTRIBUTE_TYPE_VERTEX_3D: {
            return 4;
        };
        case GPU_ATTRIBUTE_TYPE_VERTEX_3D_NORMAL: {
            return 2;
        };
        case GPU_ATTRIBUTE_TYPE_VERTEX_3D_COLOR: {
            return 2;
        };
        case GPU_ATTRIBUTE_TYPE_VERTEX_3D_TEXTURE_COLOR: {
            return 2;
        };
        case GPU_ATTRIBUTE_TYPE_VERTEX_3D_SAMPLER_TEXTURE_COLOR: {
            return 3;
        };
        case GPU_ATTRIBUTE_TYPE_VERTEX_2D_TEXTURE: {
            return 2;
        };
        default:
            UNREACHABLE();
    };
}

#endif