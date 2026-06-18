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

    UI_ELEMENT_TYPE_SIZE,
};

enum UIElementState : byte {
    UI_ELEMENT_STATE_ACTIVE = 1 << 0,
    UI_ELEMENT_STATE_VISIBLE = 1 << 1,
    UI_ELEMENT_STATE_FOCUSED = 1 << 2,
    UI_ELEMENT_STATE_CLICKABLE = 1 << 3,

    // Are we currently in an animation?
    // @question Do we even need this? Can't we just use the animation state (start == 0)
    UI_ELEMENT_STATE_ANIMATION = 1 << 4,

    // Flag to indicate that the element changed
    // Just checking style_old and style_new is not enough, since we may have an ongoing animation
    // We also need to be able to check the parent element in case the parent element changed position
    //      -> we also need to change position of all child elements (parent sets this flag for all child elements)
    UI_ELEMENT_STATE_CHANGED = 1 << 5,
};

#endif