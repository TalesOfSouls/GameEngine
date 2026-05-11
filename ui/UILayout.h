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

// Modified for every scene
struct UILayout {
    // This array has the size of the game window and represents in color codes where interactible ui elements are
    // Size is based on screen size (we don't need full screen size since we assume an interactible element is at least 4 pixels width and height)
    //      width = 25% of screen size
    //      height = 25% of screen size
    uint16 width;
    uint16 height;

    // Contains all UI elements also dynamic ones (e.g. movable windows)
    // Every ui element has it's own color code and with that we can identify the currently hovered one
    uint32* ui_chroma_codes;

    // @question It feels weird that this is here, especially considering we could have multiple fonts
    // Maybe we should have an array of fonts (e.g. allow up to 3 fonts per theme?)
    FontSystem* font;

    // Used to directly find element by name
    // The values are pointers to the UIElements
    // @todo Should be a perfect hash map
    HashMap hash_map;

    // Holds the ui elements
    // The structure of the data is as follows:
    //      1. HashMap data (points to 2.a.)
    //      2. UIElements (default)
    //          a. General UIElement
    //          b. Children array (uint32)
    //          c. Element specific state
    //          d. Element specific active style (very important for animations)
    //          e. Element specific default style (not the other styles)
    //      3. Additional UI styles (see c.), dynamically created when the theme is loaded
    // We effectively create a tree in data where the individual elements can get directly accessed through the hashmap
    // WARNING: This memory is shared between different layouts
    //      1. When we load a new layout we assign a temp memory buffer to this pointer
    //      2. Once we are ready to switch the scene we copy the temporary memory into this data pointer
    uint32 data_size;
    byte* data; // Owner of the actual data

    // @todo replace bools with bit field
    //      Or completely remove because we have gpu_updated which defines the state per widget
    //      At that point we also no longer differentiate between static and dynamic content

    // Contains both static and dynamic content
    // @todo The vertices shouldn't be an Asset, it's more like a ECS, maybe it's not even in RAM and only in VRAM?!
    // One of the reasons for this being an asset is also that it is easy to log ram/vram usage but that can be fixed
    Asset* ui_asset;

    // @question Should we maybe also hold the font atlas asset here?

    // Bitfield array defining which ui widgets are visible
    // @performance is this still used?
    uint64 visible[5];

    // Bitfield array defining where the data is already stored on the gpu
    // Assuming a triple buffer = 3 bits per widget define where it lives
    // This information then can be used to check if it still needs to be updated
    // in the respective gpu buffer
    // @performance is this still used?
    uint64 gpu_updated[15];

    // Testing
    // @question Consider to replace this with its own data type ArrayVector
    //int16 ui_offset_root_length;
    //int16 ui_offset_root_pos;
    // This array links into the ui_offset_buffer
    //int32* ui_offset_root;
    ArrayVector<int32> ui_offset_root;

    BufferMemory ui_offset_buffer;
    BufferMemory ui_element_buffer;

    // Stores all of the vertex data (even for different states e.g. hover, ...)
    //int32 ui_vertex_total_length;
    //int32 ui_vertex_total_pos;
    //Vertex3DSamplerTextureColor* ui_vertex_total;
    ArrayVector<Vertex3DSamplerTextureColor> ui_vertex_total;

    // Stores the current version
    //int32 ui_vertex_cache_length;
    //int32 ui_vertex_cache_pos;
    //Vertex3DSamplerTextureColor* ui_vertex_cache;
    ArrayVector<Vertex3DSamplerTextureColor> ui_vertex_cache;
    ArrayVector<int32> ui_index_cache;
};

#endif