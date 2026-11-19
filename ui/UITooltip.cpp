#pragma once
#ifndef COMS_UI_TOOLTIP_C
#define COMS_UI_TOOLTIP_C

#include "../stdlib/Stdlib.h"
#include "UIStyleDefaults.h"
#include "UICore.cpp"
#include "UIPanel.cpp"
#include "UILabel.cpp"

// Max text length of a tooltip incl. \0 (the text memory is reserved once)
#define UI_TOOLTIP_MAX_LENGTH 512
#define UI_TOOLTIP_PADDING 6.0f

// Distance between the pointer and the tooltip
#define UI_TOOLTIP_OFFSET 16.0f

/**
 * Small info box that follows the pointer (e.g. details of the hovered part of a custom element)
 *
 * Tree (root child, created hidden):
 *      UIPanel  tooltip (pointer transparent)
 *          UILabel  text (multi line)
 *
 * The owner decides when it is shown (ui_tooltip_hide), usually in an update function
 */
inline
UIPanel* ui_tooltip_create(UILayout* const layout, f32 font_size = UI_DEFAULT_FONT_SIZE) NO_EXCEPT
{
    UIPanel* const tooltip = ui_panel_create(layout, NULL, 0x1A1A1AF0);
    tooltip->core.flags = UI_ELEMENT_FLAG_IGNORE_HIT;
    tooltip->core.state |= UI_STATE_HIDDEN;
    tooltip->border.color = UI_DEFAULT_CONTROL_BORDER;
    ui_border_width_set(&tooltip->border, 1);

    UILabel* const label = ui_label_create(layout, CHAR_TYPE_CHAR, 0, UI_TOOLTIP_MAX_LENGTH, &tooltip->core);
    label->core.flags = UI_ELEMENT_FLAG_IGNORE_HIT;
    label->core.dimension.pos_raw = {UI_TOOLTIP_PADDING, UI_TOOLTIP_PADDING};
    label->font.size = font_size;
    label->font.color = UI_DEFAULT_TEXT_COLOR;

    return tooltip;
}


// @check remove
FORCE_INLINE
UILabel* ui_tooltip_label(const UILayout* const layout, const UIPanel* const tooltip) NO_EXCEPT
{
    // The label is the first child (allocated directly after the tooltip)
    return (UILabel *) ui_element_get(layout, tooltip->core.next_offset);
}

// Size + position of the tooltip after its text was set
static inline
void ui_tooltip_place(
    UILayout* const layout, UIPanel* const tooltip, UILabel* const label,
    int32 length, bool changed,
    f32 x, f32 y
) NO_EXCEPT
{
    const char* const content = (const char *) (layout->ui_element_buffer.memory + label->content);

    // Size = widest line + padding
    int32 lines = 1;
    f32 text_width = 0.0f;
    int32 line_start = 0;
    for (int32 i = 0; i <= length; ++i) {
        if (i < length && content[i] != '\n') {
            continue;
        }

        if (layout->font) {
            text_width = max_branched(text_width, font_system_text_range_width(layout->font, content, line_start, i, label->font.size));
        }

        lines += i < length;
        line_start = i + 1;
    }

    const f32 width = text_width + 2.0f * UI_TOOLTIP_PADDING;
    const f32 height = (layout->font ? font_line_height(&layout->font->base, label->font.size) : label->font.size) * (f32) lines
        + 2.0f * UI_TOOLTIP_PADDING;

    // Bottom right of the position, flipped if it doesn't fit (y-up)
    ASSERT_TRUE(layout->ui_root);
    const v2_f32 screen = layout->ui_root->dimension.dim;

    f32 left = x + UI_TOOLTIP_OFFSET;
    if (left + width > screen.x) {
        left = max_branched(x - UI_TOOLTIP_OFFSET - width, 0.0f);
    }

    f32 top = y - UI_TOOLTIP_OFFSET;
    if (top - height < 0.0f) {
        top = min_branched(y + UI_TOOLTIP_OFFSET + height, screen.y);
    }

    // pos_raw = offset from the top left corner of the root (y is positive downwards)
    const v2_f32 pos = {left, screen.y - top};
    UIAttributeDimension* const dim = &tooltip->core.dimension;
    if (dim->pos_raw.x != pos.x || dim->pos_raw.y != pos.y || dim->dim_raw.x != width || dim->dim_raw.y != height) {
        dim->pos_raw = pos;
        dim->dim_raw = {width, height};
        changed = true;
    }

    if (tooltip->core.state & UI_STATE_HIDDEN) {
        tooltip->core.state &= ~UI_STATE_HIDDEN;

        // Above everything else (except UI_ELEMENT_FLAG_ALWAYS_ON_TOP elements like the cursor)
        ui_element_move_to_front(layout, &tooltip->core);
        changed = true;
    }

    if (changed) {
        tooltip->core.change_type |= UI_ELEMENT_CHANGE_CONTENT;
        label->core.change_type |= UI_ELEMENT_CHANGE_CONTENT;
        layout->dirty |= UI_LAYOUT_DIRTY;
    }
}

/**
 * Shows the tooltip next to a position (usually the pointer)
 * The tooltip is placed bottom right of the position and stays on screen
 * Only marks the ui as changed if the text or position changed
 *
 * @param const char*   text    Multi line text (\n), cut at UI_TOOLTIP_MAX_LENGTH - 1 bytes
 * @param int32         length  Length in characters excl. \0
 * @param f32           x, y    Position in ui coordinates
 */
inline
void ui_tooltip_show(
    UILayout* const layout, UIPanel* const tooltip,
    const char* const text, int32 length,
    f32 x, f32 y
) NO_EXCEPT
{
    UILabel* const label = ui_tooltip_label(layout, tooltip);

    // The text is cut if it doesn't fit
    char* const content = (char *) (layout->ui_element_buffer.memory + label->content);
    length = min_branched(length, ui_string_max_length(layout, label->content));

    const bool changed = memcmp(content, text, length) != 0 || content[length] != '\0';
    if (changed) {
        str_copy(content, text, length);
        ui_string_length_set(layout, label->content, length);
    }

    ui_tooltip_place(layout, tooltip, label, length, changed, x, y);
}

// Same as above but the length is calculated
// @check remove
inline
void ui_tooltip_show(UILayout* const layout, UIPanel* const tooltip, const char* const text, f32 x, f32 y) NO_EXCEPT
{
    ui_tooltip_show(layout, tooltip, text, (int32) strlen(text), x, y);
}

inline
void ui_tooltip_hide(UILayout* const layout, UIPanel* const tooltip) NO_EXCEPT
{
    if (tooltip->core.state & UI_STATE_HIDDEN) {
        return;
    }

    tooltip->core.state |= UI_STATE_HIDDEN;
    tooltip->core.change_type |= UI_ELEMENT_CHANGE_CONTENT;
    layout->dirty |= UI_LAYOUT_DIRTY;
}

#endif
