#pragma once
#ifndef COMS_UI_COLOR_PICKER_C
#define COMS_UI_COLOR_PICKER_C

#include "../stdlib/Stdlib.h"
#include "../gpuapi/RenderUtils.h"
#include "../camera/Camera.cpp"
#include "attribute/UIAttributeDimension.cpp"
#include "attribute/UIAttributeBorder.cpp"
#include "UIAlignment.h"
#include "UIStyleDefaults.h"
#include "UICore.cpp"
#include "UILabel.cpp"
#include "UIPanel.cpp"
#include "UIInput.cpp"
#include "UIColorPicker.h"

// Gradient popup layout (relative to the color panel)
#define UI_COLOR_GRADIENT_SIZE 160.0f
#define UI_COLOR_BAR_WIDTH 20.0f
#define UI_COLOR_GAP 8.0f

// Amount of cells per axis used to approximate the gradients
// (the vertex color can't be interpolated since it is packed into the texture coordinates)
#define UI_COLOR_GRADIENT_CELLS 16
#define UI_COLOR_HUE_CELLS 32

#define UI_COLOR_PALETTE_COLUMNS 16
#define UI_COLOR_PALETTE_CELL 15.0f

enum UIColorPanelArea : byte {
    UI_COLOR_PANEL_AREA_NONE,
    UI_COLOR_PANEL_AREA_GRADIENT,
    UI_COLOR_PANEL_AREA_HUE,
    UI_COLOR_PANEL_AREA_ALPHA,
};

////////////////////////////////////////////////////////////////
// Color helpers
////////////////////////////////////////////////////////////////

CONSTEXPR static FORCE_INLINE
uint32 ui_color_rgba(uint32 r, uint32 g, uint32 b, uint32 a) NO_EXCEPT
{
    return (r << 24) | (g << 16) | (b << 8) | a;
}

CONSTEXPR static FORCE_INLINE
uint32 ui_color_channel(uint32 color, int32 channel) NO_EXCEPT
{
    return (color >> (24 - 8 * channel)) & 0xFF;
}

// h, s, v, a = 0.0 - 1.0
CONSTEXPR inline
uint32 ui_color_from_hsv(f32 h, f32 s, f32 v, f32 a = 1.0f) NO_EXCEPT
{
    h = (h - (f32) (int32) h) * 6.0f;
    if (h < 0.0f) {
        h += 6.0f;
    }

    const int32 sector = (int32) h;
    const f32 f = h - (f32) sector;
    const f32 p = v * (1.0f - s);
    const f32 q = v * (1.0f - s * f);
    const f32 t = v * (1.0f - s * (1.0f - f));

    f32 r, g, b;
    switch (sector) {
        case 0: {
            r = v;
            g = t;
            b = p;
        } break;
        case 1: {
            r = q;
            g = v;
            b = p;
        } break;
        case 2: {
            r = p;
            g = v;
            b = t;
        } break;
        case 3: {
            r = p;
            g = q;
            b = v;
        } break;
        case 4: {
            r = t;
            g = p;
            b = v;
        } break;
        default: {
            r = v;
            g = p;
            b = q;
        } break;
    }

    return ui_color_rgba(
        (uint32) (r * 255.0f + 0.5f),
        (uint32) (g * 255.0f + 0.5f),
        (uint32) (b * 255.0f + 0.5f),
        (uint32) (oms_clamp(a, 0.0f, 1.0f) * 255.0f + 0.5f)
    );
}

/**
 * @param f32* hue Is only changed if the color has a hue (not grey)
 */
CONSTEXPR inline
void ui_color_to_hsv(uint32 color, f32* const __restrict hue, f32* const __restrict saturation, f32* const __restrict value) NO_EXCEPT
{
    const f32 r = (f32) ui_color_channel(color, 0) / 255.0f;
    const f32 g = (f32) ui_color_channel(color, 1) / 255.0f;
    const f32 b = (f32) ui_color_channel(color, 2) / 255.0f;

    const f32 max = oms_max(r, oms_max(g, b));
    const f32 min = oms_min(r, oms_min(g, b));
    const f32 delta = max - min;

    *value = max;
    *saturation = max > 0.0f ? delta / max : 0.0f;

    if (delta <= 0.0f) {
        return;
    }

    f32 h;
    if (max == r) {
        h = (g - b) / delta;
    } else if (max == g) {
        h = 2.0f + (b - r) / delta;
    } else {
        h = 4.0f + (r - g) / delta;
    }

    h /= 6.0f;
    *hue = h < 0.0f ? h + 1.0f : h;
}

