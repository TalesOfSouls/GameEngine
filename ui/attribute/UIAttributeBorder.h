#pragma once
#ifndef COMS_UI_ATTRIBUTE_BORDER_H
#define COMS_UI_ATTRIBUTE_BORDER_H

#include "../../stdlib/Stdlib.h"
#include "UIAttributeImage.h"

enum UIBorderType : byte {
    UI_BORDER_TL,
    UI_BORDER_T,
    UI_BORDER_TR,
    UI_BORDER_R,
    UI_BORDER_BR,
    UI_BORDER_B,
    UI_BORDER_BL,
    UI_BORDER_L,
    UI_BORDER_SIZE,
};

// A border is either
//      1. a solid color border (sampler == UI_SAMPLER_NONE) with a uniform width
//      2. a 9-slice border made from 8 atlas images (corners keep their size, edges get stretched)
//
// The atlas data is resolved once and stored inline (unorm16 uv)
// This way rendering never has to look up the atlas
//
// 88 bytes (the previous version was 8 * 76 = 608 bytes)
struct UIAttributeBorder {
    // UI_SAMPLER_NONE = solid color border
    int16 sampler;

    // Width in px of a solid color border
    uint8 width;

    byte _pad;

    // RGBA of a solid color border
    uint32 color;

    // Natural size (width, height) in px of every piece (@see UIBorderType)
    // Edges only use their thickness (T/B = height, L/R = width)
    uint8 size[UI_BORDER_SIZE][2];

    // bl.x, bl.y, tr.x, tr.y per piece as unorm16
    uint16 uv[UI_BORDER_SIZE][4];
};

FORCE_INLINE
bool ui_border_is_visible(const UIAttributeBorder* const border) NO_EXCEPT
{
    return border->sampler != UI_SAMPLER_NONE
        || ((border->color & 0xFF) && border->width);
}

#endif
