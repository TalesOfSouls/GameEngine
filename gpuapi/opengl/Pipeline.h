/**
 * This is a shader or maybe alternatively a program/pipeline
 *
 * This is *NOT* a vertex shader, fragment shader etc.
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_GPUAPI_OPENGL_SHADER_H
#define COMS_GPUAPI_OPENGL_SHADER_H

#include "../../stdlib/Stdlib.h"
#include "OpenglDescriptorSetLayoutBinding.h"

// @question Should we rename this to pipeline?
struct Pipeline {
    uint32 id;

    OpenglDescriptorSetLayoutBinding descriptor_set_layout[7];
};

#endif