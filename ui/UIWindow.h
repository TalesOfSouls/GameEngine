#pragma once
#ifndef COMS_UI_WINDOW_H
#define COMS_UI_WINDOW_H

#include "../stdlib/Stdlib.h"
#include "attribute/UIAttributeBorder.h"
#include "UICore.h"

// A window is created together with its fixed children which are allocated directly after the window (pre-order):
//      UIWindow
//          UIPanel  title (movable)
//              UILabel  title label
//              UIButton close button
//          ... content added later
//          UIPanel  resize grip (optional, @see ui_window_set_resizable)
//
// The children are real elements in the tree, the offsets below are just shortcuts
// 176 bytes (the previous version with embedded children was 4368 bytes)
struct UIWindow {
    UICore core;

    // RGBA, no alpha = no background
    uint32 background_color;

    UIAttributeBorder border;

    // Offsets of the fixed children (0 = doesn't exist)
    int32 title;
    int32 title_label;
    int32 button_close;
    int32 resize_grip;

    // Min size while resizing
    v2_f32 min_dim;
};

#endif
