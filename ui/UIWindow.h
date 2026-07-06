#pragma once
#ifndef COMS_UI_WINDOW_H
#define COMS_UI_WINDOW_H

#include "../stdlib/Stdlib.h"
#include "attribute/UIAttributeShadow.h"
#include "attribute/UIAttributeDimension.h"
#include "attribute/UIAttributeBackground.h"
#include "UIAnimation.h"
#include "UICore.h"
#include "UILabel.h"
#include "UIPanel.h"

enum UIWindowComponentFlag : uint32 {
    UI_WINDOW_COMPONENT_FLAG_TITLE_LABEL = 1 << 0,
    UI_WINDOW_COMPONENT_FLAG_TITLE_BORDER = 1 << 1,
    UI_WINDOW_COMPONENT_FLAG_TITLE_PANEL = 1 << 2,
    UI_WINDOW_COMPONENT_FLAG_TITLE = (1 << 8) - 1, // This allows us to check if any title component exists

    UI_WINDOW_COMPONENT_FLAG_MAIN_BORDER = 1 << 8,
    UI_WINDOW_COMPONENT_FLAG_MAIN_BUTTONS = 1 << 9,
};

struct UIWindowTitle {
    UICore core;
    UIAttributeBorder border[8];
    UIPanel panel;
    UILabel label;
};

struct UIWindow {
    UICore core;
    
    UIWindowTitle title;

    UIAttributeBorder border[8];
    UIPanel panel;
};

#endif