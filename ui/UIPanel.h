#pragma once
#ifndef COMS_UI_PANEL_H
#define COMS_UI_PANEL_H

#include "../stdlib/Stdlib.h"
#include "attribute/UIAttributeBorder.h"
#include "UICore.h"

enum UIPanelFlag : byte {
    // Scrollable content (@see ui_scroll_panel_create)
    UI_PANEL_FLAG_SCROLL_X = 1 << 0,
    UI_PANEL_FLAG_SCROLL_Y = 1 << 1,

    // Children are clipped to the panel (scroll panels always clip)
    UI_PANEL_FLAG_CLIP = 1 << 2,

    // The content child of a scroll panel, it is positioned by the scroll offset of its parent
    UI_PANEL_FLAG_SCROLL_CONTENT = 1 << 3,

    // Hidden until opened (@see ui_popup_open), drawn on top of everything and gets the pointer input first
    // Pressing outside of the popup closes it
    UI_PANEL_FLAG_POPUP = 1 << 4,

    // Popup only: clicking an element in the popup closes it (e.g. context menu)
    UI_PANEL_FLAG_CLOSE_ON_CLICK = 1 << 5,
};

#define UI_PANEL_FLAG_SCROLL (UI_PANEL_FLAG_SCROLL_X | UI_PANEL_FLAG_SCROLL_Y)

// Generic container with an optional background and border
// Also used for the root element, the window title, scroll panels and popups
// 180 bytes
struct UIPanel {
    UICore core;

    // RGBA, no alpha = no background
    uint32 background_color;

    UIAttributeBorder border;

    // @see UIPanelFlag
    byte panel_flags;

    // Scroll panel: offset of the content child (all children of the scroll panel are added to the content)
    int32 content;

    // Scroll panel: scroll offset in px (0 = start, positive = content moved left/up)
    v2_f32 scroll;

    // Scroll panel: size of the content, calculated by ui_cache() from the content children
    v2_f32 content_size;

    // Popup: element the popup is attached to (e.g. select), the popup is placed below it
    // 0 = the popup is placed at the position it was opened at (e.g. context menu)
    int32 popup_owner;
};

#endif
