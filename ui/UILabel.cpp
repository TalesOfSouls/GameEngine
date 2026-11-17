#pragma once
#ifndef COMS_UI_LABEL_C
#define COMS_UI_LABEL_C

#include "../stdlib/Stdlib.h"
#include "../gpuapi/RenderUtils.h"
#include "../memory/BufferMemory.cpp"
#include "../utils/SimpleString.h"
#include "../utils/SimpleStringUtils.h"
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
 * Text with a known length in the ui_element_buffer (label content/pattern, input text/placeholder)
 *
 *      SimpleStringInline      length = current length, max_length = capacity - 1 (both in characters)
 *      characters[capacity]    <- the returned offset, always \0 terminated
 *
 * The offset points to a normal C string -> code that only reads the text doesn't have to care about the header.
 * The length is available without strlen (ui_string_get).
 *
 * WARNING: Code that changes the characters directly must update the length
 *
 * @param capacity Reserved length in characters incl. \0
 *
 * @return Offset of the characters in the ui_element_buffer
 */
static inline
int32 ui_string_reserve(UILayout* const layout, CharType char_type, int32 capacity) NO_EXCEPT
{
    ASSERT_TRUE(capacity > 0);

    // The header is the same for all character types
    const int32 size = (int32) sizeof(SimpleStringInline<char>) + ui_char_type_size(char_type) * capacity;
    byte* const memory = memory_get(&layout->ui_element_buffer, size, alignof(SimpleStringInline<char>));
    memset(memory, 0, size);

    SimpleStringInline<char>* const str = simple_string_inline_init<char>(memory, capacity - 1);

    return ui_element_offset(layout, str + 1);
}

// Length in characters excl. \0 (0 if there is no text)
FORCE_INLINE
int32 ui_string_length(const UILayout* const layout, int32 offset) NO_EXCEPT
{
    return offset
        ? simple_string_inline_from_str((char *) (layout->ui_element_buffer.memory + offset))->length
        : 0;
}

// Must be called by everyone who changes the characters of a text directly
FORCE_INLINE
void ui_string_length_set(const UILayout* const layout, int32 offset, int32 length) NO_EXCEPT
{
    SimpleStringInline<char>* const str = simple_string_inline_from_str(
        (char *) (layout->ui_element_buffer.memory + offset)
    );
    ASSERT_TRUE(length >= 0 && length <= str->max_length);

    str->length = length;
}

// char text incl. its length ({0, 0, ""} if there is no text), the text is \0 terminated
FORCE_INLINE
SimpleString<const char> ui_string_get(const UILayout* const layout, int32 offset) NO_EXCEPT
{
    ASSERT_TRUE(offset);
    const SimpleString<const char> str = simple_string_view(
        simple_string_inline_from_str((const char *) (layout->ui_element_buffer.memory + offset))
    );

    // Somebody changed the characters without updating the length
    ASSERT_TRUE(str.str[str.length] == '\0');
    ASSERT_STRICT((int32) strlen(str.str) == str.length);

    return str;
}

/**
 * Copies a \0 terminated text into a fixed size buffer (cut at capacity - 1) in a single pass
 * Compares while copying -> no strlen + strcmp/strncmp before the copy
 *
 * @param changed Set to true if the buffer content changed
 *
 * @return Amount of copied characters excl. \0 (src[return] != '\0' -> the text was cut)
 */
static inline
int32 ui_text_copy_length(
    char* const __restrict dst, int32 capacity,
    const char* const __restrict src,
    bool* const changed
) NO_EXCEPT
{
    ASSERT_TRUE(capacity > 0);

    int32 diff = 0;
    int32 i = 0;
    for (; i < capacity - 1 && src[i]; ++i) {
        diff |= dst[i] ^ src[i];
        dst[i] = src[i];
    }

    diff |= dst[i];
    dst[i] = '\0';

    *changed = diff != 0;

    return i;
}

