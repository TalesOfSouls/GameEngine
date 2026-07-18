#pragma once
#ifndef COMS_UI_WINDOW_H
#define COMS_UI_WINDOW_H

#include "../stdlib/Stdlib.h"
#include "attribute/UIAttributeShadow.h"
#include "attribute/UIAttributeDimension.h"
#include "UIAnimation.h"
#include "UICore.h"
#include "UILabel.h"
#include "UIPanel.h"
#include "UIButton.h"

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

    UIButton button_close;
    UIButton button_minimize;
    UIButton button_maximize;
};

#endif