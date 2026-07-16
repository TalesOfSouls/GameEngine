#pragma once
#ifndef COMS_UI_ATTRIBUTE_IMAGE_C
#define COMS_UI_ATTRIBUTE_IMAGE_C

#include "UIAttributeImage.h"
#include "../object/TextureAtlas.cpp"
#include "../UIAlignment.h"

inline
void ui_image_attribute_from_atlas(
    const TextureAtlas* const __restrict atlas,
    int32 atlas_texture_id,
    UIAttributeImage* const __restrict img
) NO_EXCEPT
{
    int32 uv_start = ui_atlas->elements[atlas_texture_id].uv_start;
    // @todo change to use actual sampler id instead of hardcoded "2"
    // +1 due to 1-index instead of 0 index (0 = no sampler)
    img->texture = 2 + 1;
    img->tex_coord[0] = ui_atlas->uv[uv_start + 0];
    img->tex_coord[1] = ui_atlas->uv[uv_start + 1];
    img->tex_coord[2] = ui_atlas->uv[uv_start + 2];
    img->tex_coord[3] = ui_atlas->uv[uv_start + 3];
    img->pos = {0.0f, 0.0f};
    img->dimension = {
        (img->tex_coord[1].x - img->tex_coord[0].x) * 512.0f,
        (img->tex_coord[0].y - img->tex_coord[3].y) * 512.0f,
    };
}

FORCE_INLINE
void ui_vertices_cache(
    const UIAttributeImage* const __restrict image,
    ArrayVector<Vertex3DSamplerTextureColor>* const __restrict vertex_cache, ArrayVector<int32>* const __restrict index_cache, f32 zindex, GpuApiType gpu_api_type,
    const v2_f32* const __restrict anchor_pos, const v2_f32* const __restrict anchor_dim, const UIAttributeImage* const __restrict image
) NO_EXCEPT
{
    const f32 width = anchor_dim->x > 0.0f
        ? OMS_MIN(anchor_dim->x, image->dimension.width)
        : image->dimension.width;

    const f32 height = anchor_dim->y > 0.0f
        ? OMS_MIN(anchor_dim->y, image->dimension.height)
        : image->dimension.height;

    vertex_rect_create(
        vertex_cache, index_cache, zindex, image->texture - 1,
        {anchor_pos->x + image->pos.x, anchor_pos->y + image->pos.y, width, height},
        UI_ALIGN_V_TOP | UI_ALIGN_H_LEFT,
        0, image->tex_coord[3], image->tex_coord[1]
    );
}

#endif