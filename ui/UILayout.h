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

#define UI_LAYOUT_VERSION 10

/**
 * SNAPSHOT FORMAT (runtime state of the created elements), version UI_LAYOUT_VERSION
 * Written by layout_to_data(), read by layout_from_data().
 * NOT the .layoutbin file format: that is the parsed layout document + the data buffers (@see UILayoutBin.h),
 * the snapshot already contains the theme styles + language strings and replaces the whole layout.
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
 * Hash map header (chunk memory dump / chunk_dump)
 *
 *      offset  size    type    field
 *      0       4       int32   capacity        amount of name slots
 *      4       4       int32   last_pos        last reserved slot (-1 = none)
 *      8       4       uint32  free_offset     capacity * 32
 *      12      S       byte[]  slot memory (slots first, then the free bitmap)
 *
 * Slots are HashEntryStrT<int32> (32 bytes: char[22] key, uint16 next, uint16 head,
 * uint16 tag, int32 value = byte offset of the element), the free bitmap follows the
 * slots (8 bytes per 64 slots, bit set = slot in use).
 *
 * Element buffer: the raw content of UILayout::ui_element_buffer, i.e. the element
 * tree in pre-order (parent, child 1, grandchild 1.1, child 2, ...). Every element
 * starts with UICore followed by the type specific fields and the inline
 * strings (class name, text). All links are byte offsets into the buffer, offset 0 is
 * always the root, so the block is position independent.
 *
 * Label and input texts are C strings with an 8 byte SimpleStringInline header in
 * front of them (int32 length, int32 max_length), the elements store the offset of
 * the characters ui_string_reserve().
 *
 * WARNING: the element buffer is a memory dump without any endian or layout
 * conversion. It can only be loaded by a build with the same struct layout and
 * the same endianness.
 *
 * Text format: .layouttxt,
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

typedef struct UILayout UILayout;
struct UICursor;

typedef void *(*UIUpdateFunc)(
    void* user_data,
    UILayout* layout,
    UICore* core
) NO_EXCEPT;

// Called by user interaction (e.g. UIButton::on_click, UICheckbox::on_change)
// Same signature as UIUpdateFunc, the separate name documents the role of the callback tables
typedef UIUpdateFunc UIActionFunc;

typedef void *(*UIRenderFunc)(
    void* user_data,
    UICore* element,
    GpuApiType gpu_api_type,
    UILayout* const layout, f32 zindex,
    BufferMemory* const __restrict mem
) NO_EXCEPT;

// Current pointer (mouse) interaction state (ui_input_pointer)
// All elements are offsets into ui_element_buffer (0 = none)
// @performance The struct below is very verbose but I think a lot of variables could be merged
//              in combination with a helper variable
struct UIInteraction {
    // Element with UI_STATE_HOVER
    int32 hovered;

    // Element the primary button was pressed on (UI_STATE_ACTIVE), receives the click on release
    int32 active;

    // Element with UI_STATE_FOCUS (e.g. the window that was clicked last)
    int32 focused;

    // Element that is moved by dragging (e.g. the window while dragging its title)
    int32 dragged;

    // Element that was clicked in this tick
    // (pointer click or enter/space on the focused element, reset by ui_input_pointer())
    int32 clicked;

    // Text element that receives the keyboard input (UIInput/UITextarea)
    int32 editing;

    // Non text element focused by navigation (e.g. tab, controller d-pad, ui_focus_move())
    // Text elements use editing instead, both are never set at the same time
    int32 nav_focus;

    // Open popup ui_popup_open(), e.g. the option list of a select, a color picker or a context menu
    int32 popup;

    // Element that was hit by the secondary button (right click) in the last ui_input_pointer() call
    // The application decides what happens (e.g. open a context menu)
    int32 context_target;

    // Element that is resized by dragging a UI_ELEMENT_FLAG_RESIZABLE element (e.g. window by its resize grip)
    int32 resized;

    // Element whose scrollbar thumb is dragged
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
    // e.g. for tooltips of custom elements (ui_tooltip_show)
    v2_f32 pointer;

    // Primary button state of the last ui_input_pointer() call
    bool pointer_down;

    // The UI uses the pointer (pointer is over the UI, pressing a UI element or dragging)
    // -> the game should ignore the pointer input
    bool pointer_captured;

    // Hit test of the last ui_input_pointer() call (element offsets, 0 = none)
    // Reused while the pointer didn't move and the layout didn't change (not dirty + same UILayout::hit_epoch)
    v2_f32 hit_pointer;
    uint32 hit_epoch;
    int32 hit_element;
    int32 hit_action;
    bool hit_valid;
};

// Modified for every scene
// What the last ui_cache() changed in its output (ui_vertex_cache/ui_index_cache)
// Lets the application copy/upload only the changed part of the ui
// @todo Profile if this is actually happening in reasonable amounts
//      or if this is actually useless since it never applies
struct UICachePatch {
    // true  = full update/upload required
    // false = partial update/upload possible
    bool full;

    // Changed vertex/index range, start >= end = nothing changed
    int32 vertex_start;
    int32 vertex_end;
    int32 index_start;
    int32 index_end;
};

enum UILayoutDirty : byte {
    UI_LAYOUT_DIRTY_NONE = 0,

    // Something changed since the last ui_cache()
    // -> may not update immediately if the last update was too recent
    UI_LAYOUT_DIRTY = 1 << 0,

    // The change was caused by user interaction
    // -> cache immediately
    // -> ignores ui update limitations
    UI_LAYOUT_DIRTY_INTERACTION = 1 << 1,
};

enum UILayoutPointerForced : byte {
    UI_POINTER_FORCED_NONE = 0,

    // Every open window that needs the pointer sets this during the tick
    UI_POINTER_FORCED_REQUEST = 1 << 0,

    // At least one window needed the pointer last tick, the application shows it even if its own pointer mode is off
    // The application turns the requests into this state once per tick and clears the requests
    UI_POINTER_FORCED = 1 << 1,
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

    // @type UILayoutDirty
    byte dirty;

    // The pointer is not only a UI rendering aspect it also determins camera behavior/availability
    // @type UILayoutPointerForced
    byte pointer_forced;

    // This is where we actually store the elements
    // It has to use an arbitrary buffer size per element since the element size is not uniform
    // This also hold custom data for elements (e.g. UI hotbars)
    BufferMemory ui_element_buffer;

    // Output of the last ui_cache() (vertices + indices of all visible elements)
    // Unchanged elements are copied from here instead of re-generating them
    ArrayVector<Vertex3DSamplerTextureColor> ui_vertex_total;
    ArrayVector<int32> ui_index_total;

    // Stores the current version
    ArrayVector<Vertex3DSamplerTextureColor> ui_vertex_cache;
    // This is a index cache that references ui_vertex_cache to reduce the stored vertices
    ArrayVector<int32> ui_index_cache;

    // Counts the ui_cache() calls, the cache data of an element is only valid for the last call
    uint32 cache_frame;

    // Incremented by every ui_cache() incl. the in-place patch (which keeps cache_frame)
    // Every change of the layout marks it dirty until the next ui_cache() -> not dirty + same epoch = unchanged layout
    uint32 hit_epoch;

    // A simple hash created from values that affect all elements (e.g. font, font scale),
    // if they change everything is re-generated
    uint64 cache_global_hash;

    // Statistics of the last ui_cache() call
    // @question Consider to move too STATS logging
    int32 cache_elements_generated;
    int32 cache_elements_reused;

    // What the last ui_cache() changed
    // This doesn't describe a single element that changed but an entire range that changed
    // ui_vertex_cache and ui_index_cache
    UICachePatch cache_patch;

    // Scrollbar drag state of the last ui_cache(),
    // the scrollbars are only re-generated by a full ui_cache()
    int32 cache_scroll_dragged;
    byte cache_scroll_drag_axis;

    const UIUpdateFunc* update;
    const UIRenderFunc* render;

    // Action callbacks, e.g. UIButton::on_click (1-indexed)
    const UIActionFunc* on_actions;
};

#endif