/**
 * Default palette: the 256 ansi (xterm) colors in display order (UI_COLOR_PALETTE_COLUMNS cells per row, 0 = empty cell)
 *      1. grey colors (dark -> light)
 *      2. other colors: columns = hue (equal amount of colors per column), rows = light -> dark
 *
 * The ansi palette repeats colors (the system colors 0 - 15 are also part of the 6x6x6 color cube / grey ramp),
 * only the 247 unique colors are listed.
 */
static const uint32 UI_COLOR_PALETTE_ANSI[] = {
    0x000000FF, 0x080808FF, 0x121212FF, 0x1C1C1CFF, 0x262626FF, 0x303030FF, 0x3A3A3AFF, 0x444444FF, 0x4E4E4EFF, 0x585858FF, 0x5F5F5FFF, 0x626262FF, 0x6C6C6CFF, 0x767676FF, 0x808080FF, 0x878787FF,
    0x8A8A8AFF, 0x949494FF, 0x9E9E9EFF, 0xA8A8A8FF, 0xAFAFAFFF, 0xB2B2B2FF, 0xBCBCBCFF, 0xC0C0C0FF, 0xC6C6C6FF, 0xD0D0D0FF, 0xD7D7D7FF, 0xDADADAFF, 0xE4E4E4FF, 0xEEEEEEFF, 0xFFFFFFFF, 0x00000000,
    0xD7AFAFFF, 0xFFD7D7FF, 0xFFD787FF, 0xFFFFD7FF, 0xD7FFAFFF, 0xAFFF87FF, 0xD7FFD7FF, 0xAFFFD7FF, 0x87FFFFFF, 0xD7FFFFFF, 0xAFD7FFFF, 0xD7D7FFFF, 0xD7AFFFFF, 0xD75FFFFF, 0xFFD7FFFF, 0xFFAFD7FF,
    0xD78787FF, 0xFFAFAFFF, 0xFFD75FFF, 0xFFFFAFFF, 0xD7FF87FF, 0x87FF5FFF, 0xAFFFAFFF, 0x87FFD7FF, 0x5FFFFFFF, 0xAFFFFFFF, 0x87AFFFFF, 0xAFAFFFFF, 0xAF87FFFF, 0xAF87AFFF, 0xFFAFFFFF, 0xFF87D7FF,
    0xFF5F5FFF, 0xFFD7AFFF, 0xD7AF5FFF, 0xD7D7AFFF, 0xD7FF5FFF, 0x87D75FFF, 0x87FF87FF, 0x5FFFAFFF, 0x87D7D7FF, 0xAFD7D7FF, 0x5FAFFFFF, 0x8787FFFF, 0xD787FFFF, 0xAF5FAFFF, 0xD7AFD7FF, 0xFF87AFFF,
    0xAF8787FF, 0xFF8787FF, 0xFFAF00FF, 0xFFFF87FF, 0xAFD787FF, 0x5FD75FFF, 0xAFD7AFFF, 0x87D7AFFF, 0x5FD7D7FF, 0x87D7FFFF, 0x87AFD7FF, 0xAFAFD7FF, 0xAF5FFFFF, 0xD700FFFF, 0xFF87FFFF, 0xFF5FAFFF,
    0xD75F5FFF, 0xFFAF87FF, 0xFFD700FF, 0xD7D787FF, 0xAFFF5FFF, 0x5FAF5FFF, 0x87FFAFFF, 0x5FFFD7FF, 0x87AFAFFF, 0x5FD7FFFF, 0x5F87FFFF, 0x5F5FFFFF, 0xAF87D7FF, 0xFF00FFFF, 0xD787D7FF, 0xD787AFFF,
    0xAF5F5FFF, 0xFF875FFF, 0xFFFF00FF, 0xFFFF5FFF, 0xAFD75FFF, 0x5FFF00FF, 0x5FFF5FFF, 0x5FD7AFFF, 0x5FAFAFFF, 0x5FAFD7FF, 0x5F87D7FF, 0x8787D7FF, 0xAF5FD7FF, 0x875F87FF, 0xFF5FFFFF, 0xFF5F87FF,
    0xFF0000FF, 0xD7AF87FF, 0xD78700FF, 0xAFAF87FF, 0x87AF5FFF, 0x00FF00FF, 0x87D787FF, 0x00FF87FF, 0x00FFFFFF, 0x00D7FFFF, 0x5F87AFFF, 0x875FFFFF, 0x875FAFFF, 0xAF00D7FF, 0xFF5FD7FF, 0xD75FAFFF,
    0x875F5FFF, 0xFFAF5FFF, 0xD7AF00FF, 0xD7D75FFF, 0xAFFF00FF, 0x5F875FFF, 0x5FFF87FF, 0x00FFAFFF, 0x5F8787FF, 0x00AFFFFF, 0x005FFFFF, 0x5F5FD7FF, 0x5F00FFFF, 0xD700D7FF, 0xD75FD7FF, 0xD75F87FF,
    0xD70000FF, 0xD7875FFF, 0xAF8700FF, 0xAFAF5FFF, 0x87FF00FF, 0x5FD700FF, 0x87AF87FF, 0x00FFD7FF, 0x00D7D7FF, 0x0087FFFF, 0x0000FFFF, 0x8787AFFF, 0x8700FFFF, 0x8700AFFF, 0xFF00D7FF, 0xAF5F87FF,
    0xAF0000FF, 0xAF875FFF, 0x875F00FF, 0xD7FF00FF, 0x87D700FF, 0x00D700FF, 0x5FD787FF, 0x00D787FF, 0x00AFAFFF, 0x00AFD7FF, 0x005FD7FF, 0x875FD7FF, 0xAF00FFFF, 0xAF00AFFF, 0xFF00AFFF, 0xFF0087FF,
    0x870000FF, 0xFF5F00FF, 0x878700FF, 0x87875FFF, 0x87AF00FF, 0x00AF00FF, 0x5FAF87FF, 0x00D7AFFF, 0x008787FF, 0x0087D7FF, 0x000087FF, 0x5F5FAFFF, 0x5F00D7FF, 0x5F0087FF, 0xD700AFFF, 0xFF005FFF,
    0x800000FF, 0xFF8700FF, 0x808000FF, 0xD7D700FF, 0x5FAF00FF, 0x008700FF, 0x00FF5FFF, 0x00AF5FFF, 0x008080FF, 0x0087AFFF, 0x000080FF, 0x5F5F87FF, 0x8700D7FF, 0x870087FF, 0xAF0087FF, 0xD70087FF,
    0x5F0000FF, 0xD75F00FF, 0x5F5F00FF, 0xAFD700FF, 0x5F8700FF, 0x008000FF, 0x00D75FFF, 0x00AF87FF, 0x005F5FFF, 0x005FAFFF, 0x00005FFF, 0x0000D7FF, 0x5F00AFFF, 0x800080FF, 0x87005FFF, 0xD7005FFF,
    0x00000000, 0xAF5F00FF, 0x00000000, 0xAFAF00FF, 0x00000000, 0x005F00FF, 0x00000000, 0x00875FFF, 0x00000000, 0x005F87FF, 0x00000000, 0x0000AFFF, 0x00000000, 0x5F005FFF, 0x00000000, 0xAF005FFF,
};

