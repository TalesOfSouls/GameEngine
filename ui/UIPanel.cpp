#pragma once
#ifndef COMS_UI_PANEL_C
#define COMS_UI_PANEL_C

#include "UIPanel.h"
#include "UIAlignment.h"

inline
UIPanel* ui_root_create(UILayout* layout, f32 width, f32 height) NO_EXCEPT
{
    UIPanel* element = (UIPanel*) BUFFER_ELEMENT_GET(&layout->ui_element_buffer, UIPanel);
    MEMORY_ELEMENT_ZERO(element);

    element->core.type = UI_ELEMENT_TYPE_VIEW_PANEL;
    element->core.opacity = 0xFF;
    element->core.dimension.dim = {width, height};

    // @bug I hate that we have to define y = height instead of 0/0
    //      The reason for this is the mixing of coordinate systems
    //      Our UI starts in the bottom left corner BUT the rendering happens from top to bottom
    element->core.dimension.pos = {0, height};
    element->core.dimension.alignment = UI_ALIGN_H_LEFT | UI_ALIGN_V_TOP;
    element->core.dimension.anchor = UI_ANCHOR_H_LEFT | UI_ANCHOR_V_TOP;

    return element;
}

#endif