/**
 * How does UI rendering work?
 *
 * 1. Load UI components from layout file or create programmatically
 * 2. Style elements by loading theme from file
 *
 * 3. Update state of UI components
 * 4. (Re-)Create vertices cache of changed UI components (1. stage)
 *
 * 5. (Re-)Create and add (2. stage) cache for *some* components to CPU cache (GpuAtlasCache)
 * 6. Render *some* UI components and elements to a on gpu texture cache
 * 7. Copy vertices to compact/continuous vertex array on CPU (3. stage)
 *
 * 8. Draw vertices/send cache (3. stage) to GPU
 *
 * Data layout
 * ============================
 * All elements are stored in pre-order (depth-first) in UILayout::ui_element_buffer (@see UICore)
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
#include "UIInput.cpp"
#include "UISelect.cpp"
#include "UITab.cpp"
#include "UITable.cpp"
#include "UIColorPicker.cpp"
#include "UIClip.h"
#include "UIScrollbar.h"

// Starting z index of the UI, every element moves a step closer to the camera
#define UI_ZINDEX_START 10.0f

// Max size of an element/text snapshot used to detect changes made by update functions
#define UI_UPDATE_SNAPSHOT_SIZE 512

// Max nesting of clipping panels (e.g. scroll panel in a scroll panel)
#define UI_CLIP_STACK_SIZE 16

/**
 * Does the ui need to be re-cached?
 *
 * The caller decides WHEN to cache (e.g. fps limit). The dirty flag stays set until ui_cache() runs.
 */
FORCE_INLINE
bool ui_layout_needs_cache(const UILayout* const layout) NO_EXCEPT
{
    return layout->dirty || layout->has_custom_render;
}

// Is the element and all of its parents visible?
inline
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
 * Only marks the layout as dirty if an update function actually changed something
 * (e.g. the fps label only changes if the fps changed)
 *
 * MUST be called before ui_cache(), ui_cache() itself never changes the element state
 *
 * @return true if something changed
 */
bool ui_update(void* app, UILayout* const layout) NO_EXCEPT
{
    PROFILE_DEBUG(PROFILE_UI_UPDATE);

    if (!layout->ui_root || !layout->update) {
        return false;
    }

    bool changed = false;

    int32 offset = layout->ui_root->next_offset;
    while (offset) {
        UICore* const element = ui_element_get(layout, offset);

        if (element->state & UI_STATE_HIDDEN) {
            offset = element->skip_offset;
            continue;
        }

        if (element->update_func) {
            // Snapshot of the element (and its text) to detect changes
            byte snapshot[UI_UPDATE_SNAPSHOT_SIZE];
            char text_snapshot[UI_UPDATE_SNAPSHOT_SIZE];

            const int32 element_size = min_branched(ui_element_type_size(element->type), (int32) sizeof(snapshot));
            memcpy(snapshot, element, element_size);

            const char* text = NULL;
            int32 text_size = 0;
            if (element->type == UI_ELEMENT_TYPE_LABEL && ((UILabel *) element)->content) {
                const UILabel* const label = (UILabel *) element;
                text = (const char *) (layout->ui_element_buffer.memory + label->content);
                text_size = min_branched(label->content_length * ui_char_type_size(label->char_type), (int32) sizeof(text_snapshot));
                memcpy(text_snapshot, text, text_size);
            }

            layout->update[element->update_func - 1](app, layout, element);

            if (memcmp(snapshot, element, element_size) != 0
                || (text && memcmp(text_snapshot, text, text_size) != 0)
            ) {
                element->change_type |= UI_ELEMENT_CHANGE_CONTENT;
                changed = true;
            }
        }

        offset = element->next_offset;
    }

    if (changed) {
        ui_layout_mark_dirty(layout);
    }

    return changed;
}

////////////////////////////////////////////////////////////////
// Scrolling (generic for all scrollable elements)
////////////////////////////////////////////////////////////////

/**
 * Scroll info of an element for one axis (@see UIScrollbar.h)
 * Uses the dimensions of the last ui_cache()
 *
 * @return false if the element can't scroll in this axis (or the content fits)
 */
inline
bool ui_element_scroll_info(const UILayout* const layout, const UICore* const element, UIScrollAxis axis, UIScrollInfo* const info) NO_EXCEPT
{
    switch (element->type) {
        case UI_ELEMENT_TYPE_VIEW_PANEL:
            return ui_panel_scroll_info((const UIPanel *) element, axis, info);
        case UI_ELEMENT_TYPE_TEXTAREA:
            return axis == UI_SCROLL_AXIS_Y && ui_input_scroll_info(layout, (const UIInput *) element, info);
        case UI_ELEMENT_TYPE_OPTION_LIST:
            return axis == UI_SCROLL_AXIS_Y && ui_option_list_scroll_info(layout, (const UIOptionList *) element, info);
        case UI_ELEMENT_TYPE_TABLE:
            return axis == UI_SCROLL_AXIS_Y && ui_table_scroll_info((const UITable *) element, info);
        default:
            return false;
    }
}

