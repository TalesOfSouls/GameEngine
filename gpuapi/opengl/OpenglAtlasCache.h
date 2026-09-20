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
#include "../GpuAssetType.h"
#include "Pipeline.h"

// Per cache
struct OpenglAtlasCache {
    uint32 fbo;
    OpenGLTextureData texture;
};

// All caches use the same pipeline
struct OpenglAtlasBlitter {
    Pipeline blit_pipeline;

    uint32 blit_vao;
    uint32 blit_vbo;
};

#endif