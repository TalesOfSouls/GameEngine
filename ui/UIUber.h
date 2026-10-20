#pragma once
#ifndef COMS_UI_UBER_H
#define COMS_UI_UBER_H

#include "../stdlib/Stdlib.h"
#include "../utils/SimpleString.h"
#include "UICore.h"
#include "attribute/UIAttributeFont.h"
#include "UIAlignment.h"

// A Uber element that is very versatile and can hold basically all possible attributes
// It is only used as temporary storage while parsing a layout file
struct UIUber {
    UICore core;

    CharType char_type;

    // Reserved length incl. \0
    int32 pattern_length;
    SimpleString<const char> pattern;

    // Reserved length incl. \0
    int32 content_length;
    SimpleString<const char> content;

    UIAttributeFont font;

    // RGBA
    uint32 background_color;
};

#endif
