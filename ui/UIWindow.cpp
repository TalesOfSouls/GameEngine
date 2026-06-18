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
#include "UIOffset.h"
#include "UIAlignment.h"
#include "UIPanel.h"
#include "UILabel.cpp"
#include "UIStyleType.h"
#include "UIWindow.h"
#include "UIWindowOffset.h"

enum UIWindowComponentFlag : uint32 {
    UI_WINDOW_COMPONENT_FLAG_TITLE_LABEL = 1 << 0,
    UI_WINDOW_COMPONENT_FLAG_TITLE_BORDER = 1 << 1,
    UI_WINDOW_COMPONENT_FLAG_TITLE_PANEL = 1 << 2,
    UI_WINDOW_COMPONENT_FLAG_TITLE = (1 << 8) - 1, // This allows us to check if any title component exists

    UI_WINDOW_COMPONENT_FLAG_MAIN_BORDER = 1 << 8,
    UI_WINDOW_COMPONENT_FLAG_MAIN_BUTTONS = 1 << 9,
};

void ui_window_title_add(UILayout* layout, UIWindowOffset* window, uint32 component_flags)
{
    // Title - Start
    // We don't have an element since a title is 100% composite of other elements

    void* first_element = NULL;

    // Title - Border
    if (component_flags & UI_WINDOW_COMPONENT_FLAG_TITLE_BORDER) {
        UIAttributeBorder* title_border_tl = (UIAttributeBorder*) BUFFER_ELEMENT_GET(&layout->ui_element_buffer, UIAttributeBorder);
        MEMORY_ELEMENT_ZERO(title_border_tl);
        window->title.border[0].self.element = (int32) MEMORY_OFFSET(title_border_tl, layout->ui_element_buffer.memory);

        first_element = title_border_tl;

        UIAttributeBorder* title_border_t = (UIAttributeBorder*) BUFFER_ELEMENT_GET(&layout->ui_element_buffer, UIAttributeBorder);
        MEMORY_ELEMENT_ZERO(title_border_t);
        window->title.border[1].self.element = (int32) MEMORY_OFFSET(title_border_t, layout->ui_element_buffer.memory);

        UIAttributeBorder* title_border_tr = (UIAttributeBorder*) BUFFER_ELEMENT_GET(&layout->ui_element_buffer, UIAttributeBorder);
        MEMORY_ELEMENT_ZERO(title_border_tr);
        window->title.border[2].self.element = (int32) MEMORY_OFFSET(title_border_tr, layout->ui_element_buffer.memory);

        UIAttributeBorder* title_border_r = (UIAttributeBorder*) BUFFER_ELEMENT_GET(&layout->ui_element_buffer, UIAttributeBorder);
        MEMORY_ELEMENT_ZERO(title_border_r);
        window->title.border[3].self.element = (int32) MEMORY_OFFSET(title_border_r, layout->ui_element_buffer.memory);

        UIAttributeBorder* title_border_br = (UIAttributeBorder*) BUFFER_ELEMENT_GET(&layout->ui_element_buffer, UIAttributeBorder);
        MEMORY_ELEMENT_ZERO(title_border_br);
        window->title.border[4].self.element = (int32) MEMORY_OFFSET(title_border_br, layout->ui_element_buffer.memory);

        UIAttributeBorder* title_border_b = (UIAttributeBorder*) BUFFER_ELEMENT_GET(&layout->ui_element_buffer, UIAttributeBorder);
        MEMORY_ELEMENT_ZERO(title_border_b);
        window->title.border[5].self.element = (int32) MEMORY_OFFSET(title_border_b, layout->ui_element_buffer.memory);

        UIAttributeBorder* title_border_bl = (UIAttributeBorder*) BUFFER_ELEMENT_GET(&layout->ui_element_buffer, UIAttributeBorder);
        MEMORY_ELEMENT_ZERO(title_border_bl);
        window->title.border[6].self.element = (int32) MEMORY_OFFSET(title_border_bl, layout->ui_element_buffer.memory);

        UIAttributeBorder* title_border_l = (UIAttributeBorder*) BUFFER_ELEMENT_GET(&layout->ui_element_buffer, UIAttributeBorder);
        MEMORY_ELEMENT_ZERO(title_border_l);
        window->title.border[7].self.element = (int32) MEMORY_OFFSET(title_border_l, layout->ui_element_buffer.memory);
    }

    // Title - Panel
    if (component_flags & UI_WINDOW_COMPONENT_FLAG_TITLE_PANEL) {
        UIPanel* title_panel = (UIPanel*) BUFFER_ELEMENT_GET(&layout->ui_element_buffer, UIPanel);
        MEMORY_ELEMENT_ZERO(title_panel);
        window->title.panel.self.element = (int32) MEMORY_OFFSET(title_panel, layout->ui_element_buffer.memory);

        if (!first_element) {
            first_element = title_panel;
        }
    }

    // Title - Label
    if (component_flags & UI_WINDOW_COMPONENT_FLAG_TITLE_LABEL) {
        UILabel* title_label = (UILabel*) BUFFER_ELEMENT_GET(&layout->ui_element_buffer, UILabel);
        MEMORY_ELEMENT_ZERO(title_label);
        window->title.label.self.element = (int32) MEMORY_OFFSET(title_label, layout->ui_element_buffer.memory);

        const wchar_t title[] = L"Title";
        title_label->content = (wchar_t*) memory_get(&layout->ui_element_buffer, sizeof(title), alignof(wchar_t));
        memcpy(title_label->content, title, sizeof(title));

        window->title.label.self.element = (int32) MEMORY_OFFSET(title_label, layout->ui_element_buffer.memory);
        window->title.label.self.parent_offset = (int32) MEMORY_OFFSET(&window->title.label, &window->title);

        if (!first_element) {
            first_element = title_label;
        }
    }

    // We MUST set the title element to the first element that is actually used
    // Why? because when rendering the title we check if .element != 0
    window->title.self.element = (int32) MEMORY_OFFSET(first_element, layout->ui_element_buffer.memory);
    window->title.self.parent_offset = (int32) MEMORY_OFFSET(&window->title, window);
}

