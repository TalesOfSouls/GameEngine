#pragma once
#ifndef COMS_UI_ATTRIBUTE_BORDER_H
#define COMS_UI_ATTRIBUTE_BORDER_H

#include "../../stdlib/Stdlib.h"
#include "../../gpuapi/RenderUtils.h"
#include "../../camera/Camera.cpp"
#include "../UIAlignment.h"

enum UIBorderType {
    UI_BORDER_TL,
    UI_BORDER_T,
    UI_BORDER_TR,
    UI_BORDER_R,
    UI_BORDER_BR,
    UI_BORDER_B,
    UI_BORDER_BL,
    UI_BORDER_L,
};

struct UIAttributeBorder {
    uint32 color;

    uint32 texture;
    // position is a offset to its natural position
    v2_f32 pos;
    v2_f32 dimension;

    v2_f32 tex_coord[4];
};

void cache_border_vertices(
    ArrayVector<Vertex3DSamplerTextureColor>* vertex_cache, ArrayVector<int32>* index_cache, f32 zindex, GpuApiType gpu_api_type,
    v2_f32* anchor_pos, v2_f32* anchor_dim, UIAttributeBorder* borders
)
{
    if (borders[UI_BORDER_T].color & 0xFF) {
        UIAttributeBorder* border = (UIAttributeBorder*) &borders[UI_BORDER_T];

        vertex_rect_create(
            vertex_cache, index_cache, zindex, 2,
            {anchor_pos->x + border->pos.x, anchor_pos->y + border->pos.y, anchor_dim->width, border->dimension.height},
            UI_ALIGN_V_TOP | UI_ALIGN_H_LEFT,
            0, border->tex_coord[3], border->tex_coord[1]
        );
    }

    if (borders[UI_BORDER_R].color & 0xFF) {
        UIAttributeBorder* border = (UIAttributeBorder*) &borders[UI_BORDER_R];

        vertex_rect_create(
            vertex_cache, index_cache, zindex, 2,
            {anchor_dim->width - border->dimension.width + anchor_pos->x + border->pos.x, anchor_pos->y + border->pos.y, border->dimension.width, anchor_dim->height},
            UI_ALIGN_V_TOP | UI_ALIGN_H_LEFT,
            0, border->tex_coord[3], border->tex_coord[1]
        );
    }

    if (borders[UI_BORDER_B].color & 0xFF) {
        UIAttributeBorder* border = (UIAttributeBorder*) &borders[UI_BORDER_B];

        vertex_rect_create(
            vertex_cache, index_cache, zindex, 2,
            {anchor_pos->x + border->pos.x, anchor_pos->y + border->pos.y - anchor_dim->height + border->dimension.height, anchor_dim->width, border->dimension.height},
            UI_ALIGN_V_TOP | UI_ALIGN_H_LEFT,
            0, border->tex_coord[3], border->tex_coord[1]
        );
    }

    if (borders[UI_BORDER_L].color & 0xFF) {
        UIAttributeBorder* border = (UIAttributeBorder*) &borders[UI_BORDER_L];

        vertex_rect_create(
            vertex_cache, index_cache, zindex, 2,
            {anchor_pos->x + border->pos.x, anchor_pos->y + border->pos.y, border->dimension.width, anchor_dim->height},
            UI_ALIGN_V_TOP | UI_ALIGN_H_LEFT,
            0, border->tex_coord[3], border->tex_coord[1]
        );
    }

    zindex = camera_step_closer(gpu_api_type, zindex);

    if (borders[UI_BORDER_TL].color & 0xFF) {
        UIAttributeBorder* border = (UIAttributeBorder*) &borders[UI_BORDER_TL];

        vertex_rect_create(
            vertex_cache, index_cache, zindex, 2,
            {anchor_pos->x + border->pos.x, anchor_pos->y + border->pos.y, border->dimension.width, border->dimension.height},
            UI_ALIGN_V_TOP | UI_ALIGN_H_LEFT,
            0, border->tex_coord[3], border->tex_coord[1]
        );
    }

    if (borders[UI_BORDER_TR].color & 0xFF) {
        UIAttributeBorder* border = (UIAttributeBorder*) &borders[UI_BORDER_TR];

        vertex_rect_create(
            vertex_cache, index_cache, zindex, 2,
            {anchor_dim->width - border->dimension.width + anchor_pos->x + border->pos.x, anchor_pos->y + border->pos.y, border->dimension.width, border->dimension.height},
            UI_ALIGN_V_TOP | UI_ALIGN_H_LEFT,
            0, border->tex_coord[3], border->tex_coord[1]
        );
    }

    if (borders[UI_BORDER_BR].color & 0xFF) {
        UIAttributeBorder* border = (UIAttributeBorder*) &borders[UI_BORDER_BR];

        vertex_rect_create(
            vertex_cache, index_cache, zindex, 2,
            {anchor_dim->width - border->dimension.width + anchor_pos->x + border->pos.x, anchor_pos->y + border->pos.y - anchor_dim->height + border->dimension.height, border->dimension.width, border->dimension.height},
            UI_ALIGN_V_TOP | UI_ALIGN_H_LEFT,
            0, border->tex_coord[3], border->tex_coord[1]
        );
    }

    if (borders[UI_BORDER_BL].color & 0xFF) {
        UIAttributeBorder* border = (UIAttributeBorder*) &borders[UI_BORDER_BL];

        vertex_rect_create(
            vertex_cache, index_cache, zindex, 2,
            {anchor_pos->x + border->pos.x, anchor_pos->y + border->pos.y - anchor_dim->height + border->dimension.height, border->dimension.width, border->dimension.height},
            UI_ALIGN_V_TOP | UI_ALIGN_H_LEFT,
            0, border->tex_coord[3], border->tex_coord[1]
        );
    }
}

#endif