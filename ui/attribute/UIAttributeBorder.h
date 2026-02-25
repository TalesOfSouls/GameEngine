#ifndef COMS_UI_ATTRIBUTE_BORDER_H
#define COMS_UI_ATTRIBUTE_BORDER_H

#include "../../stdlib/Stdlib.h"

struct UIAttributeBorder {
    // 4 bits per side
    uint16 thickness;
    uint32 color;
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