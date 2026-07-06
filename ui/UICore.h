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
    void* app,
    ArrayVector<Vertex3DSamplerTextureColor>* vertices, ArrayVector<int32>* indices, byte* mem
) NO_EXCEPT;

struct UICore {
    // UIElementType
    // @performance we could probably move some of the date into one of the element bytes
    UIElementType type;

    byte opacity;

    // @question Consider to pull out into this struct to reduce alignment paddings
    //          We are currently wasting at least 3 bytes after opacity due to alignment
    UIAttributeDimension dimension;

    // 1-indexed, 0 = no update function defined
    int32 update_func;

    // Defines the style it uses
    int32 class_name;

    // Offset of the parent element
    // WARNING: this is a relative offset to this element
    int32 parent_offset;

    int32 vertex_count;
    int32 vertices;
};

#endif