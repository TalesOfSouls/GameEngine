/**
 * How does UI rendering work?
 *
 * 1. Load UI components from layout file or create programmatically
 * 2. Style elements by loading theme from file
 *
 * 3. Update state of UI components
 * 4. (Re-)Create vertices cache of changed UI components
 *
 * 5. (Re-)Create and add cache for *some* components to CPU cache (GpuAtlasCache)
 * 6. Render *some* UI components and elements to a on gpu texture cache
 * 7. Copy vertices to compact/continuous vertex array on CPU
 *
 * 8. Draw vertices/send cache to GPU
 *
 * Data layout
 * ============================
 * All elements are stored in pre-order (depth-first) in UILayout::ui_element_buffer
 * Elements only contain resolved (inline) data, e.g. border/image uv coordinates are copied from the atlas once
 * -> Caching is a single linear walk through the element buffer without any lookups
 */
#pragma once
#ifndef COMS_UI_C
#define COMS_UI_C

#include "UILayout.cpp"
#include "../camera/Camera.cpp"
#include "UICore.cpp"
#include "UIPanel.cpp"
#include "UILabel.cpp"
#include "UIButton.cpp"
#include "UIWindow.cpp"
#include "UICursor.cpp"
#include "UIImage.cpp"
#include "UICheckbox.cpp"
#include "UISlider.cpp"
#include "UINavigation.h"
#include "UIInput.cpp"
#include "UISelect.cpp"
#include "UITab.cpp"
#include "UITable.cpp"
#include "UIColorPicker.cpp"
#include "UITooltip.cpp"
#include "UIClip.h"
#include "UIScrollbar.h"
#include "../hash/GeneralHash.h"
#include "../input/Input.cpp"

// Starting z index of the UI, every element moves a step closer to the camera
#define UI_ZINDEX_START 10.0f

// Max nesting of clipping panels (e.g. scroll panel in a scroll panel)
#define UI_CLIP_STACK_SIZE 4

// Input state that holds the cursor/pointer
// @question Consider to move to UILayout instead of global state
static int32 _ui_pointer_input = 0;

// Is the element and all of its parents visible?
static inline
bool ui_element_is_visible(const UILayout* const layout, const UICore* element) NO_EXCEPT
{
    while (element) {
        if (element->state & UI_STATE_HIDDEN) {
            return false;
        }

        element = ui_element_parent(layout, element);
    }

    return true;
}

/**
 * Runs the update functions (UICore::update_func) of all visible elements
 *
 * The update functions are responsible for flagging their changes (UICore::change_type + UILayout::dirty)
 * and only if something actually changed (e.g. the fps label only if the fps changed).
 * The ui functions that change elements (e.g. ui_label_set_text, ui_table_set_cell) flag the element themselves,
 * so an update function just has to avoid calling them if nothing changed.
 *
 * MUST be called before ui_cache(), ui_cache() itself never changes the element state
 *
 * @param bool content_tick false = only the update functions of elements with UI_ELEMENT_FLAG_UPDATE_TICK run
 *                          (e.g. the cursor), true = all of them (e.g. labels/tables with statistics)
 *
 * @return bool true if the layout needs to be re-cached
 */
bool ui_update(void* app, UILayout* const layout, bool content_tick = true) NO_EXCEPT
{
    PROFILE_DEBUG(PROFILE_UI_UPDATE);
    if (!layout->update) {
        return (layout->dirty & UI_LAYOUT_DIRTY) != 0;
    }

    int32 offset = layout->ui_root->next_offset;
    while (offset) {
        UICore* const element = ui_element_get(layout, offset);

        if (element->state & UI_STATE_HIDDEN) {
            offset = element->skip_offset;
            continue;
        }

        if (element->update_func
            && (content_tick || (element->flags & UI_ELEMENT_FLAG_UPDATE_TICK))
        ) {
            layout->update[element->update_func - 1](app, layout, element);
        }

        offset = element->next_offset;
    }

    return (layout->dirty & UI_LAYOUT_DIRTY) != 0;
}

////////////////////////////////////////////////////////////////
// Scrolling (generic for all scrollable elements)
////////////////////////////////////////////////////////////////

/**
 * Scroll info of an element for one axis
 * Uses the dimensions of the last ui_cache()
 *
 * @return bool false if the element can't scroll in this axis (or the content fits)
 */
inline
bool ui_element_scroll_info(
    const UILayout* const layout,
    const UICore* const element,
    UIScrollAxis axis,
    UIScrollInfo* const info
) NO_EXCEPT
{
    switch (element->type) {
        case UI_ELEMENT_TYPE_VIEW_PANEL:
            return ui_panel_scroll_info((const UIPanel *) element, axis, info);
        case UI_ELEMENT_TYPE_TEXTAREA:
            return axis == UI_SCROLL_AXIS_Y
                && ui_input_scroll_info(layout, (const UIInput *) element, info);
        case UI_ELEMENT_TYPE_OPTION_LIST:
            return axis == UI_SCROLL_AXIS_Y
                && ui_option_list_scroll_info(layout, (const UIOptionList *) element, info);
        case UI_ELEMENT_TYPE_TABLE:
            return axis == UI_SCROLL_AXIS_Y
                && ui_table_scroll_info((const UITable *) element, info);
        default:
            return false;
    }
}

/**
 * @return bool true if the scroll offset changed
 */
inline
bool ui_element_scroll_set(UILayout* const layout, UICore* const element, UIScrollAxis axis, f32 scroll) NO_EXCEPT
{
    switch (element->type) {
        case UI_ELEMENT_TYPE_VIEW_PANEL:
            return ui_panel_scroll_set(layout, (UIPanel *) element, axis, scroll);
        case UI_ELEMENT_TYPE_TEXTAREA:
            return ui_input_scroll_set(layout, (UIInput *) element, axis, scroll);
        case UI_ELEMENT_TYPE_OPTION_LIST:
            return axis == UI_SCROLL_AXIS_Y
                && ui_option_list_scroll_set(layout, (UIOptionList *) element, scroll);
        case UI_ELEMENT_TYPE_TABLE:
            return axis == UI_SCROLL_AXIS_Y
                && ui_table_scroll_set(layout, (UITable *) element, scroll);
        default:
            return false;
    }
}

////////////////////////////////////////////////////////////////
// Caching
////////////////////////////////////////////////////////////////

/**
 * Places an open popup
 * Called by ui_cache() right before the popup is cached -> the owner is already calculated for this frame
 */
inline
void ui_popup_layout(UILayout* const layout, UIPanel* const popup) NO_EXCEPT
{
    PROFILE_DEBUG(PROFILE_UI_CACHE_POPUP_LAYOUT);

    v2_f32 size = popup->core.dimension.dim_raw;

    if (popup->popup_owner) {
        const UICore* const owner = ui_element_get(layout, popup->popup_owner);
        if (owner->type == UI_ELEMENT_TYPE_SELECT) {
            // The size depends on the amount of (filtered) options
            size = ui_select_popup_size(layout, (const UISelect *) owner);
        }
    }

    ui_popup_place(layout, popup, size);
}

// Clipping panel whose subtree is currently cached
// A clipping panel is a panel that is potentially only partially shown inside a scrollable panel
struct UICacheClip {
    // Offset of the element after the subtree (= end of the clipped area)
    int32 end;

    // Offset of the clipping panel
    int32 element;

    UIClipRect clip;
};

////////////////////////////////////////////////////////////////
// Incremental caching
////////////////////////////////////////////////////////////////
// Generating the vertices (especially text) is the expensive part of ui_cache().
// Most of the time only a few elements change (e.g. a label of the hud, a hovered button)
// -> ui_cache() copies the vertices of unchanged elements from the output of the last ui_cache()
//
//      ui_vertex_total/ui_index_total  = copy of the output of the last ui_cache()
//      UICore::cache_*                 = vertex/index range + zindex of the element in that output
//      UICore::cache_hash              = hash of everything the vertices of the element depend on
//
// An element is re-generated if its hash changed (element data, parent position/size, clipping, text, ...),
// it was marked as changed (UICore::change_type, e.g. custom elements)
// or it wasn't visible in the last ui_cache() (no cached vertices).
// Copied elements get their indices rebased and their zindex shifted (if the zindex steps of previous elements changed).
//
// The output is the same as re-generating everything (cache_incremental to switch it off)
// The gpu upload still uploads the whole vertex buffer.

// Elements that are always re-generated since they depend on other elements (e.g. the option list on its select)
// They only exist in popups, so this is rare
static inline
bool ui_element_cache_volatile(const UICore* const element) NO_EXCEPT
{
    switch (element->type) {
        case UI_ELEMENT_TYPE_OPTION_LIST:
        case UI_ELEMENT_TYPE_COLOR_PANEL:
            return true;
        case UI_ELEMENT_TYPE_CURSOR:
            // Doesn't use the zindex (always on top) + moves all the time anyway
            return true;
        case UI_ELEMENT_TYPE_VIEW_PANEL:
            // Popups are placed relative to their owner
            return ((const UIPanel *) element)->panel_flags & UI_PANEL_FLAG_POPUP;
        default:
            return false;
    }
}

FORCE_INLINE
uint64 ui_cache_hash_bytes(uint64 hash, const void* const data, size_t size) NO_EXCEPT
{
    return size ? hash_murmur3_64(data, size, hash) : hash;
}

FORCE_INLINE
uint64 ui_cache_hash_str(uint64 hash, const char* const str) NO_EXCEPT
{
    // + 1 = including the \0, otherwise "ab" + "c" == "a" + "bc"
    // NULL = only a distinct value is mixed in (no need to actually hash anything)
    // 0x9E3779B97F4A7C15 = 2^64 = golden ratio
    // Any value with well mixed bits works, it only has to make NULL different from ""
    return str
        ? hash_murmur3_64(str, strlen(str) + 1, hash)
        : hash ^ 0x9E3779B97F4A7C15ULL;
}

// Same as ui_cache_hash_str but with a known length (the string must be \0 terminated)
// NULL mixes in the same golden ratio constant as above
FORCE_INLINE
uint64 ui_cache_hash_str(uint64 hash, SimpleString<const char> str) NO_EXCEPT
{
    // 0x9E3779B97F4A7C15 = 2^64 = golden ratio
    // Any value with well mixed bits works, it only has to make NULL different from ""
    return str.str
        ? hash_murmur3_64(str.str, (size_t) str.length + 1, hash)
        : hash ^ 0x9E3779B97F4A7C15ULL;
}

static inline
uint64 ui_cache_hash_text(
    uint64 hash,
    const UILayout* const layout,
    int32 offset,
    CharType char_type
) NO_EXCEPT
{
    if (!offset) {
        return hash;
    }

    // The length of the text is stored with the text -> no strlen
    if (char_type == CHAR_TYPE_CHAR) {
        return ui_cache_hash_str(hash, ui_string_get(layout, offset));
    }

    const char* const text = (const char *) (layout->ui_element_buffer.memory + offset);

    return ui_cache_hash_bytes(hash, text, (size_t) ui_string_length(layout, offset) * char_type_size(char_type));
}

/**
 * Hash of everything the vertices of an element depend on
 *
 * Called before caching the element (did something change?) and after (stored for the next ui_cache())
 * since caching also calculates values that are stored in the element (e.g. the dimension)
 *
 * WARNING: If you add an element that reads other elements or layout data while caching, add it here
 */
static
uint64 ui_element_cache_hash(
    const UILayout* const layout,
    const UICore* const element,
    int32 offset,
    const UICacheClip* const clip
) NO_EXCEPT
{
    PROFILE_DEBUG(PROFILE_UI_CACHE_ELEMENT_HASH);

    // Element data incl. state, styles, scroll offsets, ...
    // without the change flags and the cache members at the end of UICore
    const byte* const data = (const byte *) element;
    uint64 hash = ui_cache_hash_bytes(0, data, offsetof(UICore, change_type));
    hash = ui_cache_hash_bytes(
        hash,
        data + offsetof(UICore, change_type) + sizeof(element->change_type),
        offsetof(UICore, cache_frame) - offsetof(UICore, change_type) - sizeof(element->change_type)
    );

    const int32 type_size = ui_element_type_size(element->type);
    if (type_size > (int32) sizeof(UICore)) {
        hash = ui_cache_hash_bytes(hash, data + sizeof(UICore), type_size - sizeof(UICore));
    }

    // The position/size is calculated from the parent
    const UICore* const parent = ui_element_get(layout, element->parent_offset);
    hash = ui_cache_hash_bytes(hash, &parent->dimension.pos, sizeof(parent->dimension.pos));
    hash = ui_cache_hash_bytes(hash, &parent->dimension.dim, sizeof(parent->dimension.dim));

    if (parent->type == UI_ELEMENT_TYPE_VIEW_PANEL) {
        // The content of a scroll panel depends on the scroll offset + scrollbars
        const UIPanel* const parent_panel = (const UIPanel *) parent;
        hash = ui_cache_hash_bytes(hash, &parent_panel->scroll, sizeof(parent_panel->scroll));
        hash = ui_cache_hash_bytes(hash, &parent_panel->content_size, sizeof(parent_panel->content_size));
    }

    // Clipping (e.g. scroll panel viewport)
    if (clip) {
        hash = ui_cache_hash_bytes(hash, &clip->clip, sizeof(clip->clip));
    }

    // Interaction state that is not stored in the element
    const UIInteraction* const interaction = &layout->interaction;
    const uint32 interaction_state = (interaction->nav_focus == offset ? 1 : 0)
        | (interaction->scroll_dragged == offset ? (2 | (interaction->scroll_drag_axis << 2)) : 0);
    hash = ui_cache_hash_bytes(hash, &interaction_state, sizeof(interaction_state));

    // Content that is stored outside of the element (e.g. text) or in other elements
    switch (element->type) {
        case UI_ELEMENT_TYPE_LABEL: {
            const UILabel* const label = (const UILabel *) element;
            hash = ui_cache_hash_text(hash, layout, label->content, label->char_type);
            hash = ui_cache_hash_text(hash, layout, label->pattern, label->char_type);
        } break;
        case UI_ELEMENT_TYPE_INPUT:
        case UI_ELEMENT_TYPE_TEXTAREA: {
            const UIInput* const input = (const UIInput *) element;
            hash = ui_cache_hash_text(hash, layout, input->content, CHAR_TYPE_CHAR);
            hash = ui_cache_hash_text(hash, layout, input->placeholder, CHAR_TYPE_CHAR);
        } break;
        case UI_ELEMENT_TYPE_SELECT: {
            const UISelect* const select = (const UISelect *) element;

            char buffer[128];
            hash = ui_cache_hash_str(hash, ui_select_display_text(layout, select, buffer, (int32) sizeof(buffer)));

            const bool is_open = select->popup && interaction->popup == select->popup;
            hash = ui_cache_hash_bytes(hash, &is_open, sizeof(is_open));
        } break;
        // Tab titles and table columns are string lists that are never changed in place
        // (new texts = new list = new offset in the element data, which is already hashed above)
        case UI_ELEMENT_TYPE_TABLE: {
            const UITable* const table = (const UITable *) element;
            if (table->column_widths) {
                hash = ui_cache_hash_bytes(
                    hash,
                    layout->ui_element_buffer.memory + table->column_widths,
                    sizeof(f32) * table->column_count
                );
            }

            // The cells and the footer are not hashed (can be thousands), every cell/row/footer change goes through
            // the ui_table_*() functions which set UI_ELEMENT_CHANGE_CONTENT
        } break;
        case UI_ELEMENT_TYPE_COLOR_PICKER: {
            const UIColorPicker* const picker = (const UIColorPicker *) element;
            const bool is_open = picker->popup && interaction->popup == picker->popup;
            hash = ui_cache_hash_bytes(hash, &is_open, sizeof(is_open));
        } break;
        case UI_ELEMENT_TYPE_CHECKBOX:
        case UI_ELEMENT_TYPE_RADIO: {
            // The size includes the label
            const UICheckbox* const checkbox = (const UICheckbox *) element;
            if (checkbox->label) {
                const UICore* const label = ui_element_get(layout, checkbox->label);
                hash = ui_cache_hash_bytes(hash, &label->dimension.dim, sizeof(label->dimension.dim));
            }
        } break;
        default: {}
    }

    return hash;
}

