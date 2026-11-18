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

enum UIBorderEdge : byte {
    UI_BORDER_EDGE_T,
    UI_BORDER_EDGE_R,
    UI_BORDER_EDGE_B,
    UI_BORDER_EDGE_L,
    UI_BORDER_EDGE_SIZE,
};

enum UIBorderCorner : byte {
    UI_BORDER_CORNER_TL,
    UI_BORDER_CORNER_TR,
    UI_BORDER_CORNER_BR,
    UI_BORDER_CORNER_BL,
    UI_BORDER_CORNER_SIZE,
};

// A border is either
//      1. a solid color border (sampler == UI_SAMPLER_NONE), the width of every side is the thickness of its edge
//      2. a 9-slice border made from 8 atlas images (corners keep their size, edges get stretched)
//
// The atlas data is resolved once and stored inline (unorm16 uv)
// This way rendering never has to look up the atlas
//
struct UIAttributeBorder {
    // UI_SAMPLER_NONE = solid color border
    int16 sampler;

    byte _pad[2];

    // RGBA of a solid color border
    uint32 color;

    // Thickness in px of the edges (UIBorderEdge: T/B = height, L/R = width)
    //      image border:       natural thickness of the edge images (the length is stretched)
    //      solid color border: width of the side
    uint8 edge[UI_BORDER_EDGE_SIZE];

    // Natural size (width, height) in px of the corner images (UIBorderCorner), only used by image borders
    v2_uint8 corner[UI_BORDER_CORNER_SIZE];

    // bl.x, bl.y, tr.x, tr.y per piece as unorm16
    uint16 uv[UI_BORDER_SIZE][4];
};

FORCE_INLINE
bool ui_border_is_visible(const UIAttributeBorder* const border) NO_EXCEPT
{
    return border->sampler != UI_SAMPLER_NONE
        || ((border->color & 0xFF)
            && (border->edge[UI_BORDER_EDGE_T] | border->edge[UI_BORDER_EDGE_R]
                | border->edge[UI_BORDER_EDGE_B] | border->edge[UI_BORDER_EDGE_L]
            )
        );
}

// Same width in px for all sides (e.g. solid color border)
FORCE_INLINE
void ui_border_width_set(UIAttributeBorder* const border, uint8 width) NO_EXCEPT
{
    border->edge[UI_BORDER_EDGE_T] = width;
    border->edge[UI_BORDER_EDGE_R] = width;
    border->edge[UI_BORDER_EDGE_B] = width;
    border->edge[UI_BORDER_EDGE_L] = width;
}

#endif
