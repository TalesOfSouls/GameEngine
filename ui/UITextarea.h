#pragma once
#ifndef COMS_UI_TEXTAREA_H
#define COMS_UI_TEXTAREA_H

#include "../stdlib/Stdlib.h"

struct UITextareaOffset {
    UIOffset self;
};

struct UITextarea {
    UICore core;
    UIAttributeFont font;
};

#endif