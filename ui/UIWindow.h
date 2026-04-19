#pragma once
#ifndef COMS_UI_WINDOW_H
#define COMS_UI_WINDOW_H

#include "../stdlib/Stdlib.h"
#include "../gpuapi/RenderUtils.h"
#include "../camera/Camera.cpp"
#include "attribute/UIAttributeBorder.h"
#include "attribute/UIAttributeShadow.h"
#include "attribute/UIAttributeFont.h"
#include "attribute/UIAttributeBackground.h"
#include "attribute/UIAttributeDimension.h"
#include "UIAnimation.h"
#include "UIAlignment.h"
#include "UIPanel.h"
#include "UILabel.h"
#include "UIStyleType.h"

struct UIOffset {
    int32 element;
    int32 type;

    // We sometimes need to reference parent elements because we need information from them
    // e.g. A window title text (label) is part of a title panel, which is in turn part of a window
    //      We need to recursively find the parent element which the anchor position
    //      to calculate the position of this (child) element:
    //      Example: window.x + title.x + label.x = 10 + 5 + 0
    //              Yes, in this example the title bar is offset to the window (unusual but possible)
    int32 parent_offset;
    int32 parent_type;

    // How many vertices do we have in this element
    int32 vertices_count;

    // Where do the vertices for this element start in the vertex buffer
    int32 vertices;

    int32 children;
    int32 children_count;
};

struct UILabelOffset {
    UIOffset self;

    int32 text;
};

struct UIPanelOffset {
    UIOffset self;

    int32 background;
};

struct UIBorderOffset {
    UIOffset self;
};

struct UIWindowTitleOffset {
    UIOffset self;
    UIBorderOffset border[8];
    UIPanelOffset panel;
    UILabelOffset label;
};

struct UIWindowOffset {
    UIOffset self;
    UIWindowTitleOffset title;
    UIBorderOffset border[8];
    UIPanelOffset panel;
};

struct UIWindowState {
};

struct UIWindow {
    UIAttributeDimension dimension;
    UIAnimation animation;
    byte padding;
    byte alignment;
    byte opacity;

    uintptr_t background;
    UIBackgroundStyle background_style;
    UIAttributeShadow shadow_outer;
    UIAttributeShadow shadow_inner;

    UIWindow* styles[UI_STYLE_TYPE_SIZE];
};

void ui_window_state_serialize(const UIWindowState* __restrict state, byte** __restrict pos)
{
    (void *) state;
    (void **) pos;
}

void ui_window_state_unserialize(UIWindowState* __restrict state, const byte** __restrict pos)
{
    (void *) state;
    (void **) pos;
}

void ui_window_state_populate(const UIAttributeGroup* __restrict group, UIWindowState* __restrict state)
{
    (void *) group;
    (void *) state;
}

void ui_window_element_serialize(const UIWindow* __restrict details, byte** __restrict pos)
{
    (void *) details;
    (void **) pos;
}

void ui_window_element_unserialize(UIWindow* __restrict details, const byte** __restrict pos)
{
    (void *) details;
    (void **) pos;
}

void ui_window_element_populate(
    UILayout* const layout,
    UIElement* const element,
    const UIAttributeGroup* const __restrict  group,
    UIWindow* const __restrict window
) {
    PSEUDO_USE(layout);
    PSEUDO_USE(element);
    PSEUDO_USE(group);
    PSEUDO_USE(window);

    /*
    v4_f32 parent_dimension = {0};
    if (element->parent) {
        UIElement* parent = (UIElement *) (layout->data + element->parent);
        // @bug How to ensure that the parent is initialized before the child element
        // Currently the order of the initialization depends on the theme file, NOT the layout file
        // We could fix it by loading the style based on the layout order but this would result in many misses when looking up styles
        //      The reason for these misses are, that often only 1-2 style_types exist per element

        switch (parent->type) {
            case UI_ELEMENT_TYPE_VIEW_PANEL: {
                    UIPanel* parent_window = (UIPanel *) (layout->data + parent->style_types[UI_STYLE_TYPE_ACTIVE]);
                    parent_dimension = parent_window->dimension.dimension;
                } break;
            default:
                UNREACHABLE();
        }
    }

    if (!element->vertices_active_offset && !element->vertex_count_max) {
        element->vertices_active_offset = layout->active_vertex_offset;
        const UIAttribute* const vertex_attr = ui_attribute_from_group(group, UI_ATTRIBUTE_TYPE_VERTEX_COUNT);

        // @todo Strongly depends on the window components (e.g. title bar, close button, ...)
        element->vertex_count_max = (uint16) (vertex_attr ? vertex_attr->value_int : 8);

        layout->active_vertex_offset += element->vertex_count_max;
    }

    const UIAttribute* const attributes = (UIAttribute *) (group + 1);

    // First set all values, which we can set immediately
    for (int i = 0; i < group->attribute_count; ++i) {
        switch (attributes[i].attribute_id) {
            case UI_ATTRIBUTE_TYPE_POSITION_X:
            case UI_ATTRIBUTE_TYPE_DIMENSION_WIDTH:
            case UI_ATTRIBUTE_TYPE_POSITION_Y:
            case UI_ATTRIBUTE_TYPE_DIMENSION_HEIGHT: {
                    ui_theme_assign_dimension(&window->dimension, &attributes[i]);
                } break;
        }
    }
    */
}

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

        if (!first_element) {
            first_element = title_border_tl;
        }

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
        title_label->content = (wchar_t*) buffer_memory_get(&layout->ui_element_buffer, sizeof(title), alignof(wchar_t));
        memcpy(title_label->content, title, sizeof(title));

        window->title.label.self.element = (int32) MEMORY_OFFSET(title_label, layout->ui_element_buffer.memory);

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
    // @todo We need an enum of types
    window->self.type = 1;

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
    window->title.panel.self.element = (int32) MEMORY_OFFSET(main_panel, layout->ui_element_buffer.memory);

    return window;
}

