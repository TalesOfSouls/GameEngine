#pragma once
#ifndef COMS_UI_TABLE_H
#define COMS_UI_TABLE_H

#include "../stdlib/Stdlib.h"
#include "attribute/UIAttributeFont.h"
#include "UICore.h"

// Table with a header row, a scrollable body and an optional footer row
//
// The table renders all cells itself (no element per cell/row) and only creates vertices for the visible rows.
// Data in the ui_element_buffer (all strings are \0 separated, one string per column):
//      columns:        header titles
//      column_widths:  f32 per column, fraction of the table width (sum = 1.0)
//      rows:           int32 per row = offset of the cell strings of that row
//      footer:         footer cell strings (0 = no footer)
//
// Static tables (cell_capacity = 0): every added row stores its strings once (ui_table_add_row)
// Dynamic tables (cell_capacity > 0): every cell has a fixed size buffer that is reserved at creation
//      -> the content can be changed at any time without using more memory (ui_table_set_cell, ui_table_set_row_count)
//      e.g. profiler, log output, stats that change every frame
//
// 152 bytes
struct UITable {
    UICore core;

    // RGBA
    uint32 header_color;
    uint32 row_color;
    uint32 row_color_alt;
    uint32 row_color_hover;
    uint32 row_color_selected;
    uint32 footer_color;
    uint32 border_color;

    UIAttributeFont font;

    int32 columns;
    int32 column_widths;
    int32 rows;
    int32 footer;

    // 1-indexed action that is called when the selected row changes (layout->on_actions, 0 = none)
    int32 on_change;

    // Height of every row incl. header and footer
    f32 row_height;

    // Vertical scroll offset of the body in px
    f32 scroll;

    int16 column_count;
    int16 row_count;

    // Max rows that can be added (memory for the row offsets is reserved at creation)
    int16 row_capacity;

    // -1 = none
    int16 selected;
    int16 hovered;

    // Bytes per cell incl. \0 (0 = static table @see above)
    int16 cell_capacity;
};

#endif