/**
 * Copies the vertices of an unchanged element from the last ui_cache() output
 *
 * @param f32 zindex_delta Difference between the new and the old zindex
 */
static inline
void ui_element_cache_copy(UILayout* const layout, UICore* const element, f32 zindex_delta) NO_EXCEPT
{
    PROFILE_DEBUG(PROFILE_UI_CACHE_ELEMENT_COPY);

    ArrayVector<Vertex3DSamplerTextureColor>* const vertices = &layout->ui_vertex_cache;
    ArrayVector<int32>* const indices = &layout->ui_index_cache;

    const int32 vertex_start = vertices->count;
    const int32 index_start = indices->count;

    ASSERT_TRUE(vertex_start + element->cache_vertex_count <= vertices->capacity);
    ASSERT_TRUE(index_start + element->cache_index_count <= indices->capacity);

    Vertex3DSamplerTextureColor* const vertex_dst = vertices->elements + vertex_start;
    memcpy(
        vertex_dst,
        layout->ui_vertex_total.elements + element->cache_vertex_start,
        element->cache_vertex_count * sizeof(Vertex3DSamplerTextureColor)
    );

    if (zindex_delta != 0.0f) {
        for (int32 i = 0; i < element->cache_vertex_count; ++i) {
            vertex_dst[i].position.z += zindex_delta;
        }
    }

    int32* const index_dst = indices->elements + index_start;
    const int32* const index_src = layout->ui_index_total.elements + element->cache_index_start;
    const int32 index_shift = vertex_start - element->cache_vertex_start;

    // The indices may need shifting in the new cache array
    // since there could be other vertices in between the total cache
    for (int32 i = 0; i < element->cache_index_count; ++i) {
        index_dst[i] = index_src[i] + index_shift;
    }

    vertices->count += element->cache_vertex_count;
    indices->count += element->cache_index_count;

    element->cache_vertex_start = vertex_start;
    element->cache_index_start = index_start;
    element->cache_zindex_start += zindex_delta;
    element->cache_zindex_end += zindex_delta;
}

/**
 * Generates the vertices of one element (incl. the focus frame)
 * The vertices are not clipped yet
 */
static
void ui_element_cache_element(
    void* app,
    GpuApiType gpu_api_type,
    UILayout* const layout,
    UICore* const element,
    int32 offset,
    f32* const zindex,
    BufferMemory* const __restrict mem
) NO_EXCEPT
{
    PROFILE_DEBUG(PROFILE_UI_CACHE_ELEMENT_GENERATE);

    ArrayVector<Vertex3DSamplerTextureColor>* const vertices = &layout->ui_vertex_cache;
    ArrayVector<int32>* const indices = &layout->ui_index_cache;

    const int32 vertex_start = vertices->count;

    switch (element->type) {
        case UI_ELEMENT_TYPE_BUTTON: {
            ui_button_cache((UIButton *) element, gpu_api_type, layout, zindex);
        } break;
        case UI_ELEMENT_TYPE_LABEL: {
            ui_label_cache((UILabel *) element, layout, *zindex, mem);
        } break;
        case UI_ELEMENT_TYPE_IMAGE: {
            ui_image_cache((UIImage *) element, layout, *zindex);
        } break;
        case UI_ELEMENT_TYPE_VIEW_WINDOW: {
            ui_window_cache((UIWindow *) element, gpu_api_type, layout, zindex);
        } break;
        case UI_ELEMENT_TYPE_VIEW_PANEL: {
            UIPanel* const panel = (UIPanel *) element;
            if (panel->panel_flags & UI_PANEL_FLAG_POPUP) {
                ui_popup_layout(layout, panel);
            }

            ui_panel_cache(panel, gpu_api_type, layout, zindex);
        } break;
        case UI_ELEMENT_TYPE_CURSOR: {
            ui_cursor_cache((UICursor *) element, layout);
        } break;
        case UI_ELEMENT_TYPE_CUSTOM: {
            ui_dimension_calculate(layout, element);

            if (element->render_func) {
                // Own name, otherwise it looks like ui_element_cache_element in the profiler
                PROFILE_SCOPE_DEBUG(PROFILE_UI_CACHE_CUSTOM, "ui_custom_render_func");

                layout->render[element->render_func - 1](
                    app,
                    element,
                    gpu_api_type,
                    layout,
                    *zindex,
                    mem
                );

                // Custom render functions manage their own zindex
                // -> continue after the last vertex they created
                if (vertices->count > vertex_start) {
                    *zindex = min_branched(*zindex, vertices->elements[vertices->count - 1].position.z);
                }
            }
        } break;
        case UI_ELEMENT_TYPE_CHECKBOX:
        case UI_ELEMENT_TYPE_RADIO: {
            ui_checkbox_cache((UICheckbox *) element, gpu_api_type, layout, zindex);
        } break;
        case UI_ELEMENT_TYPE_SLIDER: {
            ui_slider_cache((UISlider *) element, gpu_api_type, layout, zindex);
        } break;
        case UI_ELEMENT_TYPE_INPUT:
        case UI_ELEMENT_TYPE_TEXTAREA: {
            ui_input_cache((UIInput *) element, gpu_api_type, layout, zindex, mem);
        } break;
        case UI_ELEMENT_TYPE_SELECT: {
            ui_select_cache((UISelect *) element, gpu_api_type, layout, zindex, mem);
        } break;
        case UI_ELEMENT_TYPE_OPTION_LIST: {
            ui_option_list_cache((UIOptionList *) element, gpu_api_type, layout, zindex, mem);
        } break;
        case UI_ELEMENT_TYPE_VIEW_TAB: {
            ui_tab_cache((UITab *) element, gpu_api_type, layout, zindex, mem);
        } break;
        case UI_ELEMENT_TYPE_TABLE: {
            ui_table_cache((UITable *) element, gpu_api_type, layout, zindex, mem);
        } break;
        case UI_ELEMENT_TYPE_COLOR_PICKER: {
            ui_color_picker_cache((UIColorPicker *) element, gpu_api_type, layout, zindex);
        } break;
        case UI_ELEMENT_TYPE_COLOR_PANEL: {
            ui_color_panel_cache((UIColorPanel *) element, gpu_api_type, layout, zindex);
        } break;
        case UI_ELEMENT_TYPE_TEXT:
        case UI_ELEMENT_TYPE_LINK:
        case UI_ELEMENT_TYPE_MANUAL: {
            // @todo Not implemented yet, we still need the dimension for the children and hit testing
            ui_dimension_calculate(layout, element);
        } break;
        default:
            UNREACHABLE();
    }

    // Keyboard focus (e.g. tab navigation) is shown as a frame around the element
    if (offset == layout->interaction.nav_focus) {
        *zindex = camera_step_closer(gpu_api_type, *zindex);

        const UIAttributeDimension* const dim = &element->dimension;
        ui_border_solid_vertices(
            {dim->pos.x - 2.0f, dim->pos.y + 2.0f, dim->dim.x + 4.0f, dim->dim.y + 4.0f},
            UI_DEFAULT_CONTROL_BORDER_FOCUS, 2,
            vertices, indices, *zindex
        );
    }
}

// Max. elements a patch re-generates, more changes = the normal ui_cache()
#define UI_CACHE_PATCH_MAX_ELEMENTS 64

/**
 * Fast path of ui_cache(): patches the output of the last ui_cache() in place
 *
 * Most changes don't change the structure of the output (e.g. the cursor moved, a hovered button changed its color,
 * a label got a text of the same length). The changed elements are re-generated with their old zindex and written over
 * their old vertex/index range -> the unchanged elements are neither copied nor re-generated and the application only
 * has to copy/upload the changed ranges.
 *
 * The elements a rejected patch already re-generated are marked as changed, the normal ui_cache re-generates them.
 * A rejected patch means the vertex count differs.
 *
 * The cache_frame of the layout stays the same: the output is still the output of the last normal ui_cache
 *
 * @return bool false = not patched (the normal ui_cache re-builds the output)
 */
static
bool ui_cache_in_place(
    void* app,
    GpuApiType gpu_api_type,
    UILayout* const layout,
    BufferMemory* const __restrict mem
) NO_EXCEPT
{
    ArrayVector<Vertex3DSamplerTextureColor>* const vertices = &layout->ui_vertex_cache;
    ArrayVector<int32>* const indices = &layout->ui_index_cache;

    const int32 vertex_count = vertices->count;
    const int32 index_count = indices->count;

    // The changed elements are generated behind the output first (scratch area)
    // -> as much free space as the output itself
    if (vertices->capacity - vertex_count < vertex_count || indices->capacity - index_count < index_count) {
        return false;
    }

    const UIInteraction* const interaction = &layout->interaction;
    if (interaction->scroll_dragged != layout->cache_scroll_dragged
        || interaction->scroll_drag_axis != layout->cache_scroll_drag_axis
    ) {
        return false;
    }

    const uint32 frame = layout->cache_frame;

    // Clipping panels of the current subtree (same as ui_cache) + did something change inside of them
    struct UICachePatchClip {
        UICacheClip clip;
        bool scroll;
        bool changed;
    };

    UICachePatchClip clips[UI_CLIP_STACK_SIZE];
    int32 clip_count = 0;

    UICore* regenerated[UI_CACHE_PATCH_MAX_ELEMENTS];
    int32 regenerated_count = 0;
    int32 visited = 0;

    UICachePatch patch = {false, INT32_MAX, 0, INT32_MAX, 0};

    bool ok = true;
    int32 offset = layout->ui_root->next_offset;
    while (ok) {
        // Leaving clipped subtrees
        while (clip_count > 0 && clips[clip_count - 1].clip.end == offset) {
            const UICachePatchClip* const left = &clips[--clip_count];
            if (!left->changed) {
                continue;
            }

            // The content size + scrollbars of a scroll panel depend on its children
            if (left->scroll) {
                ok = false;
                break;
            }

            if (clip_count) {
                clips[clip_count - 1].changed = true;
            }
        }

        if (!ok || !offset) {
            break;
        }

        UICore* const element = ui_element_get(layout, offset);
        if (element->state & UI_STATE_HIDDEN) {
            // Was visible in the output -> its vertices would have to be removed
            if (element->cache_frame == frame) {
                ok = false;
                break;
            }

            offset = element->skip_offset;
            continue;
        }

        // Not part of the output (newly visible)
        if (element->cache_frame != frame) {
            ok = false;
            break;
        }

        ++visited;

        const UICacheClip* const clip = clip_count ? &clips[clip_count - 1].clip : NULL;
        const bool changed = element->change_type
            || ui_element_cache_volatile(element)
            || element->cache_hash != ui_element_cache_hash(layout, element, offset, clip);

        if (changed) {
            const bool scroll_panel = element->type == UI_ELEMENT_TYPE_VIEW_PANEL
                && (((const UIPanel *) element)->panel_flags & UI_PANEL_FLAG_SCROLL);

            if (scroll_panel || regenerated_count >= UI_CACHE_PATCH_MAX_ELEMENTS) {
                ok = false;
                break;
            }

            regenerated[regenerated_count++] = element;
            if (clip_count) {
                clips[clip_count - 1].changed = true;
            }

            // Re-generate the element behind the output with its old zindex
            const int32 vertex_start = vertices->count;
            const int32 index_start = indices->count;
            f32 zindex = element->cache_zindex_start;

            ui_element_cache_element(app, gpu_api_type, layout, element, offset, &zindex, mem);

            if (clip) {
                ui_vertices_clip_or_remove(vertices, indices, vertex_start, index_start, clip->clip);
            }

            const int32 new_vertex_count = vertices->count - vertex_start;
            const int32 new_index_count = indices->count - index_start;

            // The structure must stay the same (same range in the output, same zindex steps for the following elements)
            if (new_vertex_count != element->cache_vertex_count
                || new_index_count != element->cache_index_count
                || zindex != element->cache_zindex_end
            ) {
                ok = false;
                break;
            }

            // Write over the old range, the indices are rebased from the scratch area to the old range
            if (new_vertex_count) {
                memcpy(
                    vertices->elements + element->cache_vertex_start,
                    vertices->elements + vertex_start,
                    new_vertex_count * sizeof(Vertex3DSamplerTextureColor)
                );

                patch.vertex_start = OMS_MIN(patch.vertex_start, element->cache_vertex_start);
                patch.vertex_end = OMS_MAX(patch.vertex_end, element->cache_vertex_start + new_vertex_count);
            }

            if (new_index_count) {
                const int32 index_shift = element->cache_vertex_start - vertex_start;
                int32* const index_dst = indices->elements + element->cache_index_start;
                const int32* const index_src = indices->elements + index_start;
                for (int32 i = 0; i < new_index_count; ++i) {
                    index_dst[i] = index_src[i] + index_shift;
                }

                patch.index_start = OMS_MIN(patch.index_start, element->cache_index_start);
                patch.index_end = OMS_MAX(patch.index_end, element->cache_index_start + new_index_count);
            }

            // Drop the scratch area
            vertices->count = vertex_start;
            indices->count = index_start;

            element->cache_hash = ui_element_cache_hash(layout, element, offset, clip);
            element->change_type = UI_ELEMENT_CHANGE_NONE;
        }

        // Entering the subtree of a clipping panel (same as ui_cache)
        if (ui_panel_is_clipping(element)
            && element->next_offset != element->skip_offset
            && clip_count < UI_CLIP_STACK_SIZE
        ) {
            const UIPanel* const panel = (const UIPanel *) element;
            UIClipRect panel_clip = ui_clip_from_rect(
                (panel->panel_flags & UI_PANEL_FLAG_SCROLL)
                    ? ui_panel_viewport(panel)
                    : ui_dimension_rect(&element->dimension)
            );

            if (clip_count) {
                panel_clip = ui_clip_intersect(panel_clip, clips[clip_count - 1].clip.clip);
            }

            UICachePatchClip* const entry = &clips[clip_count++];
            entry->clip = {element->skip_offset, offset, panel_clip};
            entry->scroll = (panel->panel_flags & UI_PANEL_FLAG_SCROLL) != 0;
            entry->changed = false;
        }

        offset = element->next_offset;
    }

    if (!ok) {
        // The normal ui_cache re-builds the output. The elements re-generated here only exist in the patched
        // output, not in ui_vertex_total -> they must be re-generated again
        vertices->count = vertex_count;
        indices->count = index_count;

        for (int32 i = 0; i < regenerated_count; ++i) {
            regenerated[i]->cache_frame = 0;
            regenerated[i]->change_type |= UI_ELEMENT_CHANGE_CONTENT;
        }

        return false;
    }

    // Keep the copy of the output in sync (the next normal ui_cache copies the unchanged elements from there)
    if (patch.vertex_start < patch.vertex_end) {
        memcpy(
            layout->ui_vertex_total.elements + patch.vertex_start,
            vertices->elements + patch.vertex_start,
            (patch.vertex_end - patch.vertex_start) * sizeof(Vertex3DSamplerTextureColor)
        );
    } else {
        patch.vertex_start = 0;
        patch.vertex_end = 0;
    }

    if (patch.index_start < patch.index_end) {
        memcpy(
            layout->ui_index_total.elements + patch.index_start,
            indices->elements + patch.index_start,
            (patch.index_end - patch.index_start) * sizeof(int32)
        );
    } else {
        patch.index_start = 0;
        patch.index_end = 0;
    }

    layout->cache_patch = patch;
    layout->cache_elements_generated = regenerated_count;
    layout->cache_elements_reused = visited - regenerated_count;
    layout->dirty = 0;

    return true;
}

