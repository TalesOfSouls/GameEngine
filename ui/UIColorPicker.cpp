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

FORCE_INLINE
uint32 ui_color_rgba(uint32 r, uint32 g, uint32 b, uint32 a) NO_EXCEPT
{
    return (r << 24) | (g << 16) | (b << 8) | a;
}

FORCE_INLINE
uint32 ui_color_channel(uint32 color, int32 channel) NO_EXCEPT
{
    return (color >> (24 - 8 * channel)) & 0xFF;
}

// h, s, v, a = 0.0 - 1.0
inline
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
        case 0: r = v; g = t; b = p; break;
        case 1: r = q; g = v; b = p; break;
        case 2: r = p; g = v; b = t; break;
        case 3: r = p; g = q; b = v; break;
        case 4: r = t; g = p; b = v; break;
        default: r = v; g = p; b = q; break;
    }

    return ui_color_rgba(
        (uint32) (r * 255.0f + 0.5f),
        (uint32) (g * 255.0f + 0.5f),
        (uint32) (b * 255.0f + 0.5f),
        (uint32) (clamp_branched(a, 0.0f, 1.0f) * 255.0f + 0.5f)
    );
}

/**
 * @param hue Is only changed if the color has a hue (not grey)
 */
inline
void ui_color_to_hsv(uint32 color, f32* const hue, f32* const saturation, f32* const value) NO_EXCEPT
{
    const f32 r = (f32) ui_color_channel(color, 0) / 255.0f;
    const f32 g = (f32) ui_color_channel(color, 1) / 255.0f;
    const f32 b = (f32) ui_color_channel(color, 2) / 255.0f;

    const f32 max = max_branched(r, max_branched(g, b));
    const f32 min = min_branched(r, min_branched(g, b));
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
 * The 256 ansi (xterm) colors
 *      0 - 15:    system colors
 *      16 - 231:  6x6x6 color cube
 *      232 - 255: grey ramp
 */
inline
uint32 ui_color_ansi256(int32 index) NO_EXCEPT
{
    static const uint32 system_colors[16] = {
        0x000000FF, 0x800000FF, 0x008000FF, 0x808000FF, 0x000080FF, 0x800080FF, 0x008080FF, 0xC0C0C0FF,
        0x808080FF, 0xFF0000FF, 0x00FF00FF, 0xFFFF00FF, 0x0000FFFF, 0xFF00FFFF, 0x00FFFFFF, 0xFFFFFFFF,
    };

    if (index < 16) {
        return system_colors[index < 0 ? 0 : index];
    }

    if (index < 232) {
        static const uint32 levels[6] = { 0, 95, 135, 175, 215, 255 };
        const int32 i = index - 16;

        return ui_color_rgba(levels[i / 36], levels[(i / 6) % 6], levels[i % 6], 255);
    }

    const uint32 grey = 8 + 10 * (uint32) (min_branched(index, 255) - 232);

    return ui_color_rgba(grey, grey, grey, 255);
}

////////////////////////////////////////////////////////////////
// Color picker
////////////////////////////////////////////////////////////////

FORCE_INLINE
int32 ui_color_picker_palette_count(const UIColorPicker* const picker) NO_EXCEPT
{
    return picker->palette ? picker->palette_count : 256;
}

inline
uint32 ui_color_picker_palette_color(const UILayout* const layout, const UIColorPicker* const picker, int32 index) NO_EXCEPT
{
    return picker->palette
        ? ((const uint32 *) (layout->ui_element_buffer.memory + picker->palette))[index]
        : ui_color_ansi256(index);
}

/**
 * Calculates the display order of a palette as a gradient
 *      1. duplicate colors are removed
 *      2. grey colors first (dark -> light, row by row)
 *      3. other colors: columns = hue (equal amount of colors per column), rows = light -> dark
 *
 * @param order Output: palette index per grid cell (-1 = empty), size >= count + 2 * UI_COLOR_PALETTE_COLUMNS
 *
 * @return Amount of grid cells (multiple of UI_COLOR_PALETTE_COLUMNS)
 */
inline
int32 ui_color_palette_order(const uint32* const colors, int32 count, int16* const order) NO_EXCEPT
{
    // Work arrays (palettes are small, max 1024 colors)
    int16 greys[1024];
    int16 chroma[1024];
    f32 keys[1024];
    int32 grey_count = 0;
    int32 chroma_count = 0;

    count = min_branched(count, 1024);

    for (int32 i = 0; i < count; ++i) {
        bool duplicate = false;
        for (int32 j = 0; j < i; ++j) {
            if (colors[j] == colors[i]) {
                duplicate = true;
                break;
            }
        }

        if (duplicate) {
            continue;
        }

        f32 h = 0.0f, s = 0.0f, v = 0.0f;
        ui_color_to_hsv(colors[i], &h, &s, &v);

        if (s < 0.1f) {
            greys[grey_count++] = (int16) i;
        } else {
            chroma[chroma_count++] = (int16) i;
        }
    }

    // Insertion sort by key (ascending)
    auto sort_by_key = [&](int16* const items, int32 n) {
        for (int32 i = 1; i < n; ++i) {
            const int16 item = items[i];
            const f32 key = keys[item];

            int32 j = i - 1;
            while (j >= 0 && keys[items[j]] > key) {
                items[j + 1] = items[j];
                --j;
            }

            items[j + 1] = item;
        }
    };

    // Greys: dark -> light
    for (int32 i = 0; i < grey_count; ++i) {
        f32 h = 0.0f, s = 0.0f, v = 0.0f;
        ui_color_to_hsv(colors[greys[i]], &h, &s, &v);
        keys[greys[i]] = v;
    }

    sort_by_key(greys, grey_count);

    // Colors: by hue
    for (int32 i = 0; i < chroma_count; ++i) {
        f32 h = 0.0f, s = 0.0f, v = 0.0f;
        ui_color_to_hsv(colors[chroma[i]], &h, &s, &v);
        keys[chroma[i]] = h;
    }

    sort_by_key(chroma, chroma_count);

    // Every column gets the same amount of colors, a column goes from light (top) to dark (bottom)
    for (int32 i = 0; i < chroma_count; ++i) {
        f32 h = 0.0f, s = 0.0f, v = 0.0f;
        ui_color_to_hsv(colors[chroma[i]], &h, &s, &v);
        keys[chroma[i]] = -(v * (1.0f - s * 0.5f)); // hsl lightness, descending
    }

    int32 rows = 0;
    for (int32 c = 0; c < UI_COLOR_PALETTE_COLUMNS; ++c) {
        const int32 start = c * chroma_count / UI_COLOR_PALETTE_COLUMNS;
        const int32 end = (c + 1) * chroma_count / UI_COLOR_PALETTE_COLUMNS;
        sort_by_key(chroma + start, end - start);
        rows = max_branched(rows, end - start);
    }

    // Grid
    int32 cell = 0;
    const int32 grey_cells = ((grey_count + UI_COLOR_PALETTE_COLUMNS - 1) / UI_COLOR_PALETTE_COLUMNS) * UI_COLOR_PALETTE_COLUMNS;
    for (; cell < grey_cells; ++cell) {
        order[cell] = cell < grey_count ? greys[cell] : -1;
    }

    for (int32 r = 0; r < rows; ++r) {
        for (int32 c = 0; c < UI_COLOR_PALETTE_COLUMNS; ++c) {
            const int32 start = c * chroma_count / UI_COLOR_PALETTE_COLUMNS;
            const int32 end = (c + 1) * chroma_count / UI_COLOR_PALETTE_COLUMNS;

            order[cell++] = start + r < end ? chroma[start + r] : -1;
        }
    }

    return cell;
}

// Palette index of a grid cell (-1 = empty cell)
FORCE_INLINE
int32 ui_color_picker_cell_index(const UILayout* const layout, const UIColorPicker* const picker, int32 cell) NO_EXCEPT
{
    if (cell < 0 || cell >= picker->palette_cells) {
        return -1;
    }

    return ((const int16 *) (layout->ui_element_buffer.memory + picker->palette_order))[cell];
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
        text[length] = '\0';

        ui_input_set_text(layout, (UIInput *) ui_element_get(layout, picker->inputs[i]), text);
    }
}