FORCE_INLINE
void* ui_get_element(UILayout* const layout, int32 offset) {
    return layout->ui_element_buffer.memory + offset;
}

int32 cache_vertices(
    UILabelOffset* offset_data,
    UILayout* const layout, f32 zindex,
    RingMemory* const __restrict ring
) {
    int32 idx = layout->ui_vertex_cache.count;

    FontSystem* const font = layout->font;
    UILabel* label = (UILabel*) (layout->ui_element_buffer.memory + offset_data->self.element);

    v3_int32 text_dim = vertex_text_create(
        layout->ui_vertex_cache.elements + idx, zindex, 1,
        {1024.0f, 512.0f, 0.0f, 0.0f}, UI_ALIGN_H_LEFT | UI_ALIGN_V_BOTTOM,
        font, label->content, 11.0f, 0xFFFFFFFF,
        ring
    );

    layout->ui_vertex_cache.count += text_dim.z;

    idx += text_dim.z;

    return idx;
}

int32 cache_vertices(
    UIWindowTitleOffset* offset_data, GpuApiType gpu_api_type,
    UILayout* const layout, f32 zindex,
    RingMemory* const __restrict ring
) {
    int32 vertex_count = 0;

    UIOffset* parent = &offset_data->self;

    // Iterate all parents to get window position
    // 1 is just a placeholder for the window type id
    while (parent && parent->type != 1) {
        if (!parent->parent_offset) {
            parent = NULL;
            break;
        }

        parent = (UIOffset*) (((uintptr_t) offset_data) - parent->parent_offset);
    }

    UIWindow* window = (UIWindow *) (layout->ui_element_buffer.memory + parent->element);
    ArrayVector<Vertex3DSamplerTextureColor>* vertex_cache = &layout->ui_vertex_cache;

    // @todo the title needs access to the parent (window)
    if (offset_data->panel.self.element) {
        array_vector_insert(vertex_cache, {{window->dimension.pos.x, window->dimension.pos.y, zindex}, 2, {}});
        array_vector_insert(vertex_cache, {{window->dimension.pos.x, window->dimension.pos.y, zindex}, 2, {}});
        array_vector_insert(vertex_cache, {{window->dimension.pos.x, window->dimension.pos.y, zindex}, 2, {}});

        array_vector_insert(vertex_cache, {{window->dimension.pos.x, window->dimension.pos.y, zindex}, 2, {}});
        array_vector_insert(vertex_cache, {{window->dimension.pos.x, window->dimension.pos.y, zindex}, 2, {}});
        array_vector_insert(vertex_cache, {{window->dimension.pos.x, window->dimension.pos.y, zindex}, 2, {}});

        vertex_count += 6;
    }

    for (int32 i = 0; i < ARRAY_COUNT(offset_data->border); ++i) {
        if (offset_data->border[i].self.element) {
            // @todo implement border
        }
    }

    // @question Do I also need to check for empty text here?
    if (offset_data->label.self.element) {
        vertex_count += cache_vertices(
            &offset_data->label,
            layout, camera_step_closer(gpu_api_type, zindex),
            ring
        );
    }

    return vertex_count;
}