/**
 * Creates the vertices for all visible elements
 *
 * The tree is traversed in pre-order which means:
 *      1. a parent is always calculated before its children (dimension calculation only needs the parent)
 *      2. later elements are drawn on top of earlier elements (zindex moves closer with every element)
 *
 * Children of clipping panels (e.g. scroll panels) are clipped on the cpu.
 * The clip rects are a stack, a subtree ends when the traversal reaches the skip_offset of the panel.
 *
 * Unchanged elements are copied from the last output if layout->cache_incremental
 *
 * ui_cache() only renders the current state, it never changes it (ui_input_pointer)
 * Exception: calculated values like dimensions or the content size of scroll panels.
 * If such a value changed in a way that affects other elements (e.g. scrollbars appear) the layout stays dirty.
 */
void ui_cache(
    void* app,
    GpuApiType gpu_api_type,
    UILayout* const layout,
    BufferMemory* const __restrict mem
) NO_EXCEPT
{
    PROFILE_DEBUG(PROFILE_UI_CACHE);

    ArrayVector<Vertex3DSamplerTextureColor>* const vertices = &layout->ui_vertex_cache;
    ArrayVector<int32>* const indices = &layout->ui_index_cache;

    // Values that affect every element, if one of them changed nothing can be re-used
    // (no need to hash them, the font pointer, font scale + gpu api fit into one value)
    // The values of a layout only change one at a time -> a xor of them always results in a different value
    const uint32 font_scale_bits = layout->font ? BITCAST(layout->font->base.scale, uint32) : 0;

    const uint64 cache_global_hash = (uint64) (uintptr_t) layout->font
        ^ ((uint64) font_scale_bits << 16)
        ^ ((uint64) gpu_api_type << 56);

    // Fast path: patch the last output in place
    // Only if the last output + its copy (ui_vertex_total) are valid for the current global values
    if (layout->ui_root
        && layout->cache_frame
        && layout->cache_global_hash == cache_global_hash
        && layout->ui_vertex_total.elements
        && layout->ui_index_total.elements
        && layout->ui_vertex_total.count == vertices->count
        && layout->ui_index_total.count == indices->count
        && vertices->count > 1
        && ui_cache_in_place(app, gpu_api_type, layout, mem)
    ) {
        return;
    }

    vertices->count = 0;
    indices->count = 0;

    layout->cache_elements_generated = 0;
    layout->cache_elements_reused = 0;

    // Degenerate starting point
    array_vector_insert(vertices, {{0.0f, 0.0f, UI_ZINDEX_START}, 0, {0}});

    // The element cache data is only valid for the last ui_cache() call
    const uint32 previous_frame = layout->cache_frame;
    const uint32 frame = ++layout->cache_frame;

    const bool incremental = layout->ui_vertex_total.elements
        && layout->ui_index_total.elements
        && layout->cache_global_hash == cache_global_hash
        // 0 = never cached, overflow of the frame counter -> old cache data might look valid
        && previous_frame
        && frame > previous_frame;

    layout->cache_global_hash = cache_global_hash;

    const UIInteraction* const interaction = &layout->interaction;

    f32 zindex = UI_ZINDEX_START;
    // Is another ui_cache() needed right after this one?
    // A scroll panel only knows its content size after all of its children were laid out.
    // If the content size changed in a way that makes a scrollbar appear/disappear, the viewport of the panel changes
    // -> its children were laid out for the old viewport and must be laid out again.
    // -> In that case the layout stays dirty.
    bool needs_recache = false;

    UICacheClip clips[UI_CLIP_STACK_SIZE];
    int32 clip_count = 0;

    // The root itself is never rendered
    layout->ui_root->change_type = UI_ELEMENT_CHANGE_NONE;

    int32 offset = layout->ui_root->next_offset;
    while (true) {
        // Leaving clipped subtrees
        while (clip_count > 0 && clips[clip_count - 1].end == offset) {
            const int32 panel_offset = clips[--clip_count].element;
            UIPanel* const panel = (UIPanel *) ui_element_get(layout, panel_offset);

            if (panel->panel_flags & UI_PANEL_FLAG_SCROLL) {
                // The children are calculated now -> content size + scrollbars on top of the content
                // The scrollbars are always re-generated (they are not part of the cached element vertices)
                zindex = camera_step_closer(gpu_api_type, zindex);

                const int32 vertex_start = vertices->count;
                const int32 index_start = indices->count;

                const bool dragged = interaction->scroll_dragged == panel_offset;
                needs_recache |= ui_panel_scroll_finish(
                    panel, gpu_api_type, layout, &zindex,
                    dragged && interaction->scroll_drag_axis == UI_SCROLL_AXIS_X,
                    dragged && interaction->scroll_drag_axis == UI_SCROLL_AXIS_Y
                );

                if (clip_count) {
                    ui_vertices_clip_or_remove(
                        vertices, indices,
                        vertex_start, index_start,
                        clips[clip_count - 1].clip
                    );
                }

                // The panel data changed (content size) -> update the hash, otherwise it would be re-generated every time
                if (panel->core.cache_frame == frame) {
                    panel->core.cache_hash = ui_element_cache_hash(
                        layout, &panel->core, panel_offset,
                        clip_count ? &clips[clip_count - 1] : NULL
                    );
                }
            }
        }

        if (!offset) {
            break;
        }

        UICore* const element = ui_element_get(layout, offset);

        if (element->state & UI_STATE_HIDDEN) {
            // Skip the entire subtree
            offset = element->skip_offset;
            continue;
        }

        zindex = camera_step_closer(gpu_api_type, zindex);

        const UICacheClip* const clip = clip_count ? &clips[clip_count - 1] : NULL;

        bool reuse = false;
        if (incremental) {
            reuse = !element->change_type
                && element->cache_frame == previous_frame
                && !ui_element_cache_volatile(element)
                && element->cache_hash == ui_element_cache_hash(layout, element, offset, clip);
        }

        if (reuse) {
            ui_element_cache_copy(layout, element, zindex - element->cache_zindex_start);
            zindex = element->cache_zindex_end;

            ++layout->cache_elements_reused;
        } else {
            const int32 vertex_start = vertices->count;
            const int32 index_start = indices->count;
            const f32 zindex_start = zindex;

            ui_element_cache_element(app, gpu_api_type, layout, element, offset, &zindex, mem);

            // Clip everything this element created to the clipping panels it is in
            if (clip) {
                ui_vertices_clip_or_remove(
                    vertices, indices,
                    vertex_start, index_start,
                    clip->clip
                );
            }

            element->cache_vertex_start = vertex_start;
            element->cache_vertex_count = vertices->count - vertex_start;
            element->cache_index_start = index_start;
            element->cache_index_count = indices->count - index_start;
            element->cache_zindex_start = zindex_start;
            element->cache_zindex_end = zindex;

            element->cache_hash = ui_element_cache_hash(layout, element, offset, clip);

            ++layout->cache_elements_generated;
        }

        element->cache_frame = frame;

        // Entering the subtree of a clipping panel
        if (ui_panel_is_clipping(element)
            && element->next_offset != element->skip_offset
            && clip_count < UI_CLIP_STACK_SIZE
        ) {
            const UIPanel* const panel = (const UIPanel *) element;
            UIClipRect panel_clip = ui_clip_from_rect(
                (panel->panel_flags & UI_PANEL_FLAG_SCROLL)
                    ? ui_panel_viewport(panel)
                    : ui_dimension_rect(&element->dimension)
            );

            if (clip_count) {
                panel_clip = ui_clip_intersect(panel_clip, clips[clip_count - 1].clip);
            }

            clips[clip_count++] = {element->skip_offset, offset, panel_clip};
        }

        element->change_type = UI_ELEMENT_CHANGE_NONE;
        offset = element->next_offset;
    }

    // Keep the output for the next ui_cache() (unchanged elements are copied from here)
    if (layout->ui_vertex_total.capacity >= vertices->count
        && layout->ui_index_total.capacity >= indices->count
    ) {
        PROFILE_SCOPE_DEBUG(PROFILE_UI_CACHE_TOTAL_COPY, "ui_cache_total_copy");

        memcpy(
            layout->ui_vertex_total.elements,
            vertices->elements,
            vertices->count * sizeof(Vertex3DSamplerTextureColor)
        );
        memcpy(
            layout->ui_index_total.elements,
            indices->elements,
            indices->count * sizeof(int32)
        );
        layout->ui_vertex_total.count = vertices->count;
        layout->ui_index_total.count = indices->count;
    } else {
        // Nothing to copy from -> the next ui_cache() re-generates everything
        layout->cache_frame += 1;
    }

    // The whole output changed
    layout->cache_patch = {true, 0, vertices->count, 0, indices->count};
    layout->cache_scroll_dragged = interaction->scroll_dragged;
    layout->cache_scroll_drag_axis = interaction->scroll_drag_axis;

    // A needed re-cache (e.g. scrollbars appeared) should converge immediately
    layout->dirty = needs_recache ? (byte) (UI_LAYOUT_DIRTY | UI_LAYOUT_DIRTY_INTERACTION) : (byte) 0;
}

////////////////////////////////////////////////////////////////
// Hit testing
////////////////////////////////////////////////////////////////

FORCE_INLINE
bool ui_element_has_action(const UICore* const element) NO_EXCEPT
{
    if (element->state & UI_STATE_DISABLED) {
        return false;
    } else if (element->flags & UI_ELEMENT_FLAG_ACTION_MASK) {
        return true;
    }

    return element->type == UI_ELEMENT_TYPE_BUTTON
        && ((const UIButton *) element)->on_click;
}

/**
 * Finds the UI element at a given position
 *
 * Coordinates are in the same space as UICore::dimension (y-up, origin bottom left).
 * If your mouse coordinates are y-down you need to convert them first (y = screen_height - mouse_y)
 * The dimensions are only valid after ui_cache() calculated them.
 *
 * Since the tree is stored in pre-order, which is also the render order,
 * the LAST element in pre-order that contains the position is the top most (and deepest) element.
 * -> One linear walk through the element buffer, no recursion needed
 * Children of clipping panels (e.g. scroll panels) can only be hit inside of the visible area,
 * if the position is outside of it the whole subtree is skipped.
 *
 * Action bubbling:
 *      If the top most element has no action bound, the action "bubbles" up to the closest parent that has one.
 *      e.g. click on the window title label (no action) -> window title (movable) is returned
 *           click on a button label (no action) -> button (on_click) is returned
 *
 * @param UICore** hit_element (optional) Top most element at (x, y) regardless of actions.
 *                             If it is NULL the UI wasn't hit at all (e.g. forward the input to the game world)
 *
 * @return UICore* Element with an action bound or NULL
 */
