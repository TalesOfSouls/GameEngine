#pragma once
#ifndef COMS_UI_TABLE_C
#define COMS_UI_TABLE_C

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
#include "UITable.h"

// Copies strings into the ui_element_buffer ("a\0b\0c\0")
inline
int32 ui_table_strings_store(UILayout* const layout, const char* const* strings, int32 count) NO_EXCEPT
{
    int32 size = 0;
    for (int32 i = 0; i < count; ++i) {
        size += (strings[i] ? (int32) strlen(strings[i]) : 0) + 1;
    }

    const int32 offset = ui_text_reserve(layout, CHAR_TYPE_CHAR, max_branched(size, 1));
    char* pos = (char *) (layout->ui_element_buffer.memory + offset);
    for (int32 i = 0; i < count; ++i) {
        const int32 length = strings[i] ? (int32) strlen(strings[i]) : 0;
        if (length) {
            memcpy(pos, strings[i], length);
        }

        pos[length] = '\0';
        pos += length + 1;
    }

    return offset;
}

// n-th string of a string block (@see ui_table_strings_store)
inline
const char* ui_table_string(const UILayout* const layout, int32 offset, int32 index) NO_EXCEPT
{
    if (!offset) {
        return "";
    }

    const char* str = (const char *) (layout->ui_element_buffer.memory + offset);
    while (index-- > 0) {
        str += strlen(str) + 1;
    }

    return str;
}

// Cell string of a row block (static: "a\0b\0c\0", dynamic: fixed size cells)
inline
const char* ui_table_row_string(const UILayout* const layout, const UITable* const table, int32 offset, int32 column) NO_EXCEPT
{
    if (!offset) {
        return "";
    }

    if (table->cell_capacity) {
        return (const char *) (layout->ui_element_buffer.memory + offset + column * table->cell_capacity);
    }

    return ui_table_string(layout, offset, column);
}

/**
 * @param titles Header titles (one per column)
 * @param row_capacity Max amount of rows (@see ui_table_add_row)
 * @param parent NULL = root
 * @param cell_capacity > 0 = dynamic table with fixed size cells incl. \0 (@see ui_table_set_cell)
 *                      The memory for all rows (+ footer) is reserved immediately: row_capacity * column_count * cell_capacity
 */
inline
UITable* ui_table_create(
    UILayout* const layout,
    const char* const* titles, int32 column_count,
    int32 row_capacity,
    UICore* parent = NULL,
    int32 cell_capacity = 0
) NO_EXCEPT
{
    UITable* const table = UI_ELEMENT_ALLOC(layout, UI_ELEMENT_TYPE_TABLE, UITable, parent);
    table->core.flags = UI_ELEMENT_FLAG_INTERACTABLE | UI_ELEMENT_FLAG_HOVERABLE;
    table->core.dimension.align_anchor = UI_ANCHOR_ALIGN_TOP_LEFT;

    table->header_color = UI_DEFAULT_HEADER_BACKGROUND;
    table->row_color = UI_DEFAULT_CONTROL_BACKGROUND;
    table->row_color_alt = UI_DEFAULT_ROW_ALT_BACKGROUND;
    table->row_color_hover = 0x3A3A3AFF;
    table->row_color_selected = UI_DEFAULT_ROW_SELECTED;
    table->footer_color = UI_DEFAULT_HEADER_BACKGROUND;
    table->border_color = UI_DEFAULT_CONTROL_BORDER;
    table->font.size = UI_DEFAULT_FONT_SIZE;
    table->font.color = UI_DEFAULT_TEXT_COLOR;
    table->row_height = UI_DEFAULT_ROW_HEIGHT;
    table->selected = -1;
    table->hovered = -1;

    table->column_count = (int16) column_count;
    table->columns = ui_table_strings_store(layout, titles, column_count);

    // Equal column widths by default
    table->column_widths = ui_text_reserve(layout, CHAR_TYPE_CHAR, (int32) sizeof(f32) * max_branched(column_count, 1));
    f32* const widths = (f32 *) (layout->ui_element_buffer.memory + table->column_widths);
    for (int32 i = 0; i < column_count; ++i) {
        widths[i] = 1.0f / (f32) column_count;
    }

    table->row_capacity = (int16) row_capacity;
    table->rows = ui_text_reserve(layout, CHAR_TYPE_CHAR, (int32) sizeof(int32) * max_branched(row_capacity, 1));

    if (cell_capacity > 0) {
        table->cell_capacity = (int16) cell_capacity;

        // All rows + the footer as one block, every row points to its part of the block
        const int32 row_size = column_count * cell_capacity;
        const int32 cells = ui_text_reserve(layout, CHAR_TYPE_CHAR, row_size * (row_capacity + 1));
        memset(layout->ui_element_buffer.memory + cells, 0, (size_t) row_size * (row_capacity + 1));

        int32* const rows = (int32 *) (layout->ui_element_buffer.memory + table->rows);
        for (int32 i = 0; i < row_capacity; ++i) {
            rows[i] = cells + i * row_size;
        }

        // The footer block (after the rows) is reserved but only shown after ui_table_set_footer()
        table->footer = 0;
    }

    return table;
}

