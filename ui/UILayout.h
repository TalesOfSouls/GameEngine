#pragma once
#ifndef COMS_UI_LAYOUT_H
#define COMS_UI_LAYOUT_H

#include "../stdlib/Stdlib.h"
#include "../stdlib/HashMapT.h"
#include "../stdlib/ArrayVector.h"
#include "../asset/Asset.h"
#include "../font/FontSystem.h"
#include "../object/Vertex.h"
#include "../gpuapi/GpuApiType.h"
#include "UICore.h"

// Version 2: flat pre-order element tree (@see UICore)
// Version 3: UIPanel scroll/popup data, new element types (tabs, table, option list, color picker)
// Version 4: UIColorPicker palette order, UITable dynamic cells
// Version 5: UICore incremental caching members
// Version 8: Label/input texts are stored with their length (@see ui_string_reserve)
// Version 9: New string hash of the element hash map (@see hashmap_str_hash)
// Version 10: UITable row values + user value
#define UI_LAYOUT_VERSION 10

/**
 * BINARY FORMAT: .layoutbin, version UI_LAYOUT_VERSION
 * Written by layout_to_data(), read by layout_from_data() (@see UILayout.cpp).
 *
 *      offset      size    type        field
 *      0           4       int32       version (UI_LAYOUT_VERSION)
 *      4           12      -           hash map header
 *      16          S       byte[S]     hash map slots (+ free bitmap)
 *      16 + S      4       int32       buffer_size = B
 *      20 + S      B       byte[B]     element buffer
 *
 *      S = capacity * 32 + ceil(capacity / 64) * 8 + 8      (capacity = element name slots)
 *
 * Hash map header (chunk memory dump, @see hashmap_dump / chunk_dump)
 *
 *      offset  size    type    field
 *      0       4       int32   capacity        amount of name slots
 *      4       4       int32   last_pos        last reserved slot (-1 = none)
 *      8       4       uint32  free_offset     capacity * 32
 *      12      S       byte[]  slot memory (slots first, then the free bitmap)
 *
 * Slots are HashEntryStrT<int32> (32 bytes: char[22] key, uint16 next, uint16 head,
 * uint16 tag, int32 value = byte offset of the element), the free bitmap follows the
 * slots (8 bytes per 64 slots, bit set = slot in use). @see UITheme.h for the details.
 *
 * Element buffer: the raw content of UILayout::ui_element_buffer, i.e. the element
 * tree in pre-order (parent, child 1, grandchild 1.1, child 2, ...). Every element
 * starts with UICore (96 bytes) followed by the type specific fields and the inline
 * strings (class name, text). All links are byte offsets into the buffer, offset 0 is
 * always the root, so the block is position independent.
 *
 * Label and input texts are C strings with an 8 byte SimpleStringInline header in
 * front of them (int32 length, int32 max_length), the elements store the offset of
 * the characters (@see ui_string_reserve).
 *
 * WARNING: the element buffer is a memory dump without any endian or layout
 * conversion. It can only be loaded by a build with the same struct layout and
 * the same endianness.
 *
 * Text format: .layouttxt, @see UILayoutTxt.h.
 */

///////////////////////////////
// ui_element_buffer memory layout
// ============================
// Root (offset 0)
// Element A
//      Child A.1 (e.g. fixed child that was created together with A)
//      Child A.2
// Element B
// String data of B (e.g. label content)
// ...
// Element C (added later, linked as child of A through next_offset/skip_offset)
// ============================

struct UIChromaCodes {
    // This array has the size of the game window and represents in color codes where interactible ui elements are
    // Size is based on screen size (we don't need full screen size since we assume an interactible element is at least 4 pixels width and height)
    //      width = 25% of screen size
    //      height = 25% of screen size
    // WARNING: what if we want to support 8k resolution? in that case 4 pixel get reduced to 1 pixel
    //      That means a border doesn't really have a good hitbox
    uint16 width;
    uint16 height;

    // Contains all UI elements also dynamic ones (e.g. movable windows)
    // Every ui element has it's own color code and with that we can identify the currently hovered one
    uint32* codes;
};

typedef struct UILayout UILayout;
struct UICursor;

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
    BufferMemory* const __restrict mem
) NO_EXCEPT;

// Current pointer (mouse) interaction state (@see ui_input_pointer)
// All elements are offsets into ui_element_buffer (0 = none)
// @performance The struct below is very verbose but I think a lot of variables could be merged
//              in combination with a helper variable
//              e.g. it's impossible to have different elements as focused and clicked at the same time
//              same goes for editing and active and so on. you could probably group some offsets/ids and create a
//              bitfield that defines if active, focused etc.
struct UIInteraction {
    // Element with UI_STATE_HOVER
    int32 hovered;

    // Element the primary button was pressed on (UI_STATE_ACTIVE), receives the click on release
    int32 active;

    // Element with UI_STATE_FOCUS (e.g. the window that was clicked last)
    int32 focused;

    // Element that is moved by dragging (e.g. the window while dragging its title)
    int32 dragged;

    // Element that was clicked in this tick (pointer click or enter/space on the focused element, reset by ui_input_pointer())
    int32 clicked;