UICore* ui_element_at_position(
    const UILayout* const layout,
    f32 x, f32 y,
    UICore** hit_element = NULL
) NO_EXCEPT
{
    UICore* hit = NULL;

    if (layout->ui_root) {
        int32 offset = layout->ui_root->next_offset;
        while (offset) {
            UICore* const element = ui_element_get(layout, offset);

            if (element->state & UI_STATE_HIDDEN) {
                offset = element->skip_offset;
                continue;
            }

            if (!(element->flags & UI_ELEMENT_FLAG_IGNORE_HIT)
                && ui_dimension_contains(&element->dimension, x, y)
            ) {
                hit = element;
            }

            // Children of a clipping panel are only visible inside of it
            if (ui_panel_is_clipping(element)) {
                const UIPanel* const panel = (const UIPanel *) element;
                const UIClipRect clip = ui_clip_from_rect(
                    (panel->panel_flags & UI_PANEL_FLAG_SCROLL)
                        ? ui_panel_viewport(panel)
                        : ui_dimension_rect(&element->dimension)
                );

                if (!ui_clip_contains(clip, x, y)) {
                    offset = element->skip_offset;
                    continue;
                }
            }

            offset = element->next_offset;
        }
    }

    if (hit_element) {
        *hit_element = hit;
    }

    // Bubble up to the closest element with an action
    UICore* found = hit;
    while (found && !ui_element_has_action(found)) {
        found = ui_element_parent(layout, found);
    }

    // The root is never a valid result
    return found == layout->ui_root ? NULL : found;
}

/**
 * Find the element a scrollbar belongs to
 *
 * @return UICore* Element the scrollbar belongs to or NULL
 */
inline
UICore* ui_scrollbar_at(
    const UILayout* const layout, UICore* element, f32 x, f32 y,
    UIScrollAxis* const axis, UIScrollInfo* const info
) NO_EXCEPT
{
    while (element && element != layout->ui_root) {
        if (ui_element_scroll_info(layout, element, UI_SCROLL_AXIS_Y, info)
            && ui_scrollbar_contains(info, x, y)
        ) {
            *axis = UI_SCROLL_AXIS_Y;
            return element;
        } else if (ui_element_scroll_info(layout, element, UI_SCROLL_AXIS_X, info)
            && ui_scrollbar_contains(info, x, y)
        ) {
            *axis = UI_SCROLL_AXIS_X;
            return element;
        }

        element = ui_element_parent(layout, element);
    }

    return NULL;
}

////////////////////////////////////////////////////////////////
// Input handling
////////////////////////////////////////////////////////////////
//
// Order per update tick:
//      1. poll input                                   (on_update_input)
//      2. update the ui state from the input           (ui_input_*), uses the rects of the last ui_cache()
//      3. game logic                                   (may also change ui state)
//      4. rebuild the vertices from the current state  (ui_cache)
//
// Rendering (ui_cache) only reads the state, it never changes it.
// This keeps ui_cache independent from the input and allows to throttle it (e.g. gpu_ui_fps)
// while the state is still updated every tick.
// The hit test uses the rects of the previous ui_cache() call -> 1 frame latency in accuracy

/**
 * Converts window coordinates (y-down, origin top left) to ui coordinates (y-up, origin bottom left)
 */
FORCE_INLINE
v2_f32 ui_window_to_ui_position(const UILayout* const layout, f32 x, f32 y) NO_EXCEPT
{
    return {
        x,
        layout->ui_root
            ? layout->ui_root->dimension.dim.y - y
            : y
    };
}

// Pointer (mouse) state for one tick
// @question Should I move this to Input.h?
struct UIPointerInput {
    // Pointer position in ui coordinates
    f32 x;
    f32 y;

    // Primary button is currently down
    bool down;

    // Primary button was released this tick
    // Required to detect a press + release within the same tick (key state PRESSED | RELEASED -> down = false, released = true)
    bool released;

    // Secondary button (e.g. right mouse button) was pressed this tick (context_target)
    bool secondary_pressed;

    // Mouse wheel movement this tick in notches (y: positive = away from the user, x: positive = right)
    f32 wheel_x;
    f32 wheel_y;

    // Current time in microseconds, used for double/triple clicks (0 = no multi click detection)
    uint64 time;
};

// Max time between clicks of a double/triple click
// @question shouldn't this be part of the Input.h?
#define UI_MULTI_CLICK_TIME 500000

enum UIInputResult : byte {
    UI_INPUT_RESULT_NONE = 0,

    // Some element state/position/order changed -> ui needs to be re-cached
    UI_INPUT_RESULT_CHANGED = 1 << 0,

    // The pointer is used by the UI -> the game should ignore the pointer input
    UI_INPUT_RESULT_CAPTURED = 1 << 1,

    // An element got clicked
    UI_INPUT_RESULT_CLICKED = 1 << 2,

    // An element got right clicked (context_target), e.g. open a context menu
    UI_INPUT_RESULT_CONTEXT = 1 << 3,
};

/**
 * Sets/removes a state flag of an element and marks it as changed
 *
 * @param bool enable Defines wether the state value should be added(set) or removed(unset)
 *
 * @return bool true if the state changed
 */
inline
bool ui_element_state_set(UILayout* const layout, int32 offset, byte state, bool enable) NO_EXCEPT
{
    ASSERT_TRUE(offset);

    UICore* const element = ui_element_get(layout, offset);
    const byte new_state = enable
        ? (byte) (element->state | state)
        : (byte) (element->state & ~state);

    if (new_state == element->state) {
        return false;
    }

    element->state = new_state;
    element->change_type |= UI_ELEMENT_CHANGE_CONTENT;

    return true;
}

// Closest element (self or parent) with any of the flags that is not disabled
inline
UICore* ui_element_closest_with_flag(const UILayout* const layout, UICore* element, uint16 flags) NO_EXCEPT
{
    while (element
        && element != layout->ui_root
        && (!(element->flags & flags) || (element->state & UI_STATE_DISABLED))
    ) {
        element = ui_element_parent(layout, element);
    }

    return element == layout->ui_root ? NULL : element;
}

/**
 * Element that gets moved/resized when dragging a movable/resizable element
 * A movable element inside of a window moves the window (e.g. window title), otherwise the element moves itself
 */
inline
UICore* ui_element_drag_target(const UILayout* const layout, UICore* const movable) NO_EXCEPT
{
    if (movable->type == UI_ELEMENT_TYPE_VIEW_WINDOW) {
        return movable;
    }

    UICore* const window = ui_parent_element_by_type(layout, movable, UI_ELEMENT_TYPE_VIEW_WINDOW);

    return window ? window : movable;
}

/**
 * Moves the dragged element by the pointer movement since the drag start
 *
 * @return bool true if the position changed
 */
inline
bool ui_drag_update(UILayout* const layout, f32 x, f32 y) NO_EXCEPT
{
    UIInteraction* const interaction = &layout->interaction;
    UICore* const element = ui_element_get(layout, interaction->dragged);
    const UICore* const parent = ui_element_get(layout, element->parent_offset);
    UIAttributeDimension* const dim = &element->dimension;

    // Pointer movement in ui coordinates (y-up)
    f32 dx = x - interaction->drag_start_pointer.x;
    f32 dy = y - interaction->drag_start_pointer.y;

    // Relative positions are fractions of the parent size
    if (dim->flag & UI_DIMENSION_POS_X_RELATIVE) {
        dx = parent->dimension.dim.x > 0.0f ? dx / parent->dimension.dim.x : 0.0f;
    }

    if (dim->flag & UI_DIMENSION_POS_Y_RELATIVE) {
        dy = parent->dimension.dim.y > 0.0f ? dy / parent->dimension.dim.y : 0.0f;
    }

    // pos_raw is an offset from the anchor where positive values point towards the inside of the parent
    // -> the sign depends on the anchor
    const v2_f32 new_pos = {
        interaction->drag_start_pos.x + ((dim->align_anchor & UI_ANCHOR_H_RIGHT) ? -dx : dx),
        interaction->drag_start_pos.y + ((dim->align_anchor & UI_ANCHOR_V_TOP) ? -dy : dy)
    };

    if (new_pos.x == dim->pos_raw.x && new_pos.y == dim->pos_raw.y) {
        return false;
    }

    // @todo Consider to clamp the position so the element can't be moved off screen
    dim->pos_raw = new_pos;
    element->change_type |= UI_ELEMENT_CHANGE_POS;

    return true;
}

/**
 * Resizes the resized element (e.g. window) by the pointer movement since the resize start
 * The top left corner stays where it is (the position is corrected for other alignments)
 * Only pixel sizes can be resized (relative/fill sizes are kept)
 *
 * @return bool true if the size changed
 */
inline
bool ui_resize_update(UILayout* const layout, f32 x, f32 y) NO_EXCEPT
{
    UIInteraction* const interaction = &layout->interaction;
    UICore* const element = ui_element_get(layout, interaction->resized);
    UIAttributeDimension* const dim = &element->dimension;

    v2_f32 min_dim = {40.0f, 20.0f};
    if (element->type == UI_ELEMENT_TYPE_VIEW_WINDOW
        && ((const UIWindow *) element)->min_dim.x > 0.0f
    ) {
        min_dim = ((const UIWindow *) element)->min_dim;
    }

    // Pointer movement: right and down make the element larger
    const f32 dx = x - interaction->drag_start_pointer.x;
    const f32 dy = interaction->drag_start_pointer.y - y;

    const v2_f32 old_dim = dim->dim_raw;
    const v2_f32 old_pos = dim->pos_raw;

    if (!(dim->flag & UI_DIMENSION_DIM_X_RELATIVE) && interaction->drag_start_dim.x > 0.0f) {
        const f32 width = max_branched(interaction->drag_start_dim.x + dx, min_dim.x);
        const f32 delta = width - interaction->drag_start_dim.x;
        dim->dim_raw.x = width;

        // pos.x = base_x - width * align -> move the base to keep the left side in place
        const f32 align = (dim->align_anchor & UI_ALIGN_H_RIGHT)
            ? 1.0f
            : ((dim->align_anchor & UI_ALIGN_H_CENTER)
                ? 0.5f
                : 0.0f
            );

        if (align > 0.0f && !(dim->flag & UI_DIMENSION_POS_X_RELATIVE)) {
            dim->pos_raw.x = interaction->drag_start_pos.x
                + ((dim->align_anchor & UI_ANCHOR_H_RIGHT) ? -1.0f : 1.0f) * align * delta;
        }
    }

    if (!(dim->flag & UI_DIMENSION_DIM_Y_RELATIVE) && interaction->drag_start_dim.y > 0.0f) {
        const f32 height = max_branched(interaction->drag_start_dim.y + dy, min_dim.y);
        const f32 delta = height - interaction->drag_start_dim.y;
        dim->dim_raw.y = height;

        // pos.y (top) = base_y + height * align -> move the base to keep the top in place
        const f32 align = (dim->align_anchor & UI_ALIGN_V_TOP)
            ? 0.0f
            : ((dim->align_anchor & UI_ALIGN_V_CENTER)
                ? 0.5f
                : 1.0f
            );

        if (align > 0.0f && !(dim->flag & UI_DIMENSION_POS_Y_RELATIVE)) {
            dim->pos_raw.y = interaction->drag_start_pos.y
                + ((dim->align_anchor & UI_ANCHOR_V_TOP) ? 1.0f : -1.0f) * align * delta;
        }
    }

    if (old_dim.x == dim->dim_raw.x && old_dim.y == dim->dim_raw.y
        && old_pos.x == dim->pos_raw.x && old_pos.y == dim->pos_raw.y
    ) {
        return false;
    }

    element->change_type |= UI_ELEMENT_CHANGE_DIM_OTHER;

    return true;
}

/**
 * Moves the content of the element whose scrollbar thumb is dragged
 *
 * @return bool true if the scroll offset changed
 */
inline
bool ui_scroll_drag_update(UILayout* const layout, f32 x, f32 y) NO_EXCEPT
{
    UIInteraction* const interaction = &layout->interaction;
    UICore* const element = ui_element_get(layout, interaction->scroll_dragged);
    const UIScrollAxis axis = (UIScrollAxis) interaction->scroll_drag_axis;

    UIScrollInfo info;
    if (!ui_element_scroll_info(layout, element, axis, &info)) {
        return false;
    }

    // Movement towards the end of the track (down/right)
    const f32 delta = axis == UI_SCROLL_AXIS_Y
        ? interaction->drag_start_pointer.y - y
        : x - interaction->drag_start_pointer.x;

    return ui_element_scroll_set(
        layout, element, axis,
        interaction->scroll_drag_start + ui_scrollbar_pointer_to_scroll(&info, axis, delta)
    );
}

// Calls a 1-indexed action (layout->on_actions, 0 = none)
static FORCE_INLINE
void ui_element_action(void* app, UILayout* const layout, UICore* const element, int32 action) NO_EXCEPT
{
    if (action && layout->on_actions) {
        layout->on_actions[action - 1](app, layout, element);
    }
}

static FORCE_INLINE
bool ui_element_is_text_input(const UICore* const element) NO_EXCEPT
{
    return element->type == UI_ELEMENT_TYPE_INPUT || element->type == UI_ELEMENT_TYPE_TEXTAREA;
}

/**
 * Changes which text element receives the keyboard input (0 = none)
 * The caret is placed at the end of the text
 *
 * @return bool true if it changed
 */
inline
bool ui_input_set_editing(UILayout* const layout, int32 offset) NO_EXCEPT
{
    UIInteraction* const interaction = &layout->interaction;
    if (interaction->editing == offset) {
        return false;
    }

    if (interaction->editing) {
        ui_element_state_set(layout, interaction->editing, UI_STATE_FOCUS, false);
    }

    interaction->editing = offset;

    if (offset) {
        ui_element_state_set(layout, offset, UI_STATE_FOCUS, true);
        UIInput* const input = (UIInput *) ui_element_get(layout, offset);
        input->caret = ui_string_length(layout, input->content);
        input->selection_anchor = input->caret;
        ui_input_scroll_to_caret(layout, input);
    }

    layout->dirty |= UI_LAYOUT_DIRTY;

    return true;
}

static FORCE_INLINE
bool ui_select_is_open(const UILayout* const layout, const UISelect* const select) NO_EXCEPT
{
    return select->popup && layout->interaction.popup == select->popup;
}

/**
 * Opens/closes the popup (option list) of a select
 * A search field is cleared and receives the keyboard input
 */