/**
 * @param fraction Fraction of the table width (all columns should sum up to 1.0)
 */
inline
void ui_table_set_column_width(UILayout* const layout, UITable* const table, int32 column, f32 fraction) NO_EXCEPT
{
    if (column < 0 || column >= table->column_count) {
        return;
    }

    ((f32 *) (layout->ui_element_buffer.memory + table->column_widths))[column] = fraction;
    ui_layout_mark_dirty(layout);
}

// Offset of a fixed size cell of a dynamic table (row = row_capacity -> footer)
FORCE_INLINE
int32 ui_table_cell_offset(const UILayout* const layout, const UITable* const table, int32 row, int32 column) NO_EXCEPT
{
    const int32* const rows = (const int32 *) (layout->ui_element_buffer.memory + table->rows);

    return rows[0] + (row * table->column_count + column) * table->cell_capacity;
}

// Copies the text into a fixed size cell (truncated), returns true if the content changed
inline
bool ui_table_cell_copy(UILayout* const layout, const UITable* const table, int32 offset, const char* const text) NO_EXCEPT
{
    char* const cell = (char *) (layout->ui_element_buffer.memory + offset);

    const int32 length = text ? min_branched((int32) strlen(text), table->cell_capacity - 1) : 0;
    if (strncmp(cell, text ? text : "", length) == 0 && cell[length] == '\0') {
        return false;
    }

    if (length) {
        memcpy(cell, text, length);
    }

    cell[length] = '\0';

    return true;
}

/**
 * Dynamic tables only (@see ui_table_create cell_capacity): changes the text of a cell
 * Too long texts are truncated to cell_capacity - 1 bytes
 * Only marks the ui as dirty if the text changed
 *
 * @return true if the text changed
 */
inline
bool ui_table_set_cell(UILayout* const layout, UITable* const table, int32 row, int32 column, const char* const text) NO_EXCEPT
{
    if (!table->cell_capacity || row < 0 || row >= table->row_capacity || column < 0 || column >= table->column_count) {
        return false;
    }

    if (!ui_table_cell_copy(layout, table, ui_table_cell_offset(layout, table, row, column), text)) {
        return false;
    }

    if (row < table->row_count) {
        table->core.change_type |= UI_ELEMENT_CHANGE_CONTENT;
        ui_layout_mark_dirty(layout);
    }

    return true;
}

/**
 * Dynamic tables only: amount of visible rows (the cell content of hidden rows is kept)
 */
inline
void ui_table_set_row_count(UILayout* const layout, UITable* const table, int32 count) NO_EXCEPT
{
    count = OMS_CLAMP(count, 0, (int32) table->row_capacity);
    if (!table->cell_capacity || count == table->row_count) {
        return;
    }

    table->row_count = (int16) count;
    if (table->selected >= count) {
        table->selected = -1;
    }

    if (table->hovered >= count) {
        table->hovered = -1;
    }

    table->core.change_type |= UI_ELEMENT_CHANGE_CONTENT;
    ui_layout_mark_dirty(layout);
}

/**
 * @param cells One string per column
 *
 * @return Row index or -1 if the table is full
 */
