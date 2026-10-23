#pragma once
#ifndef COMS_UI_PANEL_C
#define COMS_UI_PANEL_C

#include "../stdlib/Stdlib.h"
#include "../gpuapi/RenderUtils.h"
#include "../camera/Camera.cpp"
#include "attribute/UIAttributeDimension.cpp"
#include "attribute/UIAttributeBorder.cpp"
#include "UIAlignment.h"
#include "UIStyleDefaults.h"
#include "UIScrollbar.h"
#include "UICore.cpp"
#include "UILayout.h"
#include "UIPanel.h"

/**
 * Sets the root to the screen size
 * Must be called whenever the window/viewport size changes
 */
inline
void ui_root_resize(UILayout* const layout, f32 width, f32 height) NO_EXCEPT
{
    UICore* const root = layout->ui_root;
    if (!root) {
        return;
    }

    root->dimension.dim_raw = {width, height};
    root->dimension.dim = {width, height};

    // y = height since pos is the top left corner in our y-up coordinate system
    root->dimension.pos_raw = {0.0f, height};
    root->dimension.pos = {0.0f, height};

    root->change_type |= UI_ELEMENT_CHANGE_DIM_OTHER;
    ui_layout_mark_dirty(layout);
}

/**
 * Creates the root element which MUST be the first element in the ui_element_buffer (offset 0)
 * If the layout already has a root (e.g. loaded from a layout file) the existing root is resized and returned
 */
inline
UICore* ui_root_create(UILayout* layout, f32 width, f32 height) NO_EXCEPT
{
    if (layout->ui_element_buffer.head != layout->ui_element_buffer.memory) {
        // The buffer already contains elements -> the root must be at offset 0
        layout->ui_root = (UICore *) layout->ui_element_buffer.memory;
        ASSERT_TRUE(layout->ui_root->type == UI_ELEMENT_TYPE_VIEW_PANEL);

        ui_root_resize(layout, width, height);

        return layout->ui_root;
    }

    UIPanel* const element = (UIPanel *) memory_get(&layout->ui_element_buffer, sizeof(UIPanel), alignof(UIPanel));
    ASSERT_TRUE((byte *) element == layout->ui_element_buffer.memory);
    MEMORY_ELEMENT_ZERO(element);

    element->core.type = UI_ELEMENT_TYPE_VIEW_PANEL;
    element->core.dimension.align_anchor = UI_ANCHOR_ALIGN_TOP_LEFT;

    // The root covers the whole screen, a hit on the root is not a hit on the UI
    element->core.flags = UI_ELEMENT_FLAG_IGNORE_HIT;

    // Root has no parent/next/skip -> all 0
    layout->ui_root = &element->core;
    ui_root_resize(layout, width, height);

    return layout->ui_root;
}

/**
 * @param parent NULL = root
 */
inline
UIPanel* ui_panel_create(
    UILayout* const layout,
    UICore* parent = NULL,
    uint32 background_color = 0
) NO_EXCEPT
{
    UIPanel* const element = UI_ELEMENT_ALLOC(layout, UI_ELEMENT_TYPE_VIEW_PANEL, UIPanel, parent);
    element->core.dimension.align_anchor = UI_ANCHOR_ALIGN_TOP_LEFT;
    element->background_color = background_color;

    return element;
}

FORCE_INLINE
bool ui_panel_is_clipping(const UICore* const element) NO_EXCEPT
{
    return element->type == UI_ELEMENT_TYPE_VIEW_PANEL
        && (((const UIPanel *) element)->panel_flags & UI_PANEL_FLAG_CLIP);
}

////////////////////////////////////////////////////////////////
// Scroll panel
////////////////////////////////////////////////////////////////
// A scroll panel consists of 2 elements:
//      scroll panel (viewport, clips its children, draws the scrollbars)
//          content panel (moved by the scroll offset, all elements are added to the content)
//              ... your elements
//
// The content size is calculated from the children of the content panel (@see ui_cache)

/**
 * @param flags UI_PANEL_FLAG_SCROLL_X and/or UI_PANEL_FLAG_SCROLL_Y
 * @param parent NULL = root
 */
inline
UIPanel* ui_scroll_panel_create(
    UILayout* const layout,
    byte flags = UI_PANEL_FLAG_SCROLL_Y,
    UICore* parent = NULL,
    uint32 background_color = 0
) NO_EXCEPT
{
    UIPanel* const panel = ui_panel_create(layout, parent, background_color);
    panel->panel_flags = (byte) ((flags & UI_PANEL_FLAG_SCROLL) | UI_PANEL_FLAG_CLIP);

    // The content is created directly after the panel (pre-order)
    UIPanel* const content = ui_panel_create(layout, &panel->core);
    content->panel_flags = UI_PANEL_FLAG_SCROLL_CONTENT;
    panel->content = ui_element_offset(layout, content);

    return panel;
}