/**
 * Sets the color (rgba)
 *
 * @return true if the color changed
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
    ui_layout_mark_dirty(layout);

    return true;
}

/**
 * Sets the color by hsv (keeps the hue even for grey colors)
 *
 * @return true if the color changed
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
        ui_layout_mark_dirty(layout);
    }

    return changed;
}

/**
 * Reads the rgba input fields (called when one of them changed @see ui_input_owner_changed)
 *
 * @return true if the color changed
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

        channels[i] = min_branched(value, (uint32) 255);
    }

    const uint32 color = ui_color_rgba(channels[0], channels[1], channels[2], channels[3]);
    if (color == picker->color) {
        return false;
    }

    // Not ui_color_picker_set() since that would overwrite the input that is currently edited
    picker->color = color;
    ui_color_to_hsv(color, &picker->hue, &picker->saturation, &picker->value);
    picker->core.change_type |= UI_ELEMENT_CHANGE_CONTENT;
    ui_layout_mark_dirty(layout);

    return true;
}

/**
 * @param type @see UIColorPickerType
 * @param palette Palette colors (rgba), NULL = 256 ansi colors (only used by UI_COLOR_PICKER_TYPE_PALETTE)
 * @param parent NULL = root
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
        picker->palette = ui_text_reserve(layout, CHAR_TYPE_CHAR, palette_count * (int32) sizeof(uint32));
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
        // Display order (gradient)
        const int32 count = min_branched(ui_color_picker_palette_count(picker), 1024);
        uint32 colors[1024];
        for (int32 i = 0; i < count; ++i) {
            colors[i] = ui_color_picker_palette_color(layout, picker, i);
        }

        int16 order[1024 + 2 * UI_COLOR_PALETTE_COLUMNS];
        const int32 cells = ui_color_palette_order(colors, count, order);

        picker->palette_order = ui_text_reserve(layout, CHAR_TYPE_CHAR, max_branched(cells, 1) * (int32) sizeof(int16));
        memcpy(layout->ui_element_buffer.memory + picker->palette_order, order, cells * sizeof(int16));
        picker->palette_cells = (int16) cells;

        const int32 rows = cells / UI_COLOR_PALETTE_COLUMNS;
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
            ui_label_set_text(layout, label, names[i]);
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

FORCE_INLINE
UIColorPicker* ui_color_panel_picker(const UILayout* const layout, const UIColorPanel* const panel) NO_EXCEPT
{
    return (UIColorPicker *) ui_element_get(layout, panel->picker);
}

// Rects of the gradient areas (x, y = top left)
inline
v4_f32 ui_color_panel_area_rect(const UIColorPanel* const panel, UIColorPanelArea area) NO_EXCEPT
{
    const UIAttributeDimension* const dim = &panel->core.dimension;

    switch (area) {
        case UI_COLOR_PANEL_AREA_HUE:
            return {dim->pos.x + UI_COLOR_GRADIENT_SIZE + UI_COLOR_GAP, dim->pos.y, UI_COLOR_BAR_WIDTH, UI_COLOR_GRADIENT_SIZE};
        case UI_COLOR_PANEL_AREA_ALPHA:
            return {
                dim->pos.x + UI_COLOR_GRADIENT_SIZE + 2.0f * UI_COLOR_GAP + UI_COLOR_BAR_WIDTH, dim->pos.y,
                UI_COLOR_BAR_WIDTH, UI_COLOR_GRADIENT_SIZE
            };
        default:
            return {dim->pos.x, dim->pos.y, UI_COLOR_GRADIENT_SIZE, UI_COLOR_GRADIENT_SIZE};
    }
}

FORCE_INLINE
bool ui_rect_contains(v4_f32 rect, f32 x, f32 y) NO_EXCEPT
{
    return x >= rect.x && x < rect.x + rect.width && y <= rect.y && y > rect.y - rect.height;
}

/**
 * @return Grid cell at the position or -1 (@see ui_color_picker_cell_index)
 */
