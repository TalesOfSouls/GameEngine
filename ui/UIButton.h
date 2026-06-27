#pragma once
#ifndef COMS_UI_BUTTON_H
#define COMS_UI_BUTTON_H

#include "../stdlib/Stdlib.h"
#include "attribute/UIAttribute.h"
#include "attribute/UIAttributeDimension.h"
#include "UIStyleType.h"
#include "UIImage.h"
#include "UILabel.h"
#include "UIPanel.h"
#include "UILayout.h"

struct UIButtonOffset {
    UIOffset self;
    UIBorderOffset border[8];
    UIPanelOffset panel;
    UILabelOffset label;
};

struct UIButton {
    UIOffset self;
};

#endif