inline
int32 ui_table_add_row(UILayout* const layout, UITable* const table, const char* const* cells) NO_EXCEPT
{
    if (table->row_count >= table->row_capacity) {
        return -1;
    }

    if (table->cell_capacity) {
        for (int32 i = 0; i < table->column_count; ++i) {
            ui_table_cell_copy(layout, table, ui_table_cell_offset(layout, table, table->row_count, i), cells[i]);
        }
    } else {
        // Every row has its own string block, the row array only stores the offsets
        const int32 cell_offset = ui_table_strings_store(layout, cells, table->column_count);
        ((int32 *) (layout->ui_element_buffer.memory + table->rows))[table->row_count] = cell_offset;
    }

    table->core.change_type |= UI_ELEMENT_CHANGE_CONTENT;
    ui_layout_mark_dirty(layout);

    return table->row_count++;
}

/**
 * Footer row (one string per column, NULL = no footer)
 * Dynamic tables reuse their footer memory -> can be called every frame
 */
inline
void ui_table_set_footer(UILayout* const layout, UITable* const table, const char* const* cells) NO_EXCEPT
{
    if (table->cell_capacity && cells) {
        bool changed = !table->footer;
        table->footer = ui_table_cell_offset(layout, table, table->row_capacity, 0);
        for (int32 i = 0; i < table->column_count; ++i) {
            changed |= ui_table_cell_copy(layout, table, table->footer + i * table->cell_capacity, cells[i]);
        }

        if (!changed) {
            return;
        }
    } else {
        table->footer = cells ? ui_table_strings_store(layout, cells, table->column_count) : 0;
    }

    table->core.change_type |= UI_ELEMENT_CHANGE_CONTENT;
    ui_layout_mark_dirty(layout);
}

inline
const char* ui_table_cell(const UILayout* const layout, const UITable* const table, int32 row, int32 column) NO_EXCEPT
{
    if (row < 0 || row >= table->row_count || column < 0 || column >= table->column_count) {
        return NULL;
    }

    const int32* const rows = (const int32 *) (layout->ui_element_buffer.memory + table->rows);

    return ui_table_row_string(layout, table, rows[row], column);
}

// Scrollable body (between header and footer), x, y = top left
inline
v4_f32 ui_table_body_rect(const UITable* const table) NO_EXCEPT
{
    const UIAttributeDimension* const dim = &table->core.dimension;
    const f32 footer = table->footer ? table->row_height : 0.0f;

    return {
        dim->pos.x,
        dim->pos.y - table->row_height,
        dim->dim.x,
        max_branched(dim->dim.y - table->row_height - footer, 0.0f)
    };
}

inline
bool ui_table_scroll_info(const UITable* const table, UIScrollInfo* const info) NO_EXCEPT
{
    const v4_f32 body = ui_table_body_rect(table);
    const f32 content = table->row_height * (f32) table->row_count;
    if (content <= body.height) {
        return false;
    }

    info->track = {body.x + body.width - UI_SCROLLBAR_SIZE, body.y, UI_SCROLLBAR_SIZE, body.height};
    info->view = body.height;
    info->content = content;
    info->scroll = table->scroll;

    return true;
}

inline
bool ui_table_scroll_set(UILayout* const layout, UITable* const table, f32 scroll) NO_EXCEPT
{
    scroll = ui_scroll_clamp(scroll, table->row_height * (f32) table->row_count, ui_table_body_rect(table).height);
    if (scroll == table->scroll) {
        return false;
    }

    table->scroll = scroll;
    table->core.change_type |= UI_ELEMENT_CHANGE_CONTENT;
    ui_layout_mark_dirty(layout);

    return true;
}

/**
 * @return Body row at the position or -1 (header/footer/empty area)
 */
inline
int32 ui_table_row_at(const UITable* const table, f32 x, f32 y) NO_EXCEPT
{
    const v4_f32 body = ui_table_body_rect(table);
    if (x < body.x || x >= body.x + body.width || y > body.y || y <= body.y - body.height) {
        return -1;
    }

    const int32 row = (int32) ((body.y - y + table->scroll) / table->row_height);

    return row < table->row_count ? row : -1;
}

/**
 * Column at the x position (e.g. the clicked cell is ui_table_row_at() + ui_table_column_at())
 *
 * @return Column or -1 if the position is outside of the columns (e.g. scrollbar)
 */
