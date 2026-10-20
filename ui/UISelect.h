#pragma once
#ifndef COMS_UI_SELECT_H
#define COMS_UI_SELECT_H

#include "../stdlib/Stdlib.h"
#include "attribute/UIAttributeFont.h"
#include "UICore.h"

enum UISelectFlag : byte {
    // The popup has a search field, only the options containing the search text are shown
    UI_SELECT_FLAG_SEARCH = 1 << 0,

    // Multiple options can be selected (shown as checkboxes), the popup stays open while selecting
    UI_SELECT_FLAG_MULTI = 1 << 1,
};

// Max. visible options in the popup, more options are scrollable
#define UI_SELECT_MAX_VISIBLE_OPTIONS 6

// Drop down list
//
// Tree (all created by ui_select_create):
//      select (in your layout)
//      popup (UIPanel, child of the root, @see ui_popup_create)
//          search (UIInput, only with UI_SELECT_FLAG_SEARCH)
//          option list (UIOptionList)
//
// 144 bytes
struct UISelect {
    UICore core;

    // RGBA
    uint32 background_color;
    uint32 border_color;
    uint32 border_color_hover;
    uint32 option_hover_color;
    uint32 check_color;

    UIAttributeFont font;

    // Offset of the option strings, every option is \0 terminated
    int32 options;

    // Offset of the selection bit set (1 bit per option), only with UI_SELECT_FLAG_MULTI
    int32 multi_selected;

    // 1-indexed action that is called when the selection changes (layout->on_actions, 0 = none)
    int32 on_change;

    // Offsets of the popup and its children (0 = none)
    int32 popup;
    int32 search;
    int32 list;

    int16 option_count;

    // -1 = nothing selected (single selection)
    int16 selected;

    // @see UISelectFlag
    byte select_flags;
};

// The (filtered) option list in the popup of a select
// Renders the options itself (no element per option), only the visible rows create vertices
// 80 bytes
struct UIOptionList {
    UICore core;

    // Offset of the select the list belongs to
    int32 select;

    // Vertical scroll offset in px
    f32 scroll;

    // Row under the pointer (-1 = none), row = index in the filtered list
    int16 hovered;
};

#endif
