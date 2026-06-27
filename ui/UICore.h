#pragma once
#ifndef COMS_UI_CORE_H
#define COMS_UI_CORE_H

#include "../stdlib/Stdlib.h"
#include "../stdlib/ArrayVector.h"
#include "attribute/UIAttributeDimension.h"
#include "UILayout.h"
#include "UIOffset.h"

// Only updates data
typedef void* (*UIUpdateFunction)(
    void* app, UILayout* layout, UIOffset* offset, void* element
) NO_EXCEPT;

// Creates the vertices in a custom way if necessary
// This is rarely necessary and recommended since the element itself has a default render function
typedef void* (*UIRenderFunction)(
    void* app,
    ArrayVector<Vertex3DSamplerTextureColor>* vertices, ArrayVector<int32>* indices, byte* mem
) NO_EXCEPT;

struct UICore {
    byte opacity;

    // @question Consider to pull out into this struct to reduce alignment paddings
    //          We are currently wasting at least 3 bytes after opacity due to alignment
    UIAttributeDimension dimension;
    UIUpdateFunction update_func;

    // Defines the style it uses
    char* class_name;
};

#endif