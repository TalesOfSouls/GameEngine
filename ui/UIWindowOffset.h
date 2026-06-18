#pragma once
#ifndef COMS_UI_WINDOW_OFFSET_H
#define COMS_UI_WINDOW_OFFSET_H

#include "../stdlib/Stdlib.h"
#include "UIOffset.h"
#include "UILabelOffset.h"
#include "attribute/UIAttributeBorderOffset.h"
#include "UIPanelOffset.h"

struct UIWindowTitleOffset {
    UIOffset self;
    UIBorderOffset border[8];
    UIPanelOffset panel;
    UILabelOffset label;
};

struct UIWindowOffset {
    UIOffset self;
    UIWindowTitleOffset title;
    UIBorderOffset border[8];
    UIPanelOffset panel;
};

#endif