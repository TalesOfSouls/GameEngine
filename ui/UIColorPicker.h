#pragma once
#ifndef COMS_UI_COLOR_PICKER_H
#define COMS_UI_COLOR_PICKER_H

#include "../stdlib/Stdlib.h"
#include "UICore.h"

enum UIColorPickerType : byte {
    // Saturation/value gradient + hue bar + alpha bar + rgba input fields
    UI_COLOR_PICKER_TYPE_GRADIENT,

    // Grid of predefined colors (default: the 256 ansi colors)
    UI_COLOR_PICKER_TYPE_PALETTE,
};

// Color swatch that opens a color popup when clicked
//
// Tree (created by ui_color_picker_create):
//      color picker (swatch in your layout)
//      popup (UIPanel, child of the root)
//          color panel (UIColorPanel = gradient or palette)
//          r, g, b, a inputs + labels (gradient only)
//
// 136 bytes
struct UIColorPicker {
    UICore core;

    // RGBA
    uint32 color;

    uint32 border_color;
    uint32 border_color_hover;

    // HSV of the color (0.0 - 1.0), the hue is kept for grey colors where it can't be calculated from the rgb value
    f32 hue;
    f32 saturation;
    f32 value;

    // Offsets of the popup children (0 = none)
    int32 popup;
    int32 panel;
    int32 inputs[4];

    // Palette colors (uint32 rgba), 0 = 256 ansi colors
    int32 palette;

    // Display order of the palette (int16 palette index per grid cell, -1 = empty cell)
    // The colors are shown as a gradient (@see ui_color_palette_order), duplicates are removed
    int32 palette_order;
    int16 palette_cells;

    // 1-indexed action that is called when the color changes (layout->on_actions, 0 = none)
    int32 on_change;

    int16 palette_count;

    // @see UIColorPickerType
    byte picker_type;
};

// Gradient or palette area in the popup of a color picker
// 72 bytes
struct UIColorPanel {
    UICore core;

    // Offset of the color picker
    int32 picker;

    // Palette cell under the pointer (-1 = none)
    int16 hovered;

    // Gradient area that is dragged (@see UIColorPanelArea)
    byte drag_area;
};

#endif
