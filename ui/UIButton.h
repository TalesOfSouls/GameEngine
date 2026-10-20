#pragma once
#ifndef COMS_UI_BUTTON_H
#define COMS_UI_BUTTON_H

#include "../stdlib/Stdlib.h"
#include "attribute/UIAttributeImage.h"
#include "UICore.h"

// The optional text is a UILabel child element (@see ui_button_create)
// 92 bytes
struct UIButton {
    UICore core;

    // RGBA, no alpha = no background
    uint32 background_color;

    // RGBA used while the button has UI_STATE_HOVER, no alpha = use background_color
    uint32 background_color_hover;

    // Fills the entire button
    UIAttributeImage image;

    // 1-indexed callback index, same idiom as UICore::update_func (0 = none).
    int32 on_click;

    // Offset of the label child (0 = none)
    int32 label;
};

#endif
