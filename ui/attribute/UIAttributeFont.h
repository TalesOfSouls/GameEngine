#pragma once
#ifndef COMS_UI_ATTRIBUTE_FONT_H
#define COMS_UI_ATTRIBUTE_FONT_H

#include "../../stdlib/Stdlib.h"
#include "UIAttributeShadow.h"

enum UIFontDecoration : byte {
    UI_FONT_DECORATION_UNDERLINE = 1 << 0,
    UI_FONT_DECORATION_ITALIC = 1 << 1,
};

struct UIAttributeFont {
    f32 size;
    f32 line_height;
    uint32 color;
    f32 weight;

    uint32 outline_color;
    f32 outline_weight;
    UIAttributeShadow shadow_outer;

    byte decoration;

    // @question Do we even need this? Isn't this handled in UICore::dimension.alignment?
    byte alignment;
};

#endif