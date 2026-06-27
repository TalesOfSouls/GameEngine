#pragma once
#ifndef COMS_UI_ATTRIBUTE_H
#define COMS_UI_ATTRIBUTE_H

#include "../../stdlib/Stdlib.h"
#include "../../utils/StringUtils.h"
#include "UIAttributeType.h"
#include "UIAttributeDimension.h"
#include "UIAttributeFont.h"
#include "../../math/Evaluator.h"

enum UIAttributeDataType : byte {
    UI_ATTRIBUTE_DATA_TYPE_INT,
    UI_ATTRIBUTE_DATA_TYPE_F32,
    UI_ATTRIBUTE_DATA_TYPE_STR,
    UI_ATTRIBUTE_DATA_TYPE_V4_F32,
};

struct UIAttribute {
    // Attributes use ids (=type name) instead of strings
    UIAttributeType attribute_id;
    UIAttributeDataType datatype;

    union {
        // @performance The string makes this struct really large when it is not needed in 95% of the cases
        char value_str[32];
        int32 value_int;
        uint32 value_uint;
        f32 value_float;
        v4_f32 value_v4_f32;
    };
};

// Basically a class/id in HTML/CSS terms
struct UIAttributeGroup {
    // We are using int32 to at least ensure 4 byte alignment of pointer manipulation (group + 1)
    int32 attribute_count;
    // We don't use a pointer since this would prevent us from copying around the main data owner
    // The UIAttribute values come directly after UIAttributeGroup (e.g. group + 1 in memory)
    //UIAttribute* attributes;
};

#endif