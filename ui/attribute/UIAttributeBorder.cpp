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
 * Resolves 8 atlas elements (order) into the inline 9-slice border data
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
        const uint8 width = (uint8) (img.width > 255 ? 255 : img.width);
        const uint8 height = (uint8) (img.height > 255 ? 255 : img.height);

        // Even pieces = corners (TL, TR, BR, BL), odd pieces = edges (T, R, B, L)
        if (i & 1) {
            border->edge[i >> 1] = (i == UI_BORDER_T || i == UI_BORDER_B) ? height : width;
        } else {
            border->corner[i >> 1] = {width, height};
        }
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
 * @param v4_f32 rect x, y = top left corner, width, height
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
        if (!(border->color & 0xFF)) {
            return;
        }

        // Width of every side
        const f32 top = (f32) border->edge[UI_BORDER_EDGE_T];
        const f32 bottom = (f32) border->edge[UI_BORDER_EDGE_B];
        const f32 left = (f32) border->edge[UI_BORDER_EDGE_L];
        const f32 right = (f32) border->edge[UI_BORDER_EDGE_R];

        // The top + bottom edge span the full width, the left + right edge are between them
        const v4_f32 rects[4] = {
            {x, y, w, top}, // top
            {x, y - h + bottom, w, bottom}, // bottom
            {x, y - top, left, h - top - bottom}, // left
            {x + w - right, y - top, right, h - top - bottom}, // right
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

    const v2_uint8* const corner = border->corner;
    const uint8* const edge = border->edge;

    // Corners keep their natural size
    ui_border_piece_vertices(border, UI_BORDER_TL,
        x, y,
        corner[UI_BORDER_CORNER_TL].width, corner[UI_BORDER_CORNER_TL].height,
        vertex_cache, index_cache, zindex
    );

    ui_border_piece_vertices(border, UI_BORDER_TR,
        x + w - corner[UI_BORDER_CORNER_TR].width, y,
        corner[UI_BORDER_CORNER_TR].width, corner[UI_BORDER_CORNER_TR].height,
        vertex_cache, index_cache, zindex
    );

    ui_border_piece_vertices(border, UI_BORDER_BR,
        x + w - corner[UI_BORDER_CORNER_BR].width, y - h + corner[UI_BORDER_CORNER_BR].height,
        corner[UI_BORDER_CORNER_BR].width, corner[UI_BORDER_CORNER_BR].height,
        vertex_cache, index_cache, zindex
    );

    ui_border_piece_vertices(border, UI_BORDER_BL,
        x, y - h + corner[UI_BORDER_CORNER_BL].height,
        corner[UI_BORDER_CORNER_BL].width, corner[UI_BORDER_CORNER_BL].height,
        vertex_cache, index_cache, zindex
    );

    // Edges are stretched between the corners
    ui_border_piece_vertices(border, UI_BORDER_T,
        x + corner[UI_BORDER_CORNER_TL].width, y,
        w - corner[UI_BORDER_CORNER_TL].width - corner[UI_BORDER_CORNER_TR].width, edge[UI_BORDER_EDGE_T],
        vertex_cache, index_cache, zindex
    );

    ui_border_piece_vertices(border, UI_BORDER_B,
        x + corner[UI_BORDER_CORNER_BL].width, y - h + edge[UI_BORDER_EDGE_B],
        w - corner[UI_BORDER_CORNER_BL].width - corner[UI_BORDER_CORNER_BR].width, edge[UI_BORDER_EDGE_B],
        vertex_cache, index_cache, zindex
    );

    ui_border_piece_vertices(border, UI_BORDER_L,
        x, y - corner[UI_BORDER_CORNER_TL].height,
        edge[UI_BORDER_EDGE_L], h - corner[UI_BORDER_CORNER_TL].height - corner[UI_BORDER_CORNER_BL].height,
        vertex_cache, index_cache, zindex
    );

    ui_border_piece_vertices(border, UI_BORDER_R,
        x + w - edge[UI_BORDER_EDGE_R], y - corner[UI_BORDER_CORNER_TR].height,
        edge[UI_BORDER_EDGE_R], h - corner[UI_BORDER_CORNER_TR].height - corner[UI_BORDER_CORNER_BR].height,
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
    ui_border_width_set(&border, width);

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
