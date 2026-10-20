#pragma once
#ifndef COMS_UI_CHECKBOX_H
#define COMS_UI_CHECKBOX_H

#include "../stdlib/Stdlib.h"
#include "UICore.h"

// Used for UI_ELEMENT_TYPE_CHECKBOX and UI_ELEMENT_TYPE_RADIO
// The optional text is a UILabel child element right of the box
// 88 bytes
struct UICheckbox {
    UICore core;

    // RGBA
    uint32 box_color;
    uint32 border_color;
    uint32 border_color_hover;
    uint32 check_color;

    // 1-indexed action that is called when the value changes (layout->on_actions, 0 = none)
    int32 on_change;

    // Offset of the label child (0 = none)
    int32 label;

    // Radio only: radios with the same group and the same parent are mutually exclusive
    int16 group;

    bool checked;
};

// A radio button is a checkbox that can only be unchecked by checking another radio of the same group
typedef UICheckbox UIRadio;

#endif
