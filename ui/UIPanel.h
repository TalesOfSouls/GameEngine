#pragma once
#ifndef COMS_UI_PANEL_H
#define COMS_UI_PANEL_H

#include "../stdlib/Stdlib.h"
#include "attribute/UIAttributeDimension.h"
#include "attribute/UIAttributeBackground.h"
#include "UICore.h"

struct UIPanel {
    UICore core;
    UIAttributeBackground background_style;
    // @todo also needs color and texture

    // For some time we considered to add borders to the panel
    // It felt natural that a panel should have borders (e.g. title bar, window, input field, ...)
    // However, We may use panels in different situations as well and then they would have these
    // hard coded panels always be part of the memory layout which is dumb
};

#endif