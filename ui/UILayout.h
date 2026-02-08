#ifndef COMS_UI_LAYOUT_H
#define COMS_UI_LAYOUT_H

#include "../stdlib/Stdlib.h"
#include "../stdlib/HashMap.h"
#include "../asset/Asset.h"
#include "../font/Font.h"
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

    // @question Maybe we should have an array of fonts (e.g. allow up to 3 fonts per layout?)
    Font* font;

    // Used to directly find element by name
    // The values are pointers to the UIElements
    // @todo Should be a perfect hash map
    HashMap hash_map;

    // UI data
    // Only the size of the fixed layout, doesn't include the theme specific data
    // This is needed to know where we can add theme specific data into the data memory area
    uint32 layout_size;

    // Total used size (hard limited to 4 GB)
    // Most likely the theme has some additional free data available
    // This is because we might want to dynamically grow the theme
    uint32 data_size;

    // This is how much we actually use in the theme
    uint32 used_data_size;

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
    byte* data; // Owner of the actual data

    // @todo replace bools with bit field
    //      Or completely remove because we have gpu_updated which defines the state per widget
    //      At that point we also no longer differentiate between static and dynamic content

    // Changes on a as needed basis
    uint32 vertex_count_static;
    bool static_content_changed;

    // Changes every frame
    uint32 vertex_count_dynamic;
    bool dynamic_content_changed;

    // Contains both static and dynamic content
    // @todo The vertices shouldn't be an Asset, it's more like a ECS, maybe it's not even in RAM and only in VRAM?!
    // One of the reasons for this being an asset is also that it is easy to log ram/vram usage but that can be fixed
    Asset* ui_asset;

    // Total count of the ui_asset vertices
    uint32 vertex_count_max;

    // @question Should we maybe also hold the font atlas asset here?

    // Cache for elements to be used for rendering
    // This is very similar to the currently rendered UI output but may have some empty space between elements
    // The reason for this is that some elements may need different vertex counts for different states (e.g. input field)
    // WARNING: This memory is shared between different layouts
    uint32 active_vertex_count;

    // Currently this is a separate memory area than the data above
    // @question Do we want to make this part of the data above?
    Vertex3DSamplerTextureColor* vertices_active;

    // Used during the initialization so that every element knows where we currently are during the setup process
    uint32 active_vertex_offset;

    // Bitfield array defining which ui widgets are visible
    uint64 visible[5];

    // Bitfield array defining where the data is already stored on the gpu
    // Assuming a triple buffer = 3 bits per widget define where it lives
    // This information then can be used to check if it still needs to be updated
    // in the respective gpu buffer
    uint64 gpu_updated[15];
};

#endif