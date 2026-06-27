#pragma once
#ifndef COMS_UI_LABEL_H
#define COMS_UI_LABEL_H

#include "../stdlib/Stdlib.h"
#include "attribute/UIAttribute.h"
#include "attribute/UIAttributeFont.h"
#include "attribute/UIAttributeDimension.h"
#include "UICore.h"
#include "UIOffset.h"
#include "../utils/SimpleString.h"

struct UILabelOffset {
    UIOffset self;
};

struct UILabel {
    UICore core;
    UIAttributeFont font;
    int32 content_length;
    int32 pattern_length;

    CharType char_type;
    char* content;
    char* pattern;
};

#endif