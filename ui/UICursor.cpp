#pragma once
#ifndef COMS_UI_CURSOR_C
#define COMS_UI_CURSOR_C

#include "UILayout.h"
#include "UICursor.h"
#include "attribute/UIAttributeImage.cpp"
#include "attribute/UIAttributeDimension.cpp"

void ui_vertices_cache(
    void* app,
    UICursor* cursor,
    UILayout* const layout, f32,
    byte*
) NO_EXCEPT
{
    ArrayVector<Vertex3DSamplerTextureColor>* const vertex_cache = &layout->ui_vertex_cache;
    ArrayVector<int32>* const index_cache = &layout->ui_index_cache;

    // @performance Do I really want to do it here or somewhere else, maybe separate from the caching
    //              But that would mean iterating the elements twice
    if (cursor->core.update_func) {
        layout->update[cursor->core.update_func - 1](app, layout, &cursor->core);
    }

    // @performance This is slow, we shouldn't do this for cursor
    ui_dimension_calculate(layout, &cursor->core);

    // @performance aren't we recalculating the dimension.dim unnecessarily
    ui_dimension_calculate(&cursor->core.dimension, &cursor->image.dimension);
    ui_vertices_cache(
        &cursor->image, vertex_cache, index_cache, 1.0f
    );
}

static
UICursor* ui_cursor_create(
    UILayout* layout,
    UICore* element = NULL
) NO_EXCEPT
{
    UICursor* cursor = (UICursor *) element;

    if (!cursor) {
        cursor = (UICursor*) BUFFER_ELEMENT_GET(&layout->ui_element_buffer, UICursor);
        MEMORY_ELEMENT_ZERO(cursor);
    }

    cursor->core.type = UI_ELEMENT_TYPE_CURSOR;
    cursor->core.dimension.flag = UI_DIMENSION_POS_X_PX | UI_DIMENSION_POS_Y_PX
        | UI_DIMENSION_DIM_X_PX | UI_DIMENSION_DIM_Y_PX;
    cursor->core.dimension.anchor = UI_ANCHOR_H_LEFT | UI_ANCHOR_V_TOP;
    cursor->core.dimension.alignment = UI_ALIGN_H_LEFT | UI_ALIGN_V_BOTTOM;

    cursor->image.dimension.flag = UI_DIMENSION_POS_X_RELATIVE | UI_DIMENSION_POS_Y_RELATIVE
        | UI_DIMENSION_DIM_X_RELATIVE | UI_DIMENSION_DIM_Y_RELATIVE;
    cursor->image.dimension.dim_raw = {1.0f, 1.0f};
    cursor->image.dimension.anchor = UI_ANCHOR_H_LEFT | UI_ANCHOR_V_TOP;

    cursor->core.parent_offset = (int32) MEMORY_OFFSET(layout->ui_root, layout->ui_element_buffer.memory);

    return cursor;
}

#endif