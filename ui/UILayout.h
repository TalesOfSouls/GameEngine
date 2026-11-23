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
#include "../object/TextureAtlas.h"
#include "../localization/Language.h"
#include "UIDocument.h"
#include "UITheme.h"
#include "UIData.h"

#define UI_LAYOUT_VERSION 10

/**
 * SNAPSHOT FORMAT (runtime state of the created elements), version UI_LAYOUT_VERSION
 * Written by layout_to_data(), read by layout_from_data().
 * NOT the .layoutbin file format: that is the parsed layout document + the data buffers,
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

////////////////////////////////////////////////////////////////
// Text layout files (.layouttxt)
////////////////////////////////////////////////////////////////

// Version of the .layouttxt format
#define UI_LAYOUT_TXT_VERSION 2

/**
 * TEXT FORMAT: .layouttxt, version UI_LAYOUT_TXT_VERSION
 *
 * Defines the element tree of a scene/hud. The syntax is the ui document syntax
 *:
 *
 *      version 2
 *
 *      // Comment (only full line comments)
 *      window test_window              <- "type [name]", the name is used by ui_element_get()
 *          style: window_default       <- styles of the theme, applied before the own properties
 *          anchor: top left
 *          pos: 512 256
 *          size: 560 480
 *          title: Test window
 *          $close                      <- part: fixed child created by the element itself
 *              on_click: window_close
 *
 *          label status                <- child element (more indentation than the parent)
 *              text: Status: -
 *
 *      indentation:  spaces, a tab counts as 4
 *      block:        "type [name]"
 *      property:     "key: value"
 *      part:         "$part [argument]"
 *
 * Value syntax
 *      numbers       12, -3.5, 50% (= relative to the parent for pos/size)
 *      size          0 = content size, negative = parent size minus the value
 *      colors        #RRGGBBAA or #RRGGBB
 *      lists         "Option A | Option B | Option C"
 *      text          \n new line, \t tab, \\ backslash, \@ literal @
 *      strings       @key  = string of the language of this file (UILayoutTxtContext::language)
 *                    @@key = global string of the application (UILayoutTxtContext::language_global)
 *      flags         hitbox interactable movable resizable hoverable selectable ignore_hit always_on_top
 *                    +flag / -flag add/remove, otherwise the flags are replaced
 *      anchor        "<parent point> [/ <element point>]", points: left|center|right top|center|bottom,
 *                    e.g. "top right" or "top right / top left"
 *
 * Order in which the properties are applied
 *      1. create arguments (e.g. columns of a table)
 *      2. type default styles of the themes ("@table")
 *      3. styles listed in "style:" (in this order)
 *      4. own properties and parts (file order)
 *      5. child elements
 *
 * Element types: window, panel, label, button, image, cursor, custom, checkbox,
 * radio, slider, input, textarea, select, tab, table, colorpicker, tooltip, data.
 * The actual list of the properties per element type is the table at the
 * top of the text parser in UILayout.cpp (all, label, button, ..., tooltip) - keep both in sync.
 *
 * Data elements (binary user data stored in the layout, e.g. the slots of a quick access bar):
 *      "data <name>" with size (bytes) + version, never drawn, the content is defined by the application
 *
 * BINARY FORMAT: .layoutbin = the parsed document + the data buffers (@see BINARY FORMAT below)
 * (layout_to_data / layout_from_data in UILayout.h are a runtime snapshot of the created elements, not this format)
 */

// Name -> id, e.g. function names of the layout file -> UICore::update_func
struct UINameId {
    const char* name;
    int32 id;
};

/**
 * Everything the layout file references by name
 * Unknown names are reported (LOG) and ignored
 */
struct UILayoutTxtContext {
    // Styles, later themes overwrite earlier themes (e.g. general theme, scene theme)
    const UITheme* themes[4];
    int32 theme_count;

    // Images (image, border_image), the id is the atlas element
    const TextureAtlas* atlas;
    const UINameId* images;
    int32 image_count;

    // Functions, the id is the 1-based index into UILayout::update/render/on_actions
    const UINameId* update_functions;
    int32 update_function_count;

    const UINameId* render_functions;
    int32 render_function_count;

    const UINameId* actions;
    int32 action_count;

