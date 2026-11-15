#pragma once
#ifndef COMS_UI_NAVIGATION_H
#define COMS_UI_NAVIGATION_H

#include "../stdlib/Stdlib.h"

// Device independent ui actions
// The keyboard (ui_input_code()) and the hotkeys of any device (ui_input_navigate()) are converted to these
enum UINavigation : byte {
    UI_NAVIGATION_NONE,

    // Next/previous focusable element, also starts the navigation if nothing has the focus
    UI_NAVIGATION_NEXT,
    UI_NAVIGATION_PREV,

    UI_NAVIGATION_UP,
    UI_NAVIGATION_DOWN,
    UI_NAVIGATION_LEFT,
    UI_NAVIGATION_RIGHT,

    // e.g. min/max of a slider, first/last row of a table, start/end of the text
    UI_NAVIGATION_FIRST,
    UI_NAVIGATION_LAST,

    // e.g. click a button, open a select, submit an input
    UI_NAVIGATION_CONFIRM,

    // e.g. close the popup, stop editing, remove the focus
    UI_NAVIGATION_CANCEL,

    UI_NAVIGATION_CURSOR_MOVEMENT1,
    UI_NAVIGATION_POINTER_PRIMARY_UI,
    UI_NAVIGATION_POINTER_SECONDARY_UI,
    UI_NAVIGATION_SCROLL_UP_UI,
    UI_NAVIGATION_SCROLL_DOWN_UI,

    UI_NAVIGATION_SIZE
};

#endif
