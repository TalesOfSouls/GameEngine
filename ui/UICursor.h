#pragma once
#ifndef COMS_UI_CURSOR_H
#define COMS_UI_CURSOR_H

#include "../stdlib/Stdlib.h"
#include "attribute/UIAttributeDimension.h"
#include "UILayout.h"

struct UICursorState {
};

struct UICursor {
    UIAttributeDimension dimension;
    byte opacity; // 1 byte alpha channel
};

#endif