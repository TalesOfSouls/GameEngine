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
// 12 bytes (2 bytes padding)
struct UIAttributeFont {
    f32 size;

    // RGBA
    uint32 color;

    // @type UIFontDecoration
    byte decoration;

    // Text alignment inside of the element (only the UI_ALIGN_* bits are used)
    byte alignment;
};

#endif