inline
void ui_select_open(UILayout* const layout, UISelect* const select, bool open) NO_EXCEPT
{
    ASSERT_TRUE(select->popup);

    if (!open) {
        if (ui_select_is_open(layout, select)) {
            ui_popup_close(layout);
        }

        return;
    }

    UIOptionList* const list = (UIOptionList *) ui_element_get(layout, select->list);
    list->scroll = 0.0f;
    list->hovered = -1;

    ui_popup_open(layout, (UIPanel *) ui_element_get(layout, select->popup));

    if (select->search) {
        ui_input_set_text(layout, (UIInput *) ui_element_get(layout, select->search), "", 0);
        ui_input_set_editing(layout, select->search);
    }

    // Show the selected option
    // Clamped like the rendering: an option near the end can't be centered,
    // an unclamped scroll offset the hovered row against the cursor
    // The list dimension isn't calculated yet (next ui_cache)
    // -> the view height is calculated from the row count
    if (select->selected > 0) {
        const f32 row_height = ui_select_row_height(select);
        const int32 count = ui_select_filtered_count(layout, select);
        const f32 view = row_height * (f32) clamp_branched(count, 1, UI_SELECT_MAX_VISIBLE_OPTIONS);

        list->scroll = ui_scroll_clamp(
            row_height * (f32) (select->selected - UI_SELECT_MAX_VISIBLE_OPTIONS / 2),
            row_height * (f32) count,
            view
        );
    }
}

/**
 * Replaces the options (e.g. options that are only known at runtime like the connected monitors)
 * The old option texts stay in the element buffer (unused) -> don't call this every frame
 *
 * @param int32 selected Selected option (single selection, -1 = none), multi selection: nothing is checked
 */
inline
void ui_select_set_options(
    UILayout* const layout, UISelect* const select,
    const SimpleString<const char>* options, int32 option_count,
    int32 selected = 0
) NO_EXCEPT
{
    ui_select_open(layout, select, false);

    select->options = ui_string_list_store(layout, options, option_count);
    select->option_count = (int16) option_count;

    if (select->select_flags & UI_SELECT_FLAG_MULTI) {
        select->multi_selected = option_count > 0
            ? ui_buffer_reserve(layout, (option_count + 7) / 8, 1) // @performance fix division
            : 0;
        select->selected = -1;
    } else {
        select->selected = (int16) ((selected >= 0 && selected < option_count)
            ? selected
            : (option_count > 0 ? 0 : -1)
        );
    }

    select->core.change_type |= UI_ELEMENT_CHANGE_CONTENT;
    layout->dirty |= UI_LAYOUT_DIRTY;
}

// Opens/closes the popup of a popup owner (select, color picker)
inline
void ui_popup_toggle(UILayout* const layout, int32 popup) NO_EXCEPT
{
    ASSERT_TRUE(popup);

    if (layout->interaction.popup == popup) {
        ui_popup_close(layout);
    } else {
        ui_popup_open(layout, (UIPanel *) ui_element_get(layout, popup));
    }
}

/**
 * Executes the click behaviour of an element
 *      button:       on_click action
 *      checkbox:     toggles the value
 *      radio:        checks the radio (unchecks the other radios of the group)
 *      select:       opens/closes the option list
 *      option list:  selects (single) or toggles (multi) the option at the position
 *      tab:          activates the tab at the position
 *      table:        selects the row at the position
 *      color picker: opens/closes the color popup
 *      color panel:  palette -> picks the color at the position
 */
inline
void ui_element_click(void* app, UILayout* const layout, UICore* const element, f32 x, f32 y) NO_EXCEPT
{
    switch (element->type) {
        case UI_ELEMENT_TYPE_BUTTON: {
            ui_element_action(app, layout, element, ((const UIButton *) element)->on_click);
        } break;
        case UI_ELEMENT_TYPE_CHECKBOX: {
            UICheckbox* const checkbox = (UICheckbox *) element;
            if (ui_checkbox_set(layout, checkbox, !checkbox->checked)) {
                ui_element_action(app, layout, element, checkbox->on_change);
            }
        } break;
        case UI_ELEMENT_TYPE_RADIO: {
            UIRadio* const radio = (UIRadio *) element;
            if (ui_checkbox_set(layout, radio, true)) {
                ui_element_action(app, layout, element, radio->on_change);
            }
        } break;
        case UI_ELEMENT_TYPE_SELECT: {
            UISelect* const select = (UISelect *) element;
            ui_select_open(layout, select, !ui_select_is_open(layout, select));
        } break;
        case UI_ELEMENT_TYPE_OPTION_LIST: {
            UIOptionList* const list = (UIOptionList *) element;
            UISelect* const select = ui_option_list_select(layout, list);

            const int32 option = ui_select_filtered_option(layout, select, ui_option_list_row_at(layout, list, x, y));
            if (option < 0) {
                break;
            }

            if (select->select_flags & UI_SELECT_FLAG_MULTI) {
                // The popup stays open to select more options
                if (ui_select_set(layout, select, option, !ui_select_is_checked(layout, select, option))) {
                    ui_element_action(app, layout, &select->core, select->on_change);
                }
            } else {
                if (ui_select_set(layout, select, option)) {
                    ui_element_action(app, layout, &select->core, select->on_change);
                }

                ui_select_open(layout, select, false);
            }
        } break;
        case UI_ELEMENT_TYPE_VIEW_TAB: {
            UITab* const tab = (UITab *) element;
            if (ui_tab_set_active(layout, tab, ui_tab_at(tab, x, y))) {
                ui_element_action(app, layout, element, tab->on_change);
            }
        } break;
        case UI_ELEMENT_TYPE_TABLE: {
            UITable* const table = (UITable *) element;
            const int32 row = ui_table_row_at(table, x, y);
            if (row >= 0 && ui_table_select(layout, table, row)) {
                ui_element_action(app, layout, element, table->on_change);
            }
        } break;
        case UI_ELEMENT_TYPE_COLOR_PICKER: {
            ui_popup_toggle(layout, ((const UIColorPicker *) element)->popup);
        } break;
        case UI_ELEMENT_TYPE_COLOR_PANEL: {
            UIColorPanel* const panel = (UIColorPanel *) element;
            UIColorPicker* const picker = (UIColorPicker *) ui_element_get(layout, panel->picker);

            if (picker->picker_type == UI_COLOR_PICKER_TYPE_PALETTE && ui_color_panel_cell_at(layout, panel, x, y) >= 0) {
                if (ui_color_panel_pick(layout, panel, x, y)) {
                    ui_element_action(app, layout, &picker->core, picker->on_change);
                }

                ui_popup_close(layout);
            }
        } break;
        default: {}
    }
}

/**
 * Value elements follow the pointer while they are pressed (slider, color gradient)
 *
 * @param bool start true = the element was just pressed
 *
 * @return bool true if the value changed
 */
inline
bool ui_element_drag_value(void* app, UILayout* const layout, UICore* const element, f32 x, f32 y, bool start) NO_EXCEPT
{
    switch (element->type) {
        case UI_ELEMENT_TYPE_SLIDER: {
            UISlider* const slider = (UISlider *) element;
            if (ui_slider_set_from_position(layout, slider, x)) {
                ui_element_action(app, layout, element, slider->on_change);

                return true;
            }
        } break;
        case UI_ELEMENT_TYPE_INPUT:
        case UI_ELEMENT_TYPE_TEXTAREA: {
            // Text selection with the pointer (click = caret, drag = select, double click = word, triple click = all)
            if (layout->interaction.editing == ui_element_offset(layout, element)) {
                return ui_input_select_at(layout, (UIInput *) element, x, y, start, layout->interaction.click_count);
            }
        } break;
        case UI_ELEMENT_TYPE_COLOR_PANEL: {
            UIColorPanel* const panel = (UIColorPanel *) element;
            if (ui_color_panel_set_from_position(layout, panel, x, y, start)) {
                UIColorPicker* const picker = (UIColorPicker *) ui_element_get(layout, panel->picker);
                ui_element_action(app, layout, &picker->core, picker->on_change);

                return true;
            }
        } break;
        default: {}
    }

    return false;
}

/**
 * Updates the "sub element" under the pointer of elements that render multiple items (option rows, tabs, ...)
 *
 * @param f32   x, y    Pointer position
 * @param bool  reset   true = the element is not hovered anymore
 *
 * @return bool true if it changed
 */
inline
bool ui_element_hover_item(UILayout* const layout, UICore* const element, f32 x, f32 y, bool reset) NO_EXCEPT
{
    switch (element->type) {
        case UI_ELEMENT_TYPE_OPTION_LIST: {
            UIOptionList* const list = (UIOptionList *) element;
            return ui_option_list_hover(layout, list, reset ? -1 : ui_option_list_row_at(layout, list, x, y));
        }
        case UI_ELEMENT_TYPE_VIEW_TAB: {
            UITab* const tab = (UITab *) element;
            return ui_tab_hover(layout, tab, reset ? -1 : ui_tab_at(tab, x, y));
        }
        case UI_ELEMENT_TYPE_TABLE: {
            UITable* const table = (UITable *) element;
            return ui_table_hover(layout, table, reset ? -1 : ui_table_row_at(table, x, y));
        }
        case UI_ELEMENT_TYPE_COLOR_PANEL: {
            UIColorPanel* const panel = (UIColorPanel *) element;
            return ui_color_panel_hover(layout, panel, reset ? -1 : ui_color_panel_cell_at(layout, panel, x, y));
        }
        default:
            return false;
    }
}

/**
 * Marks every change made during the scope as caused by user interaction (UI_LAYOUT_DIRTY_INTERACTION)
 *
 * Everything that marks the layout dirty while the input is processed
 * (e.g. hover, press, focus, typing, scrolling, the actions of clicked elements, ...) is interaction.
 * Changes outside of them (e.g. update functions) only set UI_LAYOUT_DIRTY -> the application can throttle them.
 */
struct UIInteractionDirtyScope {
    UILayout* layout;
    byte was_dirty;

    explicit UIInteractionDirtyScope(UILayout* const l) NO_EXCEPT
        : layout(l), was_dirty(l->dirty)
    {
        layout->dirty = UI_LAYOUT_DIRTY_NONE;
    }

    ~UIInteractionDirtyScope() NO_EXCEPT
    {
        if (layout->dirty & UI_LAYOUT_DIRTY) {
            layout->dirty |= UI_LAYOUT_DIRTY_INTERACTION;
        }

        layout->dirty |= was_dirty;
    }
};

/**
 * Updates the hover, press (active), focus, drag, resize, scroll and click state from the pointer input
 *
 *      hover:      closest UI_ELEMENT_FLAG_HOVERABLE element under the pointer (unchanged while dragging)
 *      press:      closest element with an action under the pointer gets UI_STATE_ACTIVE
 *                  closest UI_ELEMENT_FLAG_HITBOX element gets UI_STATE_FOCUS and is moved to the front (e.g. window)
 *                  pressing an UI_ELEMENT_FLAG_MOVABLE element starts dragging
 *                  pressing an UI_ELEMENT_FLAG_RESIZABLE element starts resizing (e.g. window resize grip)
 *                  pressing a scrollbar starts dragging its thumb (pressing the track jumps to the position)
 *      drag:       the dragged element follows the pointer until the button is released
 *      click:      button released over the pressed element (and it wasn't dragged) ->
 *      value:      sliders/color gradients follow the pointer while pressed
 *      wheel:      scrolls the closest scrollable element under the pointer
 *      right:      the hit element is reported as context target (UI_INPUT_RESULT_CONTEXT)
 *      text:       pressing an input/textarea starts editing it, pressing anything else stops it
 *      popup:      the open popup is on top -> it gets the pointer input first, pressing outside of it closes it
 *
 * MUST be called after the input was polled and before ui_cache()
 *
 * @return byte UIInputResult flags
 */
