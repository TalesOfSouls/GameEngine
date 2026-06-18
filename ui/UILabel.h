#pragma once
#ifndef COMS_UI_LABEL_H
#define COMS_UI_LABEL_H

#include "../stdlib/Stdlib.h"
#include "attribute/UIAttribute.h"
#include "attribute/UIAttributeFont.h"
#include "attribute/UIAttributeDimension.h"
#include "UICore.h"

struct UILabelState {
    char* content;
};

struct UILabel {
    UICore core;
    UIAttributeFont font;
    int32 content_length;
    int32 pattern_length;
    wchar_t* content;
    wchar_t* pattern;
};

#endif