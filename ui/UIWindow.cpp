#pragma once
#ifndef COMS_UI_WINDOW_C
#define COMS_UI_WINDOW_C

#include "../stdlib/Stdlib.h"
#include "../gpuapi/RenderUtils.h"
#include "../camera/Camera.cpp"
#include "attribute/UIAttributeDimension.cpp"
#include "attribute/UIAttributeBorder.cpp"
#include "UIAlignment.h"
#include "UICore.cpp"
#include "UIPanel.cpp"
#include "UILabel.cpp"
#include "UIButton.cpp"
#include "UIWindow.h"

#define UI_WINDOW_DEFAULT_TITLE_HEIGHT 30.0f

/**
 * Creates the window and its fixed children directly after it (pre-order):
 *      window
 *          title (UIPanel, movable)
 *              title label
 *              close button
 *                  close label (empty, a theme can set "text: X" instead of an image)
 *
 * @param parent NULL = root
 */
UIWindow* ui_window_create(UILayout* const layout, UICore* parent = NULL) NO_EXCEPT
{
    UIWindow* const window = UI_ELEMENT_ALLOC(layout, UI_ELEMENT_TYPE_VIEW_WINDOW, UIWindow, parent);

    // Clicking anywhere on the window focuses it
    window->core.flags = UI_ELEMENT_FLAG_HITBOX;
    window->core.dimension.align_anchor = UI_ANCHOR_ALIGN_TOP_LEFT;

    // title
    UIPanel* const title = ui_panel_create(layout, &window->core);
    title->core.flags = UI_ELEMENT_FLAG_MOVABLE;
    title->core.dimension.flag = UI_DIMENSION_DIM_X_RELATIVE;
    title->core.dimension.dim_raw = {1.0f, UI_WINDOW_DEFAULT_TITLE_HEIGHT};
    window->title = ui_element_offset(layout, title);

    // title label
    UILabel* const title_label = ui_label_create(layout, CHAR_TYPE_CHAR, 0, 0, &title->core);
    title_label->core.dimension.align_anchor = UI_ANCHOR_ALIGN_LEFT_CENTER;
    title_label->core.dimension.pos_raw = {8.0f, 0.0f};
    window->title_label = ui_element_offset(layout, title_label);

    // close button (size is defined by its image unless set otherwise)
    // Always has a (empty) label -> the theme can use a text (e.g. "X") instead of an image
    UIButton* const button_close = ui_button_create(layout, "", &title->core);
    button_close->core.dimension.align_anchor = UI_ANCHOR_ALIGN_RIGHT_CENTER;
    button_close->core.dimension.pos_raw = {4.0f, 0.0f};
    window->button_close = ui_element_offset(layout, button_close);

    return window;
}

FORCE_INLINE
UIPanel* ui_window_title(const UILayout* const layout, const UIWindow* const window) NO_EXCEPT
{
    return window->title ? (UIPanel *) ui_element_get(layout, window->title) : NULL;
}

FORCE_INLINE
UILabel* ui_window_title_label(const UILayout* const layout, const UIWindow* const window) NO_EXCEPT
{
    return window->title_label ? (UILabel *) ui_element_get(layout, window->title_label) : NULL;
}

FORCE_INLINE
UIButton* ui_window_button_close(const UILayout* const layout, const UIWindow* const window) NO_EXCEPT
{
    return window->button_close ? (UIButton *) ui_element_get(layout, window->button_close) : NULL;
}

inline
void ui_window_set_title(UILayout* const layout, UIWindow* const window, const char* const text) NO_EXCEPT
{
    UILabel* const label = ui_window_title_label(layout, window);
    if (label) {
        ui_label_set_text(layout, label, text);
    }
}

#define UI_WINDOW_RESIZE_GRIP_SIZE 14.0f

/**
 * Adds a resize grip to the bottom right corner (dragging it resizes the window)
 * The grip stays on top of the window content (UI_ELEMENT_FLAG_ALWAYS_ON_TOP)
 */
inline
UIPanel* ui_window_set_resizable(UILayout* const layout, UIWindow* const window, v2_f32 min_dim = {160.0f, 100.0f}) NO_EXCEPT
{
    window->min_dim = min_dim;

    if (window->resize_grip) {
        return (UIPanel *) ui_element_get(layout, window->resize_grip);
    }

    UIPanel* const grip = UI_ELEMENT_ALLOC(layout, UI_ELEMENT_TYPE_VIEW_PANEL, UIPanel, &window->core);
    grip->core.flags = UI_ELEMENT_FLAG_RESIZABLE | UI_ELEMENT_FLAG_HOVERABLE | UI_ELEMENT_FLAG_ALWAYS_ON_TOP;
    grip->core.dimension.align_anchor = UI_ANCHOR_ALIGN_BOTTOM_RIGHT;
    grip->core.dimension.dim_raw = {UI_WINDOW_RESIZE_GRIP_SIZE, UI_WINDOW_RESIZE_GRIP_SIZE};
    grip->background_color = 0x5A5A5A80;

    // ALWAYS_ON_TOP is only respected when linking -> re-link as last child
    ui_element_move_to_front(layout, &grip->core);

    window->resize_grip = ui_element_offset(layout, grip);

    return grip;
}

/**
 * Only renders the window itself, the children (title, buttons, content) are rendered by the tree traversal
 */
void ui_window_cache(
    UIWindow* const window,
    GpuApiType gpu_api_type,
    UILayout* const layout,
    f32* const zindex
) NO_EXCEPT
{
    PROFILE_DEBUG(PROFILE_UI_CACHE_WINDOW);

    ui_dimension_calculate(layout, &window->core);
    const v4_f32 rect = ui_dimension_rect(&window->core.dimension);

    if (OMS_HAS_ALPHA(window->background_color)) {
        vertex_rect_create(
            &layout->ui_vertex_cache, &layout->ui_index_cache, *zindex, 0,
            rect,
            UI_ALIGN_V_TOP | UI_ALIGN_H_LEFT,
            window->background_color
        );

        *zindex = camera_step_closer(gpu_api_type, *zindex);
    }

    if (ui_border_is_visible(&window->border)) {
        ui_border_vertices(&window->border, rect, &layout->ui_vertex_cache, &layout->ui_index_cache, *zindex);
        *zindex = camera_step_closer(gpu_api_type, *zindex);
    }
}

#endif