// Element new children should be added to (the content of a scroll panel, otherwise the panel itself)
FORCE_INLINE
UICore* ui_panel_content(const UILayout* const layout, UIPanel* const panel) NO_EXCEPT
{
    return panel->content ? ui_element_get(layout, panel->content) : &panel->core;
}

/**
 * Visible area of a scroll panel (excluding the scrollbars)
 * Uses the dimension and content size of the last ui_cache()
 *
 * @return x, y = top left, width, height
 */
inline
v4_f32 ui_panel_viewport(const UIPanel* const panel, bool* const has_x = NULL, bool* const has_y = NULL) NO_EXCEPT
{
    const UIAttributeDimension* const dim = &panel->core.dimension;

    bool scroll_y = (panel->panel_flags & UI_PANEL_FLAG_SCROLL_Y) && panel->content_size.y > dim->dim.y;
    const bool scroll_x = (panel->panel_flags & UI_PANEL_FLAG_SCROLL_X)
        && panel->content_size.x > dim->dim.x - (scroll_y ? UI_SCROLLBAR_SIZE : 0.0f);

    // The horizontal scrollbar takes space away -> the content might not fit vertically anymore
    if (!scroll_y && scroll_x
        && (panel->panel_flags & UI_PANEL_FLAG_SCROLL_Y)
        && panel->content_size.y > dim->dim.y - UI_SCROLLBAR_SIZE
    ) {
        scroll_y = true;
    }

    if (has_x) {
        *has_x = scroll_x;
    }

    if (has_y) {
        *has_y = scroll_y;
    }

    return {
        dim->pos.x,
        dim->pos.y,
        max_branched(dim->dim.x - (scroll_y ? UI_SCROLLBAR_SIZE : 0.0f), 0.0f),
        max_branched(dim->dim.y - (scroll_x ? UI_SCROLLBAR_SIZE : 0.0f), 0.0f)
    };
}

/**
 * Scroll info for the scrollbar of an axis (@see UIScrollbar.h)
 *
 * @return false if the panel doesn't have a scrollbar in this axis
 */
inline
bool ui_panel_scroll_info(const UIPanel* const panel, UIScrollAxis axis, UIScrollInfo* const info) NO_EXCEPT
{
    if (!(panel->panel_flags & UI_PANEL_FLAG_SCROLL)) {
        return false;
    }

    bool has_x, has_y;
    const v4_f32 viewport = ui_panel_viewport(panel, &has_x, &has_y);

    if (axis == UI_SCROLL_AXIS_Y) {
        if (!has_y) {
            return false;
        }

        info->track = {viewport.x + viewport.width, viewport.y, UI_SCROLLBAR_SIZE, viewport.height};
        info->view = viewport.height;
        info->content = panel->content_size.y;
        info->scroll = panel->scroll.y;
    } else {
        if (!has_x) {
            return false;
        }

        info->track = {viewport.x, viewport.y - viewport.height, viewport.width, UI_SCROLLBAR_SIZE};
        info->view = viewport.width;
        info->content = panel->content_size.x;
        info->scroll = panel->scroll.x;
    }

    return true;
}

/**
 * @return true if the scroll offset changed
 */
inline
bool ui_panel_scroll_set(UILayout* const layout, UIPanel* const panel, UIScrollAxis axis, f32 scroll) NO_EXCEPT
{
    const v4_f32 viewport = ui_panel_viewport(panel);

    f32* const value = axis == UI_SCROLL_AXIS_Y ? &panel->scroll.y : &panel->scroll.x;
    const f32 clamped = axis == UI_SCROLL_AXIS_Y
        ? ui_scroll_clamp(scroll, panel->content_size.y, viewport.height)
        : ui_scroll_clamp(scroll, panel->content_size.x, viewport.width);

    if (clamped == *value) {
        return false;
    }

    *value = clamped;
    panel->core.change_type |= UI_ELEMENT_CHANGE_CONTENT;
    ui_layout_mark_dirty(layout);

    return true;
}

/**
 * Size of the content of a scroll panel = extent of the visible children of the content panel
 * The children MUST already be calculated (@see ui_cache, called after the subtree was cached)
 */
