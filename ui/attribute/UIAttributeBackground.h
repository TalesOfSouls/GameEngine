#pragma once
#ifndef COMS_UI_ATTRIBUTE_BACKGROUND_STYLE_H
#define COMS_UI_ATTRIBUTE_BACKGROUND_STYLE_H

#include "../../stdlib/Stdlib.h"

enum UIBackgroundStyle : byte {
    UI_BACKGROUND_STYLE_NONE = 1 << 0,
    UI_BACKGROUND_STYLE_COLOR_IMG = 1 << 1, // 0 = color, 1 = img
    UI_BACKGROUND_STYLE_STRETCH_X = 1 << 2, // 0 = none, 1 = stretch
    UI_BACKGROUND_STYLE_STRETCH_Y = 1 << 3, // 0 = none, 1 = stretch
    UI_BACKGROUND_STYLE_REPEAT_X = 1 << 4, // 0 = none, 1 = repeat
    UI_BACKGROUND_STYLE_REPEAT_Y = 1 << 5, // 0 = none, 1 = repeat
};

struct UIAttributeBackground {
    UIBackgroundStyle style;
    union {
        uint32 background_image;
        uint32 color;
    };
};

#endif