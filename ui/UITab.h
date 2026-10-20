#pragma once
#ifndef COMS_UI_TAB_H
#define COMS_UI_TAB_H

#include "../stdlib/Stdlib.h"
#include "attribute/UIAttributeFont.h"
#include "UICore.h"

enum UITabOrientation : byte {
    // Tab headers on top
    UI_TAB_ORIENTATION_HORIZONTAL,

    // Tab headers on the left side
    UI_TAB_ORIENTATION_VERTICAL,
};

// Tab view
//
// Tree (created by ui_tab_create):
//      tab view (renders the tab headers itself)
//          page 1 (UIPanel, only the active page is visible)
//          page 2
//          ...
// Add your elements to the pages (@see ui_tab_page)
//
// 120 bytes
struct UITab {
    UICore core;

    // RGBA
    uint32 header_color;
    uint32 tab_color;
    uint32 tab_color_hover;
    uint32 tab_color_active;
    uint32 page_color;
    uint32 border_color;

    UIAttributeFont font;

    // Offset of the tab titles, every title is \0 terminated
    int32 titles;

    // 1-indexed action that is called when the active tab changes (layout->on_actions, 0 = none)
    int32 on_change;

    // Horizontal: height of the header row, vertical: width of the header column
    f32 header_size;

    // Size of one tab header along the header (horizontal: width, vertical: height)
    // Horizontal tabs have the width of the longest title (calculated by ui_cache)
    f32 tab_size;

    int16 tab_count;
    int16 active;

    // Tab header under the pointer (-1 = none)
    int16 hovered;

    // @see UITabOrientation
    byte orientation;
};

#endif