    // Strings of the layout file ("@key"), NULL = no layout language
    // Strings in a theme file use the language of the theme (UITheme::language)
    const Language* language;

    // Global strings of the application ("@@key"), NULL = none
    const Language* language_global;

    // File the values of the data elements of this layout file are saved in (ui_data_file_create), 0 = not saved
    int32 data_file;
};

////////////////////////////////////////////////////////////////
// Binary layout files (.layoutbin)
////////////////////////////////////////////////////////////////

/**
 * BINARY FORMAT: .layoutbin, version UI_LAYOUT_BIN_VERSION
 *
 * The parsed layout document + the values of its data elements.
 * The elements are created from the document exactly like from the text file (ui_layout_from_document):
 * the styles of the theme + the strings of the language are applied when the elements are created,
 * several layout files can still be added to one UILayout.
 *
 * Used for
 *      1. The compiled layouts of a release (theme_builder: .layouttxt -> .layoutbin)
 *      2. The user copy of a layout (e.g. user/themes/<theme>/<name>.layoutbin): the elements of the original
 *         file + the values of its data elements changed by the user. Saving the user data only rewrites the
 *         data section (ui_layout_bin_data_update).
 *         The data elements are created zeroed, the data section is applied afterwards: by name, only if the
 *         size + version still match (a changed structure never receives old bytes, UI_DATA_FLAG_LOADED stays 0
 *         -> the application writes its defaults). Only data elements with user data are written.
 *         source_hash = hash of the original file -> a changed original (e.g. an update) is detected,
 *         the user copy is created again and the user data is carried over by name.
 *
 * All numbers are little endian, every field is written separately (no struct dumps -> independent of
 * the struct layout of a build).
 *
 *      offset  size    type        field
 *      0       4       uint32      magic UI_LAYOUT_BIN_MAGIC ("LDOC")
 *      4       4       int32       version UI_LAYOUT_BIN_VERSION
 *      8       8       uint64      source_hash (hash of the original file, 0 = unknown)
 *      16      4       int32       document version (version line of the text file)
 *      20      4       int32       node_count (incl. the reserved empty node 0)
 *      24      4       int32       property_count (incl. the reserved empty property 0)
 *      28      4       int32       pool_size (bytes of the string pool)
 *      32      4       int32       first_node (first top level node)
 *      36      4       int32       data_offset (byte offset of the data section)
 *      40      N * 41  -           nodes (node 0 included)
 *                                      byte kind, int32 type offset/length, int32 name offset/length,
 *                                      int32 line, first_property, last_property, first_child, last_child, next_sibling
 *              P * 24  -           properties (property 0 included)
 *                                      int32 key offset/length, int32 value offset/length, int32 next, int32 line
 *              S       char[S]     string pool (all strings of the nodes + properties, the offsets point into it)
 *      data_offset                 data section (the data elements of the file with user data)
 *              4       int32       element count
 *                                  per element: char[UI_LAYOUT_BIN_DATA_NAME_LENGTH] name, int32 version,
 *                                               int32 size, byte[size] content (as it is, the application defines it)
 */
#define UI_LAYOUT_BIN_MAGIC 0x434F444C // "LDOC"
// 2: data elements with binary content (size + version) instead of typed values
#define UI_LAYOUT_BIN_VERSION 2

#define UI_LAYOUT_BIN_HEADER_SIZE 40
#define UI_LAYOUT_BIN_NODE_SIZE 41
#define UI_LAYOUT_BIN_PROPERTY_SIZE 24
// Fixed size of the names in the data section (>= UI_DATA_NAME_LENGTH)
#define UI_LAYOUT_BIN_DATA_NAME_LENGTH 32
#define UI_LAYOUT_BIN_DATA_ENTRY_HEADER_SIZE (UI_LAYOUT_BIN_DATA_NAME_LENGTH + 4 + 4)

static_assert(UI_LAYOUT_BIN_DATA_NAME_LENGTH >= UI_DATA_NAME_LENGTH, "The data names must fit into the data section");

struct UILayoutBinHeader {
    uint32 magic;
    int32 version;
    uint64 source_hash;
    int32 doc_version;
    int32 node_count;
    int32 property_count;
    int32 pool_size;
    int32 first_node;
    int32 data_offset;
};

#endif
