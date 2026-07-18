#pragma once
#ifndef COMS_UI_WINDOW_C
#define COMS_UI_WINDOW_C

#include "../stdlib/Stdlib.h"
#include "../gpuapi/RenderUtils.h"
#include "../camera/Camera.cpp"
#include "attribute/UIAttributeShadow.h"
#include "attribute/UIAttributeFont.h"
#include "attribute/UIAttributeDimension.h"
#include "attribute/UIAttributeBorder.cpp"
#include "UIAnimation.h"
#include "UIAlignment.h"
#include "UIPanel.h"
#include "UIButton.cpp"
#include "UILabel.cpp"
#include "UIStyleType.h"
#include "UIWindow.h"

UIWindow* ui_window_create(UILayout* layout) NO_EXCEPT
{
    UIWindow* element = (UIWindow*) BUFFER_ELEMENT_GET(&layout->ui_element_buffer, UIWindow);
    MEMORY_ELEMENT_ZERO(element);

    element->core.type = UI_ELEMENT_TYPE_VIEW_WINDOW;
    element->core.opacity = 0xFF;
    element->core.parent_offset = (int32) MEMORY_OFFSET(layout->ui_root, layout->ui_element_buffer.memory);
    element->core.dimension.anchor = UI_ANCHOR_H_LEFT | UI_ANCHOR_V_TOP;
    element->core.dimension.alignment = UI_ALIGN_H_LEFT | UI_ALIGN_V_BOTTOM;
    element->title.core.parent_offset = (int32) MEMORY_OFFSET(element, layout->ui_element_buffer.memory);

    // title
    {
        // Only the height is pixel based
        element->title.core.dimension.flag = UI_DIMENSION_DIM_X_RELATIVE
            | UI_DIMENSION_DIM_Y_PX;
        element->title.core.dimension.anchor = UI_ANCHOR_H_LEFT | UI_ANCHOR_V_TOP;
        element->title.core.dimension.alignment = UI_ALIGN_H_LEFT | UI_ALIGN_V_BOTTOM;

        UILabel* const title_label = &element->title.label;
        title_label->core.dimension.pos_raw = { 0.5f, 0.5f };
        title_label->core.dimension.alignment = UI_ALIGN_H_LEFT | UI_ALIGN_V_BOTTOM;
    }

    // button_close
    {
        UIButton* const button_close = &element->button_close;
        button_close->core.parent_offset = (int32) MEMORY_OFFSET(element, layout->ui_element_buffer.memory);
        button_close->core.dimension.flag = UI_DIMENSION_POS_X_RELATIVE | UI_DIMENSION_POS_Y_RELATIVE
            | UI_DIMENSION_DIM_X_PX | UI_DIMENSION_DIM_Y_PX;
        button_close->core.dimension.anchor = UI_ANCHOR_H_RIGHT | UI_ANCHOR_V_TOP;
        button_close->core.dimension.alignment = UI_ALIGN_H_RIGHT | UI_ALIGN_V_BOTTOM;

        button_close->image.dimension.flag = UI_DIMENSION_DIM_X_RELATIVE | UI_DIMENSION_DIM_Y_RELATIVE;
        button_close->image.dimension.anchor = UI_ANCHOR_V_TOP;
        button_close->image.dimension.dim_raw = {1.0f, 1.0f};
    }

    return element;
}

static
void ui_vertices_cache(
    void* app,
    UIWindowTitle* window_title, GpuApiType gpu_api_type,
    UILayout* const layout, f32* zindex,
    byte* const __restrict mem
) NO_EXCEPT {
    ArrayVector<Vertex3DSamplerTextureColor>* vertex_cache = &layout->ui_vertex_cache;
    ArrayVector<int32>* index_cache = &layout->ui_index_cache;

    ui_dimension_calculate(layout, &window_title->core);

    if (window_title->core.opacity) {
        UIPanel* title_panel = &window_title->panel;

        // @question consider to use panel dimensions instead/call panel render function
        //          If we change to panel rendering we should avoid calculating the dimension here
        //          Otherwise we would calculate the same thing twice
        vertex_rect_create(
            vertex_cache, index_cache, *zindex, 0,
            {window_title->core.dimension.pos.x, window_title->core.dimension.pos.y, window_title->core.dimension.dim.width, window_title->core.dimension.dim.height},
            UI_ALIGN_V_TOP | UI_ALIGN_H_LEFT,
            title_panel->background_color
        );

        *zindex = camera_step_closer(gpu_api_type, *zindex);
    }

    // Border
    ui_vertices_cache(
        vertex_cache, index_cache, *zindex, gpu_api_type,
        &window_title->core.dimension.pos, &window_title->core.dimension.dim, window_title->border
    );

    // @question Do I also need to check for empty text here?
    if (OMS_HAS_ALPHA(window_title->label.font.color) && window_title->label.content) {
        ui_vertices_cache(
            app,
            &window_title->label,
            layout, camera_step_closer(gpu_api_type, *zindex),
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

    ui_dimension_calculate(layout, &window->core);

    if (window->panel.core.opacity || OMS_HAS_ALPHA(window->panel.background_color)) {
        vertex_rect_create(
            vertex_cache, index_cache, zindex, 0,
            {
                window->core.dimension.pos.x,
                window->core.dimension.pos.y,
                window->core.dimension.dim.width,
                window->core.dimension.dim.height
            },
            UI_ALIGN_V_TOP | UI_ALIGN_H_LEFT,
            window->panel.background_color
        );

        zindex = camera_step_closer(gpu_api_type, zindex);
    }

    // @todo make border part of panel
    /*
    ui_vertices_cache(
        vertex_cache, index_cache, zindex, gpu_api_type,
        &window->core.dimension.pos, &title_dim, offset_data->border,
        layout->ui_element_buffer.memory
    );
    */

    if (window->title.core.opacity) {
        zindex = camera_step_closer(gpu_api_type, zindex);
        ui_vertices_cache(
            app, &window->title, gpu_api_type,
            layout, &zindex,
            mem
        );
    }

    if (window->button_close.image.texture) {
        // We need two since our buttons might otherwise be behind the title/panel border
        zindex = camera_step_closer(gpu_api_type, zindex);
        zindex = camera_step_closer(gpu_api_type, zindex);

        ui_vertices_cache(
            app,
            &window->button_close, gpu_api_type,
            layout, zindex,
            mem
        );
    }
}

#endif