// Amount of grid cells of the palette
CONSTEXPR FORCE_INLINE
int32 ui_color_picker_cell_count(const UIColorPicker* const picker) NO_EXCEPT
{
    return picker->palette ? picker->palette_count : (int32) ARRAY_COUNT(UI_COLOR_PALETTE_ANSI);
}

// Color of a grid cell (0 = empty cell or outside of the palette)
FORCE_INLINE
uint32 ui_color_picker_cell_color(const UILayout* const __restrict layout, const UIColorPicker* const __restrict picker, int32 cell) NO_EXCEPT
{
    if (cell < 0 || cell >= ui_color_picker_cell_count(picker)) {
        return 0;
    }

    return picker->palette
        ? ((const uint32 *) (layout->ui_element_buffer.memory + picker->palette))[cell]
        : UI_COLOR_PALETTE_ANSI[cell];
}

// Writes the rgba values into the input fields (except the one that is currently edited)
inline
void ui_color_picker_update_inputs(UILayout* const layout, UIColorPicker* const picker) NO_EXCEPT
{
    for (int32 i = 0; i < 4; ++i) {
        if (!picker->inputs[i] || picker->inputs[i] == layout->interaction.editing) {
            continue;
        }

        char text[12];
        const int32 length = int_to_str((int64) ui_color_channel(picker->color, i), text);

        ui_input_set_text(layout, (UIInput *) ui_element_get(layout, picker->inputs[i]), text, length);
    }
}

/**
 * Sets the color (rgba)
 *
 * @return bool true if the color changed
 */
inline
bool ui_color_picker_set(UILayout* const layout, UIColorPicker* const picker, uint32 color) NO_EXCEPT
{
    if (picker->color == color) {
        return false;
    }

    picker->color = color;
    ui_color_to_hsv(color, &picker->hue, &picker->saturation, &picker->value);
    ui_color_picker_update_inputs(layout, picker);

    picker->core.change_type |= UI_ELEMENT_CHANGE_CONTENT;
    layout->dirty |= UI_LAYOUT_DIRTY;

    return true;
}

