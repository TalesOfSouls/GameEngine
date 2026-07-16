#pragma once
#ifndef COMS_UI_BUTTON_C
#define COMS_UI_BUTTON_C

#include "../stdlib/Stdlib.h"
#include "../gpuapi/RenderUtils.h"
#include "UIAlignment.h"
#include "attribute/UIAttributeBorder.cpp"
#include "attribute/UIAttributeImage.cpp"
#include "UICore.cpp"
#include "UILabel.cpp"
#include "UIButton.h"
#include "../object/Vertex.h"

UIButton* ui_button_create(
    UILayout* layout,
    const char* text = NULL
) NO_EXCEPT
{
    UIButton* element = (UIButton*) BUFFER_ELEMENT_GET(&layout->ui_element_buffer, UIButton);
    MEMORY_ELEMENT_ZERO(element);

    element->core.type = UI_ELEMENT_TYPE_BUTTON;
    element->core.opacity = 0xFF;

    element->panel.core.parent_offset = (int32) MEMORY_OFFSET(&element->panel, element);
    // @performance I really don't like that I have both opacity and color in many places
    //              both get used to determine if the element gets rendered
    element->panel.core.opacity = 0xFF;
    element->panel.background_style.color = 0x000000FF;

    element->label.core.opacity = 0xFF;
    element->label.char_type = CHAR_TYPE_CHAR;
    element->label.content_length = ui_label_reserve_text(layout, &element->label.content, text);
    element->label.core.parent_offset = (int32) MEMORY_OFFSET(&element->label, element);

    return element;
}

void ui_vertices_cache(
    void* app, UIButton* button, GpuApiType gpu_api_type,
    UILayout* const layout, f32 zindex,
    byte* const __restrict mem
) NO_EXCEPT {
    ArrayVector<Vertex3DSamplerTextureColor>* const vertex_cache = &layout->ui_vertex_cache;
    ArrayVector<int32>* const index_cache = &layout->ui_index_cache;

    ui_dimension_calculate(layout, &button->core);

    if (button->panel.core.opacity || OMS_HAS_ALPHA(button->panel.background_style.color)) {
        vertex_rect_create(
            vertex_cache, index_cache, zindex, 0,
            {
                button->core.dimension.pos.x,
                button->core.dimension.pos.y,
                button->core.dimension.dimension.width,
                button->core.dimension.dimension.height
            },
            UI_ALIGN_V_TOP | UI_ALIGN_H_LEFT,
            button->panel.background_style.color
        );

        zindex = camera_step_closer(gpu_api_type, zindex);
    }

    if (button->image.texture) {
        uint32 parent_flags = 0;
        OMS_BIT_SET(parent_flags, UI_ELEMENT_TYPE_VIEW_WINDOW);
        OMS_BIT_SET(parent_flags, UI_ELEMENT_TYPE_VIEW_PANEL);
        OMS_BIT_SET(parent_flags, UI_ELEMENT_TYPE_INPUT);
        OMS_BIT_SET(parent_flags, UI_ELEMENT_TYPE_SELECT);

        const UICore* const parent = ui_parent_element_by_type(&button->core, parent_flags);
        v2_f32 parent_dim = {core->dimension.dimension.width, 0.0f};
        ui_vertices_cache(
            &button->image, vertex_cache, index_cache, zindex, gpu_api_type,
            &parent->dimension.pos, &parent_dim, button->border
        );
    }

    if (OMS_HAS_ALPHA(button->label.font.color) && button->label.content) {
        ui_vertices_cache(
            app,
            &button->label,
            layout, camera_step_closer(gpu_api_type, zindex),
            mem
        );
    }
}

#endif