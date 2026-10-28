#pragma once
#ifndef COMS_UI_SELECT_C
#define COMS_UI_SELECT_C

#include "../stdlib/Stdlib.h"
#include "../gpuapi/RenderUtils.h"
#include "../camera/Camera.cpp"
#include "attribute/UIAttributeDimension.cpp"
#include "attribute/UIAttributeBorder.cpp"
#include "UIAlignment.h"
#include "UIStyleDefaults.h"
#include "UIScrollbar.h"
#include "UIClip.h"
#include "UICore.cpp"
#include "UILabel.cpp"
#include "UIPanel.cpp"
#include "UIInput.cpp"
#include "UISelect.h"

#define UI_SELECT_CHECK_SIZE 12.0f

/**
 * @param options Option texts (copied into the ui_element_buffer)
 * @param flags @see UISelectFlag
 * @param parent NULL = root
 */
inline
UISelect* ui_select_create(
    UILayout* const layout,
    const char* const* options, int32 option_count,
    UICore* parent = NULL,
    byte flags = 0
) NO_EXCEPT
{
    UISelect* const select = UI_ELEMENT_ALLOC(layout, UI_ELEMENT_TYPE_SELECT, UISelect, parent);
    select->core.flags = UI_ELEMENT_FLAG_INTERACTABLE | UI_ELEMENT_FLAG_HOVERABLE;
    select->core.dimension.align_anchor = UI_ANCHOR_ALIGN_TOP_LEFT;
    select->core.dimension.dim_raw = {150.0f, UI_DEFAULT_ROW_HEIGHT};

    select->background_color = UI_DEFAULT_CONTROL_BACKGROUND;
    select->border_color = UI_DEFAULT_CONTROL_BORDER;
    select->border_color_hover = UI_DEFAULT_CONTROL_BORDER_FOCUS;
    select->option_hover_color = UI_DEFAULT_CONTROL_HOVER;
    select->check_color = UI_DEFAULT_CONTROL_ACCENT;
    select->font.size = UI_DEFAULT_FONT_SIZE;
    select->font.color = UI_DEFAULT_TEXT_COLOR;
    select->select_flags = flags;
    select->selected = (option_count > 0 && !(flags & UI_SELECT_FLAG_MULTI)) ? 0 : -1;

    // Options are stored directly after the select: "option 1\0option 2\0..."
    int32 size = 0;
    for (int32 i = 0; i < option_count; ++i) {
        size += (int32) strlen(options[i]) + 1;
    }

    if (size) {
        select->options = ui_text_reserve(layout, CHAR_TYPE_CHAR, size);
        char* pos = (char *) (layout->ui_element_buffer.memory + select->options);
        for (int32 i = 0; i < option_count; ++i) {
            const int32 length = (int32) strlen(options[i]) + 1;
            memcpy(pos, options[i], length);
            pos += length;
        }
    }

    select->option_count = (int16) option_count;

    if ((flags & UI_SELECT_FLAG_MULTI) && option_count > 0) {
        // Zero initialized bit set
        select->multi_selected = ui_text_reserve(layout, CHAR_TYPE_CHAR, (option_count + 7) / 8);
    }

    // Popup with the (optional) search field and the option list
    UIPanel* const popup = ui_popup_create(layout, &select->core);
    select->popup = ui_element_offset(layout, popup);

    f32 list_y = 0.0f;
    if (flags & UI_SELECT_FLAG_SEARCH) {
        UIInput* const search = ui_input_create(layout, 64, &popup->core);
        search->core.dimension.flag = UI_DIMENSION_DIM_X_RELATIVE;
        search->core.dimension.dim_raw = {1.0f, UI_DEFAULT_ROW_HEIGHT};
        search->owner = ui_element_offset(layout, select);
        ui_input_set_placeholder(layout, search, "Search...");

        select->search = ui_element_offset(layout, search);
        list_y = UI_DEFAULT_ROW_HEIGHT;
    }

    UIOptionList* const list = UI_ELEMENT_ALLOC(layout, UI_ELEMENT_TYPE_OPTION_LIST, UIOptionList, &popup->core);
    list->core.flags = UI_ELEMENT_FLAG_INTERACTABLE | UI_ELEMENT_FLAG_HOVERABLE;
    list->core.dimension.align_anchor = UI_ANCHOR_ALIGN_TOP_LEFT;
    list->core.dimension.flag = UI_DIMENSION_DIM_X_RELATIVE;
    list->core.dimension.pos_raw = {0.0f, list_y};
    list->core.dimension.dim_raw = {1.0f, 0.0f};
    list->select = ui_element_offset(layout, select);
    list->hovered = -1;
    select->list = ui_element_offset(layout, list);

    return select;
}