/**
 * Sets the color by hsv (keeps the hue even for grey colors)
 *
 * @return bool true if the color changed
 */
inline
bool ui_color_picker_set_hsv(UILayout* const layout, UIColorPicker* const picker, f32 h, f32 s, f32 v, f32 a) NO_EXCEPT
{
    const uint32 color = ui_color_from_hsv(h, s, v, a);
    const bool changed = color != picker->color
        || h != picker->hue || s != picker->saturation || v != picker->value;

    picker->color = color;
    picker->hue = h;
    picker->saturation = s;
    picker->value = v;

    if (changed) {
        ui_color_picker_update_inputs(layout, picker);
        picker->core.change_type |= UI_ELEMENT_CHANGE_CONTENT;
        layout->dirty |= UI_LAYOUT_DIRTY;
    }

    return changed;
}

/**
 * Reads the rgba input fields (called when one of them changed)
 *
 * @return bool true if the color changed
 */
inline
bool ui_color_picker_inputs_changed(UILayout* const layout, UIColorPicker* const picker) NO_EXCEPT
{
    uint32 channels[4];
    for (int32 i = 0; i < 4; ++i) {
        channels[i] = ui_color_channel(picker->color, i);
        if (!picker->inputs[i]) {
            continue;
        }

        const char* text = ui_input_get_text(layout, (const UIInput *) ui_element_get(layout, picker->inputs[i]));

        uint32 value = 0;
        while (*text >= '0' && *text <= '9') {
            value = value * 10 + (uint32) (*text - '0');
            ++text;
        }

        channels[i] = oms_min(value, (uint32) 255);
    }

    const uint32 color = ui_color_rgba(channels[0], channels[1], channels[2], channels[3]);
    if (color == picker->color) {
        return false;
    }

    // Not ui_color_picker_set() since that would overwrite the input that is currently edited
    picker->color = color;
    ui_color_to_hsv(color, &picker->hue, &picker->saturation, &picker->value);
    picker->core.change_type |= UI_ELEMENT_CHANGE_CONTENT;
    layout->dirty |= UI_LAYOUT_DIRTY;

    return true;
}

/**
 * @param UIColorPickerType type            UIColorPickerType
 * @param const uint32*     palette         Palette colors (rgba) in display order: UI_COLOR_PALETTE_COLUMNS cells per row, 0 = empty cell
 *                                          NULL = ansi colors (UI_COLOR_PALETTE_ANSI), only used by UI_COLOR_PICKER_TYPE_PALETTE
 * @param int32             palette_count   Amount of cells
 * @param UICore*           parent          NULL = root
 */
