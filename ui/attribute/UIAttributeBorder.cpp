#pragma once
#ifndef COMS_UI_ATTRIBUTE_BORDER_C
#define COMS_UI_ATTRIBUTE_BORDER_C

#include "../../stdlib/Stdlib.h"
#include "../../gpuapi/RenderUtils.h"
#include "../../object/TextureAtlas.cpp"
#include "../UIAlignment.h"
#include "UIAttributeBorder.h"
#include "UIAttributeImage.cpp"

/**
 * Resolves 8 atlas elements (order @see UIBorderType) into the inline 9-slice border data
 */
inline
void ui_border_attribute_from_atlas(
    const TextureAtlas* const __restrict ui_atlas,
    const int32 atlas_ids[UI_BORDER_SIZE],
    UIAttributeBorder* const __restrict border,
    int16 sampler = UI_SAMPLER_ATLAS
) NO_EXCEPT
{
    border->sampler = sampler;

    for (int32 i = 0; i < UI_BORDER_SIZE; ++i) {
        UIAttributeImage img;
        ui_image_attribute_from_atlas(ui_atlas, atlas_ids[i], &img, sampler);

        memcpy(border->uv[i], img.uv, sizeof(img.uv));
        border->size[i][0] = (uint8) (img.width > 255 ? 255 : img.width);
        border->size[i][1] = (uint8) (img.height > 255 ? 255 : img.height);
    }
}

FORCE_INLINE
void ui_border_piece_vertices(
    const UIAttributeBorder* const __restrict border, int32 piece,
    f32 x, f32 y, f32 width, f32 height,
    ArrayVector<Vertex3DSamplerTextureColor>* const __restrict vertex_cache,
    ArrayVector<int32>* const __restrict index_cache,
    f32 zindex
) NO_EXCEPT
{
    if (width <= 0.0f || height <= 0.0f) {
        return;
    }

    const uint16* const uv = border->uv[piece];
    vertex_rect_create(
        vertex_cache, index_cache, zindex, border->sampler,
        {x, y, width, height},
        UI_ALIGN_V_TOP | UI_ALIGN_H_LEFT,
        0,
        {ui_uv_decode(uv[0]), ui_uv_decode(uv[1])},
        {ui_uv_decode(uv[2]), ui_uv_decode(uv[3])}
    );
}

/**
 * Renders the border INSIDE of the rect
 *
 * @param rect x, y = top left corner, width, height
 */
void ui_border_vertices(
    const UIAttributeBorder* const __restrict border,
    v4_f32 rect,
    ArrayVector<Vertex3DSamplerTextureColor>* const __restrict vertex_cache,
    ArrayVector<int32>* const __restrict index_cache,
    f32 zindex
) NO_EXCEPT
{
    const f32 x = rect.x;
    const f32 y = rect.y;
    const f32 w = rect.width;
    const f32 h = rect.height;

    if (border->sampler == UI_SAMPLER_NONE) {
        if (!(border->color & 0xFF) || !border->width) {
            return;
        }

        const f32 bw = (f32) border->width;
        const v4_f32 rects[4] = {
            {x, y, w, bw}, // top
            {x, y - h + bw, w, bw}, // bottom
            {x, y - bw, bw, h - 2.0f * bw}, // left
            {x + w - bw, y - bw, bw, h - 2.0f * bw}, // right
        };

        for (int32 i = 0; i < 4; ++i) {
            if (rects[i].width <= 0.0f || rects[i].height <= 0.0f) {
                continue;
            }

            vertex_rect_create(
                vertex_cache, index_cache, zindex, 0,
                rects[i],
                UI_ALIGN_V_TOP | UI_ALIGN_H_LEFT,
                border->color
            );
        }

        return;
    }

    const uint8 (*s)[2] = border->size;

    // Corners keep their natural size
    ui_border_piece_vertices(border, UI_BORDER_TL,
        x, y,
        s[UI_BORDER_TL][0], s[UI_BORDER_TL][1],
        vertex_cache, index_cache, zindex
    );

    ui_border_piece_vertices(border, UI_BORDER_TR,
        x + w - s[UI_BORDER_TR][0], y,
        s[UI_BORDER_TR][0], s[UI_BORDER_TR][1],
        vertex_cache, index_cache, zindex
    );

    ui_border_piece_vertices(border, UI_BORDER_BR,
        x + w - s[UI_BORDER_BR][0], y - h + s[UI_BORDER_BR][1],
        s[UI_BORDER_BR][0], s[UI_BORDER_BR][1],
        vertex_cache, index_cache, zindex
    );

    ui_border_piece_vertices(border, UI_BORDER_BL,
        x, y - h + s[UI_BORDER_BL][1],
        s[UI_BORDER_BL][0], s[UI_BORDER_BL][1],
        vertex_cache, index_cache, zindex
    );

    // Edges are stretched between the corners
    ui_border_piece_vertices(border, UI_BORDER_T,
        x + s[UI_BORDER_TL][0], y,
        w - s[UI_BORDER_TL][0] - s[UI_BORDER_TR][0], s[UI_BORDER_T][1],
        vertex_cache, index_cache, zindex
    );

    ui_border_piece_vertices(border, UI_BORDER_B,
        x + s[UI_BORDER_BL][0], y - h + s[UI_BORDER_B][1],
        w - s[UI_BORDER_BL][0] - s[UI_BORDER_BR][0], s[UI_BORDER_B][1],
        vertex_cache, index_cache, zindex
    );

    ui_border_piece_vertices(border, UI_BORDER_L,
        x, y - s[UI_BORDER_TL][1],
        s[UI_BORDER_L][0], h - s[UI_BORDER_TL][1] - s[UI_BORDER_BL][1],
        vertex_cache, index_cache, zindex
    );

    ui_border_piece_vertices(border, UI_BORDER_R,
        x + w - s[UI_BORDER_R][0], y - s[UI_BORDER_TR][1],
        s[UI_BORDER_R][0], h - s[UI_BORDER_TR][1] - s[UI_BORDER_BR][1],
        vertex_cache, index_cache, zindex
    );
}

/**
 * Solid color border INSIDE of the rect (e.g. input fields)
 */
inline
void ui_border_solid_vertices(
    v4_f32 rect, uint32 color, uint8 width,
    ArrayVector<Vertex3DSamplerTextureColor>* const __restrict vertex_cache,
    ArrayVector<int32>* const __restrict index_cache,
    f32 zindex
) NO_EXCEPT
{
    UIAttributeBorder border = {};
    border.color = color;
    border.width = width;

    ui_border_vertices(&border, rect, vertex_cache, index_cache, zindex);
}

/**
 * Solid color rect (x, y = top left corner)
 */
FORCE_INLINE
void ui_rect_vertices(
    v4_f32 rect, uint32 color,
    ArrayVector<Vertex3DSamplerTextureColor>* const __restrict vertex_cache,
    ArrayVector<int32>* const __restrict index_cache,
    f32 zindex
) NO_EXCEPT
{
    if (!(color & 0xFF) || rect.width <= 0.0f || rect.height <= 0.0f) {
        return;
    }

    vertex_rect_create(
        vertex_cache, index_cache, zindex, 0,
        rect,
        UI_ALIGN_V_TOP | UI_ALIGN_H_LEFT,
        color
    );
}

#endif
