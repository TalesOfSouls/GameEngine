#pragma once
#ifndef COMS_UI_PANEL_C
#define COMS_UI_PANEL_C

#include "UIPanel.h"
#include "UIAlignment.h"

inline
UICore* ui_root_create(UILayout* layout, f32 width, f32 height) NO_EXCEPT
{
    UIPanel* element = (UIPanel*) BUFFER_ELEMENT_GET(&layout->ui_element_buffer, UIPanel);
    MEMORY_ELEMENT_ZERO(element);

    element->core.type = UI_ELEMENT_TYPE_VIEW_PANEL;
    element->core.dimension.dim = {width, height};

    // We MUST define y = height since our y is always the top left corner even for other elements e.g. window, label, ...
    element->core.dimension.pos = {0, height};
    element->core.dimension.alignment = UI_ALIGN_H_LEFT | UI_ALIGN_V_TOP;
    element->core.dimension.anchor = UI_ANCHOR_H_LEFT | UI_ANCHOR_V_TOP;

    return &element->core;
}

#endif