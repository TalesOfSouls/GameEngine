#pragma once
#ifndef COMS_UI_NAVIGATION_H
#define COMS_UI_NAVIGATION_H

#include "../stdlib/Stdlib.h"
#include "../input/Input.h"

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

    UI_NAVIGATION_SIZE
};

static_assert(UI_NAVIGATION_SIZE <= 32, "UIInputFrame::navigation is a 32 bit mask");

/**
 * Ui input of one input state for one tick
 *
 * Filled by the application (e.g. its hotkey callbacks), read by ui_input() + ui_input_navigation()
 * -> the ui doesn't know the hotkeys of the application.
 * MUST be cleared at the beginning of every tick (everything only describes the current tick)
 */
struct UIInputFrame {
    // Navigation actions of this tick, bit = 1 << UINavigation
    uint32 navigation;

    // KeyPressType of the pointer buttons this tick (0 = not active)
    // A press + release within one tick is PRESSED | RELEASED
    byte primary;
    byte secondary;

    // The input moves a virtual cursor this tick (e.g. controller), its pointer counts as moved like a mouse
    bool cursor_movement;

    // Scroll movement of this tick in notches (positive = away from the user)
    f32 scroll;

    // Signed scroll key (e.g. the mouse wheel) that was already added to scroll this tick
    // The same key can be bound to both scroll directions (the sign of its value is the direction)
    int16 scroll_code;
};

// Pointer button is down (pressed or held, a press + release within one tick is not down)
CONSTEXPR FORCE_INLINE
bool ui_input_frame_down(byte state) NO_EXCEPT
{
    return (state & KEY_PRESS_TYPE_HELD) || ((state & KEY_PRESS_TYPE_PRESSED) && !(state & KEY_PRESS_TYPE_RELEASED));
}

#endif