/**
 * @return true if the scroll offset changed
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
            return axis == UI_SCROLL_AXIS_Y && ui_option_list_scroll_set(layout, (UIOptionList *) element, scroll);
        case UI_ELEMENT_TYPE_TABLE:
            return axis == UI_SCROLL_AXIS_Y && ui_table_scroll_set(layout, (UITable *) element, scroll);
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

// Clipping panel whose subtree is currently cached (@see ui_cache)
struct UICacheClip {
    // Offset of the element after the subtree (= end of the clipped area)
    int32 end;

    // Offset of the clipping panel
    int32 element;

    UIClipRect clip;
};

/**
 * Creates the vertices for all visible elements
 *
 * The tree is traversed in pre-order which means:
 *      1. a parent is always calculated before its children (dimension calculation only needs the parent)
 *      2. later elements are drawn on top of earlier elements (zindex moves closer with every element)
 *
 * Children of clipping panels (e.g. scroll panels) are clipped on the cpu (@see UIClip.h).
 * The clip rects are a stack, a subtree ends when the traversal reaches the skip_offset of the panel.
 *
 * ui_cache() only renders the current state, it never changes it (@see ui_update, ui_input_pointer)
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

    vertices->count = 0;
    indices->count = 0;

    // Degenerate starting point (@see vertex_degenerate_create)
    array_vector_insert(vertices, {{0.0f, 0.0f, UI_ZINDEX_START}, 0, {0}});

    if (!layout->ui_root) {
        layout->ui_element_changed.count = 0;
        layout->dirty = false;

        return;
    }

    const UIInteraction* const interaction = &layout->interaction;

    f32 zindex = UI_ZINDEX_START;
    bool has_custom_render = false;
    bool needs_recache = false;

    UICacheClip clips[UI_CLIP_STACK_SIZE];
    int32 clip_count = 0;

    // The root itself is never rendered (it is just the screen)
    layout->ui_root->change_type = 0;

    int32 offset = layout->ui_root->next_offset;
    while (true) {
        // Leaving clipped subtrees
        while (clip_count > 0 && clips[clip_count - 1].end == offset) {
            const int32 panel_offset = clips[--clip_count].element;
            UIPanel* const panel = (UIPanel *) ui_element_get(layout, panel_offset);

            if (panel->panel_flags & UI_PANEL_FLAG_SCROLL) {
                // The children are calculated now -> content size + scrollbars on top of the content
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
                    ui_vertices_clip_or_remove(vertices, indices, vertex_start, index_start, clips[clip_count - 1].clip);
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

        const int32 vertex_start = vertices->count;
        const int32 index_start = indices->count;

        switch (element->type) {
            case UI_ELEMENT_TYPE_BUTTON: {
                ui_button_cache((UIButton *) element, gpu_api_type, layout, &zindex);
            } break;
            case UI_ELEMENT_TYPE_LABEL: {
                ui_label_cache((UILabel *) element, layout, zindex, mem);
            } break;
            case UI_ELEMENT_TYPE_IMAGE: {
                ui_image_cache((UIImage *) element, layout, zindex);
            } break;
            case UI_ELEMENT_TYPE_VIEW_WINDOW: {
                ui_window_cache((UIWindow *) element, gpu_api_type, layout, &zindex);
            } break;
            case UI_ELEMENT_TYPE_VIEW_PANEL: {
                UIPanel* const panel = (UIPanel *) element;
                if (panel->panel_flags & UI_PANEL_FLAG_POPUP) {
                    ui_popup_layout(layout, panel);
                }

                ui_panel_cache(panel, gpu_api_type, layout, &zindex);
            } break;
            case UI_ELEMENT_TYPE_CURSOR: {
                ui_cursor_cache((UICursor *) element, layout);
            } break;
            case UI_ELEMENT_TYPE_CUSTOM: {
                ui_dimension_calculate(layout, element);

                if (element->render_func) {
                    PROFILE_DEBUG(PROFILE_UI_CACHE_CUSTOM);
                    has_custom_render = true;

                    layout->render[element->render_func - 1](
                        app,
                        element,
                        gpu_api_type,
                        layout,
                        zindex,
                        mem
                    );

                    // Custom render functions manage their own zindex
                    // -> continue after the last vertex they created
                    if (vertices->count > vertex_start) {
                        zindex = min_branched(zindex, vertices->elements[vertices->count - 1].position.z);
                    }
                }
            } break;
            case UI_ELEMENT_TYPE_CHECKBOX:
            case UI_ELEMENT_TYPE_RADIO: {
                ui_checkbox_cache((UICheckbox *) element, gpu_api_type, layout, &zindex);
            } break;
            case UI_ELEMENT_TYPE_SLIDER: {
                ui_slider_cache((UISlider *) element, gpu_api_type, layout, &zindex);
            } break;
            case UI_ELEMENT_TYPE_INPUT:
            case UI_ELEMENT_TYPE_TEXTAREA: {
                ui_input_cache((UIInput *) element, gpu_api_type, layout, &zindex, mem);
            } break;
            case UI_ELEMENT_TYPE_SELECT: {
                ui_select_cache((UISelect *) element, gpu_api_type, layout, &zindex, mem);
            } break;
            case UI_ELEMENT_TYPE_OPTION_LIST: {
                ui_option_list_cache((UIOptionList *) element, gpu_api_type, layout, &zindex, mem);
            } break;
            case UI_ELEMENT_TYPE_VIEW_TAB: {
                ui_tab_cache((UITab *) element, gpu_api_type, layout, &zindex, mem);
            } break;
            case UI_ELEMENT_TYPE_TABLE: {
                ui_table_cache((UITable *) element, gpu_api_type, layout, &zindex, mem);
            } break;
            case UI_ELEMENT_TYPE_COLOR_PICKER: {
                ui_color_picker_cache((UIColorPicker *) element, gpu_api_type, layout, &zindex);
            } break;
            case UI_ELEMENT_TYPE_COLOR_PANEL: {
                ui_color_panel_cache((UIColorPanel *) element, gpu_api_type, layout, &zindex);
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
        if (offset == interaction->keyboard_focus) {
            zindex = camera_step_closer(gpu_api_type, zindex);

            const UIAttributeDimension* const dim = &element->dimension;
            ui_border_solid_vertices(
                {dim->pos.x - 2.0f, dim->pos.y + 2.0f, dim->dim.x + 4.0f, dim->dim.y + 4.0f},
                UI_DEFAULT_CONTROL_BORDER_FOCUS, 2,
                vertices, indices, zindex
            );
        }

        // Clip everything this element created to the clipping panels it is in
        if (clip_count) {
            ui_vertices_clip_or_remove(vertices, indices, vertex_start, index_start, clips[clip_count - 1].clip);
        }

        // Entering the subtree of a clipping panel
        if (ui_panel_is_clipping(element)
            && element->next_offset != element->skip_offset
            && clip_count < UI_CLIP_STACK_SIZE
        ) {
            const UIPanel* const panel = (const UIPanel *) element;
            UIClipRect clip = ui_clip_from_rect(
                (panel->panel_flags & UI_PANEL_FLAG_SCROLL)
                    ? ui_panel_viewport(panel)
                    : ui_dimension_rect(&element->dimension)
            );

            if (clip_count) {
                clip = ui_clip_intersect(clip, clips[clip_count - 1].clip);
            }

            clips[clip_count++] = {element->skip_offset, offset, clip};
        }

        element->change_type = 0;
        offset = element->next_offset;
    }

    layout->has_custom_render = has_custom_render;
    layout->dirty = needs_recache;
    layout->ui_element_changed.count = 0;
}

////////////////////////////////////////////////////////////////
// Hit testing
////////////////////////////////////////////////////////////////

FORCE_INLINE
bool ui_element_has_action(const UICore* const element) NO_EXCEPT
{
    if (element->state & UI_STATE_DISABLED) {
        return false;
    }

    if (element->flags & UI_ELEMENT_FLAG_ACTION_MASK) {
        return true;
    }

    return element->type == UI_ELEMENT_TYPE_BUTTON
        && ((const UIButton *) element)->on_click;
}

/**
 * Finds the UI element at a given position
 *
 * Coordinates are in the same space as UICore::dimension (y-up, origin bottom left).
 * If your mouse coordinates are y-down you need to convert them first: y = screen_height - mouse_y
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
 * @param hit_element (optional) Top most element at (x, y) regardless of actions.
 *                    If it is NULL the UI wasn't hit at all (e.g. forward the input to the game world)
 *
 * @return Element with an action bound or NULL
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
 * Scrollbar at the position (of the element or one of its parents)
 *
 * @return Element the scrollbar belongs to or NULL
 */
