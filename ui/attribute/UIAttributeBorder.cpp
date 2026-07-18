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
    const UIAttributeBorder* border = (UIAttributeBorder*) &borders[UI_BORDER_T];
    if (OMS_HAS_ALPHA(border->color) || border->image.texture) {
        vertex_rect_create(
            vertex_cache, index_cache, zindex, border->image.texture - 1,
            {
                anchor_pos->x + border->image.dimension.pos.x,
                anchor_pos->y + border->image.dimension.pos.y,
                anchor_dim->width,
                border->image.dimension.dim.height
            },
            UI_ALIGN_V_TOP | UI_ALIGN_H_LEFT,
            COLOR_NONE_RGBA, border->image.tex_coord[3], border->image.tex_coord[1]
        );
    }

    border = (UIAttributeBorder*) &borders[UI_BORDER_R];
    if (OMS_HAS_ALPHA(border->color) || border->image.texture) {
        vertex_rect_create(
            vertex_cache, index_cache, zindex, border->image.texture - 1,
            {
                anchor_dim->width - border->image.dimension.dim.width + anchor_pos->x + border->image.dimension.pos.x,
                anchor_pos->y + border->image.dimension.pos.y,
                border->image.dimension.dim.width,
                anchor_dim->height
            },
            UI_ALIGN_V_TOP | UI_ALIGN_H_LEFT,
            COLOR_NONE_RGBA, border->image.tex_coord[3], border->image.tex_coord[1]
        );
    }

    border = (UIAttributeBorder*) &borders[UI_BORDER_B];
    if (OMS_HAS_ALPHA(border->color) || border->image.texture) {
        vertex_rect_create(
            vertex_cache, index_cache, zindex, border->image.texture - 1,
            {anchor_pos->x + border->image.dimension.pos.x, anchor_pos->y + border->image.dimension.pos.y - anchor_dim->height + border->image.dimension.dim.height, anchor_dim->width, border->image.dimension.dim.height},
            UI_ALIGN_V_TOP | UI_ALIGN_H_LEFT,
            COLOR_NONE_RGBA, border->image.tex_coord[3], border->image.tex_coord[1]
        );
    }

    border = (UIAttributeBorder*) &borders[UI_BORDER_L];
    if (OMS_HAS_ALPHA(border->color) || border->image.texture) {
        vertex_rect_create(
            vertex_cache, index_cache, zindex, border->image.texture - 1,
            {anchor_pos->x + border->image.dimension.pos.x, anchor_pos->y + border->image.dimension.pos.y, border->image.dimension.dim.width, anchor_dim->height},
            UI_ALIGN_V_TOP | UI_ALIGN_H_LEFT,
            COLOR_NONE_RGBA, border->image.tex_coord[3], border->image.tex_coord[1]
        );
    }

    zindex = camera_step_closer(gpu_api_type, zindex);

    border = (UIAttributeBorder*) &borders[UI_BORDER_TL];
    if (OMS_HAS_ALPHA(border->color) || border->image.texture) {
        vertex_rect_create(
            vertex_cache, index_cache, zindex, border->image.texture - 1,
            {anchor_pos->x + border->image.dimension.pos.x, anchor_pos->y + border->image.dimension.pos.y, border->image.dimension.dim.width, border->image.dimension.dim.height},
            UI_ALIGN_V_TOP | UI_ALIGN_H_LEFT,
            COLOR_NONE_RGBA, border->image.tex_coord[3], border->image.tex_coord[1]
        );
    }

    border = (UIAttributeBorder*) &borders[UI_BORDER_TR];
    if (OMS_HAS_ALPHA(border->color) || border->image.texture) {
        vertex_rect_create(
            vertex_cache, index_cache, zindex, border->image.texture - 1,
            {anchor_dim->width - border->image.dimension.dim.width + anchor_pos->x + border->image.dimension.pos.x, anchor_pos->y + border->image.dimension.pos.y, border->image.dimension.dim.width, border->image.dimension.dim.height},
            UI_ALIGN_V_TOP | UI_ALIGN_H_LEFT,
            COLOR_NONE_RGBA, border->image.tex_coord[3], border->image.tex_coord[1]
        );
    }

    border = (UIAttributeBorder*) &borders[UI_BORDER_BR];
    if (OMS_HAS_ALPHA(border->color) || border->image.texture) {
        vertex_rect_create(
            vertex_cache, index_cache, zindex, border->image.texture - 1,
            {anchor_dim->width - border->image.dimension.dim.width + anchor_pos->x + border->image.dimension.pos.x, anchor_pos->y + border->image.dimension.pos.y - anchor_dim->height + border->image.dimension.dim.height, border->image.dimension.dim.width, border->image.dimension.dim.height},
            UI_ALIGN_V_TOP | UI_ALIGN_H_LEFT,
            COLOR_NONE_RGBA, border->image.tex_coord[3], border->image.tex_coord[1]
        );
    }

    border = (UIAttributeBorder*) &borders[UI_BORDER_BL];
    if (OMS_HAS_ALPHA(border->color) || border->image.texture) {
        vertex_rect_create(
            vertex_cache, index_cache, zindex, border->image.texture - 1,
            {anchor_pos->x + border->image.dimension.pos.x, anchor_pos->y + border->image.dimension.pos.y - anchor_dim->height + border->image.dimension.dim.height, border->image.dimension.dim.width, border->image.dimension.dim.height},
            UI_ALIGN_V_TOP | UI_ALIGN_H_LEFT,
            COLOR_NONE_RGBA, border->image.tex_coord[3], border->image.tex_coord[1]
        );
    }
}

#endif