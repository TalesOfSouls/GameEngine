#pragma once
#ifndef COMS_UI_CORE_H
#define COMS_UI_CORE_H

#include "../stdlib/Stdlib.h"
#include "../stdlib/ArrayVector.h"
#include "attribute/UIAttributeDimension.h"
#include "UIElementType.h"
#include "UILayout.h"

// Creates the vertices in a custom way if necessary
// This is rarely necessary and recommended since the element itself has a default render function
typedef void* (*UIRenderFunction)(
    void* const __restrict app,
    ArrayVector<Vertex3DSamplerTextureColor>* const __restrict vertices, ArrayVector<int32>* const __restrict indices, byte* const __restrict mem
) NO_EXCEPT;

struct UICore {
    UIElementType type;

    // @performance we could move 1 bit out of here to the type to specify if it is active or not because that is what it is used for
    byte opacity;

    // 1-indexed, 0 = no function defined
    int16 update_func;
    int16 render_func;

    // @question Consider to pull out into this struct to reduce alignment paddings
    //          We are currently wasting at least 3 bytes after opacity due to alignment
    UIAttributeDimension dimension;

    // Defines the style it uses
    int32 class_name;

    // Offset of the parent element (absolute position)
    int32 parent_offset;

    int32 vertex_count;
    int32 vertices;
};

#endif