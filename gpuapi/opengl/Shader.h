/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_GPUAPI_OPENGL_SHADER_H
#define COMS_GPUAPI_OPENGL_SHADER_H

#include "../../stdlib/Types.h"
#include "OpenglDescriptorSetLayoutBinding.h"

struct Shader {
    uint32 id;

    OpenglDescriptorSetLayoutBinding descriptor_set_layout[7];

    byte data[16];
};

#endif