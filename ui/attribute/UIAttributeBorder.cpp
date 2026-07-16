#pragma once
#ifndef COMS_UI_ATTRIBUTE_BORDER_C
#define COMS_UI_ATTRIBUTE_BORDER_C

#include "UIAttributeBorder.h"
#include "UIAttributeImage.cpp"
#include "../../image/default_colors.h"

void ui_vertices_cache(
    ArrayVector<Vertex3DSamplerTextureColor>* const __restrict vertex_cache, ArrayVector<int32>* const __restrict index_cache, f32 zindex, GpuApiType gpu_api_type,
    const v2_f32* const __restrict anchor_pos, const v2_f32* const __restrict anchor_dim, const UIAttributeBorder* const __restrict borders
) NO_EXCEPT
{
    if (OMS_HAS_ALPHA(borders[UI_BORDER_T].color) || borders[UI_BORDER_T].texture) {
        const UIAttributeBorder* const border = (UIAttributeBorder*) &borders[UI_BORDER_T];

        vertex_rect_create(
            vertex_cache, index_cache, zindex, border->image.texture - 1,
            {
                anchor_pos->x + border->image.pos.x,
                anchor_pos->y + border->image.pos.y,
                anchor_dim->width,
                border->image.dimension.height
            },
            UI_ALIGN_V_TOP | UI_ALIGN_H_LEFT,
            COLOR_NONE_RGBA, border->image.tex_coord[3], border->image.tex_coord[1]
        );
    }

    if (OMS_HAS_ALPHA(borders[UI_BORDER_R].color) || borders[UI_BORDER_R].texture) {
        const UIAttributeBorder* const border = (UIAttributeBorder*) &borders[UI_BORDER_R];

        vertex_rect_create(
            vertex_cache, index_cache, zindex, border->image.texture - 1,
            {
                anchor_dim->width - border->image.dimension.width + anchor_pos->x + border->image.pos.x,
                anchor_pos->y + border->image.pos.y,
                border->image.dimension.width,
                anchor_dim->height
            },
            UI_ALIGN_V_TOP | UI_ALIGN_H_LEFT,
            COLOR_NONE_RGBA, border->image.tex_coord[3], border->image.tex_coord[1]
        );
    }

    if (OMS_HAS_ALPHA(borders[UI_BORDER_B].color) || borders[UI_BORDER_B].texture) {
        const UIAttributeBorder* const border = (UIAttributeBorder*) &borders[UI_BORDER_B];

        vertex_rect_create(
            vertex_cache, index_cache, zindex, border->image.texture - 1,
            {anchor_pos->x + border->image.pos.x, anchor_pos->y + border->image.pos.y - anchor_dim->height + border->image.dimension.height, anchor_dim->width, border->image.dimension.height},
            UI_ALIGN_V_TOP | UI_ALIGN_H_LEFT,
            COLOR_NONE_RGBA, border->image.tex_coord[3], border->image.tex_coord[1]
        );
    }

    if (OMS_HAS_ALPHA(borders[UI_BORDER_L].color) || borders[UI_BORDER_L].texture) {
        const UIAttributeBorder* const border = (UIAttributeBorder*) &borders[UI_BORDER_L];

        vertex_rect_create(
            vertex_cache, index_cache, zindex, border->image.texture - 1,
            {anchor_pos->x + border->image.pos.x, anchor_pos->y + border->image.pos.y, border->image.dimension.width, anchor_dim->height},
            UI_ALIGN_V_TOP | UI_ALIGN_H_LEFT,
            COLOR_NONE_RGBA, border->image.tex_coord[3], border->image.tex_coord[1]
        );
    }

    zindex = camera_step_closer(gpu_api_type, zindex);

    if (OMS_HAS_ALPHA(borders[UI_BORDER_TL].color) || borders[UI_BORDER_TL].texture) {
        const UIAttributeBorder* const border = (UIAttributeBorder*) &borders[UI_BORDER_TL];

        vertex_rect_create(
            vertex_cache, index_cache, zindex, border->image.texture - 1,
            {anchor_pos->x + border->image.pos.x, anchor_pos->y + border->image.pos.y, border->image.dimension.width, border->image.dimension.height},
            UI_ALIGN_V_TOP | UI_ALIGN_H_LEFT,
            COLOR_NONE_RGBA, border->image.tex_coord[3], border->image.tex_coord[1]
        );
    }

    if (OMS_HAS_ALPHA(borders[UI_BORDER_TR].color) || borders[UI_BORDER_TR].texture) {
        const UIAttributeBorder* const border = (UIAttributeBorder*) &borders[UI_BORDER_TR];

        vertex_rect_create(
            vertex_cache, index_cache, zindex, border->image.texture - 1,
            {anchor_dim->width - border->image.dimension.width + anchor_pos->x + border->image.pos.x, anchor_pos->y + border->image.pos.y, border->image.dimension.width, border->image.dimension.height},
            UI_ALIGN_V_TOP | UI_ALIGN_H_LEFT,
            COLOR_NONE_RGBA, border->image.tex_coord[3], border->image.tex_coord[1]
        );
    }

    if (OMS_HAS_ALPHA(borders[UI_BORDER_BR].color) || borders[UI_BORDER_BR].texture) {
        const UIAttributeBorder* const border = (UIAttributeBorder*) &borders[UI_BORDER_BR];

        vertex_rect_create(
            vertex_cache, index_cache, zindex, border->image.texture - 1,
            {anchor_dim->width - border->image.dimension.width + anchor_pos->x + border->image.pos.x, anchor_pos->y + border->image.pos.y - anchor_dim->height + border->image.dimension.height, border->image.dimension.width, border->image.dimension.height},
            UI_ALIGN_V_TOP | UI_ALIGN_H_LEFT,
            COLOR_NONE_RGBA, border->image.tex_coord[3], border->image.tex_coord[1]
        );
    }

    if (OMS_HAS_ALPHA(borders[UI_BORDER_BL].color) || borders[UI_BORDER_BL].texture) {
        const UIAttributeBorder* const border = (UIAttributeBorder*) &borders[UI_BORDER_BL];

        vertex_rect_create(
            vertex_cache, index_cache, zindex, border->image.texture - 1,
            {anchor_pos->x + border->image.pos.x, anchor_pos->y + border->image.pos.y - anchor_dim->height + border->image.dimension.height, border->image.dimension.width, border->image.dimension.height},
            UI_ALIGN_V_TOP | UI_ALIGN_H_LEFT,
            COLOR_NONE_RGBA, border->image.tex_coord[3], border->image.tex_coord[1]
        );
    }
}

#endif