inline
int32 ui_table_column_at(const UILayout* const layout, const UITable* const table, f32 x) NO_EXCEPT
{
    const v4_f32 body = ui_table_body_rect(table);
    if (!table->column_widths || x < body.x) {
        return -1;
    }

    // Same width as the rendering (without the scrollbar @see ui_table_cache)
    UIScrollInfo info;
    const f32 width = body.width - (ui_table_scroll_info(table, &info) ? UI_SCROLLBAR_SIZE : 0.0f);

    const f32* const widths = (const f32 *) (layout->ui_element_buffer.memory + table->column_widths);
    f32 right = body.x;
    for (int32 column = 0; column < table->column_count; ++column) {
        right += widths[column] * width;
        if (x < right) {
            return column;
        }
    }

    return -1;
}

inline
bool ui_table_hover(UILayout* const layout, UITable* const table, int32 row) NO_EXCEPT
{
    if (row == table->hovered) {
        return false;
    }

    table->hovered = (int16) row;
    table->core.change_type |= UI_ELEMENT_CHANGE_CONTENT;
    ui_layout_mark_dirty(layout);

    return true;
}

/**
 * @return true if the selection changed
 */
inline
bool ui_table_select(UILayout* const layout, UITable* const table, int32 row) NO_EXCEPT
{
    if (row < -1 || row >= table->row_count || row == table->selected) {
        return false;
    }

    table->selected = (int16) row;
    table->core.change_type |= UI_ELEMENT_CHANGE_CONTENT;
    ui_layout_mark_dirty(layout);

    return true;
}

// Scrolls the body so that the row is visible
inline
void ui_table_scroll_to_row(UILayout* const layout, UITable* const table, int32 row) NO_EXCEPT
{
    const f32 view = ui_table_body_rect(table).height;
    const f32 top = (f32) row * table->row_height;

    if (top < table->scroll) {
        ui_table_scroll_set(layout, table, top);
    } else if (top + table->row_height > table->scroll + view) {
        ui_table_scroll_set(layout, table, top + table->row_height - view);
    }
}

// Renders the cells of one row, every cell is clipped to its column
inline
void ui_table_row_vertices(
    UILayout* const layout, const UITable* const table,
    int32 strings, bool fixed_cells, f32 row_y, f32 width,
    const UIClipRect& clip,
    f32 zindex,
    BufferMemory* const __restrict mem
) NO_EXCEPT
{
    if (!layout->font || !strings) {
        return;
    }

    ArrayVector<Vertex3DSamplerTextureColor>* const vertices = &layout->ui_vertex_cache;
    ArrayVector<int32>* const indices = &layout->ui_index_cache;

    const f32* const widths = (const f32 *) (layout->ui_element_buffer.memory + table->column_widths);
    const f32 line_height = ui_text_line_height(layout->font, table->font.size);

    f32 x = table->core.dimension.pos.x;
    for (int32 column = 0; column < table->column_count; ++column) {
        const f32 column_width = widths[column] * width;

        const int32 vertex_start = vertices->count;
        const int32 index_start = indices->count;

        ui_text_vertices(
            layout,
            fixed_cells
                ? (const char *) (layout->ui_element_buffer.memory + strings + column * table->cell_capacity)
                : ui_table_string(layout, strings, column),
            table->font.size, table->font.color,
            x + UI_DEFAULT_TEXT_PADDING, row_y - (table->row_height - line_height) * 0.5f,
            zindex, mem
        );

        // Long texts are cut at the column border
        const UIClipRect cell = ui_clip_intersect(
            clip,
            ui_clip_from_rect({x, row_y, max_branched(column_width - UI_DEFAULT_TEXT_PADDING, 0.0f), table->row_height})
        );
        ui_vertices_clip_or_remove(vertices, indices, vertex_start, index_start, cell);

        x += column_width;
    }
}

