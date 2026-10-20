#pragma once
#ifndef COMS_UI_ATTRIBUTE_FONT_H
#define COMS_UI_ATTRIBUTE_FONT_H

#include "../../stdlib/Stdlib.h"

enum UIFontDecoration : byte {
    UI_FONT_DECORATION_UNDERLINE = 1 << 0,
    UI_FONT_DECORATION_ITALIC = 1 << 1,
};

// Only contains what the text rendering actually uses
// Outline/weight/shadow can be added again once the text rendering supports them
// 16 bytes
struct UIAttributeFont {
    f32 size;

    // 0 = use the line height of the font
    f32 line_height;

    // RGBA
    uint32 color;

    // @see UIFontDecoration
    byte decoration;

    // Text alignment inside of the element (@see UIAlignAnchor, only the UI_ALIGN_* bits are used)
    byte alignment;
};

#endif