byte ui_input_pointer(void* app, UILayout* const layout, const UIPointerInput* const input) NO_EXCEPT
{
    PROFILE_DEBUG(PROFILE_UI_INPUT_POINTER);
    UIInteractionDirtyScope interaction_scope(layout);

    UIInteraction* const interaction = &layout->interaction;
    interaction->clicked = 0;
    interaction->context_target = 0;

    byte result = UI_INPUT_RESULT_NONE;
    const f32 x = input->x;
    const f32 y = input->y;

    // Edge detection
    // A press + release within one tick is only reported as released -> handle it as press AND release
    const bool was_down = interaction->pointer_down;
    const bool quick_click = input->released && !was_down;
    const bool pressed = (input->down && !was_down) || quick_click;
    const bool released = (!input->down && was_down) || quick_click;
    interaction->pointer_down = input->down;
    interaction->pointer = {input->x, input->y};

    // 1. Ongoing drags
    // They only depend on the pointer movement since the drag started
    ////////////////////////////////////////////////////////
    if (interaction->scroll_dragged && ui_scroll_drag_update(layout, x, y)) {
        result |= UI_INPUT_RESULT_CHANGED;
    }

    if (interaction->resized && ui_resize_update(layout, x, y)) {
        result |= UI_INPUT_RESULT_CHANGED;
    }

    if (interaction->dragged && ui_drag_update(layout, x, y)) {
        result |= UI_INPUT_RESULT_CHANGED;
    }

    // Values follow the pointer while pressed (e.g. slider)
    if (interaction->active && input->down
        && ui_element_drag_value(app, layout, ui_element_get(layout, interaction->active), x, y, false)
    ) {
        result |= UI_INPUT_RESULT_CHANGED;
    }

    UICore* hit;
    UICore* const action = ui_element_at_position(layout, x, y, &hit);

    // 2. Popup
    // It is on top of everything -> the hit test already prefers it
    // Pressing outside of it closes it (pressing the owner toggles it through the click)
    ////////////////////////////////////////////////////////
    if (interaction->popup) {
        UIPanel* const popup = ui_popup_get(layout);
        const UICore* const owner = popup->popup_owner ? ui_element_get(layout, popup->popup_owner) : NULL;

        if (!ui_element_is_visible(layout, &popup->core)
            || (owner && !ui_element_is_visible(layout, owner))
        ) {
            ui_popup_close(layout);
            result |= UI_INPUT_RESULT_CHANGED;
        } else if ((pressed || input->secondary_pressed)
            && !(hit && ui_element_is_self_or_child_of(layout, hit, &popup->core))
            && !(hit && owner && ui_element_is_self_or_child_of(layout, hit, owner))
        ) {
            ui_popup_close(layout);
            result |= UI_INPUT_RESULT_CHANGED;
        }
    }

    // 3. Hover
    ////////////////////////////////////////////////////////
    if (!interaction->dragged && !interaction->resized && !interaction->scroll_dragged) {
        UICore* const hovered = ui_element_closest_with_flag(layout, hit, UI_ELEMENT_FLAG_HOVERABLE);
        const int32 hovered_offset = hovered ? ui_element_offset(layout, hovered) : 0;

        if (hovered_offset != interaction->hovered) {
            if (interaction->hovered) {
                ui_element_hover_item(layout, ui_element_get(layout, interaction->hovered), x, y, true);
                ui_element_state_set(layout, interaction->hovered, UI_STATE_HOVER, false);
            }

            if (hovered_offset) {
                ui_element_state_set(layout, hovered_offset, UI_STATE_HOVER, true);
            }

            interaction->hovered = hovered_offset;
            result |= UI_INPUT_RESULT_CHANGED;
        }

        // e.g. option row, tab header, table row under the pointer
        if (hovered && ui_element_hover_item(layout, hovered, x, y, false)) {
            result |= UI_INPUT_RESULT_CHANGED;
        }
    }

    // 4. Mouse wheel
    // Scrolls the closest element under the pointer that can still scroll in that direction
    ////////////////////////////////////////////////////////
    if (input->wheel_x != 0.0f || input->wheel_y != 0.0f) {
        for (UICore* element = hit;
            element && element != layout->ui_root;
            element = ui_element_parent(layout, element)
        ) {
            UIScrollInfo info;

            if (input->wheel_y != 0.0f
                && ui_element_scroll_info(layout, element, UI_SCROLL_AXIS_Y, &info)
                && ui_element_scroll_set(layout, element, UI_SCROLL_AXIS_Y, info.scroll - input->wheel_y * UI_SCROLL_WHEEL_STEP)
            ) {
                result |= UI_INPUT_RESULT_CHANGED;
                break;
            }

            if (input->wheel_x != 0.0f
                && ui_element_scroll_info(layout, element, UI_SCROLL_AXIS_X, &info)
                && ui_element_scroll_set(layout, element, UI_SCROLL_AXIS_X, info.scroll + input->wheel_x * UI_SCROLL_WHEEL_STEP)
            ) {
                result |= UI_INPUT_RESULT_CHANGED;
                break;
            }
        }
    }

    // 5. Secondary button (e.g. context menu)
    ////////////////////////////////////////////////////////
    if (input->secondary_pressed && hit) {
        interaction->context_target = ui_element_offset(layout, hit);
        result |= UI_INPUT_RESULT_CONTEXT;
    }

    // 6. Press
    ////////////////////////////////////////////////////////
    if (pressed) {
        // The pointer takes over -> no navigation focus anymore (text elements use editing below)
        if (interaction->nav_focus) {
            interaction->nav_focus = 0;
            result |= UI_INPUT_RESULT_CHANGED;
        }

        // Double/triple click = pressed again on the same element shortly after at (almost) the same position
        const int32 action_offset = action ? ui_element_offset(layout, action) : 0;
        const bool repeated = input->time
            && action_offset
            && action_offset == interaction->last_click_element
            && input->time - interaction->last_click_time < UI_MULTI_CLICK_TIME
            && x - interaction->last_click_pointer.x < 4.0f && interaction->last_click_pointer.x - x < 4.0f
            && y - interaction->last_click_pointer.y < 4.0f && interaction->last_click_pointer.y - y < 4.0f;

        interaction->click_count = repeated ? (byte) min_branched(interaction->click_count + 1, 3) : 1;
        interaction->last_click_element = action_offset;
        interaction->last_click_time = input->time;
        interaction->last_click_pointer = {x, y};

        // Focus (e.g. the window), clicking outside of the UI removes the focus
        UICore* const focused = ui_element_closest_with_flag(layout, hit, UI_ELEMENT_FLAG_HITBOX);
        const int32 focused_offset = focused ? ui_element_offset(layout, focused) : 0;

        if (focused_offset != interaction->focused) {
            if (interaction->focused) {
                ui_element_state_set(layout, interaction->focused, UI_STATE_FOCUS, false);
            }

            if (focused_offset) {
                ui_element_state_set(layout, focused_offset, UI_STATE_FOCUS, true);
            }

            interaction->focused = focused_offset;

            result |= UI_INPUT_RESULT_CHANGED;
        }

        // The focused element is drawn on top of its siblings (e.g. overlapping windows)
        // The open popup always stays on top
        if (focused && ui_element_move_to_front(layout, focused)) {
            if (interaction->popup) {
                ui_element_move_to_front(layout, ui_element_get(layout, interaction->popup));
            }

            result |= UI_INPUT_RESULT_CHANGED;
        }

        UIScrollAxis scroll_axis;
        UIScrollInfo scroll_info;
        UICore* const scrollbar = ui_scrollbar_at(layout, hit, x, y, &scroll_axis, &scroll_info);

        if (scrollbar) {
            // Pressing the track (not the thumb) jumps to the position, then the thumb is dragged
            const v4_f32 thumb = ui_scrollbar_thumb(&scroll_info, scroll_axis);
            if (!(x >= thumb.x && x < thumb.x + thumb.width && y <= thumb.y && y > thumb.y - thumb.height)) {
                ui_element_scroll_set(layout, scrollbar, scroll_axis, ui_scrollbar_scroll_at(&scroll_info, scroll_axis, x, y));
                ui_element_scroll_info(layout, scrollbar, scroll_axis, &scroll_info);
            }

            interaction->scroll_dragged = ui_element_offset(layout, scrollbar);
            interaction->scroll_drag_axis = scroll_axis;
            interaction->scroll_drag_start = scroll_info.scroll;
            interaction->drag_start_pointer = {x, y};

            result |= UI_INPUT_RESULT_CHANGED;
        } else {
            // Keyboard input goes to the pressed text element (pressing anything else stops editing)
            // Exception: pressing something in the popup of the edited element (e.g. option list below a search field)
            const bool keep_editing = interaction->editing
                && interaction->popup
                && hit
                && !(action && ui_element_is_text_input(action))
                && ui_element_is_self_or_child_of(layout, hit, ui_element_get(layout, interaction->popup))
                && ui_element_is_self_or_child_of(layout, ui_element_get(layout, interaction->editing), ui_element_get(layout, interaction->popup));

            if (!keep_editing
                && ui_input_set_editing(layout, action && ui_element_is_text_input(action) ? ui_element_offset(layout, action) : 0)
            ) {
                result |= UI_INPUT_RESULT_CHANGED;
            }

            if (action) {
                interaction->active = ui_element_offset(layout, action);
                ui_element_state_set(layout, interaction->active, UI_STATE_ACTIVE, true);

                ui_element_drag_value(app, layout, action, x, y, true);

                if (action->flags & UI_ELEMENT_FLAG_RESIZABLE) {
                    UICore* const target = ui_element_drag_target(layout, action);

                    interaction->resized = ui_element_offset(layout, target);
                    interaction->drag_start_pointer = {x, y};
                    interaction->drag_start_pos = target->dimension.pos_raw;
                    interaction->drag_start_dim = target->dimension.dim_raw;
                } else if (action->flags & UI_ELEMENT_FLAG_MOVABLE) {
                    UICore* const target = ui_element_drag_target(layout, action);

                    interaction->dragged = ui_element_offset(layout, target);
                    interaction->drag_start_pointer = {x, y};
                    interaction->drag_start_pos = target->dimension.pos_raw;
                }

                result |= UI_INPUT_RESULT_CHANGED;
            }
        }
    }

    // 7. Release
    ////////////////////////////////////////////////////////
    if (released) {
        if (interaction->active) {
            UICore* const active = ui_element_get(layout, interaction->active);

            // Click = released over the element it was pressed on
            // Elements that were dragged/resized don't get a click (e.g. window title)
            if (!interaction->dragged
                && !interaction->resized
                && hit
                && ui_element_is_self_or_child_of(layout, hit, active)
                && !(active->state & UI_STATE_DISABLED)
            ) {
                interaction->clicked = interaction->active;
                ui_element_click(app, layout, active, x, y);

                // e.g. context menu: clicking an entry closes the menu
                const UIPanel* const popup = ui_popup_get(layout);
                if (popup
                    && (popup->panel_flags & UI_PANEL_FLAG_CLOSE_ON_CLICK)
                    && active != &popup->core
                    && ui_element_is_self_or_child_of(layout, active, &popup->core)
                ) {
                    ui_popup_close(layout);
                }

                result |= UI_INPUT_RESULT_CLICKED;
            }

            ui_element_state_set(layout, interaction->active, UI_STATE_ACTIVE, false);
            interaction->active = 0;

            result |= UI_INPUT_RESULT_CHANGED;
        }

        if (interaction->scroll_dragged) {
            interaction->scroll_dragged = 0;
            result |= UI_INPUT_RESULT_CHANGED;
        }

        interaction->dragged = 0;
        interaction->resized = 0;
    }

    interaction->pointer_captured = hit != NULL
        || interaction->active != 0
        || interaction->dragged != 0
        || interaction->resized != 0
        || interaction->scroll_dragged != 0;

    if (interaction->pointer_captured) {
        result |= UI_INPUT_RESULT_CAPTURED;
    }

    if (result & UI_INPUT_RESULT_CHANGED) {
        layout->dirty |= UI_LAYOUT_DIRTY;
    }

    return result;
}

////////////////////////////////////////////////////////////////
// Input focus (navigation)
////////////////////////////////////////////////////////////////
//
// Text elements receive the keyboard input through interaction.editing.
// Other elements can only get the focus through navigation (interaction.nav_focus),
// regardless of the device (e.g. tab on the keyboard, d-pad on a controller, see ui_input_navigate()),
// clicking them with the pointer doesn't focus them (otherwise every button click would block the game hotkeys).
// The application should enable its typing context while interaction.editing is set
// and its ui navigation context while ui_input_wants_navigation() is true.

// Does a ui element have the input focus? (text element being edited or element focused by navigation)
FORCE_INLINE
bool ui_input_has_focus(const UILayout* const layout) NO_EXCEPT
{
    return layout->interaction.editing != 0 || layout->interaction.nav_focus != 0;
}

// Can the element be reached with tab?
FORCE_INLINE
bool ui_element_is_focusable(const UICore* const element) NO_EXCEPT
{
    // Pointer transparent elements are display only (e.g. debug overlay tables)
    if ((element->state & UI_STATE_DISABLED)
        || (element->flags & UI_ELEMENT_FLAG_IGNORE_HIT)
    ) {
        return false;
    }

    switch (element->type) {
        case UI_ELEMENT_TYPE_BUTTON:
            return ((const UIButton *) element)->on_click != 0;
        case UI_ELEMENT_TYPE_CHECKBOX:
        case UI_ELEMENT_TYPE_RADIO:
        case UI_ELEMENT_TYPE_INPUT:
        case UI_ELEMENT_TYPE_TEXTAREA:
        case UI_ELEMENT_TYPE_SELECT:
        case UI_ELEMENT_TYPE_SLIDER:
        case UI_ELEMENT_TYPE_COLOR_PICKER:
        case UI_ELEMENT_TYPE_VIEW_TAB:
        case UI_ELEMENT_TYPE_TABLE:
            return true;
        default:
            return false;
    }
}

/**
 * Scrolls all scroll panels the element is in so that the element is visible
 * Uses the dimensions of the last ui_cache()
 */
inline
void ui_element_scroll_into_view(UILayout* const layout, const UICore* const element) NO_EXCEPT
{
    const UIAttributeDimension* const dim = &element->dimension;

    for (UICore* parent = ui_element_parent(layout, element);
        parent && parent != layout->ui_root;
        parent = ui_element_parent(layout, parent)
    ) {
        UIPanel* const panel = (UIPanel *) parent;

        if (parent->type != UI_ELEMENT_TYPE_VIEW_PANEL
            || !(panel->panel_flags & UI_PANEL_FLAG_SCROLL)
        ) {
            continue;
        }

        const v4_f32 viewport = ui_panel_viewport(panel);

        // Content moves up when scroll.y increases, left when scroll.x increases
        if (dim->pos.y > viewport.y) {
            ui_panel_scroll_set(
                layout,
                panel,
                UI_SCROLL_AXIS_Y,
                panel->scroll.y - (dim->pos.y - viewport.y)
            );
        } else if (dim->pos.y - dim->dim.y < viewport.y - viewport.height) {
            ui_panel_scroll_set(
                layout,
                panel,
                UI_SCROLL_AXIS_Y,
                panel->scroll.y + ((viewport.y - viewport.height) - (dim->pos.y - dim->dim.y))
            );
        }

        if (dim->pos.x < viewport.x) {
            ui_panel_scroll_set(
                layout,
                panel,
                UI_SCROLL_AXIS_X,
                panel->scroll.x - (viewport.x - dim->pos.x)
            );
        } else if (dim->pos.x + dim->dim.x > viewport.x + viewport.width) {
            ui_panel_scroll_set(
                layout,
                panel,
                UI_SCROLL_AXIS_X,
                panel->scroll.x + (dim->pos.x + dim->dim.x - (viewport.x + viewport.width))
            );
        }
    }
}

/**
 * Gives the element the focus
 * Text elements start editing with the whole text selected (typing replaces it)
 */
inline
void ui_focus_set(UILayout* const layout, int32 offset) NO_EXCEPT
{
    UIInteraction* const interaction = &layout->interaction;
    if (!offset) {
        return;
    }

    UICore* const element = ui_element_get(layout, offset);

    if (ui_element_is_text_input(element)) {
        interaction->nav_focus = 0;
        ui_input_set_editing(layout, offset);

        UIInput* const input = (UIInput *) element;
        ui_input_select_all(layout, input);
        ui_input_scroll_to_caret(layout, input);
    } else {
        ui_input_set_editing(layout, 0);
        interaction->nav_focus = offset;
    }

    element->change_type |= UI_ELEMENT_CHANGE_CONTENT;
    ui_element_scroll_into_view(layout, element);
    layout->dirty |= UI_LAYOUT_DIRTY;
}

/**
 * Moves the focus to the next/previous focusable element (e.g. tab / shift + tab, controller d-pad)
 * The focus cycles within the window/popup of the current focus
 *
 * @param int32 direction 1 = next, -1 = previous
 */
