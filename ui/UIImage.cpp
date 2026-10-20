#pragma once
#ifndef COMS_UI_IMAGE_C
#define COMS_UI_IMAGE_C

#include "../stdlib/Stdlib.h"
#include "attribute/UIAttributeDimension.cpp"
#include "attribute/UIAttributeImage.cpp"
#include "UIAlignment.h"
#include "UICore.cpp"
#include "UILayout.h"
#include "UIImage.h"

/**
 * The size is the image size unless set otherwise
 *
 * @param parent NULL = root
 */
inline
UIImage* ui_image_create(UILayout* const layout, UICore* parent = NULL) NO_EXCEPT
{
    UIImage* const image = UI_ELEMENT_ALLOC(layout, UI_ELEMENT_TYPE_IMAGE, UIImage, parent);
    image->core.dimension.align_anchor = UI_ANCHOR_ALIGN_TOP_LEFT;

    return image;
}

void ui_image_cache(UIImage* const image, UILayout* const layout, f32 zindex) NO_EXCEPT
{
    ui_dimension_calculate(layout, &image->core, {(f32) image->image.width, (f32) image->image.height});

    if (image->image.sampler) {
        ui_image_vertices(
            &image->image, ui_dimension_rect(&image->core.dimension),
            &layout->ui_vertex_cache, &layout->ui_index_cache,
            zindex
        );
    }
}

#endif