inline
const char* ui_select_option(const UILayout* const layout, const UISelect* const select, int32 index) NO_EXCEPT
{
    if (index < 0 || index >= select->option_count || !select->options) {
        return NULL;
    }

    const char* option = (const char *) (layout->ui_element_buffer.memory + select->options);
    while (index-- > 0) {
        option += strlen(option) + 1;
    }

    return option;
}

FORCE_INLINE
f32 ui_select_row_height(const UISelect* const select) NO_EXCEPT
{
    return select->core.dimension.dim.y > 0.0f ? select->core.dimension.dim.y : UI_DEFAULT_ROW_HEIGHT;
}

// Search text ("" = no search)
inline
const char* ui_select_search_text(const UILayout* const layout, const UISelect* const select) NO_EXCEPT
{
    return select->search
        ? ui_input_get_text(layout, (const UIInput *) ui_element_get(layout, select->search))
        : "";
}

// Case insensitive (ascii) "contains"
static inline
bool ui_text_contains_case_insensitive(const char* text, const char* const search) NO_EXCEPT
{
    if (!*search) {
        return true;
    }

    for (; *text; ++text) {
        const char* a = text;
        const char* b = search;

        while (*a && *b) {
            const char ca = (*a >= 'A' && *a <= 'Z') ? (char) (*a + 32) : *a;
            const char cb = (*b >= 'A' && *b <= 'Z') ? (char) (*b + 32) : *b;
            if (ca != cb) {
                break;
            }

            ++a;
            ++b;
        }

        if (!*b) {
            return true;
        }
    }

    return false;
}

/**
 * Index of the option in the n-th row of the filtered list
 *
 * @return option index or -1
 */
inline
int32 ui_select_filtered_option(const UILayout* const layout, const UISelect* const select, int32 row) NO_EXCEPT
{
    if (row < 0 || !select->options) {
        return -1;
    }

    const char* const search = ui_select_search_text(layout, select);
    const char* option = (const char *) (layout->ui_element_buffer.memory + select->options);

    for (int32 i = 0; i < select->option_count; ++i) {
        if (ui_text_contains_case_insensitive(option, search) && row-- == 0) {
            return i;
        }

        option += strlen(option) + 1;
    }

    return -1;
}

inline
int32 ui_select_filtered_count(const UILayout* const layout, const UISelect* const select) NO_EXCEPT
{
    if (!select->options) {
        return 0;
    }

    const char* const search = ui_select_search_text(layout, select);
    if (!*search) {
        return select->option_count;
    }

    const char* option = (const char *) (layout->ui_element_buffer.memory + select->options);
    int32 count = 0;

    for (int32 i = 0; i < select->option_count; ++i) {
        count += ui_text_contains_case_insensitive(option, search);
        option += strlen(option) + 1;
    }

    return count;
}

inline
bool ui_select_is_checked(const UILayout* const layout, const UISelect* const select, int32 index) NO_EXCEPT
{
    if (index < 0 || index >= select->option_count) {
        return false;
    }

    if (select->select_flags & UI_SELECT_FLAG_MULTI) {
        const byte* const bits = layout->ui_element_buffer.memory + select->multi_selected;

        return select->multi_selected && (bits[index / 8] & (1 << (index % 8)));
    }

    return select->selected == index;
}