/**
 * Same as ui_text_copy_length but only returns if the buffer content changed
 */
FORCE_INLINE
bool ui_text_copy(char* const __restrict dst, int32 capacity, const char* const __restrict src) NO_EXCEPT
{
    bool changed;
    ui_text_copy_length(dst, capacity, src, &changed);

    return changed;
}

/**
 * Same as above but with a known length (e.g. the return value of sprintf_fast)
 *
 * @param length Length in characters excl. \0
 */
static inline
bool ui_text_copy(char* const __restrict dst, int32 capacity, const char* const __restrict src, int32 length) NO_EXCEPT
{
    ASSERT_TRUE(capacity > 0);

    length = min_branched(length, capacity - 1);
    if (memcmp(dst, src, length) == 0 && dst[length] == '\0') {
        return false;
    }

    memcpy(dst, src, length);
    dst[length] = '\0';

    return true;
}

/**
 * String list in the ui_element_buffer (select options, tab titles, table rows)
 *
 *      int32 starts[count + 1]     start of every string relative to the characters, starts[count] = total size
 *      char characters[]           "a\0b\0c\0"
 *
 * -> the n-th string and its length are available without walking the list (no strlen)
 * The strings stay \0 terminated so they can still be used as C strings
 *
 * @return Offset into the ui_element_buffer (0 = count is 0)
 */
static inline
int32 ui_string_list_store(UILayout* const layout, const SimpleString<const char>* const strings, int32 count) NO_EXCEPT
{
    if (count <= 0) {
        return 0;
    }

    int32 size = 0;
    for (int32 i = 0; i < count; ++i) {
        size += strings[i].length + 1;
    }

    const int32 table_size = (int32) sizeof(int32) * (count + 1);
    byte* const memory = memory_get(&layout->ui_element_buffer, table_size + size, alignof(int32));

    int32* const starts = (int32 *) memory;
    char* const characters = (char *) (memory + table_size);

    int32 pos = 0;
    for (int32 i = 0; i < count; ++i) {
        starts[i] = pos;
        if (strings[i].length) {
            memcpy(characters + pos, strings[i].str, strings[i].length);
        }

        pos += strings[i].length;
        characters[pos++] = '\0';
    }
    starts[count] = pos;

    return ui_element_offset(layout, memory);
}

/**
 * n-th string of a string list
 *
 * @param count Amount of strings in the list (stored by the owner, e.g. UISelect::option_count)
 *
 * @return {0, 0, ""} if the list is empty
 */
FORCE_INLINE
SimpleString<const char> ui_string_list_get(const UILayout* const layout, int32 offset, int32 count, int32 index) NO_EXCEPT
{
    ASSERT_TRUE(index >= 0 && index < count);

    if (!offset) {
        SimpleString<const char> empty = { 0, 0, "" };
        return empty;
    }

    const int32* const starts = (const int32 *) (layout->ui_element_buffer.memory + offset);
    const char* const characters = (const char *) (starts + count + 1);

    const int32 length = starts[index + 1] - starts[index] - 1;
    SimpleString<const char> str = { length, length, characters + starts[index] };

    return str;
}

/**
 * Sets a \0 terminated text in the ui_element_buffer (label content/pattern)
 * New memory is only reserved if the reserved memory is too small
 *
 * @param offset Text offset (0 = no memory reserved yet)
 * @param capacity Reserved length in characters incl. \0
 *
 * @return true if the text changed
 */
