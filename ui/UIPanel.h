#pragma once
#ifndef COMS_UI_PANEL_H
#define COMS_UI_PANEL_H

#include "../stdlib/Stdlib.h"
#include "attribute/UIAttributeDimension.h"
#include "attribute/UIAttributeBackground.h"
#include "UICore.h"


struct UIPanelState {
};

struct UIPanel {
    UICore core;
    UIAttributeBackground background_style;
    // @todo also needs color and texture
};

#endif