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

// @bug I hate this forward declaration
typedef struct UILayout UILayout;
typedef struct UICore UICore;

typedef void *(*UIUpdateFunc)(
    void* user_data,
    UILayout* layout,
    UICore* core
) NO_EXCEPT;

typedef void *(*UIRenderFunc)(
    void* user_data,
    UICore* element,
    GpuApiType gpu_api_type,
    UILayout* const layout, f32 zindex,
    byte* const __restrict mem
) NO_EXCEPT;

enum UIElementChangeType {
    // If an element go larger we can update it VERY efficiently
    UI_ELEMENT_CHANGE_DIM_LARGER = 1 << 0,

    // dimensions got smaller or one axis got smaller and only one got bigger
    UI_ELEMENT_CHANGE_DIM_OTHER = 1 << 1,

    // Z-axis changed
    UI_ELEMENT_CHANGE_ORDER = 1 << 2,

    // This is the most complex change since it also results in different vertex counts
    // We now have to change the entire vertex cache/index cache after this element as well
    UI_ELEMENT_CHANGE_CONTENT = 1 << 3,
};

struct UIElementChange {
    int32 element;
    uint32 change_type;
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
    // The values are pointers to the UICore
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

    // Every element in this array is an offset to a changed element
    // This allows us to identify and re-draw changed elements quickly
    ArrayVector<UIElementChange> ui_element_changed;

    void* ui_root;

    // This array links into the ui_element_buffer via offsets
    // We need to know what the root elements are for our rendering
    // Think of this array as the first level in a tree
    // but instead of storing pointers or the elements themselves we store the offset to the "root" elements
    // When rendering we iterate over these root elements
    // and internally then over all children of these root elements
    ArrayVector<int32> ui_element_root;

    // This is where we actually store the elements
    // It has to be an arbitrary buffer since the element size is not uniform
    BufferMemory ui_element_buffer;

    // Stores all of the vertex data (even for different states e.g. hover, ...)
    ArrayVector<Vertex3DSamplerTextureColor> ui_vertex_total;

    // Stores the current version
    ArrayVector<Vertex3DSamplerTextureColor> ui_vertex_cache;
    // This is a index cache that references ui_vertex_cache to reduce the stored vertices
    ArrayVector<int32> ui_index_cache;

    const UIUpdateFunc* update;
    const UIRenderFunc* render;
    const UIUpdateFunc* on_actions;
};

#endif