inline
v2_f32 ui_panel_content_extent(const UILayout* const layout, const UIPanel* const panel) NO_EXCEPT
{
    if (!panel->content) {
        return {0.0f, 0.0f};
    }

    const UICore* const content = ui_element_get(layout, panel->content);
    const v2_f32 origin = content->dimension.pos;

    v2_f32 extent = {0.0f, 0.0f};

    UICore* child;
    ui_element_children_iterate_start(layout, content, child) {
        if (child->state & UI_STATE_HIDDEN) {
            continue;
        }

        const UIAttributeDimension* const dim = &child->dimension;
        extent.x = max_branched(extent.x, dim->pos.x + dim->dim.x - origin.x);
        extent.y = max_branched(extent.y, origin.y - (dim->pos.y - dim->dim.y));
    } ui_element_children_iterate_end;

    return extent;
}

////////////////////////////////////////////////////////////////
// Popup
////////////////////////////////////////////////////////////////
// Popups are panels that are children of the root and hidden until they are opened.
// When opened they are moved to the front -> they are drawn on top and the hit test finds them first.
// Only one popup is open at a time (UIInteraction::popup)

/**
 * @param owner Element the popup belongs to and is placed below (NULL = placed at the position ui_popup_open() gets)
 */
inline
UIPanel* ui_popup_create(UILayout* const layout, UICore* const owner = NULL, uint32 background_color = UI_DEFAULT_LIST_BACKGROUND) NO_EXCEPT
{
    // Popups are always children of the root, they are positioned in screen space
    UIPanel* const popup = ui_panel_create(layout, NULL, background_color);
    popup->panel_flags = UI_PANEL_FLAG_POPUP;
    popup->popup_owner = owner ? ui_element_offset(layout, owner) : 0;
    popup->border.color = UI_DEFAULT_CONTROL_BORDER_FOCUS;
    popup->border.width = 1;

    // Clicking inside of a popup "focuses" it -> it stays in front
    popup->core.flags = UI_ELEMENT_FLAG_HITBOX;
    popup->core.state = UI_STATE_HIDDEN;

    return popup;
}

FORCE_INLINE
UIPanel* ui_popup_get(const UILayout* const layout) NO_EXCEPT
{
    return layout->interaction.popup ? (UIPanel *) ui_element_get(layout, layout->interaction.popup) : NULL;
}

FORCE_INLINE
bool ui_popup_is_open(const UILayout* const layout, const UIPanel* const popup) NO_EXCEPT
{
    return popup && layout->interaction.popup == ui_element_offset(layout, popup);
}

// Closes the open popup (if any)
inline
void ui_popup_close(UILayout* const layout) NO_EXCEPT
{
    UIInteraction* const interaction = &layout->interaction;
    if (!interaction->popup) {
        return;
    }

    UIPanel* const popup = (UIPanel *) ui_element_get(layout, interaction->popup);
    popup->core.state |= UI_STATE_HIDDEN;
    popup->core.change_type |= UI_ELEMENT_CHANGE_CONTENT;

    // A text input in the popup (e.g. search field) can't receive keyboard input anymore
    if (interaction->editing
        && ui_element_is_self_or_child_of(layout, ui_element_get(layout, interaction->editing), &popup->core)
    ) {
        ui_element_get(layout, interaction->editing)->state &= ~UI_STATE_FOCUS;
        interaction->editing = 0;
    }

    interaction->popup = 0;
    ui_layout_mark_dirty(layout);
}

/**
 * Opens a popup (closes the currently open one)
 *
 * @param x, y Position of the top left corner in ui coordinates (only used if the popup has no owner)
 */
inline
void ui_popup_open(UILayout* const layout, UIPanel* const popup, f32 x = 0.0f, f32 y = 0.0f) NO_EXCEPT
{
    const int32 offset = ui_element_offset(layout, popup);
    if (layout->interaction.popup != offset) {
        ui_popup_close(layout);
    }

    if (!popup->popup_owner && layout->ui_root) {
        // pos_raw is the offset from the top left corner of the root (y is positive downwards)
        popup->core.dimension.pos_raw = {x, layout->ui_root->dimension.dim.y - y};
    }

    popup->core.state &= ~UI_STATE_HIDDEN;
    popup->core.change_type |= UI_ELEMENT_CHANGE_CONTENT;
    ui_element_move_to_front(layout, &popup->core);

    layout->interaction.popup = offset;
    ui_layout_mark_dirty(layout);
}

/**
 * Calculates the popup position
 * Popups with an owner are placed below the owner (or above if there is not enough space below)
 *
 * @param size Size of the popup
 */
