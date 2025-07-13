/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_GPUAPI_VULKAN_SHADER_H
#define COMS_GPUAPI_VULKAN_SHADER_H

#include "../../stdlib/Types.h"
#include <vulkan/vulkan.h>

struct Shader {
    // @todo Consider to rename to pipeline (also in opengl, directx) if this makes sense.
    // @question Does this have to be a pointer to the gpuapi->pipeline var?
    VkPipeline id;
    uint32 locations[7];
    byte data[16];
};

#endif