inline
void ui_focus_move(UILayout* const layout, int32 direction) NO_EXCEPT
{
    UIInteraction* const interaction = &layout->interaction;
    const int32 current = interaction->editing ? interaction->editing : interaction->nav_focus;

    // Container = window/popup of the current focus (closest UI_ELEMENT_FLAG_HITBOX element)
    UICore* container = NULL;
    if (current) {
        container = ui_element_closest_with_flag(
            layout,
            ui_element_parent(layout,
                ui_element_get(layout, current)
            ),
            UI_ELEMENT_FLAG_HITBOX
        );
    } else if (interaction->popup) {
        container = ui_element_get(layout, interaction->popup);
    } else if (interaction->focused) {
        container = ui_element_get(layout, interaction->focused);
    }

    if (!container) {
        container = layout->ui_root;
    }

    const int32 end = container == layout->ui_root ? 0 : container->skip_offset;

    int32 first = 0;
    int32 last = 0;
    int32 previous = 0;
    int32 next = 0;
    bool found = false;

    int32 offset = container->next_offset;
    while (offset != end) {
        const UICore* const element = ui_element_get(layout, offset);

        if (element->state & UI_STATE_HIDDEN) {
            offset = element->skip_offset;
            continue;
        }

        if (ui_element_is_focusable(element)) {
            first = first ? first : offset;
            last = offset;

            if (offset == current) {
                found = true;
            } else if (!found) {
                previous = offset;
            } else if (!next) {
                next = offset;
            }
        }

        offset = element->next_offset;
    }

    int32 target;
    if (!found) {
        target = direction > 0 ? first : last;
    } else if (direction > 0) {
        target = next ? next : first;
    } else {
        target = previous ? previous : last;
    }

    ui_focus_set(layout, target);
}

/**
 * Enter/space on the focused element = the same as a mouse click on its center
 * (interaction.clicked is set like for a pointer click, e.g. actions that only react to clicks)
 */
inline
void ui_element_click_keyboard(void* app, UILayout* const layout, UICore* const element) NO_EXCEPT
{
    UIInteraction* const interaction = &layout->interaction;
    interaction->clicked = ui_element_offset(layout, element);

    // pos = top left corner (y-up)
    ui_element_click(
        app, layout, element,
        element->dimension.pos.x + element->dimension.dim.x * 0.5f,
        element->dimension.pos.y - element->dimension.dim.y * 0.5f
    );

    // e.g. context menu: clicking an entry closes the menu
    const UIPanel* const popup = ui_popup_get(layout);
    if (popup
        && (popup->panel_flags & UI_PANEL_FLAG_CLOSE_ON_CLICK)
        && element != &popup->core
        && ui_element_is_self_or_child_of(layout, element, &popup->core)
    ) {
        ui_popup_close(layout);
    }
}

/**
 * Action (keyboard, controller, ...) for the focused (non text) element
 *      button:       confirm = click
 *      checkbox:     confirm = toggle
 *      radio:        confirm = check, directions = previous/next radio of the group
 *      slider:       directions = -/+ step, first/last = min/max
 *      select:       confirm = open/pick, up/down = previous/next option (or row while open)
 *      color picker: confirm = open/close
 *      tab:          directions = previous/next tab
 *      table:        up/down/first/last = select row, confirm = click on the selected row (the first row if none is selected)
 *
 * Confirm = mouse click
 *
 * @return bool true if something changed
 */
inline
bool ui_element_key(void* app, UILayout* const layout, UICore* const element, UINavigation action) NO_EXCEPT
{
    const bool activate = action == UI_NAVIGATION_CONFIRM;
    const bool vertical = action == UI_NAVIGATION_UP || action == UI_NAVIGATION_DOWN;
    const int32 delta = (action == UI_NAVIGATION_LEFT || action == UI_NAVIGATION_UP)
        ? -1
        : ((action == UI_NAVIGATION_RIGHT || action == UI_NAVIGATION_DOWN) ? 1 : 0);

    switch (element->type) {
        case UI_ELEMENT_TYPE_BUTTON:
        case UI_ELEMENT_TYPE_CHECKBOX:
        case UI_ELEMENT_TYPE_COLOR_PICKER: {
            if (!activate) {
                return false;
            }

            ui_element_click_keyboard(app, layout, element);

            return true;
        }
        case UI_ELEMENT_TYPE_RADIO: {
            UIRadio* const radio = (UIRadio *) element;

            if (activate) {
                if (ui_checkbox_set(layout, radio, true)) {
                    ui_element_action(app, layout, element, radio->on_change);
                }

                return true;
            }

            if (!delta) {
                return false;
            }

            // Previous/next radio of the same group
            UIRadio* previous = NULL;
            UIRadio* next = NULL;
            bool found = false;

            UICore* sibling;
            ui_element_children_iterate_start(layout, ui_element_parent(layout, element), sibling) {
                if (sibling->type != UI_ELEMENT_TYPE_RADIO || ((UIRadio *) sibling)->group != radio->group) {
                    continue;
                }

                if (sibling == element) {
                    found = true;
                } else if (!found) {
                    previous = (UIRadio *) sibling;
                } else if (!next) {
                    next = (UIRadio *) sibling;
                }
            } ui_element_children_iterate_end;

            UIRadio* const target = delta < 0 ? previous : next;
            if (!target) {
                return false;
            }

            if (ui_checkbox_set(layout, target, true)) {
                ui_element_action(app, layout, &target->core, target->on_change);
            }

            ui_focus_set(layout, ui_element_offset(layout, target));

            return true;
        }
        case UI_ELEMENT_TYPE_SLIDER: {
            UISlider* const slider = (UISlider *) element;
            const f32 step = max_branched(slider->step, 0.05f);

            f32 value = slider->value;
            if (delta) {
                value += step * (f32) delta;
            } else if (action == UI_NAVIGATION_FIRST) {
                value = 0.0f;
            } else if (action == UI_NAVIGATION_LAST) {
                value = 1.0f;
            } else {
                return false;
            }

            if (ui_slider_set(layout, slider, value)) {
                ui_element_action(app, layout, element, slider->on_change);
            }

            return true;
        }
        case UI_ELEMENT_TYPE_SELECT: {
            UISelect* const select = (UISelect *) element;

            if (ui_select_is_open(layout, select)) {
                UIOptionList* const list = (UIOptionList *) ui_element_get(layout, select->list);

                if (vertical) {
                    return ui_option_list_move_hover(layout, list, delta);
                }

                if (!activate) {
                    return false;
                }

                const int32 option = ui_select_filtered_option(layout, select, list->hovered);
                if (option >= 0 && (select->select_flags & UI_SELECT_FLAG_MULTI)) {
                    if (ui_select_set(layout, select, option, !ui_select_is_checked(layout, select, option))) {
                        ui_element_action(app, layout, element, select->on_change);
                    }
                } else {
                    if (option >= 0 && ui_select_set(layout, select, option)) {
                        ui_element_action(app, layout, element, select->on_change);
                    }

                    ui_select_open(layout, select, false);
                }

                return true;
            } else if (activate) {
                ui_select_open(layout, select, true);

                return true;
            } else if (vertical
                && !(select->select_flags & UI_SELECT_FLAG_MULTI)
            ) {
                // Closed single select: up/down changes the selection directly
                const int32 index = clamp_branched(select->selected + delta, 0, select->option_count - 1);
                if (ui_select_set(layout, select, index)) {
                    ui_element_action(app, layout, element, select->on_change);
                }

                return true;
            }

            return false;
        }
        case UI_ELEMENT_TYPE_VIEW_TAB: {
            UITab* const tab = (UITab *) element;
            if (!delta) {
                return false;
            }

            if (ui_tab_set_active(layout, tab, clamp_branched(tab->active + delta, 0, tab->tab_count - 1))) {
                ui_element_action(app, layout, element, tab->on_change);
            }

            return true;
        }
        case UI_ELEMENT_TYPE_TABLE: {
            UITable* const table = (UITable *) element;
            if (!table->row_count) {
                return false;
            }

            // Click on the selected row (also if it is already selected, like a mouse click on it)
            if (activate) {
                const int32 row = table->selected < 0 ? 0 : table->selected;
                ui_table_select(layout, table, row);
                ui_table_scroll_to_row(layout, table, row);

                layout->interaction.clicked = ui_element_offset(layout, element);
                ui_element_action(app, layout, element, table->on_change);

                return true;
            }

            int32 row;
            if (vertical) {
                row = table->selected < 0
                    ? 0
                    : clamp_branched(table->selected + delta, 0, table->row_count - 1);
            } else if (action == UI_NAVIGATION_FIRST) {
                row = 0;
            } else if (action == UI_NAVIGATION_LAST) {
                row = table->row_count - 1;
            } else {
                return false;
            }

            if (ui_table_select(layout, table, row)) {
                ui_element_action(app, layout, element, table->on_change);
            }

            ui_table_scroll_to_row(layout, table, row);

            return true;
        }
        default:
            return false;
    }
}

/**
 * Notifies the owner of an input that its text changed (e.g. search field of a select, rgba field of a color picker)
 */
inline
void ui_input_owner_changed(void* app, UILayout* const layout, UIInput* const input) NO_EXCEPT
{
    UICore* const owner = ui_element_get(layout, input->owner);

    switch (owner->type) {
        case UI_ELEMENT_TYPE_SELECT: {
            // The filtered list changed -> start at the top
            UISelect* const select = (UISelect *) owner;
            UIOptionList* const list = (UIOptionList *) ui_element_get(layout, select->list);
            list->scroll = 0.0f;
            list->hovered = -1;
            list->core.change_type |= UI_ELEMENT_CHANGE_CONTENT;
            layout->dirty |= UI_LAYOUT_DIRTY;
        } break;
        case UI_ELEMENT_TYPE_COLOR_PICKER: {
            UIColorPicker* const picker = (UIColorPicker *) owner;
            if (ui_color_picker_inputs_changed(layout, picker)) {
                ui_element_action(app, layout, owner, picker->on_change);
            }
        } break;
        default: {}
    }
}

/**
 * Keyboard code -> ui action
 * (tab = next, shift + tab = previous, enter/space = confirm, escape = cancel, arrows, home/end = first/last)
 */
static inline
UINavigation ui_navigation_from_code(uint32 code) NO_EXCEPT
{
    switch (code & ~((uint32) INPUT_TEXT_MOD_MASK)) {
        case INPUT_TEXT_TAB: return (code & INPUT_TEXT_MOD_SHIFT) ? UI_NAVIGATION_PREV : UI_NAVIGATION_NEXT;
        case INPUT_TEXT_ENTER:
        case ' ': return UI_NAVIGATION_CONFIRM;
        case INPUT_TEXT_ESCAPE: return UI_NAVIGATION_CANCEL;
        case INPUT_TEXT_UP: return UI_NAVIGATION_UP;
        case INPUT_TEXT_DOWN: return UI_NAVIGATION_DOWN;
        case INPUT_TEXT_LEFT: return UI_NAVIGATION_LEFT;
        case INPUT_TEXT_RIGHT: return UI_NAVIGATION_RIGHT;
        case INPUT_TEXT_HOME: return UI_NAVIGATION_FIRST;
        case INPUT_TEXT_END: return UI_NAVIGATION_LAST;
        default: return UI_NAVIGATION_NONE;
    }
}

/**
 * Applies an action to the element with the focus (not a text element that is edited)
 *
 *      cancel: closes the open popup or removes the focus
 *      the other actions are handled by the element in ui_element_key()
 *
 * @return byte UIInputResult flags
 */
static inline
byte ui_input_focus_action(void* app, UILayout* const layout, UINavigation action) NO_EXCEPT
{
    UIInteraction* const interaction = &layout->interaction;
    if (!interaction->nav_focus) {
        return UI_INPUT_RESULT_NONE;
    }

    byte result = UI_INPUT_RESULT_CAPTURED;

    UICore* const element = ui_element_get(layout, interaction->nav_focus);
    if (!ui_element_is_visible(layout, element)) {
        interaction->nav_focus = 0;
        result |= UI_INPUT_RESULT_CHANGED;

        return result;
    }

    if (action == UI_NAVIGATION_CANCEL) {
        if (interaction->popup) {
            ui_popup_close(layout);
        } else {
            interaction->nav_focus = 0;
        }

        layout->dirty |= UI_LAYOUT_DIRTY;
        result |= UI_INPUT_RESULT_CHANGED;

        return result;
    }

    if (ui_element_key(app, layout, element, action)) {
        result |= UI_INPUT_RESULT_CHANGED;
    }

    return result;
}

/**
 * Applies an action to the text element that is edited
 *
 *      cancel:     stops editing (and closes the popup the element is in, e.g. search field of a select)
 *      confirm:    submits the input, search field of a select = picks the hovered row or the first match
 *      up/down:    search field of a select = row in the option list
 *      directions, first/last: caret movement (shift = selection, ctrl = word wise/whole text)
 *
 * @return byte UIInputResult flags
 */
static inline
byte ui_input_editing_action(void* app, UILayout* const layout, UINavigation action, bool shift, bool ctrl) NO_EXCEPT
{
    UIInteraction* const interaction = &layout->interaction;
    byte result = UI_INPUT_RESULT_CAPTURED;

    UIInput* const input = (UIInput *) ui_element_get(layout, interaction->editing);
    if (!ui_element_is_visible(layout, &input->core)) {
        ui_input_set_editing(layout, 0);
        result |= UI_INPUT_RESULT_CHANGED;

        return result;
    }

    if (action == UI_NAVIGATION_CANCEL) {
        UIPanel* const popup = ui_popup_get(layout);
        if (popup && ui_element_is_self_or_child_of(layout, &input->core, &popup->core)) {
            ui_popup_close(layout);
        }

        ui_input_set_editing(layout, 0);
        result |= UI_INPUT_RESULT_CHANGED;

        return result;
    }

    UICore* const owner = input->owner ? ui_element_get(layout, input->owner) : NULL;

    // Search field of a select: navigate/pick in the option list
    if (owner && owner->type == UI_ELEMENT_TYPE_SELECT) {
        UISelect* const select = (UISelect *) owner;
        UIOptionList* const list = (UIOptionList *) ui_element_get(layout, select->list);

        if (action == UI_NAVIGATION_UP || action == UI_NAVIGATION_DOWN) {
            if (ui_option_list_move_hover(layout, list, action == UI_NAVIGATION_UP ? -1 : 1)) {
                result |= UI_INPUT_RESULT_CHANGED;
            }

            return result;
        }

        if (action == UI_NAVIGATION_CONFIRM) {
            const int32 option = ui_select_filtered_option(layout, select, list->hovered >= 0 ? list->hovered : 0);

            if (option >= 0) {
                if (!(select->select_flags & UI_SELECT_FLAG_MULTI)) {
                    if (ui_select_set(layout, select, option)) {
                        ui_element_action(app, layout, owner, select->on_change);
                    }

                    ui_select_open(layout, select, false);
                } else if (ui_select_set(layout, select, option, !ui_select_is_checked(layout, select, option))) {
                    ui_element_action(app, layout, owner, select->on_change);
                }
            }

            result |= UI_INPUT_RESULT_CHANGED;

            return result;
        }
    }

    // Confirm in a textarea is a new line (only the keyboard creates it, as text)
    if (action == UI_NAVIGATION_CONFIRM) {
        if (input->core.type != UI_ELEMENT_TYPE_TEXTAREA && input->on_submit && layout->on_actions) {
            layout->on_actions[input->on_submit - 1](app, layout, &input->core);
            result |= UI_INPUT_RESULT_CHANGED;
        }

        return result;
    }

    if (ui_input_caret_move(layout, input, action, shift, ctrl)) {
        result |= UI_INPUT_RESULT_CHANGED;
    }

    return result;
}

