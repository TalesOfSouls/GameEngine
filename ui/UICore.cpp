#pragma once
#ifndef COMS_UI_CORE_C
#define COMS_UI_CORE_C

#include "UICore.h"
#include "UILayout.h"

/**
 * Calculate dimension based on relative/absolute positioning
 * In case of relative positioning the position depends on a parent element
 * The function then defines the dimension in its own data structure
 */
inline
void ui_dimension_calculate(UILayout* const layout, const UIOffset* base, UICore* const core) {
    if (!(core->dimension.flag & UI_DIMENSION_DIM_RELATIVE)) {
        return;
    }

    const UIOffset* parent_offset = (UIOffset *) (layout->ui_offset_buffer.memory + base->parent_offset);
    UICore* parent_element = (UICore *) (layout->ui_element_buffer.memory + parent_offset->element);

    const UIAttributeDimension* parent_dim = &parent_element->dimension;
    core->dimension.pos = {
        parent_dim->pos.x + core->dimension.pos_rel.x,
        parent_dim->pos.y + core->dimension.pos_rel.y
    };
}

#endif