void ui_table_cache(
    UITable* const table,
    GpuApiType gpu_api_type,
    UILayout* const layout,
    f32* const zindex,
    BufferMemory* const __restrict mem
) NO_EXCEPT
{
    PROFILE_DEBUG(PROFILE_UI_CACHE_TABLE);

    ui_dimension_calculate(layout, &table->core);
    const UIAttributeDimension* const dim = &table->core.dimension;

    ArrayVector<Vertex3DSamplerTextureColor>* const vertices = &layout->ui_vertex_cache;
    ArrayVector<int32>* const indices = &layout->ui_index_cache;

    UIScrollInfo scroll_info;
    const bool has_scrollbar = ui_table_scroll_info(table, &scroll_info);
    const f32 width = dim->dim.x - (has_scrollbar ? UI_SCROLLBAR_SIZE : 0.0f);

    const v4_f32 body = ui_table_body_rect(table);
    const f32 scroll = ui_scroll_clamp(table->scroll, table->row_height * (f32) table->row_count, body.height);

    // WARNING: Overlapping geometry needs its own zindex (the depth test rejects equal depth values)
    //          layer 1: backgrounds, layer 2: row colors, layer 3: text + lines

    // Layer 1: backgrounds
    const v4_f32 header = {dim->pos.x, dim->pos.y, dim->dim.x, table->row_height};
    const v4_f32 footer = {dim->pos.x, dim->pos.y - dim->dim.y + table->row_height, dim->dim.x, table->row_height};

    ui_rect_vertices(body, table->row_color, vertices, indices, *zindex);
    ui_rect_vertices(header, table->header_color, vertices, indices, *zindex);
    if (table->footer) {
        ui_rect_vertices(footer, table->footer_color, vertices, indices, *zindex);
    }

    *zindex = camera_step_closer(gpu_api_type, *zindex);

    // Only the visible rows are rendered
    const UIClipRect body_clip = ui_clip_from_rect({body.x, body.y, width, body.height});
    const int32* const rows = (const int32 *) (layout->ui_element_buffer.memory + table->rows);
    const int32 first_row = (int32) (scroll / table->row_height);
    const int32 last_row = min_branched(table->row_count - 1, (int32) ((scroll + body.height) / table->row_height));

    // Layer 2: row colors (alternating, hover, selected)
    {
        const int32 vertex_start = vertices->count;
        const int32 index_start = indices->count;

        for (int32 row = first_row; row <= last_row; ++row) {
            const f32 row_y = body.y + scroll - table->row_height * (f32) row;

            const uint32 color = row == table->selected
                ? table->row_color_selected
                : (row == table->hovered ? table->row_color_hover : ((row & 1) ? table->row_color_alt : 0));

            if (color) {
                ui_rect_vertices({body.x, row_y, width, table->row_height}, color, vertices, indices, *zindex);
            }
        }

        ui_vertices_clip_or_remove(vertices, indices, vertex_start, index_start, body_clip);
        *zindex = camera_step_closer(gpu_api_type, *zindex);
    }

    // Layer 3: text, column separators, border
    {
        const int32 vertex_start = vertices->count;
        const int32 index_start = indices->count;

        for (int32 row = first_row; row <= last_row; ++row) {
            const f32 row_y = body.y + scroll - table->row_height * (f32) row;
            ui_table_row_vertices(layout, table, rows[row], table->cell_capacity != 0, row_y, width, body_clip, *zindex, mem);
        }

        ui_vertices_clip_or_remove(vertices, indices, vertex_start, index_start, body_clip);
    }

    ui_table_row_vertices(layout, table, table->columns, false, header.y, width, ui_clip_from_rect(header), *zindex, mem);
    if (table->footer) {
        ui_table_row_vertices(layout, table, table->footer, table->cell_capacity != 0, footer.y, width, ui_clip_from_rect(footer), *zindex, mem);
    }

    // Column separators
    {
        const f32* const widths = (const f32 *) (layout->ui_element_buffer.memory + table->column_widths);
        f32 x = dim->pos.x;
        for (int32 column = 0; column < table->column_count - 1; ++column) {
            x += widths[column] * width;
            ui_rect_vertices({x, dim->pos.y, 1.0f, dim->dim.y}, table->border_color, vertices, indices, *zindex);
        }
    }

    ui_border_solid_vertices(ui_dimension_rect(dim), table->border_color, 1, vertices, indices, *zindex);
    *zindex = camera_step_closer(gpu_api_type, *zindex);

    if (has_scrollbar) {
        scroll_info.scroll = scroll;
        ui_scrollbar_vertices(
            &scroll_info, UI_SCROLL_AXIS_Y,
            layout->interaction.scroll_dragged == ui_element_offset(layout, table),
            gpu_api_type, vertices, indices, zindex
        );
    }
}

#endif
