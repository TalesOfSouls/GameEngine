#pragma once
#ifndef COMS_UI_ATTRIBUTE_DIMENSION_H
#define COMS_UI_ATTRIBUTE_DIMENSION_H

#include "../../stdlib/Stdlib.h"

enum UIDimensionFlag : byte {
    // Are the values relative (based on container) or absolute
    UI_DIMENSION_POS_RELATIVE = 1 << 0,
    UI_DIMENSION_DIM_RELATIVE = 1 << 1,

    // Are the values dynamically calculated? Meaning they depend on the parent
    UI_DIMENSION_POS_DYN = 1 << 2,
    UI_DIMENSION_DIM_DYN = 1 << 3,

    // Are the values modifiable by the user (live)
    UI_DIMENSION_POS_MODIFIABLE = 1 << 4,
    UI_DIMENSION_DIM_MODIFIABLE = 1 << 5,

    // Do we use pixel or relative units
    UI_DIMENSION_POS_PX = 1 << 6,
    UI_DIMENSION_DIM_PX = 1 << 7,
};

struct UIAttributeDimension {
    // @see UIDimensionFlag
    byte flag;

    // @see UIAlign
    byte alignment;

    // @see UIAnchor
    byte anchor;

    // @performance I am wasting 1 byte here (at least)

    // This is the calculated position/dimension
    v2_f32 pos;
    v2_f32 dim;

    // This is the original position definition required if we move/resize the UI element or change the resolution
    v2_f32 pos_raw;
    v2_f32 dim_raw;
};

#endif