inline
UIColorPicker* ui_color_picker_create(
    UILayout* const layout,
    UIColorPickerType type = UI_COLOR_PICKER_TYPE_GRADIENT,
    UICore* parent = NULL,
    const uint32* palette = NULL, int32 palette_count = 0
) NO_EXCEPT
{
    UIColorPicker* const picker = UI_ELEMENT_ALLOC(layout, UI_ELEMENT_TYPE_COLOR_PICKER, UIColorPicker, parent);
    picker->core.flags = UI_ELEMENT_FLAG_INTERACTABLE | UI_ELEMENT_FLAG_HOVERABLE;
    picker->core.dimension.align_anchor = UI_ANCHOR_ALIGN_TOP_LEFT;
    picker->core.dimension.dim_raw = {40.0f, UI_DEFAULT_ROW_HEIGHT};

    picker->border_color = UI_DEFAULT_CONTROL_BORDER;
    picker->border_color_hover = UI_DEFAULT_CONTROL_BORDER_FOCUS;
    picker->picker_type = type;
    picker->color = 0xFF0000FF;
    picker->hue = 0.0f;
    picker->saturation = 1.0f;
    picker->value = 1.0f;

    if (palette && palette_count > 0) {
        picker->palette = ui_buffer_reserve(layout, palette_count * (int32) sizeof(uint32), alignof(uint32));
        memcpy(layout->ui_element_buffer.memory + picker->palette, palette, palette_count * sizeof(uint32));
        picker->palette_count = (int16) palette_count;
    }

    UIPanel* const popup = ui_popup_create(layout, &picker->core);
    picker->popup = ui_element_offset(layout, popup);

    UIColorPanel* const panel = UI_ELEMENT_ALLOC(layout, UI_ELEMENT_TYPE_COLOR_PANEL, UIColorPanel, &popup->core);
    panel->core.flags = UI_ELEMENT_FLAG_INTERACTABLE | UI_ELEMENT_FLAG_HOVERABLE;
    panel->core.dimension.align_anchor = UI_ANCHOR_ALIGN_TOP_LEFT;
    panel->core.dimension.pos_raw = {UI_COLOR_GAP, UI_COLOR_GAP};
    panel->picker = ui_element_offset(layout, picker);
    panel->hovered = -1;
    picker->panel = ui_element_offset(layout, panel);

    if (type == UI_COLOR_PICKER_TYPE_PALETTE) {
        const int32 rows = (ui_color_picker_cell_count(picker) + UI_COLOR_PALETTE_COLUMNS - 1) / UI_COLOR_PALETTE_COLUMNS;
        panel->core.dimension.dim_raw = {
            UI_COLOR_PALETTE_COLUMNS * UI_COLOR_PALETTE_CELL,
            (f32) rows * UI_COLOR_PALETTE_CELL
        };
    } else {
        // gradient + hue bar + alpha bar, preview below the gradient
        panel->core.dimension.dim_raw = {
            UI_COLOR_GRADIENT_SIZE + 2.0f * (UI_COLOR_GAP + UI_COLOR_BAR_WIDTH),
            UI_COLOR_GRADIENT_SIZE + UI_COLOR_GAP + UI_DEFAULT_ROW_HEIGHT
        };

        // rgba inputs right of the preview
        const char* const names[4] = { "R", "G", "B", "A" };
        const f32 input_width = 38.0f;
        for (int32 i = 0; i < 4; ++i) {
            const f32 x = UI_COLOR_GAP + 48.0f + (input_width + 4.0f) * (f32) i;

            UIInput* const input = ui_input_create(layout, 3, &popup->core);
            input->core.dimension.pos_raw = {x, UI_COLOR_GAP * 2.0f + UI_COLOR_GRADIENT_SIZE};
            input->core.dimension.dim_raw = {input_width, UI_DEFAULT_ROW_HEIGHT};
            input->input_type = UI_INPUT_TYPE_NUMERIC;
            input->owner = ui_element_offset(layout, picker);
            picker->inputs[i] = ui_element_offset(layout, input);

            UILabel* const label = ui_label_create(layout, CHAR_TYPE_CHAR, 0, 0, &popup->core);
            label->core.dimension.pos_raw = {x + 2.0f, UI_COLOR_GAP * 2.0f + UI_COLOR_GRADIENT_SIZE + UI_DEFAULT_ROW_HEIGHT + 2.0f};
            label->font.color = UI_DEFAULT_PLACEHOLDER_COLOR;
            // Single letter names
            ui_label_set_text(layout, label, names[i], 1);
        }

        ui_color_picker_update_inputs(layout, picker);
    }

    // Popup size = panel + padding (+ the label row of the gradient picker)
    popup->core.dimension.dim_raw = {
        panel->core.dimension.dim_raw.x + 2.0f * UI_COLOR_GAP,
        panel->core.dimension.dim_raw.y + 2.0f * UI_COLOR_GAP + (type == UI_COLOR_PICKER_TYPE_GRADIENT ? 16.0f : 0.0f)
    };

    return picker;
}

////////////////////////////////////////////////////////////////
// Color panel (gradient or palette in the popup)
////////////////////////////////////////////////////////////////

// Rects of the gradient areas (x, y = top left)
CONSTEXPR inline
v4_f32 ui_color_panel_area_rect(const UIColorPanel* const panel, UIColorPanelArea area) NO_EXCEPT
{
    const UIAttributeDimension* const dim = &panel->core.dimension;

    switch (area) {
        case UI_COLOR_PANEL_AREA_HUE:
            return {
                dim->pos.x + UI_COLOR_GRADIENT_SIZE + UI_COLOR_GAP,
                dim->pos.y,
                UI_COLOR_BAR_WIDTH,
                UI_COLOR_GRADIENT_SIZE
            };
        case UI_COLOR_PANEL_AREA_ALPHA:
            return {
                dim->pos.x + UI_COLOR_GRADIENT_SIZE + 2.0f * UI_COLOR_GAP + UI_COLOR_BAR_WIDTH,
                dim->pos.y,
                UI_COLOR_BAR_WIDTH,
                UI_COLOR_GRADIENT_SIZE
            };
        default:
            return {dim->pos.x, dim->pos.y, UI_COLOR_GRADIENT_SIZE, UI_COLOR_GRADIENT_SIZE};
    }
}

/**
 * @return int32 Grid cell at the position or -1
 */