static inline
bool ui_text_set(UILayout* const layout, int32* const offset, int32* const capacity, const char* const text) NO_EXCEPT
{
    if (!text) {
        if (!*offset) {
            *offset = ui_string_reserve(layout, CHAR_TYPE_CHAR, 1);
            *capacity = 1;
        }

        char* const content = (char *) (layout->ui_element_buffer.memory + *offset);
        const bool changed = *content != '\0';
        *content = '\0';
        ui_string_length_set(layout, *offset, 0);

        return changed;
    }

    int32 length = 0;
    if (*offset) {
        char* const content = (char *) (layout->ui_element_buffer.memory + *offset);

        bool changed;
        length = ui_text_copy_length(content, *capacity, text, &changed);
        if (!text[length]) {
            ui_string_length_set(layout, *offset, length);

            return changed;
        }
    }

    // Not enough memory -> the length of the remaining text is only needed in this (rare) case
    length += (int32) strlen(text + length) + 1;
    *offset = ui_string_reserve(layout, CHAR_TYPE_CHAR, length);
    *capacity = length;
    memcpy(layout->ui_element_buffer.memory + *offset, text, length);
    ui_string_length_set(layout, *offset, length - 1);

    return true;
}

/**
 * Same as above but with a known length (e.g. the return value of sprintf_fast)
 *
 * @param length Length in characters excl. \0
 */
static inline
bool ui_text_set(
    UILayout* const layout, int32* const offset, int32* const capacity,
    const char* const text, int32 length
) NO_EXCEPT
{
    if (!*offset || *capacity < length + 1) {
        *offset = ui_string_reserve(layout, CHAR_TYPE_CHAR, length + 1);
        *capacity = length + 1;

        char* const content = (char *) (layout->ui_element_buffer.memory + *offset);
        memcpy(content, text, length);
        content[length] = '\0';
        ui_string_length_set(layout, *offset, length);

        return true;
    }

    // The text fits (see above) -> it isn't cut
    const bool changed = ui_text_copy((char *) (layout->ui_element_buffer.memory + *offset), *capacity, text, length);
    ui_string_length_set(layout, *offset, length);

    return changed;
}

// Current text of a label ("" if it has none)
FORCE_INLINE
const char* ui_label_text(const UILayout* const layout, const UILabel* const label) NO_EXCEPT
{
    ASSERT_TRUE(label->char_type == CHAR_TYPE_CHAR);

    return label->content ? (const char *) (layout->ui_element_buffer.memory + label->content) : "";
}

// Pattern of a label used by update functions (e.g. "FPS: %n", "" if it has none)
FORCE_INLINE
const char* ui_label_pattern(const UILayout* const layout, const UILabel* const label) NO_EXCEPT
{
    ASSERT_TRUE(label->char_type == CHAR_TYPE_CHAR);

    return label->pattern ? (const char *) (layout->ui_element_buffer.memory + label->pattern) : "";
}

// Same as ui_label_text/ui_label_pattern but incl. the length (no strlen required)
FORCE_INLINE
SimpleString<const char> ui_label_string(const UILayout* const layout, const UILabel* const label) NO_EXCEPT
{
    ASSERT_TRUE(label->char_type == CHAR_TYPE_CHAR);

    return ui_string_get(layout, label->content);
}

FORCE_INLINE
SimpleString<const char> ui_label_pattern_string(const UILayout* const layout, const UILabel* const label) NO_EXCEPT
{
    ASSERT_TRUE(label->char_type == CHAR_TYPE_CHAR);

    return ui_string_get(layout, label->pattern);
}

/**
 * Changes the content of a label without ever reserving new memory (the text is cut if it doesn't fit)
 * Only flags the label as changed if the text changed
 *
 * @return true if the text changed
 */
static inline
bool ui_label_set_text_fixed(UILayout* const layout, UILabel* const label, const char* const text) NO_EXCEPT
{
    ASSERT_TRUE(label->char_type == CHAR_TYPE_CHAR);
    ASSERT_TRUE(label->content);

    bool changed;
    const int32 length = ui_text_copy_length(
        (char *) (layout->ui_element_buffer.memory + label->content), label->content_length,
        text, &changed
    );
    ui_string_length_set(layout, label->content, length);

    if (!changed) {
        return false;
    }

    label->core.change_type |= UI_ELEMENT_CHANGE_CONTENT;
    layout->dirty |= UI_LAYOUT_DIRTY;

    return true;
}

