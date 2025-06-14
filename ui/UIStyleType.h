#ifndef COMS_UI_STYLE_TYPE_H
#define COMS_UI_STYLE_TYPE_H

#include "../stdlib/Types.h"

enum UIStyleType : byte {
    // This can be the same as one of the style_types below but doesn't have to be
    // The most common situation where this is different is for animations
    // In case of animations we often perform interpolations so the active style is different from ANY defined style since it is dynamically calculated based on the time
    UI_STYLE_TYPE_ACTIVE,

    UI_STYLE_TYPE_DEFAULT,
    UI_STYLE_TYPE_HIDDEN, // e.g. useful as starting position for slide-in/blend-in etc. animations
    UI_STYLE_TYPE_FOCUS, // e.g. input
    UI_STYLE_TYPE_DISABLED, // disabled form elements
    UI_STYLE_TYPE_HOVER, // e.g. button
    UI_STYLE_TYPE_MANUAL,
    UI_STYLE_TYPE_SIZE,
};

constexpr
int32 ui_style_type_to_id(const char* str)
{
    if (*str == '\0') {
        return UI_STYLE_TYPE_DEFAULT;
    } else if (str_compare(":hid", str, 4) == 0) {
        return UI_STYLE_TYPE_HIDDEN;
    } else if (str_compare(":foc", str, 4) == 0) {
        return UI_STYLE_TYPE_FOCUS;
    } else if (str_compare(":dis", str, 4) == 0) {
        return UI_STYLE_TYPE_DISABLED;
    } else if (str_compare(":hov", str, 4) == 0) {
        return UI_STYLE_TYPE_HOVER;
    } else {
        return UI_STYLE_TYPE_MANUAL;
    }
}

#endif