inline
int32 ui_color_panel_cell_at(const UILayout* const layout, const UIColorPanel* const panel, f32 x, f32 y) NO_EXCEPT
{
    const UIColorPicker* const picker = ui_color_panel_picker(layout, panel);
    if (picker->picker_type != UI_COLOR_PICKER_TYPE_PALETTE || !ui_dimension_contains(&panel->core.dimension, x, y)) {
        return -1;
    }

    const int32 column = (int32) ((x - panel->core.dimension.pos.x) / UI_COLOR_PALETTE_CELL);
    const int32 row = (int32) ((panel->core.dimension.pos.y - y) / UI_COLOR_PALETTE_CELL);
    const int32 cell = row * UI_COLOR_PALETTE_COLUMNS + column;

    return column < UI_COLOR_PALETTE_COLUMNS && ui_color_picker_cell_index(layout, picker, cell) >= 0 ? cell : -1;
}

inline
bool ui_color_panel_hover(UILayout* const layout, UIColorPanel* const panel, int32 index) NO_EXCEPT
{
    if (index == panel->hovered) {
        return false;
    }

    panel->hovered = (int16) index;
    panel->core.change_type |= UI_ELEMENT_CHANGE_CONTENT;
    ui_layout_mark_dirty(layout);

    return true;
}