inline
int32 ui_color_panel_cell_at(const UILayout* const __restrict layout, const UIColorPanel* const __restrict panel, f32 x, f32 y) NO_EXCEPT
{
    const UIColorPicker* const picker = (UIColorPicker *) ui_element_get(layout, panel->picker);
    if (picker->picker_type != UI_COLOR_PICKER_TYPE_PALETTE
        || !ui_dimension_contains(&panel->core.dimension, x, y)
    ) {
        return -1;
    }

    const int32 column = (int32) ((x - panel->core.dimension.pos.x) / UI_COLOR_PALETTE_CELL);
    const int32 row = (int32) ((panel->core.dimension.pos.y - y) / UI_COLOR_PALETTE_CELL);
    const int32 cell = row * UI_COLOR_PALETTE_COLUMNS + column;

    return column < UI_COLOR_PALETTE_COLUMNS
        && ui_color_picker_cell_color(layout, picker, cell) ? cell : -1;
}

inline
bool ui_color_panel_hover(UILayout* const layout, UIColorPanel* const panel, int32 index) NO_EXCEPT
{
    if (index == panel->hovered) {
        return false;
    }

    panel->hovered = (int16) index;
    panel->core.change_type |= UI_ELEMENT_CHANGE_CONTENT;
    layout->dirty |= UI_LAYOUT_DIRTY;

    return true;
}

/**
 * Gradient: changes the color based on the pointer position (pressing and dragging)
 *
 * @param bool start true = pointer pressed (decides which area is dragged), false = pointer moved while pressed
 *
 * @return bool true if the color changed
 */
inline
bool ui_color_panel_set_from_position(UILayout* const layout, UIColorPanel* const panel, f32 x, f32 y, bool start) NO_EXCEPT
{
    UIColorPicker* const picker = (UIColorPicker *) ui_element_get(layout, panel->picker);
    if (picker->picker_type != UI_COLOR_PICKER_TYPE_GRADIENT) {
        return false;
    }

    if (start) {
        panel->drag_area = UI_COLOR_PANEL_AREA_NONE;
        for (byte area = UI_COLOR_PANEL_AREA_GRADIENT; area <= UI_COLOR_PANEL_AREA_ALPHA; ++area) {
            const v4_f32 rect = ui_color_panel_area_rect(panel, (UIColorPanelArea) area);
            // is x/y in rect?
            if (x >= rect.x && x < rect.x + rect.width && y <= rect.y && y > rect.y - rect.height) {
                panel->drag_area = area;
                break;
            }
        }
    }

    if (panel->drag_area == UI_COLOR_PANEL_AREA_NONE) {
        return false;
    }

    const v4_f32 rect = ui_color_panel_area_rect(panel, (UIColorPanelArea) panel->drag_area);
    const f32 fx = oms_clamp((x - rect.x) / rect.width, 0.0f, 1.0f);
    const f32 fy = oms_clamp((y - (rect.y - rect.height)) / rect.height, 0.0f, 1.0f); // 0 = bottom

    f32 h = picker->hue;
    f32 s = picker->saturation;
    f32 v = picker->value;
    f32 a = (f32) ui_color_channel(picker->color, 3) / 255.0f;

    switch (panel->drag_area) {
        case UI_COLOR_PANEL_AREA_GRADIENT: {
            s = fx;
            v = fy;
        } break;
        case UI_COLOR_PANEL_AREA_HUE: {
            // red at the top
            h = oms_min(1.0f - fy, 0.9999f);
        } break;
        case UI_COLOR_PANEL_AREA_ALPHA: {
            a = fy;
        } break;
        default: {}
    }

    return ui_color_picker_set_hsv(layout, picker, h, s, v, a);
}

/**
 * Palette: picks the color at the position
 *
 * @return bool true if the color changed
 */
inline
bool ui_color_panel_pick(UILayout* const layout, UIColorPanel* const panel, f32 x, f32 y) NO_EXCEPT
{
    UIColorPicker* const picker = (UIColorPicker *) ui_element_get(layout, panel->picker);
    const uint32 color = ui_color_picker_cell_color(layout, picker, ui_color_panel_cell_at(layout, panel, x, y));
    if (!color) {
        return false;
    }

    return ui_color_picker_set(layout, picker, color);
}

////////////////////////////////////////////////////////////////
// Rendering
////////////////////////////////////////////////////////////////

// Checkerboard background to visualize transparency
// Every cell is its own rect (no overlapping geometry -> one zindex is enough)
inline
void ui_color_checker_vertices(
    v4_f32 rect, f32 cell,
    ArrayVector<Vertex3DSamplerTextureColor>* const __restrict vertices,
    ArrayVector<int32>* const __restrict indices,
    f32 zindex
) NO_EXCEPT
{
    const int32 columns = (int32) (rect.width / cell + 0.999f);
    const int32 rows = (int32) (rect.height / cell + 0.999f);
    for (int32 r = 0; r < rows; ++r) {
        for (int32 c = 0; c < columns; ++c) {
            ui_rect_vertices(
                {
                    rect.x + (f32) c * cell,
                    rect.y - (f32) r * cell,
                    oms_min(cell, rect.width - (f32) c * cell),
                    oms_min(cell, rect.height - (f32) r * cell)
                },
                ((r + c) & 1) ? 0x666666FF : 0x999999FF, vertices, indices, zindex
            );
        }
    }
}

