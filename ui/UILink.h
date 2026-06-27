#pragma once
#ifndef COMS_UI_LINK_H
#define COMS_UI_LINK_H

#include "../stdlib/Stdlib.h"

struct UILinkOffset {
    UIOffset self;
};

struct UILink {
    UICore core;
    UIAttributeFont font;

    // for text
    CharType char_type;
    int32 content_length;
    int32 pattern_length;
    char* content;
    char* pattern;

    // for link
    int32 link_content_length;
    int32 link_pattern_length;
    char* link_content;
    char* link_pattern;
};

#endif