    // Text element that receives the keyboard input (UIInput/UITextarea, @see ui_input_text)
    int32 editing;

    // Non text element that receives the keyboard input (e.g. button focused with tab, @see ui_focus_move)
    // Text elements use editing instead, both are never set at the same time
    int32 keyboard_focus;

    // Open popup (@see ui_popup_open), e.g. the option list of a select, a color picker or a context menu
    int32 popup;

    // Element that was hit by the secondary button (right click) in the last ui_input_pointer() call
    // The application decides what happens (e.g. open a context menu @see ui_popup_open)
    int32 context_target;

    // Element that is resized by dragging a UI_ELEMENT_FLAG_RESIZABLE element (e.g. window by its resize grip)
    int32 resized;

    // Element whose scrollbar thumb is dragged (@see UIScrollAxis for scroll_drag_axis)
    int32 scroll_dragged;
    f32 scroll_drag_start;
    byte scroll_drag_axis;

    // Pointer position and element position (pos_raw)/size (dim_raw) when the drag/resize started
    v2_f32 drag_start_pointer;
    v2_f32 drag_start_pos;
    v2_f32 drag_start_dim;

    // Multi click detection (double click = select word, triple click = select all)
    uint64 last_click_time;
    v2_f32 last_click_pointer;
    int32 last_click_element;
    byte click_count;

    // Pointer position of the last ui_input_pointer() call in ui coordinates
    // e.g. for tooltips of custom elements (@see ui_tooltip_show)
    v2_f32 pointer;

    // Primary button state of the last ui_input_pointer() call
    bool pointer_down;

    // The UI uses the pointer (pointer is over the UI, pressing a UI element or dragging)
    // -> the game should ignore the pointer input
    bool pointer_captured;
};

// Modified for every scene
// What the last ui_cache() changed in its output (ui_vertex_cache/ui_index_cache)
// Lets the application copy/upload only the changed part of the ui (@see ui_cache)
struct UICachePatch {
    // true  = the whole output was re-built (the vertex/index counts may have changed)
    // false = the output was patched in place, only the ranges below changed (the counts are the same)
    bool full;

    // Changed vertex/index range [start, end), start >= end = nothing changed
    int32 vertex_start;
    int32 vertex_end;
    int32 index_start;
    int32 index_end;
};

struct UILayout {
    // @question It feels weird that this is here, especially considering we could have multiple fonts
    // Maybe we should have an array of fonts (e.g. allow up to 3 fonts per theme?)
    FontSystem* font;

    // Used to directly find offsets by name
    // The values are offsets into ui_element_buffer
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

    // Used for the string representation
    Asset* ui_asset;

    // Always the first element in ui_element_buffer (offset 0)
    // The root has the size of the screen
    UICore* ui_root;

    // This is just a helper because a cursor is a special UI element that is needed frequently
    UICursor* cursor;

    // Hover, press, focus, drag state
    UIInteraction interaction;

    // Something changed since the last ui_cache()
    // The flag stays set until ui_cache() runs, e.g. if caching is throttled by a fps limit
    // Custom elements (UI_ELEMENT_TYPE_CUSTOM) render through a callback, their update callback
    // has to mark them as changed if their output changes
    bool is_dirty;

    // The change was caused by user interaction (pointer, navigation, text input + the actions they trigger,
    // @see ui_input_pointer, ui_input_text, ui_input_navigate) -> the application should cache immediately
    // Other changes (only is_dirty, e.g. update functions like a fps label) can be throttled
    // Reset by ui_cache()
    bool is_dirty_interaction;

    // This is where we actually store the elements (@see UICore for the tree structure)
    // It has to use an arbitrary buffer size per element since the element size is not uniform
    BufferMemory ui_element_buffer;

    // Output of the last ui_cache() (vertices + indices of all visible elements)
    // Unchanged elements are copied from here instead of re-generating them (@see ui_cache)
    // Not allocated (capacity 0) = no incremental caching
    ArrayVector<Vertex3DSamplerTextureColor> ui_vertex_total;
    ArrayVector<int32> ui_index_total;

    // Stores the current version
    ArrayVector<Vertex3DSamplerTextureColor> ui_vertex_cache;
    // This is a index cache that references ui_vertex_cache to reduce the stored vertices
    ArrayVector<int32> ui_index_cache;

    // Counts the ui_cache() calls, the cache data of an element is only valid for the last call
    uint32 cache_frame;

    // Values that affect all elements (e.g. font), if they change everything is re-generated
    uint64 cache_global;

    // Statistics of the last ui_cache() call
    int32 cache_elements_generated;
    int32 cache_elements_reused;

    // What the last ui_cache() changed (@see UICachePatch)
    UICachePatch cache_patch;

    // Scrollbar drag state of the last ui_cache(), the scrollbars are only re-generated by a full ui_cache()
    int32 cache_scroll_dragged;
    byte cache_scroll_drag_axis;

    const UIUpdateFunc* update;
    const UIRenderFunc* render;

    // Action callbacks, e.g. UIButton::on_click (1-indexed)
    const UIUpdateFunc* on_actions;
};

#endif
