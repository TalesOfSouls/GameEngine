#pragma once
#ifndef COMS_UI_ATTRIBUTE_BORDER_H
#define COMS_UI_ATTRIBUTE_BORDER_H

#include "../../stdlib/Stdlib.h"
#include "../../gpuapi/RenderUtils.h"
#include "../../camera/Camera.cpp"
#include "../UIAlignment.h"
#include "../UIOffset.h"
#include "UIAttributeBorderOffset.h"

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
    uint32 thickness;
    uint32 color;

    uint32 texture;
    // position is a offset to its natural position
    v2_f32 pos;
    v2_f32 dimension;

    v2_f32 tex_coord[4];
};

inline
void ui_attr_border_serialize(const UIAttributeBorder* __restrict border, byte** __restrict pos)
{
    uint32 temp16 = SWAP_ENDIAN_LITTLE(border->thickness);
    memcpy(*pos, &temp16, sizeof(temp16));
    *pos += sizeof(border->thickness);

    uint32 temp32 = SWAP_ENDIAN_LITTLE(border->color);
    memcpy(*pos, &temp32, sizeof(temp32));
    *pos += sizeof(border->color);
}

inline
void ui_attr_border_unserialize(UIAttributeBorder* __restrict border, const byte** __restrict pos)
{
    memcpy(&border->thickness, *pos, sizeof(border->thickness));
    SWAP_ENDIAN_LITTLE_SELF(border->thickness);
    *pos += sizeof(border->thickness);

    memcpy(&border->color, *pos, sizeof(border->color));
    SWAP_ENDIAN_LITTLE_SELF(border->color);
    *pos += sizeof(border->color);
}

void cache_border_vertices(
    ArrayVector<Vertex3DSamplerTextureColor>* vertex_cache, ArrayVector<int32>* index_cache, f32 zindex, GpuApiType gpu_api_type,
    v2_f32* anchor_pos, v2_f32* anchor_dim, UIBorderOffset* border_offset,
    byte* element_base
)
{
    if (border_offset[UI_BORDER_T].self.element) {
        UIAttributeBorder* border = (UIAttributeBorder*) (element_base + border_offset[UI_BORDER_T].self.element);

        vertex_rect_create(
            vertex_cache, index_cache, zindex, 2,
            {anchor_pos->x + border->pos.x, anchor_pos->y + border->pos.y, anchor_dim->width, border->dimension.height},
            UI_ALIGN_V_TOP | UI_ALIGN_H_LEFT,
            0, border->tex_coord[3], border->tex_coord[1]
        );
    }

    if (border_offset[UI_BORDER_R].self.element) {
        UIAttributeBorder* border = (UIAttributeBorder*) (element_base + border_offset[UI_BORDER_R].self.element);

        vertex_rect_create(
            vertex_cache, index_cache, zindex, 2,
            {anchor_dim->width - border->dimension.width + anchor_pos->x + border->pos.x, anchor_pos->y + border->pos.y, border->dimension.width, anchor_dim->height},
            UI_ALIGN_V_TOP | UI_ALIGN_H_LEFT,
            0, border->tex_coord[3], border->tex_coord[1]
        );
    }

    if (border_offset[UI_BORDER_B].self.element) {
        UIAttributeBorder* border = (UIAttributeBorder*) (element_base + border_offset[UI_BORDER_B].self.element);

        vertex_rect_create(
            vertex_cache, index_cache, zindex, 2,
            {anchor_pos->x + border->pos.x, anchor_pos->y + border->pos.y - anchor_dim->height + border->dimension.height, anchor_dim->width, border->dimension.height},
            UI_ALIGN_V_TOP | UI_ALIGN_H_LEFT,
            0, border->tex_coord[3], border->tex_coord[1]
        );
    }

    if (border_offset[UI_BORDER_L].self.element) {
        UIAttributeBorder* border = (UIAttributeBorder*) (element_base + border_offset[UI_BORDER_L].self.element);

        vertex_rect_create(
            vertex_cache, index_cache, zindex, 2,
            {anchor_pos->x + border->pos.x, anchor_pos->y + border->pos.y, border->dimension.width, anchor_dim->height},
            UI_ALIGN_V_TOP | UI_ALIGN_H_LEFT,
            0, border->tex_coord[3], border->tex_coord[1]
        );
    }

    zindex = camera_step_closer(gpu_api_type, zindex);

    if (border_offset[UI_BORDER_TL].self.element) {
        UIAttributeBorder* border = (UIAttributeBorder*) (element_base + border_offset[UI_BORDER_TL].self.element);

        vertex_rect_create(
            vertex_cache, index_cache, zindex, 2,
            {anchor_pos->x + border->pos.x, anchor_pos->y + border->pos.y, border->dimension.width, border->dimension.height},
            UI_ALIGN_V_TOP | UI_ALIGN_H_LEFT,
            0, border->tex_coord[3], border->tex_coord[1]
        );
    }

    if (border_offset[UI_BORDER_TR].self.element) {
        UIAttributeBorder* border = (UIAttributeBorder*) (element_base + border_offset[UI_BORDER_TR].self.element);

        vertex_rect_create(
            vertex_cache, index_cache, zindex, 2,
            {anchor_dim->width - border->dimension.width + anchor_pos->x + border->pos.x, anchor_pos->y + border->pos.y, border->dimension.width, border->dimension.height},
            UI_ALIGN_V_TOP | UI_ALIGN_H_LEFT,
            0, border->tex_coord[3], border->tex_coord[1]
        );
    }

    if (border_offset[UI_BORDER_BR].self.element) {
        UIAttributeBorder* border = (UIAttributeBorder*) (element_base + border_offset[UI_BORDER_BR].self.element);

        vertex_rect_create(
            vertex_cache, index_cache, zindex, 2,
            {anchor_dim->width - border->dimension.width + anchor_pos->x + border->pos.x, anchor_pos->y + border->pos.y - anchor_dim->height + border->dimension.height, border->dimension.width, border->dimension.height},
            UI_ALIGN_V_TOP | UI_ALIGN_H_LEFT,
            0, border->tex_coord[3], border->tex_coord[1]
        );
    }

    if (border_offset[UI_BORDER_BL].self.element) {
        UIAttributeBorder* border = (UIAttributeBorder*) (element_base + border_offset[UI_BORDER_BL].self.element);

        vertex_rect_create(
            vertex_cache, index_cache, zindex, 2,
            {anchor_pos->x + border->pos.x, anchor_pos->y + border->pos.y - anchor_dim->height + border->dimension.height, border->dimension.width, border->dimension.height},
            UI_ALIGN_V_TOP | UI_ALIGN_H_LEFT,
            0, border->tex_coord[3], border->tex_coord[1]
        );
    }
}

#endif