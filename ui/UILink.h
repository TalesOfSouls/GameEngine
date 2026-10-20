#pragma once
#ifndef COMS_UI_LINK_H
#define COMS_UI_LINK_H

#include "../stdlib/Stdlib.h"
#include "attribute/UIAttributeFont.h"
#include "UICore.h"

// @todo Not implemented yet
struct UILink {
    UICore core;
    UIAttributeFont font;

    // for text
    CharType char_type;
    int32 content_length;
    int32 pattern_length;

    // Offsets into ui_element_buffer (no pointers, the buffer must stay relocatable)
    int32 content;
    int32 pattern;

    // for link
    int32 link_content_length;
    int32 link_pattern_length;
    int32 link_content;
    int32 link_pattern;
};

#endif
