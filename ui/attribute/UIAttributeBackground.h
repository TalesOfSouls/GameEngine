#ifndef COMS_UI_ATTRIBUTE_BACKGROUND_STYLE_H
#define COMS_UI_ATTRIBUTE_BACKGROUND_STYLE_H

#include "../../stdlib/Types.h"

enum UIBackgroundStyle : byte {
    UI_BACKGROUND_STYLE_NONE = 1 << 0,
    UI_BACKGROUND_STYLE_COLOR_IMG = 1 << 1, // 0 = color, 1 = img
    UI_BACKGROUND_STYLE_STRETCH = 1 << 2, // 0 = none, 1 = stretch
    UI_BACKGROUND_STYLE_REPEAT = 1 << 3, // 0 = none, 1 = repeat
};

struct UIAttributeBackground {
    UIBackgroundStyle background_style;
    union {
        uint32 background_image;
        uint32 background_color;
    };
};

inline
void ui_attr_background_serialize(const UIAttributeBackground* __restrict bg, byte** __restrict pos)
{
    **pos = bg->background_style;
    *pos += sizeof(bg->background_style);

    *((uint32 *) *pos) = SWAP_ENDIAN_LITTLE(bg->background_color);
    *pos += sizeof(bg->background_color);
}

inline
void ui_attr_background_unserialize(UIAttributeBackground* __restrict bg, const byte** __restrict pos)
{
    bg->background_style = (UIBackgroundStyle) **pos;
    *pos += sizeof(bg->background_style);

    bg->background_color = SWAP_ENDIAN_LITTLE(*((uint32 *) *pos));
    *pos += sizeof(bg->background_color);
}

#endif