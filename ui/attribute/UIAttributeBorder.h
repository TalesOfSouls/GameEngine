#pragma once
#ifndef COMS_UI_ATTRIBUTE_BORDER_H
#define COMS_UI_ATTRIBUTE_BORDER_H

#include "../../stdlib/Stdlib.h"

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

#endif