void ui_color_picker_cache(
    UIColorPicker* const picker,
    GpuApiType gpu_api_type,
    UILayout* const layout,
    f32* const zindex
) NO_EXCEPT
{
    PROFILE_DEBUG(PROFILE_UI_CACHE_COLOR_PICKER);

    ui_dimension_calculate(layout, &picker->core);
    const v4_f32 rect = ui_dimension_rect(&picker->core.dimension);

    ArrayVector<Vertex3DSamplerTextureColor>* const vertices = &layout->ui_vertex_cache;
    ArrayVector<int32>* const indices = &layout->ui_index_cache;

    const v4_f32 swatch = {rect.x + 3.0f, rect.y - 3.0f, rect.width - 6.0f, rect.height - 6.0f};
    ui_color_checker_vertices(swatch, 6.0f, vertices, indices, *zindex);
    *zindex = camera_step_closer(gpu_api_type, *zindex);

    ui_rect_vertices(swatch, picker->color, vertices, indices, *zindex);

    const bool is_open = picker->popup && layout->interaction.popup == picker->popup;
    ui_border_solid_vertices(
        rect,
        (is_open || (picker->core.state & UI_STATE_HOVER)) ? picker->border_color_hover : picker->border_color,
        1,
        vertices, indices, *zindex
    );

    *zindex = camera_step_closer(gpu_api_type, *zindex);
}

