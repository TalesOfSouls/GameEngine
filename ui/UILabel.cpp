#pragma once
#ifndef COMS_UI_LABEL_C
#define COMS_UI_LABEL_C

#include "../stdlib/Stdlib.h"
#include "../gpuapi/RenderUtils.h"
#include "UIAlignment.h"
#include "attribute/UIAttribute.h"
#include "attribute/UIAttributeFont.h"
#include "attribute/UIAttributeDimension.h"
#include "UIStyleType.h"
#include "UILayout.h"
#include "UIWindow.h"
#include "UILabel.h"
#include "UIUber.h"
#include "UICore.cpp"
#include "../object/Vertex.h"

static
void ui_vertices_cache(
    void* app,
    UICore* element,
    UILayout* const layout, f32 zindex,
    byte* const __restrict mem
) NO_EXCEPT {
    FontSystem* const font = layout->font;
    UILabel* label = (UILabel*) element;
    const f32 font_size = label->font.size / font->base.size;

    element->vertices = layout->ui_vertex_cache.count;

    // @performance Do I really want to do it hear or somewhere else, maybe separate from the caching
    //              But that would mean iterating the elements twice
    if (element->update_func) {
        layout->update[element->update_func - 1](app, layout, element);
    }

    ui_dimension_calculate(layout, element);

    void* label_content = layout->ui_element_buffer.memory + label->content;

    if (label->char_type == CHAR_TYPE_CHAR) {
        vertex_text_create(
            &layout->ui_vertex_cache, &layout->ui_index_cache, zindex, 1,
            {element->dimension.pos.x, element->dimension.pos.y - font->base.line_height * font_size, 0.0f, 0.0f}, element->dimension.alignment,
            font, (const char* const) label_content, label->font.size, label->font.color,
            mem
        );
    } else {
        vertex_text_create(
            &layout->ui_vertex_cache, &layout->ui_index_cache, zindex, 1,
            {element->dimension.pos.x, element->dimension.pos.y - font->base.line_height * font_size, 0.0f, 0.0f}, element->dimension.alignment,
            font, (const wchar_t* const) label_content, label->font.size, label->font.color,
            mem
        );
    }

    element->vertex_count = (int16) (layout->ui_vertex_cache.count - element->vertices);

    int32 element_index = (int32) MEMORY_OFFSET(label, layout->ui_element_buffer.memory);
    array_vector_remove(&layout->ui_element_changed, element_index);
}

static
UILabel* ui_label_create(
    UILayout* layout,
    CharType char_type,
    int32 pattern_length = 0,
    int32 content_length = 0,
    UICore* element = NULL
) NO_EXCEPT {
    UILabel* label = (UILabel *) element;

    if (!label) {
        label = (UILabel*) BUFFER_ELEMENT_GET(&layout->ui_element_buffer, UILabel);
        MEMORY_ELEMENT_ZERO(element);

        element = &label->core;
        element->type = UI_ELEMENT_TYPE_LABEL;
    }

    label->char_type = char_type;

    if (pattern_length) {
        const byte* temp = memory_get(
            &layout->ui_element_buffer,
            (char_type == CHAR_TYPE_CHAR ? sizeof(char) : sizeof(wchar_t)) * pattern_length,
            alignof(size_t)
        );

        label->pattern = (int32) MEMORY_OFFSET(temp, layout->ui_element_buffer.memory);
        label->pattern_length = pattern_length;
    }

    if (content_length) {
        const byte* temp = memory_get(
            &layout->ui_element_buffer,
            (char_type == CHAR_TYPE_CHAR ? sizeof(char) : sizeof(wchar_t)) * content_length,
            alignof(size_t)
        );

        label->content = (int32) MEMORY_OFFSET(temp, layout->ui_element_buffer.memory);
        label->content_length = content_length;
    }

    return label;
}

#endif