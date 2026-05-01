#pragma once
#ifndef COMS_UI_PANEL_H
#define COMS_UI_PANEL_H

#include "../stdlib/Stdlib.h"
#include "attribute/UIAttributeDimension.h"
#include "attribute/UIAttributeBackground.h"

struct UIPanelState {
};

struct UIPanel {
    UIAttributeBackground background_style;
    UIAttributeDimension dimension;
    // @todo also needs color and texture
};

#endif