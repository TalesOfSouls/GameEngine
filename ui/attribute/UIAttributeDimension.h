#pragma once
#ifndef COMS_UI_ATTRIBUTE_DIMENSION_H
#define COMS_UI_ATTRIBUTE_DIMENSION_H

#include "../../stdlib/Stdlib.h"

enum UIDimensionFlag : byte {
    // Are the values relative (based on container) or absolute
    UI_DIMENSION_POS_RELATIVE = 1 << 0,
    UI_DIMENSION_DIM_RELATIVE = 1 << 1,

    // Are the values dynamically calculated?
    UI_DIMENSION_POS_DYN = 1 << 2,
    UI_DIMENSION_DIM_DYN = 1 << 3,

    // Are the values modifiable by the user (live)
    UI_DIMENSION_POS_MODIFIABLE = 1 << 4,
    UI_DIMENSION_DIM_MODIFIABLE = 1 << 5,

    // Do we use pixel or relative units
    UI_DIMENSION_DIM_PX = 1 << 6,
    UI_DIMENSION_SIZE_PX = 1 << 7,
};

struct UIAttributeDimension {
    // @see UIDimensionFlag
    byte flag;

    // @see UIAlign
    byte alignment;

    v2_f32 pos;
    v2_f32 dimension;

    // Sometimes position and dimension are relative to a parent element
    // In such case we take the parent pos/dimension + these relative information to build the absolute pos
    // Since we are rendering back to front this nicely ensures that the parent element MUST have already been computed
    v2_f32 pos_rel;
    v2_f32 dimension_rel;
};

#endif