UIWindowOffset* ui_window_create(UILayout* layout, uint32 component_flags) {
    UIWindowOffset* window = (UIWindowOffset*) BUFFER_ELEMENT_GET(&layout->ui_offset_buffer, UIWindowOffset);
    MEMORY_ELEMENT_ZERO(window);

    // We need to add this offset to the root array for iteration later on
    array_vector_insert(&layout->ui_offset_root, (int32) MEMORY_OFFSET(window, layout->ui_offset_buffer.memory));

    UIWindow* window_element = (UIWindow*) BUFFER_ELEMENT_GET(&layout->ui_element_buffer, UIWindow);
    MEMORY_ELEMENT_ZERO(window_element);

    window->self.element = (int32) MEMORY_OFFSET(window_element, layout->ui_element_buffer.memory);
    // @todo We also need to set the types of the children
    window->self.type = UI_ELEMENT_TYPE_VIEW_WINDOW;

    if (component_flags & UI_WINDOW_COMPONENT_FLAG_TITLE) {
        ui_window_title_add(layout, window, component_flags);
    }

    if (component_flags & UI_WINDOW_COMPONENT_FLAG_MAIN_BORDER) {
        // Main - Border
        UIAttributeBorder* border_tl = (UIAttributeBorder*) BUFFER_ELEMENT_GET(&layout->ui_element_buffer, UIAttributeBorder);
        MEMORY_ELEMENT_ZERO(border_tl);
        window->border[0].self.element = (int32) MEMORY_OFFSET(border_tl, layout->ui_element_buffer.memory);

        UIAttributeBorder* border_t = (UIAttributeBorder*) BUFFER_ELEMENT_GET(&layout->ui_element_buffer, UIAttributeBorder);
        MEMORY_ELEMENT_ZERO(border_t);
        window->border[1].self.element = (int32) MEMORY_OFFSET(border_t, layout->ui_element_buffer.memory);

        UIAttributeBorder* border_tr = (UIAttributeBorder*) BUFFER_ELEMENT_GET(&layout->ui_element_buffer, UIAttributeBorder);
        MEMORY_ELEMENT_ZERO(border_tr);
        window->border[2].self.element = (int32) MEMORY_OFFSET(border_tr, layout->ui_element_buffer.memory);

        UIAttributeBorder* border_r = (UIAttributeBorder*) BUFFER_ELEMENT_GET(&layout->ui_element_buffer, UIAttributeBorder);
        MEMORY_ELEMENT_ZERO(border_r);
        window->border[3].self.element = (int32) MEMORY_OFFSET(border_r, layout->ui_element_buffer.memory);

        UIAttributeBorder* border_br = (UIAttributeBorder*) BUFFER_ELEMENT_GET(&layout->ui_element_buffer, UIAttributeBorder);
        MEMORY_ELEMENT_ZERO(border_br);
        window->border[4].self.element = (int32) MEMORY_OFFSET(border_br, layout->ui_element_buffer.memory);

        UIAttributeBorder* border_b = (UIAttributeBorder*) BUFFER_ELEMENT_GET(&layout->ui_element_buffer, UIAttributeBorder);
        MEMORY_ELEMENT_ZERO(border_b);
        window->border[5].self.element = (int32) MEMORY_OFFSET(border_b, layout->ui_element_buffer.memory);

        UIAttributeBorder* border_bl = (UIAttributeBorder*) BUFFER_ELEMENT_GET(&layout->ui_element_buffer, UIAttributeBorder);
        MEMORY_ELEMENT_ZERO(border_bl);
        window->border[6].self.element = (int32) MEMORY_OFFSET(border_bl, layout->ui_element_buffer.memory);

        UIAttributeBorder* border_l = (UIAttributeBorder*) BUFFER_ELEMENT_GET(&layout->ui_element_buffer, UIAttributeBorder);
        MEMORY_ELEMENT_ZERO(border_l);
        window->border[7].self.element = (int32) MEMORY_OFFSET(border_l, layout->ui_element_buffer.memory);
    }

    // Main - Panel
    UIPanel* main_panel = (UIPanel*) BUFFER_ELEMENT_GET(&layout->ui_element_buffer, UIPanel);
    MEMORY_ELEMENT_ZERO(main_panel);
    window->panel.self.element = (int32) MEMORY_OFFSET(main_panel, layout->ui_element_buffer.memory);

    return window;
}

