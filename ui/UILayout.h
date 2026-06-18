#pragma once
#ifndef COMS_UI_LAYOUT_H
#define COMS_UI_LAYOUT_H

#include "../stdlib/Stdlib.h"
#include "../stdlib/HashMap.h"
#include "../stdlib/ArrayVector.h"
#include "../asset/Asset.h"
#include "../font/FontSystem.h"
#include "../object/Vertex.h"

#define UI_LAYOUT_VERSION 1

///////////////////////////////
// UIElement
// ============================
// child_offset 1
// child_offset 2
// ...
// ============================
// UIElementState
// ============================
// UIElementStyle Active
// UIElementStyle Default
// ...
// ============================

// ...
// Somewhere else in the buffer
// ...

// UIAnimation 1 - Info
// ============================
// UIAnimation 1 - Keyframe 1
// UIAnimation 1 - Keyframe 2
// ...
// ============================
// UIAnimation 2
// ============================
// ...
// ============================

struct UIChromaCodes {
    // This array has the size of the game window and represents in color codes where interactible ui elements are
    // Size is based on screen size (we don't need full screen size since we assume an interactible element is at least 4 pixels width and height)
    //      width = 25% of screen size
    //      height = 25% of screen size
    uint16 width;
    uint16 height;

    // Contains all UI elements also dynamic ones (e.g. movable windows)
    // Every ui element has it's own color code and with that we can identify the currently hovered one
    uint32* codes;
};

// Modified for every scene
struct UILayout {
    // We use a simple RGBA image to detect what kind of UI component the mouse his currently hovering
    // every UI component has an ID that is translated into RGBA values
    // The color code 0 represents no element
    // the code itself also represents the offset position into ui_offset_buffer making a lookup extremely fast
    UIChromaCodes chroma_codes;

    // @question It feels weird that this is here, especially considering we could have multiple fonts
    // Maybe we should have an array of fonts (e.g. allow up to 3 fonts per theme?)
    FontSystem* font;

    // Used to directly find offsets by name
    // The values are pointers to the UIOffset
    // @todo Consider to use a perfect hash map
    HashMapT<HashEntryStrT<int32>> hash_map;

    // Total size of the layout incl. hash_map
    // Most likely the layout has some additional free data available
    // This is because we might want to dynamically grow the layout/allow modification that increases the layout size
    //      e.g. layout didn't define an outline, user wants to have an outline -> additional attribute needed
    int32 data_size;

    // This is how much we actually use in the layout right now
    int32 used_data_size;

    // This is the owner of the hashmap data
    byte* data;

    // Used for the vao, vbo, ...
    // @todo It's stupid that we use Asset for this, fix
    Asset* ui_asset;

    // This array links into the ui_offset_buffer
    ArrayVector<int32> ui_offset_root;

    BufferMemory ui_offset_buffer;
    BufferMemory ui_element_buffer;

    // Stores all of the vertex data (even for different states e.g. hover, ...)
    ArrayVector<Vertex3DSamplerTextureColor> ui_vertex_total;

    // Stores the current version
    ArrayVector<Vertex3DSamplerTextureColor> ui_vertex_cache;
    ArrayVector<int32> ui_index_cache;
};

#include "UIOffset.h"
typedef void* (*UIUpdateFunc)(void*, UILayout*, UIOffset*, void*) NO_EXCEPT;

#endif