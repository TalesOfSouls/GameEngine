/**
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_GPUAPI_OPENGL_ATLAS_CACHE_H
#define COMS_GPUAPI_OPENGL_ATLAS_CACHE_H

#include "../../stdlib/Stdlib.h"
#include "../GpuAtlasCache.h"
#include "Pipeline.h"

struct OpenglAtlasCache {
    GpuAtlasCache cache;

    uint32 fbo;
    uint32 texture_id;

    Pipeline blit_pipeline;

    uint32 blit_vao;
    uint32 blit_vbo;
};

#endif