#pragma once
#ifndef COMS_UI_LABEL_C
#define COMS_UI_LABEL_C

#include "../stdlib/Stdlib.h"
#include "../gpuapi/RenderUtils.h"
#include "../memory/BufferMemory.cpp"
#include "attribute/UIAttributeDimension.cpp"
#include "UIAlignment.h"
#include "UICore.cpp"
#include "UILayout.h"
#include "UILabel.h"

#define UI_LABEL_DEFAULT_FONT_SIZE 11.0f

FORCE_INLINE
int32 ui_char_type_size(CharType char_type) NO_EXCEPT
{
    return char_type == CHAR_TYPE_CHAR ? (int32) sizeof(char) : (int32) sizeof(wchar_t);
}

/**
 * Reserves zeroed text memory in the ui_element_buffer
 *
 * @param length Length in characters incl. \0
 *
 * @return Offset into the ui_element_buffer
 */
static inline
int32 ui_text_reserve(UILayout* const layout, CharType char_type, int32 length) NO_EXCEPT
{
    const int32 size = ui_char_type_size(char_type) * length;
    byte* const memory = memory_get(&layout->ui_element_buffer, size, alignof(wchar_t));
    memset(memory, 0, size);

    return ui_element_offset(layout, memory);
}

/**
 * Sets the content of a label
 * If the reserved memory is too small new memory is reserved
 */
static inline
void ui_label_set_text(UILayout* const layout, UILabel* const label, const char* const text) NO_EXCEPT
{
    ASSERT_TRUE(label->char_type == CHAR_TYPE_CHAR);

    const int32 length = text ? (int32) strlen(text) + 1 : 1;
    if (!label->content || label->content_length < length) {
        label->content = ui_text_reserve(layout, label->char_type, length);
        label->content_length = length;
    }

    char* const content = (char *) (layout->ui_element_buffer.memory + label->content);
    if (text) {
        memcpy(content, text, length);
    } else {
        *content = '\0';
    }

    label->core.change_type |= UI_ELEMENT_CHANGE_CONTENT;
    ui_layout_mark_dirty(layout);
}

/**
 * Same as ui_label_set_text but for the pattern which is used by update functions (e.g. "FPS: %n")
 */
static inline
void ui_label_set_pattern(UILayout* const layout, UILabel* const label, const char* const pattern) NO_EXCEPT
{
    ASSERT_TRUE(label->char_type == CHAR_TYPE_CHAR);

    const int32 length = pattern ? (int32) strlen(pattern) + 1 : 1;
    if (!label->pattern || label->pattern_length < length) {
        label->pattern = ui_text_reserve(layout, label->char_type, length);
        label->pattern_length = length;
    }

    char* const content = (char *) (layout->ui_element_buffer.memory + label->pattern);
    if (pattern) {
        memcpy(content, pattern, length);
    } else {
        *content = '\0';
    }
}

/**
 * @param pattern_length Reserved length in characters incl. \0 (0 = no pattern)
 * @param content_length Reserved length in characters incl. \0 (0 = no content)
 * @param parent NULL = root
 */
UILabel* ui_label_create(
    UILayout* const layout,
    CharType char_type,
    int32 pattern_length = 0,
    int32 content_length = 0,
    UICore* parent = NULL
) NO_EXCEPT
{
    UILabel* const label = UI_ELEMENT_ALLOC(layout, UI_ELEMENT_TYPE_LABEL, UILabel, parent);
    label->core.dimension.align_anchor = UI_ANCHOR_ALIGN_TOP_LEFT;
    label->char_type = char_type;
    label->font.size = UI_LABEL_DEFAULT_FONT_SIZE;
    label->font.color = 0xFFFFFFFF;

    // The text memory is placed directly after the label
    if (pattern_length) {
        label->pattern = ui_text_reserve(layout, char_type, pattern_length);
        label->pattern_length = pattern_length;
    }

    if (content_length) {
        label->content = ui_text_reserve(layout, char_type, content_length);
        label->content_length = content_length;
    }

    return label;
}

/**
 * The label size is the text size unless the element defines a dimension
 * The text is placed in the top left corner of the element (@see UIAttributeFont::alignment for other alignments)
 */
