#pragma once
#ifndef COMS_UI_ELEMENT_TYPE_H
#define COMS_UI_ELEMENT_TYPE_H

#include "../stdlib/Stdlib.h"

enum UIElementType : byte {
    UI_ELEMENT_TYPE_BUTTON,
    UI_ELEMENT_TYPE_SELECT,
    UI_ELEMENT_TYPE_INPUT,
    UI_ELEMENT_TYPE_LABEL,
    UI_ELEMENT_TYPE_TEXTAREA,
    UI_ELEMENT_TYPE_IMAGE,
    UI_ELEMENT_TYPE_TEXT,
    UI_ELEMENT_TYPE_LINK,
    UI_ELEMENT_TYPE_TABLE,
    UI_ELEMENT_TYPE_VIEW_WINDOW,
    UI_ELEMENT_TYPE_VIEW_PANEL,
    UI_ELEMENT_TYPE_VIEW_TAB,
    UI_ELEMENT_TYPE_CURSOR,

    // Uses a callback function for update/rendering
    UI_ELEMENT_TYPE_CUSTOM,

    // Doesn't do anything is handled completely manual
    UI_ELEMENT_TYPE_MANUAL,

    // WARNING: New types are appended to keep the values of layout files stable
    UI_ELEMENT_TYPE_CHECKBOX,
    UI_ELEMENT_TYPE_RADIO,
    UI_ELEMENT_TYPE_SLIDER,

    // Option list in the popup of a select
    UI_ELEMENT_TYPE_OPTION_LIST,

    // Color swatch that opens a color popup
    UI_ELEMENT_TYPE_COLOR_PICKER,

    // Gradient or palette area in the popup of a color picker
    UI_ELEMENT_TYPE_COLOR_PANEL,

    // Named buffer of values (user data), never drawn @see UIData.h
    UI_ELEMENT_TYPE_DATA,

    UI_ELEMENT_TYPE_SIZE,
};

#endif