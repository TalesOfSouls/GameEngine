#ifndef COMS_UI_ATTRIBUTE_BORDER_H
#define COMS_UI_ATTRIBUTE_BORDER_H

#include "../../stdlib/Types.h"

struct UIAttributeBorder {
    // 4 bits per side
    uint16 thickness;
    uint32 color;
};

inline
void ui_attr_border_serialize(const UIAttributeBorder* __restrict border, byte** __restrict pos)
{
    *((uint16 *) *pos) = SWAP_ENDIAN_LITTLE(border->thickness);
    *pos += sizeof(border->thickness);

    *((uint32 *) *pos) = SWAP_ENDIAN_LITTLE(border->color);
    *pos += sizeof(border->color);
}

inline
void ui_attr_border_unserialize(UIAttributeBorder* __restrict border, const byte** __restrict pos)
{
    border->thickness = SWAP_ENDIAN_LITTLE(*((uint16 *) *pos));
    *pos += sizeof(border->thickness);

    border->color = SWAP_ENDIAN_LITTLE(*((uint32 *) *pos));
    *pos += sizeof(border->color);
}

#endif