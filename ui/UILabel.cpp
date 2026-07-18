#pragma once
#ifndef COMS_UI_LABEL_C
#define COMS_UI_LABEL_C

#include "../stdlib/Stdlib.h"
#include "../gpuapi/RenderUtils.h"
#include "UIAlignment.h"
#include "attribute/UIAttribute.h"
#include "attribute/UIAttributeFont.h"
#include "attribute/UIAttributeDimension.cpp"
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
    UILabel* label,
    UILayout* const layout, f32 zindex,
    byte* const __restrict mem
) NO_EXCEPT {
    FontSystem* const font = layout->font;
    const f32 font_size = label->font.size / font->base.size;

    label->core.vertices = layout->ui_vertex_cache.count;

    // @performance Do I really want to do it hear or somewhere else, maybe separate from the caching
    //              But that would mean iterating the elements twice
    if (label->core.update_func) {
        layout->update[label->core.update_func - 1](app, layout, &label->core);
    }

    ui_dimension_calculate(layout, &label->core);

    void* label_content = layout->ui_element_buffer.memory + label->content;

    if (label->char_type == CHAR_TYPE_CHAR) {
        vertex_text_create(
            &layout->ui_vertex_cache, &layout->ui_index_cache, zindex, 1,
            {label->core.dimension.pos.x, label->core.dimension.pos.y - font->base.line_height * font_size, 0.0f, 0.0f}, label->core.dimension.alignment,
            font, (const char* const) label_content, label->font.size, label->font.color,
            mem
        );
    } else {
        vertex_text_create(
            &layout->ui_vertex_cache, &layout->ui_index_cache, zindex, 1,
            {label->core.dimension.pos.x, label->core.dimension.pos.y - font->base.line_height * font_size, 0.0f, 0.0f}, label->core.dimension.alignment,
            font, (const wchar_t* const) label_content, label->font.size, label->font.color,
            mem
        );
    }

    label->core.vertex_count = (int16) (layout->ui_vertex_cache.count - label->core.vertices);

    const int32 element_offset = (int32) MEMORY_OFFSET(label, layout->ui_element_buffer.memory);
    for (int32 i = 0; i < layout->ui_element_changed.count; ++i) {
        if (layout->ui_element_changed.elements[i].element == element_offset) {
            array_vector_remove_index(&layout->ui_element_changed, i);
            break;
        }
    }
}

template <typename T>
static
int32 ui_label_reserve_text(UILayout* layout, int32* offset, const T* content) NO_EXCEPT {
    if (!content) {
        return NULL;
    }

    const int32 length = (int32) str_length(content) + 1;

    if (*offset) {
        //ASSERT_TRUE(content_length > str_length(content));
        memcpy(
            (char *) (layout->ui_element_buffer.memory + *offset),
            content, length * sizeof(T)
        );
    } else {
        byte* label_content = memory_get(
            &layout->ui_element_buffer,
            length * sizeof(T),
            alignof(T)
        );
        memcpy(label_content, content, length * sizeof(T));

        *offset = (int32) MEMORY_OFFSET(label_content, layout->ui_element_buffer.memory);
    }

    return length;
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
        MEMORY_ELEMENT_ZERO(label);

        element = &label->core;
        element->type = UI_ELEMENT_TYPE_LABEL;
    }

    label->char_type = char_type;
    label->core.opacity = 0xFF;

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