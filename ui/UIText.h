#pragma once
#ifndef COMS_UI_TEXT_H
#define COMS_UI_TEXT_H

#include "../stdlib/Stdlib.h"

struct UITextOffset {
    UIOffset self;
};

struct UIText {
    UICore core;
    UIAttributeFont font;
};

#endif