/**
 * Gradient: changes the color based on the pointer position (pressing and dragging)
 *
 * @param start true = pointer pressed (decides which area is dragged), false = pointer moved while pressed
 *
 * @return true if the color changed
 */
inline
bool ui_color_panel_set_from_position(UILayout* const layout, UIColorPanel* const panel, f32 x, f32 y, bool start) NO_EXCEPT
{
    UIColorPicker* const picker = ui_color_panel_picker(layout, panel);
    if (picker->picker_type != UI_COLOR_PICKER_TYPE_GRADIENT) {
        return false;
    }

    if (start) {
        panel->drag_area = UI_COLOR_PANEL_AREA_NONE;
        for (byte area = UI_COLOR_PANEL_AREA_GRADIENT; area <= UI_COLOR_PANEL_AREA_ALPHA; ++area) {
            if (ui_rect_contains(ui_color_panel_area_rect(panel, (UIColorPanelArea) area), x, y)) {
                panel->drag_area = area;
                break;
            }
        }
    }

    if (panel->drag_area == UI_COLOR_PANEL_AREA_NONE) {
        return false;
    }

    const v4_f32 rect = ui_color_panel_area_rect(panel, (UIColorPanelArea) panel->drag_area);
    const f32 fx = clamp_branched((x - rect.x) / rect.width, 0.0f, 1.0f);
    const f32 fy = clamp_branched((y - (rect.y - rect.height)) / rect.height, 0.0f, 1.0f); // 0 = bottom

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
            h = min_branched(1.0f - fy, 0.9999f);
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
 * @return true if the color changed
 */
inline
bool ui_color_panel_pick(UILayout* const layout, UIColorPanel* const panel, f32 x, f32 y) NO_EXCEPT
{
    UIColorPicker* const picker = ui_color_panel_picker(layout, panel);
    const int32 index = ui_color_picker_cell_index(layout, picker, ui_color_panel_cell_at(layout, panel, x, y));
    if (index < 0) {
        return false;
    }

    return ui_color_picker_set(layout, picker, ui_color_picker_palette_color(layout, picker, index));
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
                    rect.x + (f32) c * cell, rect.y - (f32) r * cell,
                    min_branched(cell, rect.width - (f32) c * cell), min_branched(cell, rect.height - (f32) r * cell)
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
    ui_dimension_calculate(layout, &panel->core);
    const UIAttributeDimension* const dim = &panel->core.dimension;
    const UIColorPicker* const picker = ui_color_panel_picker(layout, panel);

    ArrayVector<Vertex3DSamplerTextureColor>* const vertices = &layout->ui_vertex_cache;
    ArrayVector<int32>* const indices = &layout->ui_index_cache;

    if (picker->picker_type == UI_COLOR_PICKER_TYPE_PALETTE) {
        // Cells in display order (@see ui_color_palette_order)
        for (int32 cell = 0; cell < picker->palette_cells; ++cell) {
            const int32 index = ui_color_picker_cell_index(layout, picker, cell);
            if (index < 0) {
                continue;
            }

            ui_rect_vertices(
                {
                    dim->pos.x + (f32) (cell % UI_COLOR_PALETTE_COLUMNS) * UI_COLOR_PALETTE_CELL,
                    dim->pos.y - (f32) (cell / UI_COLOR_PALETTE_COLUMNS) * UI_COLOR_PALETTE_CELL,
                    UI_COLOR_PALETTE_CELL - 1.0f, UI_COLOR_PALETTE_CELL - 1.0f
                },
                ui_color_picker_palette_color(layout, picker, index), vertices, indices, *zindex
            );
        }

        // Selected cell, then the hovered cell on top (own zindex since the frames may overlap)
        for (int32 pass = 0; pass < 2; ++pass) {
            *zindex = camera_step_closer(gpu_api_type, *zindex);

            for (int32 cell = 0; cell < picker->palette_cells; ++cell) {
                const int32 index = ui_color_picker_cell_index(layout, picker, cell);
                const bool show = pass == 0
                    ? index >= 0 && ui_color_picker_palette_color(layout, picker, index) == picker->color
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
    const v4_f32 preview = {dim->pos.x, dim->pos.y - UI_COLOR_GRADIENT_SIZE - UI_COLOR_GAP, 40.0f, UI_DEFAULT_ROW_HEIGHT};
    ui_color_checker_vertices(preview, 6.0f, vertices, indices, *zindex);

    *zindex = camera_step_closer(gpu_api_type, *zindex);

    const uint32 rgb = picker->color & 0xFFFFFF00;
    const f32 alpha_cell = alpha.height / UI_COLOR_GRADIENT_CELLS;
    for (int32 i = 0; i < UI_COLOR_GRADIENT_CELLS; ++i) {
        const uint32 a = (uint32) ((1.0f - ((f32) i + 0.5f) / UI_COLOR_GRADIENT_CELLS) * 255.0f);
        ui_rect_vertices({alpha.x, alpha.y - (f32) i * alpha_cell, alpha.width, alpha_cell}, rgb | a, vertices, indices, *zindex);
    }

    ui_rect_vertices(preview, picker->color, vertices, indices, *zindex);

    *zindex = camera_step_closer(gpu_api_type, *zindex);

    ui_border_solid_vertices(preview, UI_DEFAULT_CONTROL_BORDER, 1, vertices, indices, *zindex);

    // Markers of the current values
    const f32 marker_x = gradient.x + picker->saturation * gradient.width;
    const f32 marker_y = gradient.y - (1.0f - picker->value) * gradient.height;
    ui_border_solid_vertices({marker_x - 4.0f, marker_y + 4.0f, 8.0f, 8.0f}, 0x000000FF, 1, vertices, indices, *zindex);
    ui_border_solid_vertices({marker_x - 3.0f, marker_y + 3.0f, 6.0f, 6.0f}, 0xFFFFFFFF, 1, vertices, indices, *zindex);

    const f32 hue_y = hue.y - picker->hue * hue.height;
    ui_rect_vertices({hue.x - 2.0f, hue_y + 1.5f, hue.width + 4.0f, 3.0f}, 0xFFFFFFFF, vertices, indices, *zindex);

    const f32 alpha_y = alpha.y - (1.0f - (f32) ui_color_channel(picker->color, 3) / 255.0f) * alpha.height;
    ui_rect_vertices({alpha.x - 2.0f, alpha_y + 1.5f, alpha.width + 4.0f, 3.0f}, 0xFFFFFFFF, vertices, indices, *zindex);

    *zindex = camera_step_closer(gpu_api_type, *zindex);
}

#endif