/**
 * Applies a ui action, regardless of the device it comes from (keyboard, controller, ...)
 *
 *      next/previous: next/previous focusable element
 *      a text element is edited: ui_input_editing_action()
 *      an element has the focus: ui_input_focus_action()
 *      cancel without focus: closes the open popup (e.g. opened with the pointer)
 *
 * @param bool  shift   Keyboard only: extend the selection of a text element
 * @param bool  ctrl    Keyboard only: word wise/whole text caret movement of a text element
 *
 * @return byte UIInputResult flags (CAPTURED = the ui used the action)
 */
static inline
byte ui_navigation_apply(void* app, UILayout* const layout, UINavigation action, bool shift, bool ctrl) NO_EXCEPT
{
    switch (action) {
        case UI_NAVIGATION_NEXT:
        case UI_NAVIGATION_PREV: {
            ui_focus_move(layout, action == UI_NAVIGATION_NEXT ? 1 : -1);

            return (byte) (UI_INPUT_RESULT_CAPTURED | UI_INPUT_RESULT_CHANGED);
        }
        case UI_NAVIGATION_UP:
        case UI_NAVIGATION_DOWN:
        case UI_NAVIGATION_LEFT:
        case UI_NAVIGATION_RIGHT:
        case UI_NAVIGATION_FIRST:
        case UI_NAVIGATION_LAST:
        case UI_NAVIGATION_CONFIRM:
        case UI_NAVIGATION_CANCEL: {
            if (layout->interaction.editing) {
                return ui_input_editing_action(app, layout, action, shift, ctrl);
            }

            if (layout->interaction.nav_focus) {
                return ui_input_focus_action(app, layout, action);
            }

            if (action == UI_NAVIGATION_CANCEL && layout->interaction.popup) {
                ui_popup_close(layout);

                return (byte) (UI_INPUT_RESULT_CAPTURED | UI_INPUT_RESULT_CHANGED);
            }

            return UI_INPUT_RESULT_NONE;
        }
        default:
            return UI_INPUT_RESULT_NONE;
    }
}

/**
 * Applies one keyboard code to the element with the focus
 *
 * Keys with a ui meaning (tab, enter, escape, arrows, home/end) are converted to the ui action, ui_navigation_apply().
 * While a text element is edited everything else is text (characters, backspace, clipboard, ...),
 * also space and enter in a textarea.
 *
 * @param uint32 code Unicode codepoint or InputTextControl value (text)
 *
 * @return byte UIInputResult flags
 */
inline
byte ui_input_code(void* app, UILayout* const layout, uint32 code) NO_EXCEPT
{
    PROFILE_DEBUG(PROFILE_UI_INPUT_CODE);

    UIInteraction* const interaction = &layout->interaction;
    if (!ui_input_has_focus(layout)) {
        return UI_INPUT_RESULT_NONE;
    }

    const uint32 base = code & ~((uint32) INPUT_TEXT_MOD_MASK);
    const UINavigation action = ui_navigation_from_code(code);

    // Not edited as text: only the ui actions matter (other keys are captured but do nothing)
    if (!interaction->editing) {
        return action ? ui_navigation_apply(app, layout, action, false, false) : (byte) UI_INPUT_RESULT_CAPTURED;
    }

    UIInput* const input = (UIInput *) ui_element_get(layout, interaction->editing);
    const bool is_text = !action
        || base == ' '
        || (base == INPUT_TEXT_ENTER && input->core.type == UI_ELEMENT_TYPE_TEXTAREA);

    if (!is_text) {
        return ui_navigation_apply(
            app, layout, action,
            (code & INPUT_TEXT_MOD_SHIFT) != 0,
            (code & INPUT_TEXT_MOD_CTRL) != 0
        );
    }

    byte result = UI_INPUT_RESULT_CAPTURED;
    if (!ui_element_is_visible(layout, &input->core)) {
        ui_input_set_editing(layout, 0);
        result |= UI_INPUT_RESULT_CHANGED;

        return result;
    }

    // Only the owner needs to know about text changes (e.g. search field of a select)
    UICore* const owner = input->owner ? ui_element_get(layout, input->owner) : NULL;

    char before[128];
    const char* const text = ui_input_get_text(layout, input);
    const int32 before_length = ui_string_length(layout, input->content);
    const bool track_text = owner && before_length < (int32) sizeof(before);
    if (track_text) {
        memcpy(before, text, before_length + 1);
    }

    if (ui_input_handle_code(app, layout, input, code)) {
        result |= UI_INPUT_RESULT_CHANGED;

        if (track_text && strcmp(before, text) != 0) {
            ui_input_owner_changed(app, layout, input);
        }
    }

    return result;
}

/**
 * Applies the keyboard text input (typed this tick) to the element with the focus
 *
 * @param const uint32* codes Unicode codepoints or InputTextControl values (text)
 *
 * @return byte UIInputResult flags
 */
byte ui_input_text(void* app, UILayout* const layout, const uint32* const codes, int32 count) NO_EXCEPT
{
    PROFILE_DEBUG(PROFILE_UI_INPUT_TEXT);
    UIInteractionDirtyScope interaction_scope(layout);

    // Usually nothing was typed or nothing has the focus
    // The focus check is still repeated per code since a code can change it (e.g. tab, escape, enter)
    if (!count || !ui_input_has_focus(layout)) {
        return UI_INPUT_RESULT_NONE;
    }

    byte result = UI_INPUT_RESULT_NONE;
    for (int32 i = 0; i < count; ++i) {
        result |= ui_input_code(app, layout, codes[i]);
    }

    return result;
}

////////////////////////////////////////////////////////////////
// Navigation input (device independent, e.g. hotkeys of a controller or keyboard)
////////////////////////////////////////////////////////////////

/**
 * Device independent ui action (e.g. controller: shoulder buttons, d-pad, cross/circle)
 *
 * The application maps its (user defined) hotkeys to ui actions.
 * Text (typing, clipboard, ...) still comes from ui_input_text():
 * while a text element is edited the keyboard keys create text codes and don't trigger hotkeys (HOTKEY_CONTEXT_TYPING)
 *
 * @return byte UIInputResult flags (CAPTURED = the ui used the action)
 */
inline
byte ui_input_navigate(void* app, UILayout* const layout, UINavigation navigation) NO_EXCEPT
{
    PROFILE_DEBUG(PROFILE_UI_INPUT_NAVIGATE);
    UIInteractionDirtyScope interaction_scope(layout);

    return ui_navigation_apply(app, layout, navigation, false, false);
}

/**
 * The ui wants the navigation input (the application should enable HOTKEY_CONTEXT_UI)
 * e.g. an element has the focus or a popup is open
 */
FORCE_INLINE
bool ui_input_wants_navigation(const UILayout* const layout) NO_EXCEPT
{
    return ui_input_has_focus(layout) || layout->interaction.popup != 0;
}

/**
 * Scroll amount of a ui scroll hotkey in wheel notches
 *      - mouse wheel: one key for both directions, the sign of its value is the direction
 *      - analog controller key (e.g. stick): continuous scrolling while held, the sign is the direction
 *      - digital key (e.g. d-pad, keyboard): one notch per press
 *
 * @param int16* code Output: key that created the value (to detect if both directions use the same key)
 */
static inline
f32 ui_hotkey_scroll(const Input* const input, uint16 hotkey, int16* const code) NO_EXCEPT
{
    *code = 0;
    const byte state = input_hotkey_key_state(input, hotkey);
    if (!state) {
        return 0.0f;
    }

    const int16 value = input_hotkey_value(input, hotkey, code);
    if (*code == (INPUT_MOUSE_PREFIX | INPUT_MOUSE_BUTTON_WHEEL)
        || *code == (INPUT_MOUSE_PREFIX | INPUT_MOUSE_BUTTON_HWHEEL)
    ) {
        // 120 = one notch (WHEEL_DELTA), positive = away from the user
        return (f32) value / 120.0f;
    }

    if ((*code & INPUT_CONTROLLER_PREFIX) && (value > 1 || value < -1)) {
        // Full deflection = 1/4 notch per tick
        return -(f32) value / 127.0f * 0.25f;
    }

    return (state & KEY_PRESS_TYPE_PRESSED) ? 1.0f : 0.0f;
}

static inline
f32 ui_hotkey_axis(const Input* const input, uint16 hotkey_positive, uint16 hotkey_negative) NO_EXCEPT
{
    int16 code_positive;
    int16 code_negative;
    const f32 positive = ui_hotkey_scroll(input, hotkey_positive, &code_positive);
    const f32 negative = ui_hotkey_scroll(input, hotkey_negative, &code_negative);

    // Same signed key for both directions (e.g. mouse wheel) -> only count it once
    if (code_positive && code_positive == code_negative) {
        return positive;
    }

    return positive - (negative < 0.0f ? -negative : negative);
}

/**
 * Passes the input of all input devices to the ui
 * Everything is based on (user defined) hotkeys -> works for keyboard + mouse and controllers
 *      pointer:    position of the input that moved last (mouse or virtual cursor like game_cursor_movement())
 *                  buttons = EDITOR_HOTKEY_POINTER_PRIMARY_UI / EDITOR_HOTKEY_POINTER_SECONDARY_UI
 *                  wheel = EDITOR_HOTKEY_SCROLL_UP_UI / EDITOR_HOTKEY_SCROLL_DOWN_UI
 *      navigation: EDITOR_HOTKEY_NEXT_UI, ...
 *      text:       keyboard text while a text element is edited
 *
 * @param v2_f32* pointer_position Output: pointer position in ui coordinates (e.g. to open a context menu there)
 *
 * @return byte UIInputResult flags of the pointer input (e.g. UI_INPUT_RESULT_CONTEXT = right click on an element)
 */
byte ui_input(
    Input* const states, int32 state_count,
    uint64 timestamp,
    uint16* navigation_to_hotkey,
    void* const app,
    UILayout* const ui_layout,
    v2_f32* const pointer_position
) NO_EXCEPT
{
    *pointer_position = {0.0f, 0.0f};
    if (!state_count) {
        return UI_INPUT_RESULT_NONE;
    }

    // Pointer owner
    for (int32 i = 0; i < state_count; ++i) {
        const Input* const input_state = &states[i];
        if ((input_state->state.dx[0] || input_state->state.dy[0])
            && (input_state->mouse.handle[0]
                || hotkey_is_active(
                    input_state->state.active_hotkeys,
                    navigation_to_hotkey[UI_NAVIGATION_CURSOR_MOVEMENT1]
                )
            )
        ) {
            _ui_pointer_input = i;
            break;
        }
    }

    if (_ui_pointer_input < 0 || _ui_pointer_input >= state_count) {
        Input* const kbm = input_kbm_find(states, state_count);
        _ui_pointer_input = kbm ? (int32) (kbm - states) : 0;
    }

    Input* const pointer_input = &states[_ui_pointer_input];
    const v2_f32 position = ui_window_to_ui_position(
        ui_layout,
        (f32) pointer_input->state.x[0],
        (f32) pointer_input->state.y[0]
    );

    // Released keys stay visible until the next tick / input_frame_begin() is called
    // -> a press + release within one tick (fast click) is PRESSED | RELEASED
    const byte primary = input_hotkey_key_state(
        pointer_input,
        navigation_to_hotkey[UI_NAVIGATION_POINTER_PRIMARY_UI]
    );

    const UIPointerInput pointer = {
        position.x, position.y,
        input_hotkey_is_down(
            pointer_input,
            navigation_to_hotkey[UI_NAVIGATION_POINTER_PRIMARY_UI]
        ),
        (primary & KEY_PRESS_TYPE_RELEASED) != 0,
        (input_hotkey_key_state(
            pointer_input,
            navigation_to_hotkey[UI_NAVIGATION_POINTER_SECONDARY_UI]
        ) & KEY_PRESS_TYPE_PRESSED) != 0,
        0.0f,
        ui_hotkey_axis(
            pointer_input,
            navigation_to_hotkey[UI_NAVIGATION_SCROLL_UP_UI],
            navigation_to_hotkey[UI_NAVIGATION_SCROLL_DOWN_UI]
        ),
        timestamp
    };

    // Marks the layout as dirty if something changed
    const byte ui_input_result = ui_input_pointer(app, ui_layout, &pointer);
    *pointer_position = position;

    return ui_input_result;
}

/**
 * Text + focus actions of all inputs (keyboard, controller, ...)
 *
 * The text is only filled while a text element is edited (the keyboard keys don't trigger hotkeys then).
 * The focus actions come from the hotkeys of navigation_to_hotkey (UI_NAVIGATION_NEXT ... UI_NAVIGATION_CANCEL),
 * 0 = the action has no hotkey.
 *
 * @param uint16* navigation_to_hotkey Hotkey per UINavigation
 *
 * @return void
 */
void ui_input_navigation(
    Input* const states, int32 state_count,
    const uint16* const navigation_to_hotkey,
    void* const app,
    UILayout* const ui_layout
) NO_EXCEPT
{
    for (int32 i = 0; i < state_count; ++i) {
        const Input* const input_state = &states[i];

        ui_input_text(app, ui_layout, input_state->state.text, input_state->state.text_count);

        for (int32 navigation = UI_NAVIGATION_NEXT; navigation <= UI_NAVIGATION_CANCEL; ++navigation) {
            if (navigation_to_hotkey[navigation]
                && hotkey_is_active(input_state->state.active_hotkeys, navigation_to_hotkey[navigation])
            ) {
                ui_input_navigate(app, ui_layout, (UINavigation) navigation);
            }
        }
    }
}

#endif