void ui_label_cache(
    UILabel* const label,
    UILayout* const layout,
    f32 zindex,
    BufferMemory* const __restrict mem
) NO_EXCEPT
{
    FontSystem* const font = layout->font;

    // @todo Support wchar_t labels (requires a wchar_t version of text_to_glyphs)
    const bool has_text = font
        && label->content
        && label->char_type == CHAR_TYPE_CHAR
        && label->font.size > 0.0f
        && *((const char *) (layout->ui_element_buffer.memory + label->content));

    if (!has_text) {
        ui_dimension_calculate(layout, &label->core);
        return;
    }

    const char* const text = (const char *) (layout->ui_element_buffer.memory + label->content);
    const f32 scale = label->font.size / font->base.size;

    int32 glyph_count;
    const int16* const glyphs = text_to_glyphs(font, text, mem, &glyph_count);
    const v2_f32 text_dim = text_calculate_dimensions(font, glyphs, glyph_count, scale);

    ui_dimension_calculate(layout, &label->core, text_dim);

    const UIAttributeDimension* const dim = &label->core.dimension;

    // vertex_text_create expects the bottom left corner of the first line
    v4_f32 origin = {dim->pos.x, dim->pos.y - font->base.line_height * scale, 0.0f, 0.0f};

    // Text alignment inside of the element (only matters if the element is larger than the text)
    if (label->font.alignment & UI_ALIGN_H_RIGHT) {
        origin.x += dim->dim.x - text_dim.x;
    } else if (label->font.alignment & UI_ALIGN_H_CENTER) {
        origin.x += (dim->dim.x - text_dim.x) * 0.5f;
    }

    if (label->font.alignment & UI_ALIGN_V_CENTER) {
        origin.y -= (dim->dim.y - text_dim.y) * 0.5f;
    }

    vertex_text_create(
        &layout->ui_vertex_cache, &layout->ui_index_cache, zindex,
        origin, 0,
        font, glyphs, glyph_count,
        label->font.size, label->font.color
    );
}

////////////////////////////////////////////////////////////////
// Text helpers (used by all elements that render text)
////////////////////////////////////////////////////////////////

FORCE_INLINE
f32 ui_text_line_height(const FontSystem* const font, f32 size) NO_EXCEPT
{
    return font->base.line_height * (size / font->base.size);
}

/**
 * Renders utf8 text with the top left corner of the first line at (x, y)
 *
 * @return Text dimension
 */
inline
v2_f32 ui_text_vertices(
    UILayout* const layout,
    const char* const text,
    f32 size, uint32 color,
    f32 x, f32 y, f32 zindex,
    BufferMemory* const __restrict mem
) NO_EXCEPT
{
    FontSystem* const font = layout->font;
    if (!font || !text || !*text || size <= 0.0f) {
        return {0.0f, 0.0f};
    }

    const f32 scale = size / font->base.size;

    int32 glyph_count;
    const int16* const glyphs = text_to_glyphs(font, text, mem, &glyph_count);
    const v2_f32 text_dim = text_calculate_dimensions(font, glyphs, glyph_count, scale);

    vertex_text_create(
        &layout->ui_vertex_cache, &layout->ui_index_cache, zindex,
        {x, y - font->base.line_height * scale, 0.0f, 0.0f}, 0,
        font, glyphs, glyph_count,
        size, color
    );

    return text_dim;
}

/**
 * Width of the text range [start, end) (line breaks restart the width -> width of the last line)
 * No temporary memory is needed (e.g. usable during input handling)
 */
inline
f32 ui_text_range_width(
    const FontSystem* const font,
    const char* const text, int32 start, int32 end,
    f32 size
) NO_EXCEPT
{
    if (!font || !text || end <= start || size <= 0.0f) {
        return 0.0f;
    }

    const f32 scale = size / font->base.size;
    f32 width = 0.0f;

    int32 i = start;
    while (i < end) {
        uint32 codepoint;
        const int32 byte_len = utf8_decode(text + i, &codepoint);
        if (byte_len <= 0) {
            ++i;
            continue;
        }

        i += byte_len;

        if (codepoint == '\n') {
            width = 0.0f;
            continue;
        }

        const Glyph* glyph = font_glyph_find(&font->base, codepoint);
        if (!glyph && font->has_extended) {
            glyph = font_glyph_find(&font->extended, codepoint);
        }

        if (!glyph) {
            // The 0 index character is the special character for "not found" (@see text_to_glyphs)
            glyph = &font->base.glyphs[0];
        }

        width += (glyph->metrics.width + glyph->metrics.offset_x + glyph->metrics.advance_x) * scale;
    }

    return width;
}

/**
 * Width of the LAST line of the first `length` bytes of an utf8 text (e.g. to position a caret)
 */
FORCE_INLINE
f32 ui_text_width(
    const FontSystem* const font,
    const char* const text, int32 length,
    f32 size,
    BufferMemory* const __restrict /* mem */ = NULL
) NO_EXCEPT
{
    return ui_text_range_width(font, text, 0, length, size);
}

#endif
