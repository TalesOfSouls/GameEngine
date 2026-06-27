#pragma once
#ifndef COMS_UI_WINDOW_H
#define COMS_UI_WINDOW_H

#include "../stdlib/Stdlib.h"
#include "attribute/UIAttributeShadow.h"
#include "attribute/UIAttributeDimension.h"
#include "attribute/UIAttributeBackground.h"
#include "UIAnimation.h"
#include "UICore.h"
#include "../stdlib/Stdlib.h"
#include "UIOffset.h"
#include "UILabel.h"
#include "attribute/UIAttributeBorderOffset.h"
#include "UIPanel.h"

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

enum UIWindowComponentFlag : uint32 {
    UI_WINDOW_COMPONENT_FLAG_TITLE_LABEL = 1 << 0,
    UI_WINDOW_COMPONENT_FLAG_TITLE_BORDER = 1 << 1,
    UI_WINDOW_COMPONENT_FLAG_TITLE_PANEL = 1 << 2,
    UI_WINDOW_COMPONENT_FLAG_TITLE = (1 << 8) - 1, // This allows us to check if any title component exists

    UI_WINDOW_COMPONENT_FLAG_MAIN_BORDER = 1 << 8,
    UI_WINDOW_COMPONENT_FLAG_MAIN_BUTTONS = 1 << 9,
};

struct UIWindow {
    UICore core;
    UIAnimation animation;
};

#endif