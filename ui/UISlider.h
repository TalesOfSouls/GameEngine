#pragma once
#ifndef COMS_UI_SLIDER_H
#define COMS_UI_SLIDER_H

#include "../stdlib/Stdlib.h"
#include "UICore.h"

// Horizontal slider, the value is changed by pressing/dragging the slider
// 92 bytes
struct UISlider {
    UICore core;

    // RGBA
    uint32 track_color;
    uint32 fill_color;
    uint32 knob_color;
    uint32 knob_color_hover;

    // 0.0 - 1.0
    f32 value;

    // 0.0 = continuous, otherwise the value snaps to multiples of step (e.g. 0.1)
    f32 step;

    // 1-indexed action that is called when the value changes (layout->on_actions, 0 = none)
    int32 on_change;
};

#endif
