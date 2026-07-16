#pragma once
#ifndef COMS_UI_BUTTON_H
#define COMS_UI_BUTTON_H

#include "../stdlib/Stdlib.h"
#include "attribute/UIAttribute.h"
#include "attribute/UIAttributeDimension.h"
#include "attribute/UIAttributeImage.h"
#include "UIStyleType.h"
#include "UIImage.h"
#include "UILabel.h"
#include "UIPanel.h"
#include "UILayout.h"

struct UIButton {
    UICore core;

    UIPanel panel;
    UIAttributeImage image;
    UIAttributeBorder border[8];
    UILabel label;

    // 1-indexed callback index, same idiom as UICore::update_func (0 = none).
    int32 on_click;
};

#endif