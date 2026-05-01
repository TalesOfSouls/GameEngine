#pragma once
#ifndef COMS_UI_ATTRIBUTE_BACKGROUND_STYLE_H
#define COMS_UI_ATTRIBUTE_BACKGROUND_STYLE_H

#include "../../stdlib/Stdlib.h"

enum UIBackgroundStyle : byte {
    UI_BACKGROUND_STYLE_NONE = 1 << 0,
    UI_BACKGROUND_STYLE_COLOR_IMG = 1 << 1, // 0 = color, 1 = img
    UI_BACKGROUND_STYLE_STRETCH = 1 << 2, // 0 = none, 1 = stretch
    UI_BACKGROUND_STYLE_REPEAT = 1 << 3, // 0 = none, 1 = repeat
};

struct UIAttributeBackground {
    UIBackgroundStyle style;
    union {
        uint32 background_image;
        uint32 color;
    };
};

inline
void ui_attr_background_serialize(const UIAttributeBackground* __restrict bg, byte** __restrict pos)
{
    **pos = bg->style;
    *pos += sizeof(bg->style);

    uint32 temp = SWAP_ENDIAN_LITTLE(bg->color);
    memcpy(*pos, &temp, sizeof(temp));
    *pos += sizeof(bg->color);
}

inline
void ui_attr_background_unserialize(UIAttributeBackground* __restrict bg, const byte** __restrict pos)
{
    bg->style = (UIBackgroundStyle) **pos;
    *pos += sizeof(bg->style);

    memcpy(&bg->color, *pos, sizeof(bg->color));
    SWAP_ENDIAN_LITTLE_SELF(bg->color);
    *pos += sizeof(bg->color);
}

#endif