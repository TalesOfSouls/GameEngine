#pragma once
#ifndef COMS_UI_WINDOW_C
#define COMS_UI_WINDOW_C

#include "../stdlib/Stdlib.h"
#include "../gpuapi/RenderUtils.h"
#include "../camera/Camera.cpp"
#include "attribute/UIAttributeShadow.h"
#include "attribute/UIAttributeFont.h"
#include "attribute/UIAttributeBackground.h"
#include "attribute/UIAttributeDimension.h"
#include "attribute/UIAttributeBorder.h"
#include "UIAnimation.h"
#include "UIAlignment.h"
#include "UIPanel.h"
#include "UILabel.cpp"
#include "UIStyleType.h"
#include "UIWindow.h"

UIWindow* ui_window_create(UILayout* layout, uint32 component_flags) NO_EXCEPT
{
    UIWindow* element = (UIWindow*) BUFFER_ELEMENT_GET(&layout->ui_element_buffer, UIWindow);
    MEMORY_ELEMENT_ZERO(element);

    element->core.type = UI_ELEMENT_TYPE_VIEW_WINDOW;
    element->core.opacity = 255;
    element->title.core.parent_offset = (int32) MEMORY_OFFSET(&element->title, element);

    if (component_flags & UI_WINDOW_COMPONENT_FLAG_TITLE) {
        element->title.core.opacity = 255;
        element->title.label.font.color = 0x000000FF;

        // Title - Label
        if (component_flags & UI_WINDOW_COMPONENT_FLAG_TITLE_LABEL) {
            const char title[] = "Title";
            element->title.label.content_length = ui_label_reserve_text(layout, &element->title.label.content, title);
            element->title.label.core.parent_offset = (int32) MEMORY_OFFSET(&element->title.label, layout->ui_element_buffer.memory);
        }
    }

    return element;
}

static
void ui_vertices_cache(
    void* app,
    UIWindowTitle* window_title, GpuApiType gpu_api_type,
    UILayout* const layout, f32 zindex,
    byte* const __restrict mem
) NO_EXCEPT {
    UIWindow* window = (UIWindow*) ui_parent_element_by_type(&window_title->core, UI_ELEMENT_TYPE_VIEW_WINDOW);
    ArrayVector<Vertex3DSamplerTextureColor>* vertex_cache = &layout->ui_vertex_cache;
    ArrayVector<int32>* index_cache = &layout->ui_index_cache;

    v2_f32 title_dim = {window->core.dimension.dimension.width, 0.0f};

    if (window_title->core.opacity) {
        UIPanel* title_panel = &window_title->panel;

        vertex_rect_create(
            vertex_cache, index_cache, zindex, 0,
            {window->core.dimension.pos.x, window->core.dimension.pos.y, window->core.dimension.dimension.width, title_panel->core.dimension.dimension.height},
            UI_ALIGN_V_TOP | UI_ALIGN_H_LEFT,
            title_panel->background_style.color
        );

        zindex = camera_step_closer(gpu_api_type, zindex);
        title_dim.height = title_panel->core.dimension.dimension.height;
    }

    cache_border_vertices(
        vertex_cache, index_cache, zindex, gpu_api_type,
        &window->core.dimension.pos, &title_dim, window_title->border
    );

    // @question Do I also need to check for empty text here?
    if ((window_title->label.font.color & 0xFF) && window_title->label.content) {
        ui_vertices_cache(
            app,
            &window_title->label,
            layout, camera_step_closer(gpu_api_type, zindex),
            mem
        );
    }
}

void ui_vertices_cache(
    void* app, UIWindow* window, GpuApiType gpu_api_type,
    UILayout* const layout, f32 zindex,
    byte* const __restrict mem
) NO_EXCEPT {
    ArrayVector<Vertex3DSamplerTextureColor>* vertex_cache = &layout->ui_vertex_cache;
    ArrayVector<int32>* index_cache = &layout->ui_index_cache;

    if (window->panel.core.opacity || (window->panel.background_style.color & 0xFF)) {
        vertex_rect_create(
            vertex_cache, index_cache, zindex, 0,
            {
                window->core.dimension.pos.x,
                window->core.dimension.pos.y,
                window->core.dimension.dimension.width,
                window->core.dimension.dimension.height
            },
            UI_ALIGN_V_TOP | UI_ALIGN_H_LEFT,
            window->panel.background_style.color
        );
    }

    // @todo make border part of panel
    /*
    cache_border_vertices(
        vertex_cache, index_cache, zindex, gpu_api_type,
        &window->core.dimension.pos, &title_dim, offset_data->border,
        layout->ui_element_buffer.memory
    );
    */

    if (window->title.core.opacity) {
        ui_vertices_cache(
            app, &window->title, gpu_api_type,
            layout, camera_step_closer(gpu_api_type, zindex),
            mem
        );
    }

    // @todo cache window buttons
}

#endif