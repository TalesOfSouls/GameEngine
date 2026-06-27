#pragma once
#ifndef COMS_UI_CURSOR_H
#define COMS_UI_CURSOR_H

#include "../stdlib/Stdlib.h"
#include "attribute/UIAttributeDimension.h"
#include "UILayout.h"

struct UICursorOffset {
    UIOffset self;
};

struct UICursor {
    UICore core;
    int32 asset_id;
    int32 sampler;
};

#endif