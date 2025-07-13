/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_GPUAPI_OPENGL_DESCRIPTOR_SET_LAYOUT_BINDING_H
#define COMS_GPUAPI_OPENGL_DESCRIPTOR_SET_LAYOUT_BINDING_H

#include "../../stdlib/Types.h"

struct OpenglDescriptorSetLayoutBinding {
    int32 binding;
    const char* name;
};

#endif