inline
int32 ui_select_checked_count(const UILayout* const layout, const UISelect* const select) NO_EXCEPT
{
    if (!(select->select_flags & UI_SELECT_FLAG_MULTI)) {
        return select->selected >= 0 ? 1 : 0;
    }

    int32 count = 0;
    for (int32 i = 0; i < select->option_count; ++i) {
        count += ui_select_is_checked(layout, select, i);
    }

    return count;
}

/**
 * Single selection: selects the option
 * Multi selection: sets the checked state of the option
 *
 * @return true if the selection changed
 */
inline
bool ui_select_set(UILayout* const layout, UISelect* const select, int32 index, bool checked = true) NO_EXCEPT
{
    if (index < -1 || index >= select->option_count) {
        return false;
    }

    if (select->select_flags & UI_SELECT_FLAG_MULTI) {
        if (index < 0 || !select->multi_selected || ui_select_is_checked(layout, select, index) == checked) {
            return false;
        }

        byte* const bits = layout->ui_element_buffer.memory + select->multi_selected;
        if (checked) {
            bits[index / 8] |= (byte) (1 << (index % 8));
        } else {
            bits[index / 8] &= (byte) ~(1 << (index % 8));
        }
    } else {
        if (index == select->selected) {
            return false;
        }

        select->selected = (int16) index;
    }

    select->core.change_type |= UI_ELEMENT_CHANGE_CONTENT;
    ui_layout_mark_dirty(layout);

    return true;
}

/**
 * Text shown in the select box
 *      single: the selected option
 *      multi:  "None", the checked option or "first checked option +n"
 */
inline
const char* ui_select_display_text(const UILayout* const layout, const UISelect* const select, char* const buffer, int32 buffer_size) NO_EXCEPT
{
    if (!(select->select_flags & UI_SELECT_FLAG_MULTI)) {
        return ui_select_option(layout, select, select->selected);
    }

    int32 first = -1;
    int32 count = 0;
    for (int32 i = 0; i < select->option_count; ++i) {
        if (ui_select_is_checked(layout, select, i)) {
            first = first < 0 ? i : first;
            ++count;
        }
    }

    if (!count) {
        return "None";
    }

    const char* const option = ui_select_option(layout, select, first);
    if (count == 1) {
        return option;
    }

    // "Option +3"
    char number[12];
    const int32 number_length = int_to_str((int64) (count - 1), number);

    const int32 option_length = min_branched((int32) strlen(option), buffer_size - number_length - 3);
    memcpy(buffer, option, option_length);
    buffer[option_length] = ' ';
    buffer[option_length + 1] = '+';
    memcpy(buffer + option_length + 2, number, number_length);
    buffer[option_length + 2 + number_length] = '\0';

    return buffer;
}

/**
 * Size of the popup (search field + visible rows)
 */
inline
v2_f32 ui_select_popup_size(const UILayout* const layout, const UISelect* const select) NO_EXCEPT
{
    const f32 row_height = ui_select_row_height(select);
    const int32 rows = clamp_branched(ui_select_filtered_count(layout, select), 1, UI_SELECT_MAX_VISIBLE_OPTIONS);

    return {
        select->core.dimension.dim.x,
        (select->search ? UI_DEFAULT_ROW_HEIGHT : 0.0f) + row_height * (f32) rows
    };
}

////////////////////////////////////////////////////////////////
// Option list
////////////////////////////////////////////////////////////////

FORCE_INLINE
UISelect* ui_option_list_select(const UILayout* const layout, const UIOptionList* const list) NO_EXCEPT
{
    return (UISelect *) ui_element_get(layout, list->select);
}

/**
 * @return false if the list doesn't need a scrollbar
 */
