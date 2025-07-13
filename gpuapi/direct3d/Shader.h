/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_GPUAPI_DIRECT3D_SHADER_H
#define COMS_GPUAPI_DIRECT3D_SHADER_H

#include "../../stdlib/Types.h"
#include <d3d12.h>

struct Shader {
    ID3D12PipelineState* id;
    uint32 locations[7];
    byte data[16];
};

#endif