inline
void ui_popup_place(const UILayout* const layout, UIPanel* const popup, v2_f32 size) NO_EXCEPT
{
    UIAttributeDimension* const dim = &popup->core.dimension;
    const UIAttributeDimension* const root = &layout->ui_root->dimension;

    dim->dim = size;

    if (popup->popup_owner) {
        const UIAttributeDimension* const owner = &ui_element_get(layout, popup->popup_owner)->dimension;

        dim->pos.x = owner->pos.x;
        dim->pos.y = owner->pos.y - owner->dim.y;

        // Not enough space below -> above the owner
        if (dim->pos.y - size.y < root->pos.y - root->dim.y && owner->pos.y + size.y <= root->pos.y) {
            dim->pos.y = owner->pos.y + size.y;
        }
    } else {
        dim->pos.x = root->pos.x + dim->pos_raw.x;
        dim->pos.y = root->pos.y - dim->pos_raw.y;

        // Keep it inside of the screen
        if (dim->pos.y - size.y < root->pos.y - root->dim.y) {
            dim->pos.y = root->pos.y - root->dim.y + size.y;
        }
    }

    if (dim->pos.x + size.x > root->pos.x + root->dim.x) {
        dim->pos.x = max_branched(root->pos.x + root->dim.x - size.x, root->pos.x);
    }
}

////////////////////////////////////////////////////////////////
// Rendering
////////////////////////////////////////////////////////////////

void ui_panel_cache(
    UIPanel* const panel,
    GpuApiType gpu_api_type,
    UILayout* const layout,
    f32* const zindex
) NO_EXCEPT
{
    PROFILE_DEBUG(PROFILE_UI_CACHE_PANEL);

    if (panel->panel_flags & UI_PANEL_FLAG_SCROLL_CONTENT) {
        // The content of a scroll panel fills the viewport and is moved by the scroll offset
        const UIPanel* const scroll_panel = (const UIPanel *) ui_element_get(layout, panel->core.parent_offset);
        const v4_f32 viewport = ui_panel_viewport(scroll_panel);

        panel->core.dimension.pos = {viewport.x - scroll_panel->scroll.x, viewport.y + scroll_panel->scroll.y};
        panel->core.dimension.dim = {viewport.width, viewport.height};
    } else if (!(panel->panel_flags & UI_PANEL_FLAG_POPUP)) {
        // Popups are placed before (@see ui_popup_place)
        ui_dimension_calculate(layout, &panel->core);
    }

    const v4_f32 rect = ui_dimension_rect(&panel->core.dimension);

    if (OMS_HAS_ALPHA(panel->background_color)) {
        vertex_rect_create(
            &layout->ui_vertex_cache, &layout->ui_index_cache, *zindex, 0,
            rect,
            UI_ALIGN_V_TOP | UI_ALIGN_H_LEFT,
            panel->background_color
        );

        *zindex = camera_step_closer(gpu_api_type, *zindex);
    }

    if (ui_border_is_visible(&panel->border)) {
        ui_border_vertices(&panel->border, rect, &layout->ui_vertex_cache, &layout->ui_index_cache, *zindex);
        *zindex = camera_step_closer(gpu_api_type, *zindex);
    }
}

/**
 * Called by ui_cache() after the subtree of a scroll panel was cached
 *      1. updates the content size (the children are calculated now)
 *      2. draws the scrollbars on top of the content
 *
 * @return true if the content size changed (-> the ui needs another ui_cache() since the scrollbars may change)
 */
bool ui_panel_scroll_finish(
    UIPanel* const panel,
    GpuApiType gpu_api_type,
    UILayout* const layout,
    f32* const zindex,
    bool scrollbar_active_x,
    bool scrollbar_active_y
) NO_EXCEPT
{
    PROFILE_DEBUG(PROFILE_UI_CACHE_SCROLLBAR);

    const v2_f32 content_size = ui_panel_content_extent(layout, panel);
    const bool changed = content_size.x != panel->content_size.x || content_size.y != panel->content_size.y;
    panel->content_size = content_size;

    // The content may have become smaller (e.g. resized window) -> keep the scroll offset valid
    const v4_f32 viewport = ui_panel_viewport(panel);
    const v2_f32 scroll = {
        ui_scroll_clamp(panel->scroll.x, content_size.x, viewport.width),
        ui_scroll_clamp(panel->scroll.y, content_size.y, viewport.height)
    };
    const bool scroll_changed = scroll.x != panel->scroll.x || scroll.y != panel->scroll.y;
    panel->scroll = scroll;

    UIScrollInfo info;

    if (ui_panel_scroll_info(panel, UI_SCROLL_AXIS_Y, &info)) {
        ui_scrollbar_vertices(&info, UI_SCROLL_AXIS_Y, scrollbar_active_y, gpu_api_type, &layout->ui_vertex_cache, &layout->ui_index_cache, zindex);
    }

    if (ui_panel_scroll_info(panel, UI_SCROLL_AXIS_X, &info)) {
        ui_scrollbar_vertices(&info, UI_SCROLL_AXIS_X, scrollbar_active_x, gpu_api_type, &layout->ui_vertex_cache, &layout->ui_index_cache, zindex);
    }

    return changed || scroll_changed;
}

#endif
