#pragma once
#ifndef COMS_UI_ATTRIBUTE_DIMENSION_H
#define COMS_UI_ATTRIBUTE_DIMENSION_H

#include "../../stdlib/Stdlib.h"

// @todo we need per axis flags
//      we could have % position for x but px position for y
enum UIDimensionFlag : byte {
    // Are the values relative (based on container) or absolute
    UI_DIMENSION_POS_X_RELATIVE = 1 << 0,
    UI_DIMENSION_POS_Y_RELATIVE = 1 << 1,

    UI_DIMENSION_DIM_X_RELATIVE = 1 << 2,
    UI_DIMENSION_DIM_Y_RELATIVE = 1 << 3,

    // Do we use pixel or relative units
    UI_DIMENSION_POS_X_PX = 1 << 4,
    UI_DIMENSION_POS_Y_PX = 1 << 5,

    UI_DIMENSION_DIM_X_PX = 1 << 6,
    UI_DIMENSION_DIM_Y_PX = 1 << 7,
};

struct UIAttributeDimension {
    // @see UIDimensionFlag
    byte flag;

    // @performance see the enum definitions we could combine the two bit fields

    // @see UIAlign
    byte alignment;

    // @see UIAnchor
    byte anchor;

    // @performance I am wasting 1 byte here (at least). With the above remark more like 2

    // This is the calculated position/dimension
    v2_f32 pos;
    v2_f32 dim;

    // This is the original position definition required if we move/resize the UI element or change the resolution
    v2_f32 pos_raw;
    v2_f32 dim_raw;
};

#endif