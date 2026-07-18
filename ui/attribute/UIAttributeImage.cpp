#pragma once
#ifndef COMS_UI_ATTRIBUTE_IMAGE_C
#define COMS_UI_ATTRIBUTE_IMAGE_C

#include "UIAttributeImage.h"
#include "../object/TextureAtlas.cpp"
#include "../UIAlignment.h"

inline
void ui_image_attribute_from_atlas(
    const TextureAtlas* const __restrict ui_atlas,
    int32 atlas_texture_id,
    UIAttributeImage* const __restrict img
) NO_EXCEPT
{
    int32 uv_start = ui_atlas->elements[atlas_texture_id].uv_start;
    // @todo change to use actual sampler id instead of hardcoded "2"
    // +1 due to 1-index instead of 0 index (0 = no sampler)
    img->texture = 2 + 1;
    memcpy(img->tex_coord, &ui_atlas->uv[uv_start], sizeof(v2_f32) * 4);
    img->dimension.pos = {0.0f, 0.0f};
    img->dimension.dim = {
        (img->tex_coord[1].x - img->tex_coord[0].x) * ui_atlas->texture->image.width,
        (img->tex_coord[0].y - img->tex_coord[3].y) * ui_atlas->texture->image.height,
    };
}

FORCE_INLINE
void ui_vertices_cache(
    const UIAttributeImage* const __restrict image,
    ArrayVector<Vertex3DSamplerTextureColor>* const __restrict vertex_cache, ArrayVector<int32>* const __restrict index_cache, f32 zindex
) NO_EXCEPT
{
    vertex_rect_create(
        vertex_cache, index_cache, zindex, image->texture - 1,
        {image->dimension.pos.x, image->dimension.pos.y, image->dimension.dim.width, image->dimension.dim.height},
        UI_ALIGN_V_TOP | UI_ALIGN_H_LEFT,
        0, image->tex_coord[3], image->tex_coord[1]
    );
}

#endif