#pragma once
#ifndef COMS_UI_LABEL_C
#define COMS_UI_LABEL_C

#include "../stdlib/Stdlib.h"
#include "../gpuapi/RenderUtils.h"
#include "../font/FontSystem.cpp"
#include "../memory/BufferMemory.cpp"
#include "../utils/SimpleString.h"
#include "../utils/SimpleStringUtils.h"
#include "attribute/UIAttributeDimension.cpp"
#include "UIAlignment.h"
#include "UICore.cpp"
#include "UILayout.h"
#include "UILabel.h"

#define UI_LABEL_DEFAULT_FONT_SIZE 11.0f

/**
 * Reserves zeroed memory in the ui_element_buffer for element data that isn't a text
 * (e.g. bit sets, arrays of colors/offsets/widths, fixed size table cells)
 * Texts with a length use ui_string_reserve()
 *
 * @param int32 size        Size in bytes
 * @param int32 alignment   Alignment of the data (e.g. alignof(f32))
 *
 * @return int32 Offset into the ui_element_buffer
 */
static inline
int32 ui_buffer_reserve(UILayout* const layout, int32 size, int32 alignment) NO_EXCEPT
{
    byte* const memory = memory_get(&layout->ui_element_buffer, size, alignment);
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
 * @param int32 capacity Reserved length in characters incl. \0
 *
 * @return int32 Offset of the characters in the ui_element_buffer
 */
static inline
int32 ui_string_reserve(UILayout* const layout, CharType char_type, int32 capacity) NO_EXCEPT
{
    ASSERT_TRUE(capacity > 0);

    // The header is the same for all character types
    const int32 size = (int32) sizeof(SimpleStringInline<char>) + char_type_size(char_type) * capacity;
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

// Reserved length in characters excl. \0 (0 if there is no text)
FORCE_INLINE
int32 ui_string_max_length(const UILayout* const layout, int32 offset) NO_EXCEPT
{
    return offset
        ? simple_string_inline_from_str((char *) (layout->ui_element_buffer.memory + offset))->max_length
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
    ASSERT_STRICT((int32) str_length(str.str) == str.length);

    return str;
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
 * @return int32 Offset into the ui_element_buffer (0 = count is 0)
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
 * @param int32 count Amount of strings in the list (stored by the owner, e.g. UISelect::option_count)
 *
 * @return SimpleString<const char> {0, 0, ""} if the list is empty
 */
FORCE_INLINE
SimpleString<const char> ui_string_list_get(const UILayout* const layout, int32 offset, int32 count, int32 index) NO_EXCEPT
{
    ASSERT_TRUE(offset && index >= 0 && index < count);

    const int32* const starts = (const int32 *) (layout->ui_element_buffer.memory + offset);
    const char* const characters = (const char *) (starts + count + 1);

    const int32 length = starts[index + 1] - starts[index] - 1;
    SimpleString<const char> str = { length, length, characters + starts[index] };

    return str;
}

/**
 * Sets a text in the ui_element_buffer (label content/pattern)
 * New memory is only reserved if the reserved memory is too small (the reserved length is stored with the text)
 *
 * @param UILayout*     layout  UI layout
 * @param int32*        offset  Text offset (0 = no memory reserved yet)
 * @param const char*   text    Text (doesn't need to be \0 terminated)
 * @param int32         length  Length in characters excl. \0 (e.g. the return value of sprintf_fast)
 *
 * @return bool true if the text changed
 */
static inline
bool ui_text_set(
    UILayout* const layout, int32* const offset,
    const char* const text, int32 length
) NO_EXCEPT
{
    if (!*offset || length > ui_string_max_length(layout, *offset)) {
        *offset = ui_string_reserve(layout, CHAR_TYPE_CHAR, length + 1);

        char* const content = (char *) (layout->ui_element_buffer.memory + *offset);
        memcpy(content, text, length);
        content[length] = '\0';
        ui_string_length_set(layout, *offset, length);

        return true;
    }

    // The text fits (see above) -> it isn't cut
    char* const content = (char *) (layout->ui_element_buffer.memory + *offset);

    const bool changed = memcmp(content, text, length) != 0 || content[length] != '\0';
    if (changed) {
        str_copy(content, text, length);
    }

    ui_string_length_set(layout, *offset, length);

    return changed;
}

// Text at an offset in the ui_element_buffer, e.g. label->content or label->pattern ("" for offset 0)
// ui_string_get() also returns the length
FORCE_INLINE
const char* ui_text_get(const UILayout* const layout, int32 offset) NO_EXCEPT
{
    return offset ? (const char *) (layout->ui_element_buffer.memory + offset) : "";
}

/**
 * Changes the content of a label without ever reserving new memory (the text is cut if it doesn't fit)
 * Only flags the label as changed if the text changed
 *
 * @return bool true if the text changed
 */
static inline
bool ui_label_set_text_fixed(UILayout* const layout, UILabel* const label, const char* const text) NO_EXCEPT
{
    ASSERT_TRUE(label->char_type == CHAR_TYPE_CHAR);
    ASSERT_TRUE(label->content);

    // The text is cut if it doesn't fit
    char* const content = (char *) (layout->ui_element_buffer.memory + label->content);
    const int32 length = min_branched((int32) str_length(text), ui_string_max_length(layout, label->content));

    if (memcmp(content, text, length) == 0 && content[length] == '\0') {
        return false;
    }

    str_copy(content, text, length);
    ui_string_length_set(layout, label->content, length);

    label->core.change_type |= UI_ELEMENT_CHANGE_CONTENT;
    layout->dirty |= UI_LAYOUT_DIRTY;

    return true;
}

/**
 * Sets the content of a label, only flags it as changed if the text changed
 * -> can be called every frame (the text is only copied if it differs)
 * If the reserved memory is too small new memory is reserved
 *
 * @param int32 length Length in characters excl. \0 (e.g. the return value of sprintf_fast)
 *
 * @return bool true if the text changed
 */
static inline
bool ui_label_set_text(UILayout* const layout, UILabel* const label, const char* const text, int32 length) NO_EXCEPT
{
    ASSERT_TRUE(label->char_type == CHAR_TYPE_CHAR);

    if (!ui_text_set(layout, &label->content, text, length)) {
        return false;
    }

    label->core.change_type |= UI_ELEMENT_CHANGE_CONTENT;
    layout->dirty |= UI_LAYOUT_DIRTY;

    return true;
}

/**
 * Same as ui_label_set_text but for the pattern which is used by update functions (e.g. "FPS: %n")
 * The pattern isn't rendered -> the label isn't flagged as changed
 *
 * @param int32 length Length in characters excl. \0
 */
static inline
void ui_label_set_pattern(UILayout* const layout, UILabel* const label, const char* const pattern, int32 length) NO_EXCEPT
{
    ASSERT_TRUE(label->char_type == CHAR_TYPE_CHAR);

    ui_text_set(layout, &label->pattern, pattern, length);
}

/**
 * @param int32     pattern_length  Reserved length in characters incl. \0 (0 = no pattern)
 * @param int32     content_length  Reserved length in characters incl. \0 (0 = no content)
 * @param UICore*   parent          NULL = root
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
    }

    if (content_length) {
        label->content = ui_string_reserve(layout, char_type, content_length);
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
    // Same scale as vertex_text_create() (incl. the global font scale)
    const f32 scale = label->font.size * font->base.scale / font->base.size;

    // The text dimension is needed before the vertices are created (element size + alignment inside of the element)
    // -> the glyphs are created here and passed to vertex_text_create (no second conversion)
    // The byte length is an upper bound of the codepoint count (cheaper than utf8_strlen())
    const int32 byte_count = (int32) str_length(text);
    int16* const glyphs = (int16*) memory_get_temp(mem, (byte_count + 1) * sizeof(int16), alignof(uintptr_t));
    int32 glyph_count = text_to_glyphs(font, text, byte_count, glyphs);
    const v2_f32 text_dim = text_calculate_dimensions(font, glyphs, glyph_count, scale);

    ui_dimension_calculate(layout, &label->core, text_dim);

    const UIAttributeDimension* const dim = &label->core.dimension;

    // Top left corner of the text
    v4_f32 origin = {dim->pos.x, dim->pos.y, 0.0f, 0.0f};

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
        origin, UI_ALIGN_V_TOP,
        font, glyphs, glyph_count,
        label->font.size, label->font.color
    );
}

#endif
