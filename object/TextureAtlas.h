/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_MODELS_TEXTUREATLAS_H
#define COMS_MODELS_TEXTUREATLAS_H

#include "Texture.h"

struct TextureAtlasElement {
    int32 uv_count;
    int32 uv_start;
};

struct TextureAtlas {
    char texture_name[32];
    Texture* texture;

    int32 element_count;
    int32 uv_count;

    TextureAtlasElement* elements;
    v2_f32* uv;
};

#endif