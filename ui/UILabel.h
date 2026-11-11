#pragma once
#ifndef COMS_UI_LABEL_H
#define COMS_UI_LABEL_H

#include "../stdlib/Stdlib.h"
#include "attribute/UIAttributeFont.h"
#include "UICore.h"

// 96 bytes
struct UILabel {
    UICore core;
    UIAttributeFont font;

    // Reserved length (in characters incl. \0) of content/pattern
    int32 content_length;
    int32 pattern_length;

    CharType char_type;

    // Offsets into ui_element_buffer (0 = none)
    // \0 terminated C strings with a SimpleStringInline header in front of them (@see ui_string_reserve)
    // -> the current length is known (@see ui_label_string), direct writers must call ui_string_length_set
    int32 content;
    int32 pattern;
};

#endif