inline
UICore* ui_scrollbar_at(
    const UILayout* const layout, UICore* element, f32 x, f32 y,
    UIScrollAxis* const axis, UIScrollInfo* const info
) NO_EXCEPT
{
    while (element && element != layout->ui_root) {
        if (ui_element_scroll_info(layout, element, UI_SCROLL_AXIS_Y, info) && ui_scrollbar_contains(info, x, y)) {
            *axis = UI_SCROLL_AXIS_Y;
            return element;
        }

        if (ui_element_scroll_info(layout, element, UI_SCROLL_AXIS_X, info) && ui_scrollbar_contains(info, x, y)) {
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
// Order per update tick (same as most engines, e.g. Unity's EventSystem, Unreal's Slate, Godot):
//      1. poll input                                   (on_update_input)
//      2. update the ui state from the input           (ui_input_*), uses the rects of the last ui_cache()
//      3. game logic                                   (may also change ui state)
//      4. rebuild the vertices from the current state  (ui_cache)
//
// Rendering (ui_cache) only reads the state, it never changes it.
// This keeps ui_cache independent from the input and allows to throttle it (e.g. gpu_ui_fps)
// while the state is still updated every tick.
// The hit test uses the rects of the previous ui_cache() call -> 1 frame latency which isn't noticeable

/**
 * Converts window coordinates (y-down, origin top left) to ui coordinates (y-up, origin bottom left)
 */
FORCE_INLINE
v2_f32 ui_window_to_ui_position(const UILayout* const layout, f32 x, f32 y) NO_EXCEPT
{
    return {x, layout->ui_root ? layout->ui_root->dimension.dim.y - y : y};
}

// Pointer (mouse) state for one tick
struct UIPointerInput {
    // Pointer position in ui coordinates (@see ui_window_to_ui_position)
    f32 x;
    f32 y;

    // Primary button is currently down
    bool down;

    // Primary button was released this tick
    // Required to detect a press + release within the same tick (key state PRESSED | RELEASED -> down = false, released = true)
    bool released;

    // Secondary button (e.g. right mouse button) was pressed this tick (@see UIInteraction::context_target)
    bool secondary_pressed;

    // Mouse wheel movement this tick in notches (y: positive = away from the user, x: positive = right)
    f32 wheel_x;
    f32 wheel_y;

    // Current time in microseconds, used for double/triple clicks (0 = no multi click detection)
    uint64 time;
};

// Max time between clicks of a double/triple click
#define UI_MULTI_CLICK_TIME 500000

enum UIInputResult : byte {
    UI_INPUT_RESULT_NONE = 0,

    // Some element state/position/order changed -> ui needs to be re-cached
    UI_INPUT_RESULT_CHANGED = 1 << 0,

    // The pointer is used by the UI -> the game should ignore the pointer input
    UI_INPUT_RESULT_CAPTURED = 1 << 1,

    // An element got clicked (@see UIInteraction::clicked)
    UI_INPUT_RESULT_CLICKED = 1 << 2,

    // An element got right clicked (@see UIInteraction::context_target), e.g. open a context menu
    UI_INPUT_RESULT_CONTEXT = 1 << 3,
};

/**
 * Sets/removes a state flag of an element and marks it as changed
 *
 * @return true if the state changed
 */
inline
bool ui_element_state_set(UILayout* const layout, int32 offset, byte state, bool enable) NO_EXCEPT
{
    if (!offset) {
        return false;
    }

    UICore* const element = ui_element_get(layout, offset);
    const byte new_state = enable
        ? (byte) (element->state | state)
        : (byte) (element->state & ~state);

    if (new_state == element->state) {
        return false;
    }

    element->state = new_state;
    element->change_type |= UI_ELEMENT_CHANGE_CONTENT;
    array_vector_insert_safe(&layout->ui_element_changed, UIElementChange{offset, UI_ELEMENT_CHANGE_CONTENT});

    return true;
}

// Closest element (self or parent) with any of the flags that is not disabled
inline
UICore* ui_element_closest_with_flag(const UILayout* const layout, UICore* element, byte flags) NO_EXCEPT
{
    while (element
        && element != layout->ui_root
        && (!(element->flags & flags) || (element->state & UI_STATE_DISABLED))
    ) {
        element = ui_element_parent(layout, element);
    }

    return element == layout->ui_root ? NULL : element;
}

FORCE_INLINE
int32 ui_element_offset_or_none(const UILayout* const layout, const UICore* const element) NO_EXCEPT
{
    return element ? ui_element_offset(layout, element) : 0;
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
 * @return true if the position changed
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
    // (@see ui_dimension_calculate) -> the sign depends on the anchor
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
    array_vector_insert_safe(&layout->ui_element_changed, UIElementChange{interaction->dragged, UI_ELEMENT_CHANGE_POS});

    return true;
}

/**
 * Resizes the resized element (e.g. window) by the pointer movement since the resize start
 * The top left corner stays where it is (the position is corrected for other alignments)
 * Only pixel sizes can be resized (relative/fill sizes are kept)
 *
 * @return true if the size changed
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
        const f32 align = (dim->align_anchor & UI_ALIGN_H_RIGHT) ? 1.0f : ((dim->align_anchor & UI_ALIGN_H_CENTER) ? 0.5f : 0.0f);
        if (align > 0.0f && !(dim->flag & UI_DIMENSION_POS_X_RELATIVE)) {
            dim->pos_raw.x = interaction->drag_start_pos.x + ((dim->align_anchor & UI_ANCHOR_H_RIGHT) ? -1.0f : 1.0f) * align * delta;
        }
    }

    if (!(dim->flag & UI_DIMENSION_DIM_Y_RELATIVE) && interaction->drag_start_dim.y > 0.0f) {
        const f32 height = max_branched(interaction->drag_start_dim.y + dy, min_dim.y);
        const f32 delta = height - interaction->drag_start_dim.y;
        dim->dim_raw.y = height;

        // pos.y (top) = base_y + height * align -> move the base to keep the top in place
        const f32 align = (dim->align_anchor & UI_ALIGN_V_TOP) ? 0.0f : ((dim->align_anchor & UI_ALIGN_V_CENTER) ? 0.5f : 1.0f);
        if (align > 0.0f && !(dim->flag & UI_DIMENSION_POS_Y_RELATIVE)) {
            dim->pos_raw.y = interaction->drag_start_pos.y + ((dim->align_anchor & UI_ANCHOR_V_TOP) ? 1.0f : -1.0f) * align * delta;
        }
    }

    if (old_dim.x == dim->dim_raw.x && old_dim.y == dim->dim_raw.y
        && old_pos.x == dim->pos_raw.x && old_pos.y == dim->pos_raw.y
    ) {
        return false;
    }

    element->change_type |= UI_ELEMENT_CHANGE_DIM_OTHER;
    array_vector_insert_safe(&layout->ui_element_changed, UIElementChange{interaction->resized, UI_ELEMENT_CHANGE_DIM_OTHER});

    return true;
}

/**
 * Moves the content of the element whose scrollbar thumb is dragged
 *
 * @return true if the scroll offset changed
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
FORCE_INLINE
void ui_element_action(void* app, UILayout* const layout, UICore* const element, int32 action) NO_EXCEPT
{
    if (action && layout->on_actions) {
        layout->on_actions[action - 1](app, layout, element);
    }
}

FORCE_INLINE
bool ui_element_is_text_input(const UICore* const element) NO_EXCEPT
{
    return element->type == UI_ELEMENT_TYPE_INPUT || element->type == UI_ELEMENT_TYPE_TEXTAREA;
}

/**
 * Changes which text element receives the keyboard input (0 = none)
 * The caret is placed at the end of the text
 *
 * @return true if it changed
 */
inline
bool ui_input_set_editing(UILayout* const layout, int32 offset) NO_EXCEPT
{
    UIInteraction* const interaction = &layout->interaction;
    if (interaction->editing == offset) {
        return false;
    }

    ui_element_state_set(layout, interaction->editing, UI_STATE_FOCUS, false);
    ui_element_state_set(layout, offset, UI_STATE_FOCUS, true);
    interaction->editing = offset;

    if (offset) {
        UIInput* const input = (UIInput *) ui_element_get(layout, offset);
        input->caret = (int32) strlen(ui_input_get_text(layout, input));
        input->selection_anchor = input->caret;
        ui_input_scroll_to_caret(layout, input);
    }

    ui_layout_mark_dirty(layout);

    return true;
}

// Is a text element receiving the keyboard input? (e.g. to disable hotkeys while typing)
FORCE_INLINE
bool ui_input_is_editing(const UILayout* const layout) NO_EXCEPT
{
    return layout->interaction.editing != 0;
}

FORCE_INLINE
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
    if (!select->popup) {
        return;
    }

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
        ui_input_set_text(layout, (UIInput *) ui_element_get(layout, select->search), "");
        ui_input_set_editing(layout, select->search);
    }

    // Show the selected option
    if (select->selected > 0) {
        list->scroll = ui_select_row_height(select) * (f32) (select->selected - UI_SELECT_MAX_VISIBLE_OPTIONS / 2);
        list->scroll = max_branched(list->scroll, 0.0f);
    }
}

// Opens/closes the popup of a popup owner (select, color picker)
inline
void ui_popup_toggle(UILayout* const layout, int32 popup) NO_EXCEPT
{
    if (!popup) {
        return;
    }

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
            UIColorPicker* const picker = ui_color_panel_picker(layout, panel);

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
 * @param start true = the element was just pressed
 *
 * @return true if the value changed
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
                UIColorPicker* const picker = ui_color_panel_picker(layout, panel);
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
 * @param x, y Pointer position
 * @param reset true = the element is not hovered anymore
 *
 * @return true if it changed
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
 * Updates the hover, press (active), focus, drag, resize, scroll and click state from the pointer input
 *
 *      hover:      closest UI_ELEMENT_FLAG_HOVERABLE element under the pointer (unchanged while dragging)
 *      press:      closest element with an action under the pointer gets UI_STATE_ACTIVE
 *                  closest UI_ELEMENT_FLAG_HITBOX element gets UI_STATE_FOCUS and is moved to the front (e.g. window)
 *                  pressing an UI_ELEMENT_FLAG_MOVABLE element starts dragging (@see ui_element_drag_target)
 *                  pressing an UI_ELEMENT_FLAG_RESIZABLE element starts resizing (e.g. window resize grip)
 *                  pressing a scrollbar starts dragging its thumb (pressing the track jumps to the position)
 *      drag:       the dragged element follows the pointer until the button is released
 *      click:      button released over the pressed element (and it wasn't dragged) -> @see ui_element_click
 *      value:      sliders/color gradients follow the pointer while pressed (@see ui_element_drag_value)
 *      wheel:      scrolls the closest scrollable element under the pointer
 *      right:      the hit element is reported as context target (UI_INPUT_RESULT_CONTEXT)
 *      text:       pressing an input/textarea starts editing it (@see ui_input_text), pressing anything else stops it
 *      popup:      the open popup is on top -> it gets the pointer input first, pressing outside of it closes it
 *
 * MUST be called after the input was polled and before ui_cache() (@see Input handling above)
 *
 * @return UIInputResult flags
 */
byte ui_input_pointer(void* app, UILayout* const layout, const UIPointerInput* const input) NO_EXCEPT
{
    UIInteraction* const interaction = &layout->interaction;
    interaction->clicked = 0;
    interaction->context_target = 0;

    if (!layout->ui_root) {
        return UI_INPUT_RESULT_NONE;
    }

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
    // Pressing outside of it closes it (pressing the owner toggles it through the click @see ui_element_click)
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
        const int32 hovered_offset = ui_element_offset_or_none(layout, hovered);

        if (hovered_offset != interaction->hovered) {
            if (interaction->hovered) {
                ui_element_hover_item(layout, ui_element_get(layout, interaction->hovered), x, y, true);
            }

            ui_element_state_set(layout, interaction->hovered, UI_STATE_HOVER, false);
            ui_element_state_set(layout, hovered_offset, UI_STATE_HOVER, true);
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
        for (UICore* element = hit; element && element != layout->ui_root; element = ui_element_parent(layout, element)) {
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
        // The pointer takes over -> no keyboard focus anymore (text elements use editing below)
        if (interaction->keyboard_focus) {
            interaction->keyboard_focus = 0;
            result |= UI_INPUT_RESULT_CHANGED;
        }

        // Double/triple click = pressed again on the same element shortly after at (almost) the same position
        const int32 action_offset = ui_element_offset_or_none(layout, action);
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
        const int32 focused_offset = ui_element_offset_or_none(layout, focused);

        if (focused_offset != interaction->focused) {
            ui_element_state_set(layout, interaction->focused, UI_STATE_FOCUS, false);
            ui_element_state_set(layout, focused_offset, UI_STATE_FOCUS, true);
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
        ui_layout_mark_dirty(layout);
    }

    return result;
}

////////////////////////////////////////////////////////////////
// Keyboard focus (tab navigation)
////////////////////////////////////////////////////////////////
//
// Text elements receive the keyboard input through interaction.editing (@see ui_input_set_editing).
// Other elements can only get the keyboard focus through tab navigation (interaction.keyboard_focus),
// clicking them with the pointer doesn't focus them (otherwise every button click would block the game hotkeys).
// The application should enable its typing context while ui_input_has_keyboard_focus() is true.

// Does the ui receive the keyboard input? (e.g. to disable hotkeys)
FORCE_INLINE
bool ui_input_has_keyboard_focus(const UILayout* const layout) NO_EXCEPT
{
    return layout->interaction.editing != 0 || layout->interaction.keyboard_focus != 0;
}

// Can the element be reached with tab?
FORCE_INLINE
bool ui_element_is_focusable(const UICore* const element) NO_EXCEPT
{
    // Pointer transparent elements are display only (e.g. debug overlay tables)
    if ((element->state & UI_STATE_DISABLED) || (element->flags & UI_ELEMENT_FLAG_IGNORE_HIT)) {
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
        if (parent->type != UI_ELEMENT_TYPE_VIEW_PANEL || !(((UIPanel *) parent)->panel_flags & UI_PANEL_FLAG_SCROLL)) {
            continue;
        }

        UIPanel* const panel = (UIPanel *) parent;
        const v4_f32 viewport = ui_panel_viewport(panel);

        // Content moves up when scroll.y increases, left when scroll.x increases
        if (dim->pos.y > viewport.y) {
            ui_panel_scroll_set(layout, panel, UI_SCROLL_AXIS_Y, panel->scroll.y - (dim->pos.y - viewport.y));
        } else if (dim->pos.y - dim->dim.y < viewport.y - viewport.height) {
            ui_panel_scroll_set(layout, panel, UI_SCROLL_AXIS_Y, panel->scroll.y + ((viewport.y - viewport.height) - (dim->pos.y - dim->dim.y)));
        }

        if (dim->pos.x < viewport.x) {
            ui_panel_scroll_set(layout, panel, UI_SCROLL_AXIS_X, panel->scroll.x - (viewport.x - dim->pos.x));
        } else if (dim->pos.x + dim->dim.x > viewport.x + viewport.width) {
            ui_panel_scroll_set(layout, panel, UI_SCROLL_AXIS_X, panel->scroll.x + (dim->pos.x + dim->dim.x - (viewport.x + viewport.width)));
        }
    }
}

/**
 * Gives the element the keyboard focus
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
        interaction->keyboard_focus = 0;
        ui_input_set_editing(layout, offset);

        UIInput* const input = (UIInput *) element;
        ui_input_select_all(layout, input);
        ui_input_scroll_to_caret(layout, input);
    } else {
        ui_input_set_editing(layout, 0);
        interaction->keyboard_focus = offset;
    }

    element->change_type |= UI_ELEMENT_CHANGE_CONTENT;
    ui_element_scroll_into_view(layout, element);
    ui_layout_mark_dirty(layout);
}

/**
 * Moves the keyboard focus to the next/previous focusable element (tab / shift + tab)
 * The focus cycles within the window/popup of the current focus
 *
 * @param direction 1 = next, -1 = previous
 */
inline
void ui_focus_move(UILayout* const layout, int32 direction) NO_EXCEPT
{
    UIInteraction* const interaction = &layout->interaction;
    if (!layout->ui_root) {
        return;
    }

    const int32 current = interaction->editing ? interaction->editing : interaction->keyboard_focus;

    // Container = window/popup of the current focus (closest UI_ELEMENT_FLAG_HITBOX element)
    UICore* container = NULL;
    if (current) {
        container = ui_element_closest_with_flag(layout, ui_element_parent(layout, ui_element_get(layout, current)), UI_ELEMENT_FLAG_HITBOX);
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
 * Keyboard input for the focused (non text) element
 *      button:       enter/space = click
 *      checkbox:     enter/space = toggle
 *      radio:        enter/space = check, arrows = previous/next radio of the group
 *      slider:       arrows = -/+ step, home/end = min/max
 *      select:       enter/space = open/pick, up/down = previous/next option (or row while open)
 *      color picker: enter/space = open/close
 *      tab:          arrows = previous/next tab
 *      table:        up/down/home/end = select row
 *
 * @return true if something changed
 */
inline
bool ui_element_key(void* app, UILayout* const layout, UICore* const element, uint32 code) NO_EXCEPT
{
    code &= ~((uint32) INPUT_TEXT_MOD_MASK);

    const bool activate = code == INPUT_TEXT_ENTER || code == ' ';
    const int32 delta = (code == INPUT_TEXT_LEFT || code == INPUT_TEXT_UP)
        ? -1
        : ((code == INPUT_TEXT_RIGHT || code == INPUT_TEXT_DOWN) ? 1 : 0);

    switch (element->type) {
        case UI_ELEMENT_TYPE_BUTTON:
        case UI_ELEMENT_TYPE_CHECKBOX:
        case UI_ELEMENT_TYPE_COLOR_PICKER: {
            if (!activate) {
                return false;
            }

            ui_element_click(app, layout, element, element->dimension.pos.x, element->dimension.pos.y);

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
            } else if (code == INPUT_TEXT_HOME) {
                value = 0.0f;
            } else if (code == INPUT_TEXT_END) {
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

                if (code == INPUT_TEXT_UP || code == INPUT_TEXT_DOWN) {
                    return ui_option_list_move_hover(layout, list, code == INPUT_TEXT_UP ? -1 : 1);
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
            }

            if (activate) {
                ui_select_open(layout, select, true);

                return true;
            }

            // Closed single select: up/down changes the selection directly
            if ((code == INPUT_TEXT_UP || code == INPUT_TEXT_DOWN) && !(select->select_flags & UI_SELECT_FLAG_MULTI)) {
                const int32 index = clamp_branched(select->selected + (code == INPUT_TEXT_UP ? -1 : 1), 0, select->option_count - 1);
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

            int32 row;
            if (code == INPUT_TEXT_UP || code == INPUT_TEXT_DOWN) {
                row = table->selected < 0
                    ? 0
                    : clamp_branched(table->selected + (code == INPUT_TEXT_UP ? -1 : 1), 0, table->row_count - 1);
            } else if (code == INPUT_TEXT_HOME) {
                row = 0;
            } else if (code == INPUT_TEXT_END) {
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
            ui_layout_mark_dirty(layout);
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
 * Applies one keyboard code to the element with the keyboard focus (@see ui_input_text)
 *
 *      tab / shift + tab: next/previous focusable element (@see ui_focus_move)
 *      text elements (@see ui_input_set_editing):
 *          escape: stops editing (and closes the popup the element is in, e.g. search field of a select)
 *          search field of a select: up/down = row in the option list, enter = hovered row or first match
 *      other elements (@see ui_element_key):
 *          escape: closes the open popup or removes the keyboard focus
 *
 * @param code Unicode codepoint or InputTextControl value (@see InputState::text)
 *
 * @return UIInputResult flags
 */
inline
byte ui_input_code(void* app, UILayout* const layout, uint32 code) NO_EXCEPT
{
    UIInteraction* const interaction = &layout->interaction;
    if (!ui_input_has_keyboard_focus(layout)) {
        return UI_INPUT_RESULT_NONE;
    }

    byte result = UI_INPUT_RESULT_CAPTURED;
    const uint32 base = code & ~((uint32) INPUT_TEXT_MOD_MASK);

    if (base == INPUT_TEXT_TAB) {
        ui_focus_move(layout, (code & INPUT_TEXT_MOD_SHIFT) ? -1 : 1);
        result |= UI_INPUT_RESULT_CHANGED;

        return result;
    }

    // Non text element (tab navigation)
    ////////////////////////////////////////////////////////
    if (interaction->keyboard_focus) {
        UICore* const element = ui_element_get(layout, interaction->keyboard_focus);
        if (!ui_element_is_visible(layout, element)) {
            interaction->keyboard_focus = 0;
            result |= UI_INPUT_RESULT_CHANGED;

            return result;
        }

        if (base == INPUT_TEXT_ESCAPE) {
            if (interaction->popup) {
                ui_popup_close(layout);
            } else {
                interaction->keyboard_focus = 0;
            }

            ui_layout_mark_dirty(layout);
            result |= UI_INPUT_RESULT_CHANGED;

            return result;
        }

        if (ui_element_key(app, layout, element, code)) {
            result |= UI_INPUT_RESULT_CHANGED;
        }

        return result;
    }

    // Text element
    ////////////////////////////////////////////////////////
    if (!interaction->editing) {
        return result;
    }

    UIInput* const input = (UIInput *) ui_element_get(layout, interaction->editing);
    if (!ui_element_is_visible(layout, &input->core)) {
        ui_input_set_editing(layout, 0);
        result |= UI_INPUT_RESULT_CHANGED;

        return result;
    }

    UICore* const owner = input->owner ? ui_element_get(layout, input->owner) : NULL;

    if (base == INPUT_TEXT_ESCAPE) {
        UIPanel* const popup = ui_popup_get(layout);
        if (popup && ui_element_is_self_or_child_of(layout, &input->core, &popup->core)) {
            ui_popup_close(layout);
        }

        ui_input_set_editing(layout, 0);
        result |= UI_INPUT_RESULT_CHANGED;

        return result;
    }

    // Search field of a select: navigate/pick in the option list
    if (owner && owner->type == UI_ELEMENT_TYPE_SELECT) {
        UISelect* const select = (UISelect *) owner;
        UIOptionList* const list = (UIOptionList *) ui_element_get(layout, select->list);

        if (base == INPUT_TEXT_UP || base == INPUT_TEXT_DOWN) {
            if (ui_option_list_move_hover(layout, list, base == INPUT_TEXT_UP ? -1 : 1)) {
                result |= UI_INPUT_RESULT_CHANGED;
            }

            return result;
        }

        if (base == INPUT_TEXT_ENTER) {
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

    // Only the owner needs to know about text changes (not caret movements)
    char before[128];
    const char* const text = ui_input_get_text(layout, input);
    const bool track_text = owner && input->content_length <= (int32) sizeof(before);
    if (track_text) {
        memcpy(before, text, input->content_length);
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
 * Applies the keyboard text input (typed this tick) to the element with the keyboard focus
 *
 * @param codes Unicode codepoints or InputTextControl values (@see InputState::text)
 *
 * @return UIInputResult flags
 */
byte ui_input_text(void* app, UILayout* const layout, const uint32* const codes, int32 count) NO_EXCEPT
{
    byte result = UI_INPUT_RESULT_NONE;
    for (int32 i = 0; i < count; ++i) {
        result |= ui_input_code(app, layout, codes[i]);
    }

    return result;
}

////////////////////////////////////////////////////////////////
// Navigation input (device independent, e.g. hotkeys of a controller or keyboard)
////////////////////////////////////////////////////////////////

enum UINavigation : byte {
    UI_NAVIGATION_NONE,

    // Next/previous focusable element (@see ui_focus_move), also starts the navigation if nothing has the focus
    UI_NAVIGATION_NEXT,
    UI_NAVIGATION_PREV,

    UI_NAVIGATION_UP,
    UI_NAVIGATION_DOWN,
    UI_NAVIGATION_LEFT,
    UI_NAVIGATION_RIGHT,

    // e.g. click a button, open a select, submit an input
    UI_NAVIGATION_CONFIRM,

    // e.g. close the popup, stop editing, remove the focus
    UI_NAVIGATION_CANCEL,
};

/**
 * Device independent focus navigation (e.g. controller: shoulder buttons, d-pad, cross/circle)
 *
 * The application maps its (user defined) hotkeys to navigation actions.
 * Keyboard text editing still uses ui_input_text() (caret movement, selection, typing):
 * while a text element is edited the keyboard keys create text codes and don't trigger hotkeys (HOTKEY_CONTEXT_TYPING)
 *
 * @return UIInputResult flags (CAPTURED = the ui used the action)
 */
inline
byte ui_input_navigate(void* app, UILayout* const layout, UINavigation navigation) NO_EXCEPT
{
    switch (navigation) {
        case UI_NAVIGATION_NEXT:
        case UI_NAVIGATION_PREV: {
            ui_focus_move(layout, navigation == UI_NAVIGATION_NEXT ? 1 : -1);

            return (byte) (UI_INPUT_RESULT_CAPTURED | UI_INPUT_RESULT_CHANGED);
        }
        case UI_NAVIGATION_CANCEL: {
            // Nothing focused but a popup is open (e.g. opened with the pointer)
            if (!ui_input_has_keyboard_focus(layout)) {
                if (!layout->interaction.popup) {
                    return UI_INPUT_RESULT_NONE;
                }

                ui_popup_close(layout);

                return (byte) (UI_INPUT_RESULT_CAPTURED | UI_INPUT_RESULT_CHANGED);
            }

            return ui_input_code(app, layout, INPUT_TEXT_ESCAPE);
        }
        case UI_NAVIGATION_CONFIRM: {
            // Enter in a textarea is a new line, not a confirmation
            if (layout->interaction.editing
                && ui_element_get(layout, layout->interaction.editing)->type == UI_ELEMENT_TYPE_TEXTAREA
            ) {
                return UI_INPUT_RESULT_CAPTURED;
            }

            return ui_input_code(app, layout, INPUT_TEXT_ENTER);
        }
        case UI_NAVIGATION_UP: return ui_input_code(app, layout, INPUT_TEXT_UP);
        case UI_NAVIGATION_DOWN: return ui_input_code(app, layout, INPUT_TEXT_DOWN);
        case UI_NAVIGATION_LEFT: return ui_input_code(app, layout, INPUT_TEXT_LEFT);
        case UI_NAVIGATION_RIGHT: return ui_input_code(app, layout, INPUT_TEXT_RIGHT);
        default:
            return UI_INPUT_RESULT_NONE;
    }
}

/**
 * The ui wants the navigation input (the application should enable HOTKEY_CONTEXT_UI)
 * e.g. an element has the keyboard focus or a popup is open
 */
FORCE_INLINE
bool ui_input_wants_navigation(const UILayout* const layout) NO_EXCEPT
{
    return ui_input_has_keyboard_focus(layout) || layout->interaction.popup != 0;
}

#endif