int32 cache_vertices(
    UIWindowOffset* offset_data, GpuApiType gpu_api_type,
    UILayout* const layout, f32 zindex,
    RingMemory* const __restrict ring
) NO_EXCEPT {
    int32 vertex_count = 0;

    UIWindow* window = (UIWindow*) (layout->ui_element_buffer.memory + offset_data->self.element);
    ArrayVector<Vertex3DSamplerTextureColor>* vertex_cache = &layout->ui_vertex_cache;

    if (offset_data->panel.self.element) {
        array_vector_insert(vertex_cache, {{window->dimension.pos.x, window->dimension.pos.y, zindex}, 2, {}});
        array_vector_insert(vertex_cache, {{window->dimension.pos.x, window->dimension.pos.y, zindex}, 2, {}});
        array_vector_insert(vertex_cache, {{window->dimension.pos.x, window->dimension.pos.y, zindex}, 2, {}});

        array_vector_insert(vertex_cache, {{window->dimension.pos.x, window->dimension.pos.y, zindex}, 2, {}});
        array_vector_insert(vertex_cache, {{window->dimension.pos.x, window->dimension.pos.y, zindex}, 2, {}});
        array_vector_insert(vertex_cache, {{window->dimension.pos.x, window->dimension.pos.y, zindex}, 2, {}});

        vertex_count += 6;
    }

    for (int32 i = 0; i < ARRAY_COUNT(offset_data->border); ++i) {
        if (i == 0 && offset_data->border[i].self.element) {
            // @todo this should be in the UIAttributeBorder file?
            UIAttributeBorder* border = (UIAttributeBorder*) (layout->ui_element_buffer.memory + offset_data->border[i].self.element);
            array_vector_insert(vertex_cache, {{window->dimension.pos.x, window->dimension.pos.y + 19, zindex}, 2, {border->tex_coord[0].x, border->tex_coord[0].y}});
            array_vector_insert(vertex_cache, {{window->dimension.pos.x + 27, window->dimension.pos.y + 19, zindex}, 2, {border->tex_coord[1].x, border->tex_coord[1].y}});
            array_vector_insert(vertex_cache, {{window->dimension.pos.x, window->dimension.pos.y, zindex}, 2, {border->tex_coord[3].x, border->tex_coord[3].y}});

            array_vector_insert(vertex_cache, {{window->dimension.pos.x + 27, window->dimension.pos.y +19, zindex}, 2, {border->tex_coord[1].x, border->tex_coord[1].y}});
            array_vector_insert(vertex_cache, {{window->dimension.pos.x + 27, window->dimension.pos.y, zindex}, 2, {border->tex_coord[2].x, border->tex_coord[2].y}});
            array_vector_insert(vertex_cache, {{window->dimension.pos.x, window->dimension.pos.y, zindex}, 2, {border->tex_coord[3].x, border->tex_coord[3].y}});

            vertex_count += 6;
        }
    }

    if (offset_data->title.self.element) {
        vertex_count += cache_vertices(
            &offset_data->title, gpu_api_type,
            layout, camera_step_closer(gpu_api_type, zindex),
            ring
        );
    }

    // @todo cache window buttons

    return vertex_count;
}

void ui_cache(
    GpuApiType gpu_api_type,
    UILayout* const layout,
    RingMemory* const __restrict ring
) NO_EXCEPT
{
    // @todo Reset only during testing:
    array_vector_reset(&layout->ui_vertex_cache);

    /////////////////////////////////////////////////////////////////
    // Cache vertices: Should only happen on changes in this element
    /////////////////////////////////////////////////////////////////
    int32* iter;
    array_vector_iterate_start(layout->ui_offset_root, iter) {
        UIOffset* const offset = (UIOffset *) (layout->ui_offset_buffer.memory + *iter);

        switch (offset->type) {
            case 1: {
                UIWindowOffset* test_window_offset = (UIWindowOffset*) offset;

                // @bug This assert isn't really working since we don't know how large vertices_count will be
                //      We would have to simulate/guess the max vertex count and check against this
                ASSERT_TRUE(
                    test_window_offset->self.vertices_count + layout->ui_vertex_cache.count
                        <= layout->ui_vertex_cache.capacity
                );

                test_window_offset->self.vertices = layout->ui_vertex_cache.count;
                cache_vertices(
                    test_window_offset, gpu_api_type,
                    layout, 10.0f, // @todo fix actual value
                    ring
                );
                test_window_offset->self.vertices_count = layout->ui_vertex_cache.count - test_window_offset->self.vertices;

                // @question This means ui_vertex_cache MUST be tightly packed
                //          AND it mustn't have unused data (not the case for pre-calculated hover styles)
                //          For that reason we might need a vertex_array with all the possible data and one with tightly packed data
                //          In an ideal scenario the tightly packed data is just replaced by equally long data without memmoves required
            } break;
            default:
                UNREACHABLE();
        }
    } array_vector_iterate_end;
}

#endif