inline
bool ui_option_list_scroll_info(const UILayout* const layout, const UIOptionList* const list, UIScrollInfo* const info) NO_EXCEPT
{
    const UISelect* const select = ui_option_list_select(layout, list);
    const UIAttributeDimension* const dim = &list->core.dimension;

    const f32 content = ui_select_row_height(select) * (f32) ui_select_filtered_count(layout, select);
    if (content <= dim->dim.y) {
        return false;
    }

    info->track = {dim->pos.x + dim->dim.x - UI_SCROLLBAR_SIZE, dim->pos.y, UI_SCROLLBAR_SIZE, dim->dim.y};
    info->view = dim->dim.y;
    info->content = content;
    info->scroll = list->scroll;

    return true;
}

inline
bool ui_option_list_scroll_set(UILayout* const layout, UIOptionList* const list, f32 scroll) NO_EXCEPT
{
    const UISelect* const select = ui_option_list_select(layout, list);
    const f32 content = ui_select_row_height(select) * (f32) ui_select_filtered_count(layout, select);
    scroll = ui_scroll_clamp(scroll, content, list->core.dimension.dim.y);

    if (scroll == list->scroll) {
        return false;
    }

    list->scroll = scroll;
    list->core.change_type |= UI_ELEMENT_CHANGE_CONTENT;
    ui_layout_mark_dirty(layout);

    return true;
}

/**
 * @return Row (index in the filtered list) at the position or -1
 */
inline
int32 ui_option_list_row_at(const UILayout* const layout, const UIOptionList* const list, f32 x, f32 y) NO_EXCEPT
{
    if (!ui_dimension_contains(&list->core.dimension, x, y)) {
        return -1;
    }

    const UISelect* const select = ui_option_list_select(layout, list);
    const int32 row = (int32) ((list->core.dimension.pos.y - y + list->scroll) / ui_select_row_height(select));

    return row < ui_select_filtered_count(layout, select) ? row : -1;
}

inline
bool ui_option_list_hover(UILayout* const layout, UIOptionList* const list, int32 row) NO_EXCEPT
{
    if (row == list->hovered) {
        return false;
    }

    list->hovered = (int16) row;
    list->core.change_type |= UI_ELEMENT_CHANGE_CONTENT;
    ui_layout_mark_dirty(layout);

    return true;
}

/**
 * Moves the hovered row by delta (keyboard navigation) and scrolls it into view
 *
 * @return true if it changed
 */
inline
bool ui_option_list_move_hover(UILayout* const layout, UIOptionList* const list, int32 delta) NO_EXCEPT
{
    const UISelect* const select = ui_option_list_select(layout, list);
    const int32 count = ui_select_filtered_count(layout, select);
    if (!count) {
        return false;
    }

    const int32 row = list->hovered < 0
        ? (delta > 0 ? 0 : count - 1)
        : clamp_branched(list->hovered + delta, 0, count - 1);

    // Visible area of the list (the dimension is from the last ui_cache())
    const f32 row_height = ui_select_row_height(select);
    const f32 view = row_height * (f32) clamp_branched(count, 1, UI_SELECT_MAX_VISIBLE_OPTIONS);

    if ((f32) row * row_height < list->scroll) {
        ui_option_list_scroll_set(layout, list, (f32) row * row_height);
    } else if ((f32) (row + 1) * row_height > list->scroll + view) {
        ui_option_list_scroll_set(layout, list, (f32) (row + 1) * row_height - view);
    }

    return ui_option_list_hover(layout, list, row);
}

////////////////////////////////////////////////////////////////
// Rendering
////////////////////////////////////////////////////////////////

