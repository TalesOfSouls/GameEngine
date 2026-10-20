#pragma once
#ifndef COMS_UI_ATTRIBUTE_IMAGE_C
#define COMS_UI_ATTRIBUTE_IMAGE_C

#include "UIAttributeImage.h"
#include "../../object/TextureAtlas.cpp"
#include "../../gpuapi/RenderUtils.h"
#include "../UIAlignment.h"

/**
 * Resolves an atlas element into the inline image data
 * This is the only place where the atlas is accessed, rendering only uses the inline data
 */
inline
void ui_image_attribute_from_atlas(
    const TextureAtlas* const __restrict ui_atlas,
    int32 atlas_texture_id,
    UIAttributeImage* const __restrict img,
    int16 sampler = UI_SAMPLER_ATLAS
) NO_EXCEPT
{
    const v2_f32* const uv = &ui_atlas->uv[ui_atlas->elements[atlas_texture_id].uv_start];

    // The atlas stores the uv coordinates in the order: tl, tr, br, bl
    img->sampler = sampler;
    img->uv[0] = ui_uv_encode(uv[3].x);
    img->uv[1] = ui_uv_encode(uv[3].y);
    img->uv[2] = ui_uv_encode(uv[1].x);
    img->uv[3] = ui_uv_encode(uv[1].y);

    const f32 width = (uv[1].x - uv[0].x) * (f32) ui_atlas->texture->image.width;
    const f32 height = (uv[0].y - uv[3].y) * (f32) ui_atlas->texture->image.height;

    img->width = (uint16) (width < 0.0f ? -width + 0.5f : width + 0.5f);
    img->height = (uint16) (height < 0.0f ? -height + 0.5f : height + 0.5f);
}

/**
 * Renders the image so that it fills the rect
 *
 * @param rect x, y = top left corner, width, height
 */
FORCE_INLINE
void ui_image_vertices(
    const UIAttributeImage* const __restrict image,
    v4_f32 rect,
    ArrayVector<Vertex3DSamplerTextureColor>* const __restrict vertex_cache,
    ArrayVector<int32>* const __restrict index_cache,
    f32 zindex
) NO_EXCEPT
{
    vertex_rect_create(
        vertex_cache, index_cache, zindex, image->sampler,
        rect,
        UI_ALIGN_V_TOP | UI_ALIGN_H_LEFT,
        0,
        {ui_uv_decode(image->uv[0]), ui_uv_decode(image->uv[1])},
        {ui_uv_decode(image->uv[2]), ui_uv_decode(image->uv[3])}
    );
}

#endif
