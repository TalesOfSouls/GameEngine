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
    UILabelOffset* offset_data,
    UILayout* const layout, f32 zindex,
    byte* const __restrict mem
) NO_EXCEPT {
    FontSystem* const font = layout->font;
    UILabel* label = (UILabel*) (layout->ui_element_buffer.memory + offset_data->self.element);
    const f32 font_size = label->font.size / font->base.size;

    offset_data->self.vertices = layout->ui_vertex_cache.count;

    // @performance Do I really want to do it hear or somewhere else, maybe separate from the caching
    //              But that would mean iterating the elements twice
    if (label->core.update_func) {
        label->core.update_func(app, layout, (UIOffset *) offset_data, label);
    }

    ui_dimension_calculate(layout, &offset_data->self, &label->core);

    if (label->char_type == CHAR_TYPE_CHAR) {
        vertex_text_create(
            &layout->ui_vertex_cache, &layout->ui_index_cache, zindex, 1,
            {label->core.dimension.pos.x, label->core.dimension.pos.y - font->base.line_height * font_size, 0.0f, 0.0f}, label->core.dimension.alignment,
            font, label->content, label->font.size, label->font.color,
            mem
        );
    } else {
        vertex_text_create(
            &layout->ui_vertex_cache, &layout->ui_index_cache, zindex, 1,
            {label->core.dimension.pos.x, label->core.dimension.pos.y - font->base.line_height * font_size, 0.0f, 0.0f}, label->core.dimension.alignment,
            font, (wchar_t*) label->content, label->font.size, label->font.color,
            mem
        );
    }

    offset_data->self.vertices_count = (int16) (layout->ui_vertex_cache.count - offset_data->self.vertices);
    offset_data->self.is_changed = false;
}

static
UILabelOffset* ui_label_create(UILayout* layout, CharType char_type, int32 pattern_length = 0, int32 content_length = 0) NO_EXCEPT {
    UILabelOffset* label = (UILabelOffset*) BUFFER_ELEMENT_GET(&layout->ui_offset_buffer, UILabelOffset);
    MEMORY_ELEMENT_ZERO(label);

    // We need to add this offset to the root array for iteration later on
    array_vector_insert(&layout->ui_offset_root, (int32) MEMORY_OFFSET(label, layout->ui_offset_buffer.memory));

    UILabel* label_element = (UILabel*) BUFFER_ELEMENT_GET(&layout->ui_element_buffer, UILabel);
    MEMORY_ELEMENT_ZERO(label_element);

    label->self.element = (int32) MEMORY_OFFSET(label_element, layout->ui_element_buffer.memory);
    // @todo We need an enum of types
    label->self.type = UI_ELEMENT_TYPE_LABEL;
    label_element->char_type = char_type;

    if (pattern_length) {
        label_element->pattern = (char *) memory_get(
            &layout->ui_element_buffer,
            (char_type == CHAR_TYPE_CHAR ? sizeof(char) : sizeof(wchar_t)) * pattern_length,
            alignof(size_t)
        );
        label_element->pattern_length = pattern_length;
    }

    if (content_length) {
        label_element->content = (char *) memory_get(
            &layout->ui_element_buffer,
            (char_type == CHAR_TYPE_CHAR ? sizeof(char) : sizeof(wchar_t)) * content_length,
            alignof(size_t)
        );
        label_element->content_length = content_length;
    }

    return label;
}

#endif