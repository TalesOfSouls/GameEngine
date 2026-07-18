#pragma once
#ifndef COMS_UI_ATTRIBUTE_DIMENSION_C
#define COMS_UI_ATTRIBUTE_DIMENSION_C

#include "UIAttributeDimension.h"
#include "../UICore.h"
#include "../UIAlignment.h"

// @todo for more complex designs we would need padding, gutter, fill ratio per element
static inline
void ui_dimension_calculate(
    const UIAttributeDimension* const __restrict parent_dim,
    UIAttributeDimension* const __restrict dim
) NO_EXCEPT
{
    // Resolve size
    ////////////////////////////////////////////////////////
    if (dim->flag & UI_DIMENSION_DIM_X_RELATIVE) {
        // Relative: dim_raw is a 0..1 fraction of the parent's resolved dimension
        dim->dim.x = dim->dim_raw.x * parent_dim->dim.x;
    } else {
        dim->dim.x = dim->dim_raw.x;
    }

    if (dim->flag & UI_DIMENSION_DIM_Y_RELATIVE) {
        // Relative: dim_raw is a 0..1 fraction of the parent's resolved dimension
        dim->dim.y = dim->dim_raw.y * parent_dim->dim.y;
    } else {
        dim->dim.y = dim->dim_raw.y;
    }

    // Resolve anchor point from parent
    ////////////////////////////////////////////////////////
    v2_f32 anchor_point;
    if (dim->anchor & UI_ANCHOR_H_RIGHT) {
        anchor_point.x = parent_dim->pos.x + parent_dim->dim.x;
    } else if (dim->anchor & UI_ANCHOR_H_CENTER) {
        anchor_point.x = parent_dim->pos.x + (parent_dim->pos.x - parent_dim->dim.x) * 0.5f;
    } else {
        // UI_ANCHOR_H_LEFT (default)
        anchor_point.x = parent_dim->pos.x;
    }

    if (dim->anchor & UI_ANCHOR_V_TOP) {
        anchor_point.y = parent_dim->pos.y;
    } else if (dim->anchor & UI_ANCHOR_V_CENTER) {
        anchor_point.y = parent_dim->pos.y + (parent_dim->pos.y - parent_dim->dim.y) * 0.5f;
    } else {
        // UI_ANCHOR_V_BOTTOM (default)
        anchor_point.y = parent_dim->pos.y - parent_dim->dim.y;
    }

    // Resolve the offset from the anchor
    ////////////////////////////////////////////////////////
    v2_f32 pos_offset;
    if (dim->flag & UI_DIMENSION_POS_X_RELATIVE) {
        pos_offset.x = dim->pos_raw.x * parent_dim->dim.x;
    } else {
        pos_offset.x = dim->pos_raw.x;
    }

    if (dim->flag & UI_DIMENSION_POS_Y_RELATIVE) {
        pos_offset.y = dim->pos_raw.y * parent_dim->dim.y;
    } else {
        pos_offset.y = dim->pos_raw.y;
    }

    const f32 sign_x = (dim->anchor & UI_ANCHOR_H_RIGHT) ? -1.0f : 1.0f;
    const f32 sign_y = (dim->anchor & UI_ANCHOR_V_TOP) ? -1.0f : 1.0f;

    const v2_f32 base_pos = {
        anchor_point.x + pos_offset.x * sign_x,
        anchor_point.y + pos_offset.y * sign_y
    };

    // Resolve alignment
    ////////////////////////////////////////////////////////
    f32 final_x;
    if (dim->alignment & UI_ALIGN_H_RIGHT) {
        // element extends left from base_pos
        final_x = base_pos.x - dim->dim.x;
    } else if (dim->alignment & UI_ALIGN_H_CENTER) {
        final_x = base_pos.x - dim->dim.x * 0.5f;
    } else {
        // UI_ALIGN_H_LEFT (default)
        // element extends right from base_pos
        final_x = base_pos.x;
    }

    f32 final_y;
    if (dim->alignment & UI_ALIGN_V_TOP) {
        // element extends up from base_pos
        final_y = base_pos.y - dim->dim.y;
    } else if (dim->alignment & UI_ALIGN_V_CENTER) {
        final_y = base_pos.y - dim->dim.y * 0.5f;
    } else {
        // UI_ALIGN_V_BOTTOM (default)
        final_y = base_pos.y;
    }

    dim->pos = { final_x, final_y };
}

/**
 * Calculate dimension based on relative/absolute positioning
 * In case of relative positioning the position depends on a parent element
 * The function then defines the dimension in its own data structure
 */
inline
void ui_dimension_calculate(UILayout* const layout, UICore* const core) NO_EXCEPT {
    const UICore* parent_element = (UICore *) (layout->ui_element_buffer.memory + core->parent_offset);
    ui_dimension_calculate(&parent_element->dimension, &core->dimension);
}

#endif