FORCE_INLINE
UIWindowOffset* ui_element_create(UILayout* const __restrict layout, const UIUber* const __restrict) NO_EXCEPT
{
    return ui_window_create(
        layout,
        UI_WINDOW_COMPONENT_FLAG_MAIN_BORDER
            | UI_WINDOW_COMPONENT_FLAG_TITLE_PANEL
            | UI_WINDOW_COMPONENT_FLAG_TITLE_BORDER
            | UI_WINDOW_COMPONENT_FLAG_TITLE_LABEL
    );
}

void cache_vertices(
    void* app,
    UIWindowTitleOffset* offset_data, GpuApiType gpu_api_type,
    UILayout* const layout, f32 zindex,
    byte* const __restrict mem
) {
    const UIOffset* parent = ui_parent_offset_by_type(&offset_data->self, UI_ELEMENT_TYPE_VIEW_WINDOW);
    UIWindow* window = (UIWindow *) (layout->ui_element_buffer.memory + parent->element);
    ArrayVector<Vertex3DSamplerTextureColor>* vertex_cache = &layout->ui_vertex_cache;
    ArrayVector<int32>* index_cache = &layout->ui_index_cache;

    v2_f32 title_dim = {window->core.dimension.dimension.width, 0.0f};

    // @todo the title needs access to the parent (window)
    if (offset_data->panel.self.element) {
        UIPanel* title_panel = (UIPanel *) (layout->ui_element_buffer.memory + offset_data->panel.self.element);

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
        &window->core.dimension.pos, &title_dim, offset_data->border,
        layout->ui_element_buffer.memory
    );

    // @question Do I also need to check for empty text here?
    if (offset_data->label.self.element) {
        cache_vertices(
            app,
            &offset_data->label,
            layout, camera_step_closer(gpu_api_type, zindex),
            mem
        );
    }
}

void cache_vertices(
    void* app, UIWindowOffset* offset_data, GpuApiType gpu_api_type,
    UILayout* const layout, f32 zindex,
    byte* const __restrict mem
) NO_EXCEPT {
    const UIWindow* window = (UIWindow*) (layout->ui_element_buffer.memory + offset_data->self.element);
    ArrayVector<Vertex3DSamplerTextureColor>* vertex_cache = &layout->ui_vertex_cache;
    ArrayVector<int32>* index_cache = &layout->ui_index_cache;

    if (offset_data->panel.self.element) {
        const UIPanel* panel = (UIPanel *) (layout->ui_element_buffer.memory + offset_data->panel.self.element);

        vertex_rect_create(
            vertex_cache, index_cache, zindex, 0,
            {window->core.dimension.pos.x, window->core.dimension.pos.y, window->core.dimension.dimension.width, window->core.dimension.dimension.height},
            UI_ALIGN_V_TOP | UI_ALIGN_H_LEFT,
            panel->background_style.color
        );
    }

    /*
    for (int32 i = 0; i < ARRAY_COUNT(offset_data->border); ++i) {
        if (i == 0 && offset_data->border[i].self.element) {
            // @todo this should be in the UIAttributeBorder file?
            // @todo the offsets are wrong
            UIAttributeBorder* border = (UIAttributeBorder*) (layout->ui_element_buffer.memory + offset_data->border[i].self.element);
            array_vector_insert(vertex_cache, {{window->core.dimension.pos.x, window->core.dimension.pos.y + 19, zindex}, 2, border->tex_coord[0]});
            array_vector_insert(vertex_cache, {{window->core.dimension.pos.x + 27, window->core.dimension.pos.y + 19, zindex}, 2, border->tex_coord[1]});
            array_vector_insert(vertex_cache, {{window->core.dimension.pos.x, window->core.dimension.pos.y, zindex}, 2, border->tex_coord[3]});

            array_vector_insert(vertex_cache, {{window->core.dimension.pos.x + 27, window->core.dimension.pos.y +19, zindex}, 2, border->tex_coord[1]});
            array_vector_insert(vertex_cache, {{window->core.dimension.pos.x + 27, window->core.dimension.pos.y, zindex}, 2, border->tex_coord[2]});
            array_vector_insert(vertex_cache, {{window->core.dimension.pos.x, window->core.dimension.pos.y, zindex}, 2, border->tex_coord[3]});

            vertex_count += 6;
        }
    }
    */

    if (offset_data->title.self.element) {
        cache_vertices(
            app, &offset_data->title, gpu_api_type,
            layout, camera_step_closer(gpu_api_type, zindex),
            mem
        );
    }

    // @todo cache window buttons
}

#endif