/**
 * Sets the content of a label, only flags it as changed if the text changed
 * -> can be called every frame (compare + copy in a single pass, no strlen)
 * If the reserved memory is too small new memory is reserved
 *
 * @return true if the text changed
 */
static inline
bool ui_label_set_text(UILayout* const layout, UILabel* const label, const char* const text) NO_EXCEPT
{
    ASSERT_TRUE(label->char_type == CHAR_TYPE_CHAR);

    if (!ui_text_set(layout, &label->content, &label->content_length, text)) {
        return false;
    }

    label->core.change_type |= UI_ELEMENT_CHANGE_CONTENT;
    layout->dirty |= UI_LAYOUT_DIRTY;

    return true;
}

/**
 * Same as above but with a known length (e.g. the return value of sprintf_fast)
 *
 * @param length Length in characters excl. \0
 */
static inline
bool ui_label_set_text(UILayout* const layout, UILabel* const label, const char* const text, int32 length) NO_EXCEPT
{
    ASSERT_TRUE(label->char_type == CHAR_TYPE_CHAR);

    if (!ui_text_set(layout, &label->content, &label->content_length, text, length)) {
        return false;
    }

    label->core.change_type |= UI_ELEMENT_CHANGE_CONTENT;
    layout->dirty |= UI_LAYOUT_DIRTY;

    return true;
}

/**
 * Same as ui_label_set_text but for the pattern which is used by update functions (e.g. "FPS: %n")
 * The pattern isn't rendered -> the label isn't flagged as changed
 */
static inline
void ui_label_set_pattern(UILayout* const layout, UILabel* const label, const char* const pattern) NO_EXCEPT
{
    ASSERT_TRUE(label->char_type == CHAR_TYPE_CHAR);

    ui_text_set(layout, &label->pattern, &label->pattern_length, pattern);
}

static inline
void ui_label_set_pattern(UILayout* const layout, UILabel* const label, const char* const pattern, int32 length) NO_EXCEPT
{
    ASSERT_TRUE(label->char_type == CHAR_TYPE_CHAR);

    ui_text_set(layout, &label->pattern, &label->pattern_length, pattern, length);
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
        label->pattern = ui_string_reserve(layout, char_type, pattern_length);
        label->pattern_length = pattern_length;
    }

    if (content_length) {
        label->content = ui_string_reserve(layout, char_type, content_length);
        label->content_length = content_length;
    }

    return label;
}

/**
 * The label size is the text size unless the element defines a dimension
 * The text is placed in the top left corner of the element (alignment for other alignments)
 */
void ui_label_cache(
    UILabel* const label,
    UILayout* const layout,
    f32 zindex,
    BufferMemory* const __restrict mem
) NO_EXCEPT
{
    PROFILE_DEBUG(PROFILE_UI_CACHE_LABEL);

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

    // The byte length is an upper bound of the codepoint count (cheaper than utf8_strlen())
    // @question Why am I doing this stupid stuff, isn't vertex_text_create already returning the dim?
    const int32 byte_count = (int32) strlen(text);
    int16* const glyphs = (int16*) memory_get_temp(mem, (byte_count + 1) * sizeof(int16), alignof(uintptr_t));
    int32 glyph_count = text_to_glyphs(font, text, glyphs);
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

    // @question Why am I doing this stupid stuff, isn't vertex_text_create already returning the dim?
    const int32 byte_count = (int32) strlen(text);
    int16* const glyphs = (int16*) memory_get_temp(mem, (byte_count + 1) * sizeof(int16), alignof(uintptr_t));
    int32 glyph_count = text_to_glyphs(font, text, glyphs);
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
            // The 0 index character is the special character for "not found"
            glyph = &font->base.glyphs[0];
        }

        width += (glyph->metrics.width + glyph->metrics.offset_x + glyph->metrics.advance_x) * scale;
    }

    return width;
}

#endif
