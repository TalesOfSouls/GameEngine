#pragma once
#ifndef COMS_UI_CUSTOM_H
#define COMS_UI_CUSTOM_H

#include "../stdlib/Stdlib.h"
#include "UICore.h"

// Very basic custom element that only has the core data
// Rendering and updating is handled by callbacks (@see UICore::render_func, UICore::update_func)
struct UICustom {
    UICore core;
};

#endif
