#pragma once
#ifndef COMS_UI_TAB_C
#define COMS_UI_TAB_C

#include "../stdlib/Stdlib.h"
#include "../gpuapi/RenderUtils.h"
#include "../camera/Camera.cpp"
#include "attribute/UIAttributeDimension.cpp"
#include "attribute/UIAttributeBorder.cpp"
#include "UIAlignment.h"
#include "UIStyleDefaults.h"
#include "UIClip.h"
#include "UICore.cpp"
#include "UILabel.cpp"
#include "UIPanel.cpp"
#include "UITab.h"

#define UI_TAB_DEFAULT_VERTICAL_HEADER_WIDTH 110.0f
#define UI_TAB_MIN_WIDTH 60.0f
#define UI_TAB_TEXT_PADDING 12.0f

inline
const char* ui_tab_title(const UILayout* const layout, const UITab* const tab, int32 index) NO_EXCEPT
{
    if (index < 0 || index >= tab->tab_count || !tab->titles) {
        return NULL;
    }

    const char* title = (const char *) (layout->ui_element_buffer.memory + tab->titles);
    while (index-- > 0) {
        title += strlen(title) + 1;
    }

    return title;
}

// Page of a tab (the n-th child), add your elements to the page
inline
UIPanel* ui_tab_page(const UILayout* const layout, const UITab* const tab, int32 index) NO_EXCEPT
{
    UICore* child;
    ui_element_children_iterate_start(layout, &tab->core, child) {
        if (index-- == 0) {
            return (UIPanel *) child;
        }
    } ui_element_children_iterate_end;

    return NULL;
}

/**
 * Shows the page of the tab and hides all other pages
 *
 * @return true if the active tab changed
 */
inline
bool ui_tab_set_active(UILayout* const layout, UITab* const tab, int32 index) NO_EXCEPT
{
    if (index < 0 || index >= tab->tab_count) {
        return false;
    }

    const bool changed = tab->active != index;
    tab->active = (int16) index;

    int32 i = 0;
    UICore* child;
    ui_element_children_iterate_start(layout, &tab->core, child) {
        if (i++ == index) {
            child->state &= ~UI_STATE_HIDDEN;
        } else {
            child->state |= UI_STATE_HIDDEN;
        }

        child->change_type |= UI_ELEMENT_CHANGE_CONTENT;
    } ui_element_children_iterate_end;

    tab->core.change_type |= UI_ELEMENT_CHANGE_CONTENT;
    ui_layout_mark_dirty(layout);

    return changed;
}

/**
 * Creates the tab view and one page per title
 *
 * @param header_size Horizontal: header height, vertical: header width (0 = default)
 * @param parent NULL = root
 */
inline
UITab* ui_tab_create(
    UILayout* const layout,
    const char* const* titles, int32 tab_count,
    UITabOrientation orientation = UI_TAB_ORIENTATION_HORIZONTAL,
    UICore* parent = NULL,
    f32 header_size = 0.0f
) NO_EXCEPT
{
    UITab* const tab = UI_ELEMENT_ALLOC(layout, UI_ELEMENT_TYPE_VIEW_TAB, UITab, parent);
    tab->core.flags = UI_ELEMENT_FLAG_INTERACTABLE | UI_ELEMENT_FLAG_HOVERABLE;
    tab->core.dimension.align_anchor = UI_ANCHOR_ALIGN_TOP_LEFT;

    tab->header_color = UI_DEFAULT_HEADER_BACKGROUND;
    tab->tab_color = UI_DEFAULT_HEADER_BACKGROUND;
    tab->tab_color_hover = 0x444444FF;
    tab->tab_color_active = UI_DEFAULT_CONTROL_HOVER;
    tab->page_color = UI_DEFAULT_CONTROL_BACKGROUND;
    tab->border_color = UI_DEFAULT_CONTROL_BORDER;
    tab->font.size = UI_DEFAULT_FONT_SIZE;
    tab->font.color = UI_DEFAULT_TEXT_COLOR;
    tab->orientation = orientation;
    tab->hovered = -1;

    tab->header_size = header_size > 0.0f
        ? header_size
        : (orientation == UI_TAB_ORIENTATION_HORIZONTAL ? UI_DEFAULT_ROW_HEIGHT + 2.0f : UI_TAB_DEFAULT_VERTICAL_HEADER_WIDTH);

    tab->tab_size = orientation == UI_TAB_ORIENTATION_HORIZONTAL ? UI_TAB_MIN_WIDTH : UI_DEFAULT_ROW_HEIGHT + 2.0f;

    // Titles are stored directly after the tab view: "title 1\0title 2\0..."
    int32 size = 0;
    for (int32 i = 0; i < tab_count; ++i) {
        size += (int32) strlen(titles[i]) + 1;
    }

    if (size) {
        tab->titles = ui_text_reserve(layout, CHAR_TYPE_CHAR, size);
        char* pos = (char *) (layout->ui_element_buffer.memory + tab->titles);
        for (int32 i = 0; i < tab_count; ++i) {
            const int32 length = (int32) strlen(titles[i]) + 1;
            memcpy(pos, titles[i], length);
            pos += length;
        }
    }

    tab->tab_count = (int16) tab_count;

    // Pages fill the area next to the header (negative size = parent size minus the header)
    for (int32 i = 0; i < tab_count; ++i) {
        UIPanel* const page = ui_panel_create(layout, &tab->core);
        if (orientation == UI_TAB_ORIENTATION_HORIZONTAL) {
            page->core.dimension.pos_raw = {0.0f, tab->header_size};
            page->core.dimension.flag = UI_DIMENSION_DIM_X_RELATIVE;
            page->core.dimension.dim_raw = {1.0f, -tab->header_size};
        } else {
            page->core.dimension.pos_raw = {tab->header_size, 0.0f};
            page->core.dimension.flag = UI_DIMENSION_DIM_Y_RELATIVE;
            page->core.dimension.dim_raw = {-tab->header_size, 1.0f};
        }
    }

    ui_tab_set_active(layout, tab, 0);

    return tab;
}