void ui_color_panel_cache(
    UIColorPanel* const panel,
    GpuApiType gpu_api_type,
    UILayout* const layout,
    f32* const zindex
) NO_EXCEPT
{
    PROFILE_DEBUG(PROFILE_UI_CACHE_COLOR_PANEL);

    ui_dimension_calculate(layout, &panel->core);
    const UIAttributeDimension* const dim = &panel->core.dimension;
    const UIColorPicker* const picker = (UIColorPicker *) ui_element_get(layout, panel->picker);

    ArrayVector<Vertex3DSamplerTextureColor>* const vertices = &layout->ui_vertex_cache;
    ArrayVector<int32>* const indices = &layout->ui_index_cache;

    if (picker->picker_type == UI_COLOR_PICKER_TYPE_PALETTE) {
        // Cells in display order
        const int32 cells = ui_color_picker_cell_count(picker);
        for (int32 cell = 0; cell < cells; ++cell) {
            const uint32 color = ui_color_picker_cell_color(layout, picker, cell);
            if (!color) {
                continue;
            }

            ui_rect_vertices(
                {
                    dim->pos.x + (f32) (cell % UI_COLOR_PALETTE_COLUMNS) * UI_COLOR_PALETTE_CELL,
                    dim->pos.y - (f32) (cell / UI_COLOR_PALETTE_COLUMNS) * UI_COLOR_PALETTE_CELL,
                    UI_COLOR_PALETTE_CELL - 1.0f, UI_COLOR_PALETTE_CELL - 1.0f
                },
                color, vertices, indices, *zindex
            );
        }

        // Selected cell, then the hovered cell on top (own zindex since the frames may overlap)
        for (int32 pass = 0; pass < 2; ++pass) {
            *zindex = camera_step_closer(gpu_api_type, *zindex);

            for (int32 cell = 0; cell < cells; ++cell) {
                const bool show = pass == 0
                    ? picker->color && ui_color_picker_cell_color(layout, picker, cell) == picker->color
                    : cell == panel->hovered;

                if (!show) {
                    continue;
                }

                ui_border_solid_vertices(
                    {
                        dim->pos.x + (f32) (cell % UI_COLOR_PALETTE_COLUMNS) * UI_COLOR_PALETTE_CELL - 1.0f,
                        dim->pos.y - (f32) (cell / UI_COLOR_PALETTE_COLUMNS) * UI_COLOR_PALETTE_CELL + 1.0f,
                        UI_COLOR_PALETTE_CELL + 1.0f, UI_COLOR_PALETTE_CELL + 1.0f
                    },
                    pass == 0 ? UI_DEFAULT_CONTROL_ACCENT : 0xFFFFFFFF, 2, vertices, indices, *zindex
                );
            }
        }

        *zindex = camera_step_closer(gpu_api_type, *zindex);

        return;
    }

    // Saturation (x) / value (y) gradient of the current hue
    const v4_f32 gradient = ui_color_panel_area_rect(panel, UI_COLOR_PANEL_AREA_GRADIENT);
    const f32 cell = UI_COLOR_GRADIENT_SIZE / UI_COLOR_GRADIENT_CELLS;
    for (int32 r = 0; r < UI_COLOR_GRADIENT_CELLS; ++r) {
        const f32 v = 1.0f - ((f32) r + 0.5f) / UI_COLOR_GRADIENT_CELLS;
        for (int32 c = 0; c < UI_COLOR_GRADIENT_CELLS; ++c) {
            const f32 s = ((f32) c + 0.5f) / UI_COLOR_GRADIENT_CELLS;
            ui_rect_vertices(
                {gradient.x + (f32) c * cell, gradient.y - (f32) r * cell, cell, cell},
                ui_color_from_hsv(picker->hue, s, v), vertices, indices, *zindex
            );
        }
    }

    // Hue bar (red at the top)
    const v4_f32 hue = ui_color_panel_area_rect(panel, UI_COLOR_PANEL_AREA_HUE);
    const f32 hue_cell = hue.height / UI_COLOR_HUE_CELLS;
    for (int32 i = 0; i < UI_COLOR_HUE_CELLS; ++i) {
        ui_rect_vertices(
            {hue.x, hue.y - (f32) i * hue_cell, hue.width, hue_cell},
            ui_color_from_hsv(((f32) i + 0.5f) / UI_COLOR_HUE_CELLS, 1.0f, 1.0f), vertices, indices, *zindex
        );
    }

    // Alpha bar (opaque at the top) over a checkerboard
    const v4_f32 alpha = ui_color_panel_area_rect(panel, UI_COLOR_PANEL_AREA_ALPHA);
    ui_color_checker_vertices(alpha, alpha.width * 0.5f, vertices, indices, *zindex);

    // Preview of the current color below the gradient
    const v4_f32 preview = {
        dim->pos.x,
        dim->pos.y - UI_COLOR_GRADIENT_SIZE - UI_COLOR_GAP,
        40.0f,
        UI_DEFAULT_ROW_HEIGHT
    };
    ui_color_checker_vertices(preview, 6.0f, vertices, indices, *zindex);

    *zindex = camera_step_closer(gpu_api_type, *zindex);

    const uint32 rgb = picker->color & 0xFFFFFF00;
    const f32 alpha_cell = alpha.height / UI_COLOR_GRADIENT_CELLS;
    for (int32 i = 0; i < UI_COLOR_GRADIENT_CELLS; ++i) {
        const uint32 a = (uint32) ((1.0f - ((f32) i + 0.5f) / UI_COLOR_GRADIENT_CELLS) * 255.0f);
        ui_rect_vertices(
            {alpha.x, alpha.y - (f32) i * alpha_cell, alpha.width, alpha_cell},
            rgb | a,
            vertices, indices,
            *zindex
        );
    }

    ui_rect_vertices(preview, picker->color, vertices, indices, *zindex);

    *zindex = camera_step_closer(gpu_api_type, *zindex);

    ui_border_solid_vertices(preview, UI_DEFAULT_CONTROL_BORDER, 1, vertices, indices, *zindex);

    // Markers of the current values
    const f32 marker_x = gradient.x + picker->saturation * gradient.width;
    const f32 marker_y = gradient.y - (1.0f - picker->value) * gradient.height;
    ui_border_solid_vertices(
        {marker_x - 4.0f, marker_y + 4.0f, 8.0f, 8.0f},
        0x000000FF,
        1,
        vertices, indices,
        *zindex
    );
    ui_border_solid_vertices(
        {marker_x - 3.0f, marker_y + 3.0f, 6.0f, 6.0f},
        0xFFFFFFFF,
        1,
        vertices, indices,
        *zindex
    );

    const f32 hue_y = hue.y - picker->hue * hue.height;
    ui_rect_vertices(
        {hue.x - 2.0f, hue_y + 1.5f, hue.width + 4.0f, 3.0f},
        0xFFFFFFFF,
        vertices, indices,
        *zindex
    );

    const f32 alpha_y = alpha.y - (1.0f - (f32) ui_color_channel(picker->color, 3) / 255.0f) * alpha.height;
    ui_rect_vertices(
        {alpha.x - 2.0f, alpha_y + 1.5f, alpha.width + 4.0f, 3.0f},
        0xFFFFFFFF,
        vertices, indices,
        *zindex
    );

    *zindex = camera_step_closer(gpu_api_type, *zindex);
}

#endif
