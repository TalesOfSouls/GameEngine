/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_GPUAPI_SOFTWARE_SHADER_H
#define COMS_GPUAPI_SOFTWARE_SHADER_H

#include "../../stdlib/Stdlib.h"
#include "../../object/Vertex.h"
#include "SoftwareDescriptorSetLayoutBinding.h"

// Unfortunately we have to forward declare this since:
//      The shader func needs this struct
//      The SoftwareRenderer needs the Shader struct
// This results in a circular dependency
struct SoftwareRenderer;

typedef void (*SoftShaderFunc)(
    const SoftwareRenderer* const __restrict renderer,
    int32 data_index,
    int32 instance_index,
    void* __restrict data,
    int32 data_count,
    const uint32* __restrict data_indices,
    int32 data_index_count,
    void* __restrict instance_data,
    int32 instance_data_count,
    int32 steps
) NO_EXCEPT;

struct Shader {
    uint32 id;

    SoftwareDescriptorSetLayoutBinding descriptor_set_layout[12];

    int32 shader_count;
    SoftShaderFunc shader_functions[4];

    byte data[16];
};

// In the shader we very often need a camera object for vertex manipulation
struct ShaderCamera {
    f32 projection[16];
    f32 orth[16];
    f32 view[16];
};

#endif