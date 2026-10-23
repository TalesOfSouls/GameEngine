#pragma once
#ifndef COMS_UI_CURSOR_C
#define COMS_UI_CURSOR_C

#include "../stdlib/Stdlib.h"
#include "attribute/UIAttributeDimension.cpp"
#include "attribute/UIAttributeImage.cpp"
#include "UIAlignment.h"
#include "UICore.cpp"
#include "UILayout.h"
#include "UICursor.h"

// The cursor is always rendered in front of everything else
#define UI_CURSOR_ZINDEX 1.0f

/**
 * The position is the mouse position (pos_raw = window coordinates, y-down) relative to the top left of the root
 * The size is the image size unless set otherwise
 *
 * @param parent NULL = root
 */
static
UICursor* ui_cursor_create(UILayout* const layout, UICore* parent = NULL) NO_EXCEPT
{
    UICursor* const cursor = UI_ELEMENT_ALLOC(layout, UI_ELEMENT_TYPE_CURSOR, UICursor, parent);
    cursor->core.dimension.align_anchor = UI_ANCHOR_ALIGN_TOP_LEFT;

    // The cursor is always at the mouse position -> it would always be the hit element
    cursor->core.flags = UI_ELEMENT_FLAG_IGNORE_HIT | UI_ELEMENT_FLAG_ALWAYS_ON_TOP;

    layout->cursor = cursor;

    return cursor;
}

void ui_cursor_cache(UICursor* const cursor, UILayout* const layout) NO_EXCEPT
{
    PROFILE_DEBUG(PROFILE_UI_CACHE_CURSOR);

    ui_dimension_calculate(layout, &cursor->core, {(f32) cursor->image.width, (f32) cursor->image.height});

    if (cursor->image.sampler) {
        ui_image_vertices(
            &cursor->image, ui_dimension_rect(&cursor->core.dimension),
            &layout->ui_vertex_cache, &layout->ui_index_cache,
            UI_CURSOR_ZINDEX
        );
    }
}

#endif
