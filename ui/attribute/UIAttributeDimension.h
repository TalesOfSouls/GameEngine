#pragma once
#ifndef COMS_UI_ATTRIBUTE_DIMENSION_H
#define COMS_UI_ATTRIBUTE_DIMENSION_H

#include "../../stdlib/Stdlib.h"

// By default position and dimension values are pixel values
// Relative values are fractions (0.0 - 1.0) of the parent's calculated dimension
enum UIDimensionFlag : byte {
    UI_DIMENSION_POS_X_RELATIVE = 1 << 0,
    UI_DIMENSION_POS_Y_RELATIVE = 1 << 1,

    UI_DIMENSION_DIM_X_RELATIVE = 1 << 2,
    UI_DIMENSION_DIM_Y_RELATIVE = 1 << 3,

    // Pixel values are the default, these flags are only kept for readability/backwards compatibility
    UI_DIMENSION_POS_X_PX = 1 << 4,
    UI_DIMENSION_POS_Y_PX = 1 << 5,

    UI_DIMENSION_DIM_X_PX = 1 << 6,
    UI_DIMENSION_DIM_Y_PX = 1 << 7,
};

// 36 bytes
struct UIAttributeDimension {
    // @see UIDimensionFlag
    byte flag;

    // @see UIAlignAnchor
    byte align_anchor;

    // This is the calculated position/dimension
    // pos = top left corner (y-up coordinate system, @see UIAlignment.h)
    v2_f32 pos;
    v2_f32 dim;

    // This is the original position/dimension definition
    // Required if we move/resize the UI element or change the resolution
    // pos_raw = offset from the anchor point (positive values move towards the inside of the parent)
    // dim_raw > 0 = pixel size (or fraction of the parent if relative)
    // dim_raw = 0 means "use the natural size of the content" (e.g. text size, image size) if the element has one
    // dim_raw < 0 means "parent size minus the value" (e.g. -38 = fill the parent except for a 38px title)
    v2_f32 pos_raw;
    v2_f32 dim_raw;
};

#endif
