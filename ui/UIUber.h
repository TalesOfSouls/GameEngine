#pragma once
#ifndef COMS_UI_UBER_H
#define COMS_UI_UBER_H

#include "../stdlib/Stdlib.h"
#include "../utils/SimpleString.h"
#include "UICore.h"
#include "attribute/UIAttributeBorder.h"
#include "attribute/UIAttributeShadow.h"
#include "attribute/UIAttributeFont.h"
#include "UIAlignment.h"

// A Uber element that is very versatile and can hold basically all possible atributes
// Using this type is not encouraged since it is expensive in memory and when rendering
struct UIUber {
    UICore core;

    // UIAnchor
    int32 anchor;

    // UIAlign
    int32 align;
    CharType char_type;
    int32 pattern_length;
    SimpleString<const wchar_t> pattern;

    int32 content_length;
    SimpleString<const wchar_t> content;

    UIAttributeFont font;

    uint32 background;
    UIAttributeBorder border;
    UIAttributeShadow shadow_outer;
    UIAttributeShadow shadow_inner;

    // @todo implement more
};

#endif