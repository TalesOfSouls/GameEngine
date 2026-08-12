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

    element->panel.core.parent_offset = (int32) MEMORY_OFFSET(&element->panel, layout->ui_element_buffer.memory);
    element->panel.background_color = 0x000000FF;

    element->label.char_type = CHAR_TYPE_CHAR;
    element->label.content_length = ui_label_reserve_text(layout, &element->label.content, text);
    element->label.core.parent_offset = (int32) MEMORY_OFFSET(&element->label, layout->ui_element_buffer.memory);

    return element;
}

void ui_vertices_cache(
    void* app, UIButton* button, GpuApiType gpu_api_type,
    UILayout* const layout, f32 zindex,
    BufferMemory* const __restrict mem
) NO_EXCEPT
{
    ArrayVector<Vertex3DSamplerTextureColor>* const vertex_cache = &layout->ui_vertex_cache;
    ArrayVector<int32>* const index_cache = &layout->ui_index_cache;

    ui_dimension_calculate(layout, &button->core);

    if (OMS_HAS_ALPHA(button->panel.background_color)) {
        vertex_rect_create(
            vertex_cache, index_cache, zindex, 0,
            {
                button->core.dimension.pos.x,
                button->core.dimension.pos.y,
                button->core.dimension.dim.width,
                button->core.dimension.dim.height
            },
            UI_ALIGN_V_TOP | UI_ALIGN_H_LEFT,
            button->panel.background_color
        );

        zindex = camera_step_closer(gpu_api_type, zindex);
    }

    if (button->image.texture_id) {
        ui_dimension_calculate(&button->core.dimension, &button->image.dimension);
        ui_vertices_cache(
            &button->image, vertex_cache, index_cache, zindex
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