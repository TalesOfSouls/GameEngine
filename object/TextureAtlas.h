/**
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_MODELS_TEXTUREATLAS_H
#define COMS_MODELS_TEXTUREATLAS_H

#include "Texture.h"

struct TextureAtlasElement {
    int32 uv_count;

    // Offset into "uv" in the TextureAtlas struct
    int32 uv_start;
};

struct TextureAtlas {
    // The data before the elements can be considered header data
    // @performance I hate that this is here. The only place that uses this is the archive_builder
    char texture_name[32];
    Texture* texture;

    int32 element_count;
    int32 uv_count;

    TextureAtlasElement* elements;
    v2_f32* uv;
};

#endif