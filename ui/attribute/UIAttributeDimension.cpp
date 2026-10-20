#pragma once
#ifndef COMS_UI_ATTRIBUTE_DIMENSION_C
#define COMS_UI_ATTRIBUTE_DIMENSION_C

#include "UIAttributeDimension.h"
#include "../UICore.h"
#include "../UILayout.h"
#include "../UIAlignment.h"

/**
 * Calculates pos/dim from pos_raw/dim_raw relative to the parent
 *
 * @param content_dim Natural size of the content (e.g. text or image size)
 *                    Used for every axis where dim_raw is 0 and the axis is not relative
 *
 * @see UIAlignment.h for the coordinate system
 */
// @todo for more complex designs we would need padding, gutter, fill ratio per element
static inline
void ui_dimension_calculate(
    const UIAttributeDimension* const __restrict parent_dim,
    UIAttributeDimension* const __restrict dim,
    v2_f32 content_dim = {0.0f, 0.0f}
) NO_EXCEPT
{
    // Resolve size
    ////////////////////////////////////////////////////////
    // > 0 = pixel size, 0 = content size, < 0 = parent size minus the value (e.g. fill the parent minus a margin)
    if (dim->flag & UI_DIMENSION_DIM_X_RELATIVE) {
        dim->dim.x = dim->dim_raw.x * parent_dim->dim.x;
    } else if (dim->dim_raw.x > 0.0f) {
        dim->dim.x = dim->dim_raw.x;
    } else if (dim->dim_raw.x < 0.0f) {
        dim->dim.x = max_branched(parent_dim->dim.x + dim->dim_raw.x, 0.0f);
    } else {
        dim->dim.x = content_dim.x;
    }

    if (dim->flag & UI_DIMENSION_DIM_Y_RELATIVE) {
        dim->dim.y = dim->dim_raw.y * parent_dim->dim.y;
    } else if (dim->dim_raw.y > 0.0f) {
        dim->dim.y = dim->dim_raw.y;
    } else if (dim->dim_raw.y < 0.0f) {
        dim->dim.y = max_branched(parent_dim->dim.y + dim->dim_raw.y, 0.0f);
    } else {
        dim->dim.y = content_dim.y;
    }

    // Resolve anchor point on the parent
    // Remember: parent pos = top left corner, y-up
    ////////////////////////////////////////////////////////
    v2_f32 anchor_point;
    if (dim->align_anchor & UI_ANCHOR_H_RIGHT) {
        anchor_point.x = parent_dim->pos.x + parent_dim->dim.x;
    } else if (dim->align_anchor & UI_ANCHOR_H_CENTER) {
        anchor_point.x = parent_dim->pos.x + parent_dim->dim.x * 0.5f;
    } else {
        // UI_ANCHOR_H_LEFT (default)
        anchor_point.x = parent_dim->pos.x;
    }

    if (dim->align_anchor & UI_ANCHOR_V_TOP) {
        anchor_point.y = parent_dim->pos.y;
    } else if (dim->align_anchor & UI_ANCHOR_V_CENTER) {
        anchor_point.y = parent_dim->pos.y - parent_dim->dim.y * 0.5f;
    } else {
        // UI_ANCHOR_V_BOTTOM (default)
        anchor_point.y = parent_dim->pos.y - parent_dim->dim.y;
    }

    // Resolve the offset from the anchor
    // Positive offsets move towards the inside of the parent
    // (e.g. right anchor + positive x = move left, top anchor + positive y = move down)
    ////////////////////////////////////////////////////////
    const f32 offset_x = (dim->flag & UI_DIMENSION_POS_X_RELATIVE)
        ? dim->pos_raw.x * parent_dim->dim.x
        : dim->pos_raw.x;

    const f32 offset_y = (dim->flag & UI_DIMENSION_POS_Y_RELATIVE)
        ? dim->pos_raw.y * parent_dim->dim.y
        : dim->pos_raw.y;

    const f32 base_x = anchor_point.x + ((dim->align_anchor & UI_ANCHOR_H_RIGHT) ? -offset_x : offset_x);
    const f32 base_y = anchor_point.y + ((dim->align_anchor & UI_ANCHOR_V_TOP) ? -offset_y : offset_y);

    // Resolve alignment = which point of the element is placed at the base point
    ////////////////////////////////////////////////////////
    if (dim->align_anchor & UI_ALIGN_H_RIGHT) {
        dim->pos.x = base_x - dim->dim.x;
    } else if (dim->align_anchor & UI_ALIGN_H_CENTER) {
        dim->pos.x = base_x - dim->dim.x * 0.5f;
    } else {
        // UI_ALIGN_H_LEFT (default): element extends to the right
        dim->pos.x = base_x;
    }

    if (dim->align_anchor & UI_ALIGN_V_TOP) {
        // element extends downwards
        dim->pos.y = base_y;
    } else if (dim->align_anchor & UI_ALIGN_V_CENTER) {
        dim->pos.y = base_y + dim->dim.y * 0.5f;
    } else {
        // UI_ALIGN_V_BOTTOM (default): element extends upwards
        dim->pos.y = base_y + dim->dim.y;
    }
}

/**
 * Calculate dimension based on relative/absolute positioning
 * In case of relative positioning the position depends on the parent element
 * The parent MUST already be calculated (always the case when iterating in pre-order)
 */
FORCE_INLINE
void ui_dimension_calculate(
    const UILayout* const layout,
    UICore* const core,
    v2_f32 content_dim = {0.0f, 0.0f}
) NO_EXCEPT
{
    // The root is not calculated, it is defined by the screen size (@see ui_root_resize)
    if (core == layout->ui_root) {
        return;
    }

    const UICore* const parent = (const UICore *) (layout->ui_element_buffer.memory + core->parent_offset);
    ui_dimension_calculate(&parent->dimension, &core->dimension, content_dim);
}

/**
 * The calculated rect in the format the vertex functions expect (x, y = top left, width, height)
 */
FORCE_INLINE
v4_f32 ui_dimension_rect(const UIAttributeDimension* const dim) NO_EXCEPT
{
    return {dim->pos.x, dim->pos.y, dim->dim.x, dim->dim.y};
}

FORCE_INLINE
bool ui_dimension_contains(const UIAttributeDimension* const dim, f32 x, f32 y) NO_EXCEPT
{
    return x >= dim->pos.x && x < dim->pos.x + dim->dim.x
        && y <= dim->pos.y && y > dim->pos.y - dim->dim.y;
}

#endif
