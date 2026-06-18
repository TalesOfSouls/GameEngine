#pragma once
#ifndef COMS_UI_CUSTOM_H
#define COMS_UI_CUSTOM_H

#include "../stdlib/Stdlib.h"
#include "attribute/UIAttribute.h"
#include "attribute/UIAttributeDimension.h"
#include "UIStyleType.h"
#include "UILayout.h"

struct UICustomState {
};

// Very basic custom element that only must have dimension information
// All other information must be defined programatically
struct UICustom {
    UIAttributeDimension dimension;
};

#endif