void ui_select_cache(
    UISelect* const select,
    GpuApiType gpu_api_type,
    UILayout* const layout,
    f32* const zindex,
    BufferMemory* const __restrict mem
) NO_EXCEPT
{
    PROFILE_DEBUG(PROFILE_UI_CACHE_SELECT);

    ui_dimension_calculate(layout, &select->core);
    const UIAttributeDimension* const dim = &select->core.dimension;
    const v4_f32 rect = ui_dimension_rect(dim);

    ArrayVector<Vertex3DSamplerTextureColor>* const vertices = &layout->ui_vertex_cache;
    ArrayVector<int32>* const indices = &layout->ui_index_cache;

    ui_rect_vertices(rect, select->background_color, vertices, indices, *zindex);
    *zindex = camera_step_closer(gpu_api_type, *zindex);

    const bool is_open = select->popup && layout->interaction.popup == select->popup;
    ui_border_solid_vertices(
        rect,
        (is_open || (select->core.state & UI_STATE_HOVER)) ? select->border_color_hover : select->border_color,
        1,
        vertices, indices, *zindex
    );

    // Arrow on the right side (small triangle made of horizontal lines)
    const f32 arrow_x = dim->pos.x + dim->dim.x - 16.0f;
    const f32 arrow_y = dim->pos.y - dim->dim.y * 0.5f + 2.0f;
    for (int32 i = 0; i < 5; ++i) {
        ui_rect_vertices(
            {arrow_x + (f32) i, arrow_y - (f32) i, 9.0f - 2.0f * (f32) i, 1.0f},
            select->font.color, vertices, indices, *zindex
        );
    }

    *zindex = camera_step_closer(gpu_api_type, *zindex);

    if (layout->font) {
        char buffer[128];
        const char* const text = ui_select_display_text(layout, select, buffer, (int32) sizeof(buffer));
        const f32 line_height = ui_text_line_height(layout->font, select->font.size);

        const int32 vertex_start = vertices->count;
        const int32 index_start = indices->count;

        ui_text_vertices(
            layout, text, select->font.size, select->font.color,
            dim->pos.x + UI_DEFAULT_TEXT_PADDING, dim->pos.y - (dim->dim.y - line_height) * 0.5f,
            *zindex, mem
        );

        // Long texts must not overlap the arrow
        ui_vertices_clip_or_remove(
            vertices, indices, vertex_start, index_start,
            ui_clip_from_rect({dim->pos.x, dim->pos.y, max_branched(dim->dim.x - 20.0f, 0.0f), dim->dim.y})
        );
    }

    *zindex = camera_step_closer(gpu_api_type, *zindex);
}

/**
 * Renders the visible rows of the (filtered) option list
 * Multi selects show a checkbox per option
 */