// Rect of a tab header (x, y = top left)
inline
v4_f32 ui_tab_header_rect(const UITab* const tab, int32 index) NO_EXCEPT
{
    const UIAttributeDimension* const dim = &tab->core.dimension;

    return tab->orientation == UI_TAB_ORIENTATION_HORIZONTAL
        ? v4_f32{dim->pos.x + tab->tab_size * (f32) index, dim->pos.y, tab->tab_size, tab->header_size}
        : v4_f32{dim->pos.x, dim->pos.y - tab->tab_size * (f32) index, tab->header_size, tab->tab_size};
}

/**
 * @return Tab header at the position or -1
 */
inline
int32 ui_tab_at(const UITab* const tab, f32 x, f32 y) NO_EXCEPT
{
    for (int32 i = 0; i < tab->tab_count; ++i) {
        const v4_f32 rect = ui_tab_header_rect(tab, i);
        if (x >= rect.x && x < rect.x + rect.width && y <= rect.y && y > rect.y - rect.height) {
            return i;
        }
    }

    return -1;
}

inline
bool ui_tab_hover(UILayout* const layout, UITab* const tab, int32 index) NO_EXCEPT
{
    if (index == tab->hovered) {
        return false;
    }

    tab->hovered = (int16) index;
    tab->core.change_type |= UI_ELEMENT_CHANGE_CONTENT;
    ui_layout_mark_dirty(layout);

    return true;
}

/**
 * Renders the header row/column and the page background
 * The pages are children and rendered by the tree traversal
 */
void ui_tab_cache(
    UITab* const tab,
    GpuApiType gpu_api_type,
    UILayout* const layout,
    f32* const zindex,
    BufferMemory* const __restrict mem
) NO_EXCEPT
{
    PROFILE_DEBUG(PROFILE_UI_CACHE_TAB);

    ui_dimension_calculate(layout, &tab->core);
    const UIAttributeDimension* const dim = &tab->core.dimension;

    ArrayVector<Vertex3DSamplerTextureColor>* const vertices = &layout->ui_vertex_cache;
    ArrayVector<int32>* const indices = &layout->ui_index_cache;

    const bool horizontal = tab->orientation == UI_TAB_ORIENTATION_HORIZONTAL;

    // Horizontal tabs: every tab has the width of the longest title
    if (horizontal && layout->font) {
        f32 max_width = 0.0f;
        for (int32 i = 0; i < tab->tab_count; ++i) {
            const char* const title = ui_tab_title(layout, tab, i);
            max_width = max_branched(max_width, ui_text_width(layout->font, title, (int32) strlen(title), tab->font.size));
        }

        tab->tab_size = max_branched(max_width + 2.0f * UI_TAB_TEXT_PADDING, UI_TAB_MIN_WIDTH);
    }

    // Header + page background
    const v4_f32 header = horizontal
        ? v4_f32{dim->pos.x, dim->pos.y, dim->dim.x, tab->header_size}
        : v4_f32{dim->pos.x, dim->pos.y, tab->header_size, dim->dim.y};

    const v4_f32 page = horizontal
        ? v4_f32{dim->pos.x, dim->pos.y - tab->header_size, dim->dim.x, dim->dim.y - tab->header_size}
        : v4_f32{dim->pos.x + tab->header_size, dim->pos.y, dim->dim.x - tab->header_size, dim->dim.y};

    ui_rect_vertices(header, tab->header_color, vertices, indices, *zindex);
    ui_rect_vertices(page, tab->page_color, vertices, indices, *zindex);
    *zindex = camera_step_closer(gpu_api_type, *zindex);

    // Tab headers (clipped to the header area, e.g. too many tabs)
    const int32 vertex_start = vertices->count;
    const int32 index_start = indices->count;

    const f32 line_height = layout->font ? ui_text_line_height(layout->font, tab->font.size) : 0.0f;

    for (int32 i = 0; i < tab->tab_count; ++i) {
        const v4_f32 rect = ui_tab_header_rect(tab, i);

        const uint32 color = i == tab->active
            ? tab->tab_color_active
            : (i == tab->hovered ? tab->tab_color_hover : tab->tab_color);

        // 1px gap between the tabs
        ui_rect_vertices(
            horizontal
                ? v4_f32{rect.x, rect.y, rect.width - 1.0f, rect.height}
                : v4_f32{rect.x, rect.y, rect.width, rect.height - 1.0f},
            color, vertices, indices, *zindex
        );
    }

    // WARNING: The titles are on top of the tab backgrounds -> own zindex (the depth test rejects equal depth values)
    *zindex = camera_step_closer(gpu_api_type, *zindex);

    if (layout->font) {
        for (int32 i = 0; i < tab->tab_count; ++i) {
            const v4_f32 rect = ui_tab_header_rect(tab, i);
            ui_text_vertices(
                layout, ui_tab_title(layout, tab, i), tab->font.size, tab->font.color,
                rect.x + UI_TAB_TEXT_PADDING, rect.y - (rect.height - line_height) * 0.5f,
                *zindex, mem
            );
        }
    }

    ui_vertices_clip_or_remove(vertices, indices, vertex_start, index_start, ui_clip_from_rect(header));

    ui_border_solid_vertices(page, tab->border_color, 1, vertices, indices, *zindex);
    *zindex = camera_step_closer(gpu_api_type, *zindex);
}

#endif
