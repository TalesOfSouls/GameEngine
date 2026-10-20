#pragma once
#ifndef COMS_UI_INPUT_H
#define COMS_UI_INPUT_H

#include "../stdlib/Stdlib.h"
#include "attribute/UIAttributeFont.h"
#include "UICore.h"

enum UIInputType : byte {
    UI_INPUT_TYPE_TEXT,
    UI_INPUT_TYPE_NUMERIC,
    UI_INPUT_TYPE_DATE,
    UI_INPUT_TYPE_DATE_TIME,
    UI_INPUT_TYPE_TIME,
};

// Used for UI_ELEMENT_TYPE_INPUT (single line) and UI_ELEMENT_TYPE_TEXTAREA (multi line)
// The text is utf8 and stored in the ui_element_buffer directly after the element
// The text is clipped to the element and scrolled to keep the caret visible
// Textareas show a vertical scrollbar if the text is longer than the element
// 140 bytes
struct UIInput {
    UICore core;

    // RGBA
    uint32 background_color;
    uint32 border_color;

    // Border color while the element is edited (UI_STATE_FOCUS) or hovered
    uint32 border_color_focus;

    // Background of the selected text
    uint32 selection_color;

    UIAttributeFont font;

    // Offset of the utf8 text (\0 terminated)
    int32 content;

    // Reserved bytes incl. \0 -> max text length = content_length - 1 bytes
    int32 content_length;

    // Byte offset of the caret in content
    int32 caret;

    // Byte offset where the selection started (the other end is the caret)
    // selection_anchor == caret -> no selection
    int32 selection_anchor;

    // Offset of a text that is shown while the content is empty (0 = none)
    int32 placeholder;

    // 1-indexed actions (layout->on_actions, 0 = none)
    int32 on_change;
    int32 on_submit; // Enter in a single line input

    // Element that is notified internally when the text changes, e.g. the select of a search field
    // or the color picker of a rgba field (@see ui_input_owner_changed), 0 = none
    int32 owner;

    // Scroll offset of the text in px (x = single line input, y = textarea)
    v2_f32 scroll;

    UIInputType input_type;
};

#endif
