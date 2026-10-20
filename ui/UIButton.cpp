#pragma once
#ifndef COMS_UI_BUTTON_C
#define COMS_UI_BUTTON_C

#include "../stdlib/Stdlib.h"
#include "../gpuapi/RenderUtils.h"
#include "../camera/Camera.cpp"
#include "attribute/UIAttributeDimension.cpp"
#include "attribute/UIAttributeImage.cpp"
#include "UIAlignment.h"
#include "UICore.cpp"
#include "UILabel.cpp"
#include "UIButton.h"

/**
 * @param text NULL = no label child
 * @param parent NULL = root
 */
UIButton* ui_button_create(
    UILayout* const layout,
    const char* text = NULL,
    UICore* parent = NULL
) NO_EXCEPT
{
    UIButton* const button = UI_ELEMENT_ALLOC(layout, UI_ELEMENT_TYPE_BUTTON, UIButton, parent);
    button->core.flags = UI_ELEMENT_FLAG_INTERACTABLE | UI_ELEMENT_FLAG_HOVERABLE;
    button->core.dimension.align_anchor = UI_ANCHOR_ALIGN_TOP_LEFT;

    if (text) {
        // The label is a child element -> allocated directly after the button (pre-order)
        UILabel* const label = ui_label_create(layout, CHAR_TYPE_CHAR, 0, 0, &button->core);
        label->core.dimension.align_anchor = UI_ANCHOR_ALIGN_CENTER;
        ui_label_set_text(layout, label, text);

        button->label = ui_element_offset(layout, label);
    }

    return button;
}

void ui_button_cache(
    UIButton* const button,
    GpuApiType gpu_api_type,
    UILayout* const layout,
    f32* const zindex
) NO_EXCEPT
{
    // Without a defined dimension the button uses the image size
    const v2_f32 content_dim = button->image.sampler
        ? v2_f32{(f32) button->image.width, (f32) button->image.height}
        : v2_f32{0.0f, 0.0f};

    ui_dimension_calculate(layout, &button->core, content_dim);
    const v4_f32 rect = ui_dimension_rect(&button->core.dimension);

    // The state is updated before caching (@see ui_input_pointer), here we only pick the matching style
    const uint32 background_color = (button->core.state & UI_STATE_HOVER) && OMS_HAS_ALPHA(button->background_color_hover)
        ? button->background_color_hover
        : button->background_color;

    if (OMS_HAS_ALPHA(background_color)) {
        vertex_rect_create(
            &layout->ui_vertex_cache, &layout->ui_index_cache, *zindex, 0,
            rect,
            UI_ALIGN_V_TOP | UI_ALIGN_H_LEFT,
            background_color
        );

        *zindex = camera_step_closer(gpu_api_type, *zindex);
    }

    if (button->image.sampler) {
        ui_image_vertices(&button->image, rect, &layout->ui_vertex_cache, &layout->ui_index_cache, *zindex);
        *zindex = camera_step_closer(gpu_api_type, *zindex);
    }
}

#endif
