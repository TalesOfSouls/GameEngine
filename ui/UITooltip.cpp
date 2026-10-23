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
 * The owner decides when it is shown (@see ui_tooltip_show, ui_tooltip_hide), usually in an update function
 */
inline
UIPanel* ui_tooltip_create(UILayout* const layout, f32 font_size = UI_DEFAULT_FONT_SIZE) NO_EXCEPT
{
    UIPanel* const tooltip = ui_panel_create(layout, NULL, 0x1A1A1AF0);
    tooltip->core.flags = UI_ELEMENT_FLAG_IGNORE_HIT;
    tooltip->core.state |= UI_STATE_HIDDEN;
    tooltip->border.color = UI_DEFAULT_CONTROL_BORDER;
    tooltip->border.width = 1;

    UILabel* const label = ui_label_create(layout, CHAR_TYPE_CHAR, 0, UI_TOOLTIP_MAX_LENGTH, &tooltip->core);
    label->core.flags = UI_ELEMENT_FLAG_IGNORE_HIT;
    label->core.dimension.pos_raw = {UI_TOOLTIP_PADDING, UI_TOOLTIP_PADDING};
    label->font.size = font_size;
    label->font.color = UI_DEFAULT_TEXT_COLOR;

    return tooltip;
}

FORCE_INLINE
UILabel* ui_tooltip_label(const UILayout* const layout, const UIPanel* const tooltip) NO_EXCEPT
{
    // The label is the first child (allocated directly after the tooltip)
    return (UILabel *) ui_element_get(layout, tooltip->core.next_offset);
}

FORCE_INLINE
bool ui_tooltip_is_visible(const UIPanel* const tooltip) NO_EXCEPT
{
    return !(tooltip->core.state & UI_STATE_HIDDEN);
}

/**
 * Shows the tooltip next to a position (usually the pointer @see UIInteraction::pointer)
 * The tooltip is placed bottom right of the position and stays on screen
 * Only marks the ui as changed if the text or position changed
 *
 * @param text Multi line text (\n), cut at UI_TOOLTIP_MAX_LENGTH - 1 bytes
 * @param x, y Position in ui coordinates
 */
inline
void ui_tooltip_show(UILayout* const layout, UIPanel* const tooltip, const char* const text, f32 x, f32 y) NO_EXCEPT
{
    UILabel* const label = ui_tooltip_label(layout, tooltip);
    char* const content = (char *) (layout->ui_element_buffer.memory + label->content);

    bool changed = false;

    const int32 length = min_branched((int32) strlen(text), label->content_length - 1);
    if (strncmp(content, text, length) != 0 || content[length] != '\0') {
        memcpy(content, text, length);
        content[length] = '\0';
        changed = true;
    }

    // Size = widest line + padding
    int32 lines = 1;
    f32 text_width = 0.0f;
    int32 line_start = 0;
    for (int32 i = 0; i <= length; ++i) {
        if (i < length && content[i] != '\n') {
            continue;
        }

        if (layout->font) {
            text_width = max_branched(text_width, ui_text_range_width(layout->font, content, line_start, i, label->font.size));
        }

        lines += i < length;
        line_start = i + 1;
    }

    const f32 width = text_width + 2.0f * UI_TOOLTIP_PADDING;
    const f32 height = (layout->font ? ui_text_line_height(layout->font, label->font.size) : label->font.size) * (f32) lines
        + 2.0f * UI_TOOLTIP_PADDING;

    // Bottom right of the position, flipped if it doesn't fit (y-up)
    const v2_f32 screen = layout->ui_root ? layout->ui_root->dimension.dim : v2_f32{width, height};

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
        ui_element_mark_changed(layout, &tooltip->core);
        ui_element_mark_changed(layout, &label->core);
    }
}

inline
void ui_tooltip_hide(UILayout* const layout, UIPanel* const tooltip) NO_EXCEPT
{
    if (tooltip->core.state & UI_STATE_HIDDEN) {
        return;
    }

    tooltip->core.state |= UI_STATE_HIDDEN;
    ui_element_mark_changed(layout, &tooltip->core);
}

#endif