void ui_option_list_cache(
    UIOptionList* const list,
    GpuApiType gpu_api_type,
    UILayout* const layout,
    f32* const zindex,
    BufferMemory* const __restrict mem
) NO_EXCEPT
{
    PROFILE_DEBUG(PROFILE_UI_CACHE_OPTION_LIST);

    const UISelect* const select = ui_option_list_select(layout, list);
    const f32 row_height = ui_select_row_height(select);
    const int32 count = ui_select_filtered_count(layout, select);
    const int32 visible_rows = clamp_branched(count, 1, UI_SELECT_MAX_VISIBLE_OPTIONS);

    ui_dimension_calculate(layout, &list->core, {0.0f, row_height * (f32) visible_rows});
    const UIAttributeDimension* const dim = &list->core.dimension;

    ArrayVector<Vertex3DSamplerTextureColor>* const vertices = &layout->ui_vertex_cache;
    ArrayVector<int32>* const indices = &layout->ui_index_cache;

    // Same as ui_option_list_scroll_info() without counting the filtered options again
    const f32 content = row_height * (f32) count;
    const bool has_scrollbar = content > dim->dim.y;
    const f32 row_width = dim->dim.x - (has_scrollbar ? UI_SCROLLBAR_SIZE : 0.0f);
    const UIClipRect clip = ui_clip_from_rect({dim->pos.x, dim->pos.y, row_width, dim->dim.y});

    const int32 vertex_start = vertices->count;
    const int32 index_start = indices->count;

    // Only the visible rows are rendered
    const f32 scroll = ui_scroll_clamp(list->scroll, content, dim->dim.y);
    const int32 first_row = (int32) (scroll / row_height);
    const int32 last_row = min_branched(count - 1, (int32) ((scroll + dim->dim.y) / row_height));

    const bool is_multi = (select->select_flags & UI_SELECT_FLAG_MULTI) != 0;
    const f32 line_height = layout->font ? ui_text_line_height(layout->font, select->font.size) : 0.0f;

    // WARNING: Overlapping geometry needs its own zindex (the depth test rejects equal depth values)
    //          -> 3 layers: row background, checkbox background, checkbox border/check + text
    //          The rows don't overlap each other -> all layers of a row are created together
    //          (only one pass over the options instead of searching every row again)
    const f32 z_background = *zindex;
    const f32 z_box = camera_step_closer(gpu_api_type, z_background);
    const f32 z_content = camera_step_closer(gpu_api_type, z_box);
    *zindex = z_content;

    const char* const search = ui_select_search_text(layout, select);
    const char* option_text = select->options
        ? (const char *) (layout->ui_element_buffer.memory + select->options)
        : NULL;

    int32 row = 0;
    for (int32 option = 0; option_text && option < select->option_count && row <= last_row; ++option) {
        const char* const text = option_text;
        option_text += strlen(option_text) + 1;

        if (!ui_text_contains_case_insensitive(text, search) || row++ < first_row) {
            continue;
        }

        const int32 current_row = row - 1;
        const f32 row_y = dim->pos.y + scroll - row_height * (f32) current_row;
        const bool checked = ui_select_is_checked(layout, select, option);

        // Row background
        if (current_row == list->hovered) {
            ui_rect_vertices({dim->pos.x, row_y, row_width, row_height}, select->option_hover_color, vertices, indices, z_background);
        } else if (checked && !is_multi) {
            ui_rect_vertices({dim->pos.x, row_y, row_width, row_height}, UI_DEFAULT_ROW_SELECTED, vertices, indices, z_background);
        }

        f32 text_x = dim->pos.x + UI_DEFAULT_TEXT_PADDING;
        if (is_multi) {
            const v4_f32 box = {
                dim->pos.x + UI_DEFAULT_TEXT_PADDING,
                row_y - (row_height - UI_SELECT_CHECK_SIZE) * 0.5f,
                UI_SELECT_CHECK_SIZE, UI_SELECT_CHECK_SIZE
            };

            ui_rect_vertices(box, select->background_color, vertices, indices, z_box);
            ui_border_solid_vertices(box, checked ? select->check_color : select->border_color, 1, vertices, indices, z_content);

            if (checked) {
                ui_rect_vertices(
                    {box.x + 3.0f, box.y - 3.0f, box.width - 6.0f, box.height - 6.0f},
                    select->check_color, vertices, indices, z_content
                );
            }

            text_x += UI_SELECT_CHECK_SIZE + UI_DEFAULT_TEXT_PADDING;
        }

        if (layout->font) {
            ui_text_vertices(
                layout, text, select->font.size, select->font.color,
                text_x, row_y - (row_height - line_height) * 0.5f,
                z_content, mem
            );
        }
    }

    if (!count && layout->font) {
        ui_text_vertices(
            layout, "No results", select->font.size, UI_DEFAULT_PLACEHOLDER_COLOR,
            dim->pos.x + UI_DEFAULT_TEXT_PADDING, dim->pos.y - (row_height - line_height) * 0.5f,
            *zindex, mem
        );
    }

    // The rows are clipped to the list (partially visible rows while scrolling)
    ui_vertices_clip_or_remove(vertices, indices, vertex_start, index_start, clip);

    *zindex = camera_step_closer(gpu_api_type, *zindex);

    if (has_scrollbar) {
        UIScrollInfo scroll_info;
        scroll_info.track = {dim->pos.x + dim->dim.x - UI_SCROLLBAR_SIZE, dim->pos.y, UI_SCROLLBAR_SIZE, dim->dim.y};
        scroll_info.view = dim->dim.y;
        scroll_info.content = content;
        scroll_info.scroll = scroll;
        ui_scrollbar_vertices(
            &scroll_info, UI_SCROLL_AXIS_Y,
            layout->interaction.scroll_dragged == ui_element_offset(layout, list),
            gpu_api_type, vertices, indices, zindex
        );
    }
}

#endif
