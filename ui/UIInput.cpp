#pragma once
#ifndef COMS_UI_INPUT_C
#define COMS_UI_INPUT_C

#include "../stdlib/Stdlib.h"
#include "../gpuapi/RenderUtils.h"
#include "../camera/Camera.cpp"
#include "../input/Input.h"
#include "attribute/UIAttributeDimension.cpp"
#include "attribute/UIAttributeBorder.cpp"
#include "UIAlignment.h"
#include "UIStyleDefaults.h"
#include "UICore.cpp"
#include "UILabel.cpp"
#include "UIScrollbar.h"
#include "UIClip.h"
#include "UIInput.h"

#define UI_INPUT_CARET_WIDTH 1.0f

// Max bytes that can be pasted at once (stack buffer)
#define UI_INPUT_PASTE_MAX 4096

/**
 * Single line input (UI_ELEMENT_TYPE_INPUT) or multi line textarea (UI_ELEMENT_TYPE_TEXTAREA)
 *
 * @param max_length Max text length in bytes (utf8)
 * @param parent NULL = root
 */
inline
UIInput* ui_input_create(
    UILayout* const layout,
    int32 max_length,
    UICore* parent = NULL,
    UIElementType type = UI_ELEMENT_TYPE_INPUT
) NO_EXCEPT
{
    UIInput* const input = UI_ELEMENT_ALLOC(layout, type, UIInput, parent);
    input->core.flags = UI_ELEMENT_FLAG_INTERACTABLE | UI_ELEMENT_FLAG_HOVERABLE | UI_ELEMENT_FLAG_SELECTABLE;
    input->core.dimension.align_anchor = UI_ANCHOR_ALIGN_TOP_LEFT;
    input->core.dimension.dim_raw = {200.0f, type == UI_ELEMENT_TYPE_TEXTAREA ? 100.0f : 24.0f};

    input->background_color = UI_DEFAULT_CONTROL_BACKGROUND;
    input->border_color = UI_DEFAULT_CONTROL_BORDER;
    input->border_color_focus = UI_DEFAULT_CONTROL_BORDER_FOCUS;
    input->selection_color = UI_DEFAULT_TEXT_SELECTION;
    input->font.size = UI_DEFAULT_FONT_SIZE;
    input->font.color = UI_DEFAULT_TEXT_COLOR;

    // The text memory is placed directly after the element
    input->content_length = max_length + 1;
    input->content = ui_text_reserve(layout, CHAR_TYPE_CHAR, input->content_length);

    return input;
}

FORCE_INLINE
UIInput* ui_textarea_create(UILayout* const layout, int32 max_length, UICore* parent = NULL) NO_EXCEPT
{
    return ui_input_create(layout, max_length, parent, UI_ELEMENT_TYPE_TEXTAREA);
}

FORCE_INLINE
char* ui_input_get_text(const UILayout* const layout, const UIInput* const input) NO_EXCEPT
{
    return (char *) (layout->ui_element_buffer.memory + input->content);
}

/**
 * Sets the text, the text is cut if it is longer than the reserved memory
 * The caret is placed at the end
 */
inline
void ui_input_set_text(UILayout* const layout, UIInput* const input, const char* const text) NO_EXCEPT
{
    char* const content = ui_input_get_text(layout, input);
    int32 length = text ? (int32) strlen(text) : 0;
    length = min_branched(length, input->content_length - 1);

    // Don't cut an utf8 character in half
    while (length > 0 && length < (text ? (int32) strlen(text) : 0) && (text[length] & 0xC0) == 0x80) {
        --length;
    }

    if (length) {
        memcpy(content, text, length);
    }

    content[length] = '\0';
    input->caret = length;
    input->selection_anchor = length;

    input->core.change_type |= UI_ELEMENT_CHANGE_CONTENT;
    ui_layout_mark_dirty(layout);
}

inline
void ui_input_set_placeholder(UILayout* const layout, UIInput* const input, const char* const text) NO_EXCEPT
{
    const int32 length = (int32) strlen(text) + 1;
    input->placeholder = ui_text_reserve(layout, CHAR_TYPE_CHAR, length);
    memcpy(layout->ui_element_buffer.memory + input->placeholder, text, length);

    ui_layout_mark_dirty(layout);
}

// Byte offset of the previous/next utf8 character
FORCE_INLINE
int32 ui_utf8_prev(const char* const text, int32 pos) NO_EXCEPT
{
    if (pos <= 0) {
        return 0;
    }

    --pos;
    while (pos > 0 && (text[pos] & 0xC0) == 0x80) {
        --pos;
    }

    return pos;
}

FORCE_INLINE
int32 ui_utf8_next(const char* const text, int32 pos, int32 length) NO_EXCEPT
{
    if (pos >= length) {
        return length;
    }

    ++pos;
    while (pos < length && (text[pos] & 0xC0) == 0x80) {
        ++pos;
    }

    return pos;
}

FORCE_INLINE
int32 ui_text_line_start(const char* const text, int32 pos) NO_EXCEPT
{
    while (pos > 0 && text[pos - 1] != '\n') {
        --pos;
    }

    return pos;
}

FORCE_INLINE
int32 ui_text_line_end(const char* const text, int32 pos, int32 length) NO_EXCEPT
{
    while (pos < length && text[pos] != '\n') {
        ++pos;
    }

    return pos;
}

// Moves the caret to the same character column in the previous/next line
inline
int32 ui_text_caret_vertical(const char* const text, int32 caret, int32 length, bool up) NO_EXCEPT
{
    const int32 line_start = ui_text_line_start(text, caret);

    // Character column in the current line
    int32 column = 0;
    for (int32 i = line_start; i < caret; i = ui_utf8_next(text, i, length)) {
        ++column;
    }

    int32 target_start;
    if (up) {
        if (line_start == 0) {
            return 0;
        }

        target_start = ui_text_line_start(text, line_start - 1);
    } else {
        const int32 line_end = ui_text_line_end(text, caret, length);
        if (line_end >= length) {
            return length;
        }

        target_start = line_end + 1;
    }

    const int32 target_end = ui_text_line_end(text, target_start, length);
    int32 pos = target_start;
    while (column-- > 0 && pos < target_end) {
        pos = ui_utf8_next(text, pos, length);
    }

    return pos;
}

////////////////////////////////////////////////////////////////
// Selection
////////////////////////////////////////////////////////////////

FORCE_INLINE
bool ui_input_has_selection(const UIInput* const input) NO_EXCEPT
{
    return input->selection_anchor != input->caret;
}

// Selected byte range [start, end)
FORCE_INLINE
void ui_input_selection(const UIInput* const input, int32* const start, int32* const end) NO_EXCEPT
{
    *start = min_branched(input->selection_anchor, input->caret);
    *end = max_branched(input->selection_anchor, input->caret);
}

/**
 * Deletes the selected text (if any), the caret is placed at the start of the selection
 *
 * @return true if something was deleted
 */
inline
bool ui_input_delete_selection(UIInput* const input, char* const text, int32* const length) NO_EXCEPT
{
    if (!ui_input_has_selection(input)) {
        return false;
    }

    int32 start, end;
    ui_input_selection(input, &start, &end);

    memmove(text + start, text + end, *length - end + 1);
    *length -= end - start;
    input->caret = start;
    input->selection_anchor = start;

    return true;
}

////////////////////////////////////////////////////////////////
// Word navigation (ctrl + arrow/backspace/delete)
////////////////////////////////////////////////////////////////

// Bytes >= 0x80 (non ascii utf8) are considered word characters -> we never stop inside of a utf8 character
FORCE_INLINE
bool ui_text_is_word_char(char c) NO_EXCEPT
{
    return (c & 0x80)
        || (c >= 'a' && c <= 'z')
        || (c >= 'A' && c <= 'Z')
        || (c >= '0' && c <= '9')
        || c == '_';
}

FORCE_INLINE
int32 ui_text_word_prev(const char* const text, int32 pos) NO_EXCEPT
{
    while (pos > 0 && !ui_text_is_word_char(text[pos - 1])) {
        --pos;
    }

    while (pos > 0 && ui_text_is_word_char(text[pos - 1])) {
        --pos;
    }

    return pos;
}

FORCE_INLINE
int32 ui_text_word_next(const char* const text, int32 pos, int32 length) NO_EXCEPT
{
    while (pos < length && !ui_text_is_word_char(text[pos])) {
        ++pos;
    }

    while (pos < length && ui_text_is_word_char(text[pos])) {
        ++pos;
    }

    return pos;
}

////////////////////////////////////////////////////////////////
// Text insertion
////////////////////////////////////////////////////////////////

/**
 * Copies the characters of src that are allowed in the input into dst
 *      - \r is removed (\r\n -> \n), tabs become spaces
 *      - single line inputs replace \n with a space
 *      - other control characters are removed
 *      - numeric inputs only allow numeric characters
 * Only complete utf8 characters are copied
 *
 * @return Length of dst in bytes
 */
inline
int32 ui_input_filter(
    const UIInput* const __restrict input,
    const char* const __restrict src, int32 src_length,
    char* const __restrict dst, int32 dst_max
) NO_EXCEPT
{
    const bool multiline = input->core.type == UI_ELEMENT_TYPE_TEXTAREA;
    const bool numeric = input->input_type == UI_INPUT_TYPE_NUMERIC;

    int32 length = 0;
    for (int32 i = 0; i < src_length;) {
        const byte c = (byte) src[i];

        // utf8 character length
        const int32 bytes = c < 0x80 ? 1
            : (c & 0xE0) == 0xC0 ? 2
            : (c & 0xF0) == 0xE0 ? 3
            : (c & 0xF8) == 0xF0 ? 4
            : 0;

        if (!bytes) {
            // Invalid utf8 byte
            ++i;
            continue;
        }

        if (i + bytes > src_length) {
            break;
        }

        char ch = (char) c;
        if (bytes == 1) {
            if (ch == '\t' || (ch == '\n' && !multiline)) {
                ch = ' ';
            } else if ((ch < 0x20 && ch != '\n') || ch == 0x7F) {
                ++i;
                continue;
            }

            if (numeric && !((ch >= '0' && ch <= '9') || ch == '-' || ch == '.' || ch == ',')) {
                ++i;
                continue;
            }
        } else if (numeric) {
            i += bytes;
            continue;
        }

        if (length + bytes > dst_max) {
            break;
        }

        if (bytes == 1) {
            dst[length] = ch;
        } else {
            memcpy(dst + length, src + i, bytes);
        }

        length += bytes;
        i += bytes;
    }

    return length;
}

/**
 * Inserts the text at the caret (replaces the selection)
 * The text is cut if the input is full (only complete utf8 characters)
 *
 * @return Inserted bytes
 */
inline
int32 ui_input_insert(
    UIInput* const __restrict input,
    char* const __restrict text, int32* const __restrict length,
    const char* const __restrict insert, int32 insert_length
) NO_EXCEPT
{
    ui_input_delete_selection(input, text, length);

    // Space left (content_length includes the \0)
    const int32 space = input->content_length - 1 - *length;

    char filtered[UI_INPUT_PASTE_MAX];
    const int32 bytes = ui_input_filter(input, insert, insert_length, filtered, min_branched(space, (int32) sizeof(filtered)));
    if (bytes <= 0) {
        return 0;
    }

    memmove(text + input->caret + bytes, text + input->caret, *length - input->caret + 1);
    memcpy(text + input->caret, filtered, bytes);

    input->caret += bytes;
    input->selection_anchor = input->caret;
    *length += bytes;

    return bytes;
}

////////////////////////////////////////////////////////////////
// Clipboard
////////////////////////////////////////////////////////////////

// The clipboard is provided by the platform layer (@see UILayout::clipboard_set)
inline
void ui_input_copy(const UILayout* const layout, const UIInput* const input, const char* const text) NO_EXCEPT
{
    if (!layout->clipboard_set || !ui_input_has_selection(input)) {
        return;
    }

    int32 start, end;
    ui_input_selection(input, &start, &end);

    layout->clipboard_set(text + start, end - start);
}

/**
 * @return Inserted bytes
 */
inline
int32 ui_input_paste(const UILayout* const layout, UIInput* const input, char* const text, int32* const length) NO_EXCEPT
{
    if (!layout->clipboard_get) {
        return 0;
    }

    char clipboard[UI_INPUT_PASTE_MAX];
    const int32 clipboard_length = layout->clipboard_get(clipboard, (int32) sizeof(clipboard));

    if (clipboard_length <= 0) {
        return 0;
    }

    return ui_input_insert(input, text, length, clipboard, clipboard_length);
}

////////////////////////////////////////////////////////////////
// Scrolling
////////////////////////////////////////////////////////////////

FORCE_INLINE
f32 ui_input_line_height(const UILayout* const layout, const UIInput* const input) NO_EXCEPT
{
    return layout->font ? ui_text_line_height(layout->font, input->font.size) : input->font.size * 1.25f;
}

// Height of the whole text of a textarea incl. padding
inline
f32 ui_input_content_height(const UILayout* const layout, const UIInput* const input) NO_EXCEPT
{
    const char* const text = ui_input_get_text(layout, input);

    int32 lines = 1;
    for (const char* c = text; *c; ++c) {
        lines += *c == '\n';
    }

    return (f32) lines * ui_input_line_height(layout, input) + 2.0f * UI_DEFAULT_TEXT_PADDING;
}

/**
 * Vertical scrollbar of a textarea
 *
 * @return false if the element has no scrollbar (single line input or the text fits)
 */
inline
bool ui_input_scroll_info(const UILayout* const layout, const UIInput* const input, UIScrollInfo* const info) NO_EXCEPT
{
    if (input->core.type != UI_ELEMENT_TYPE_TEXTAREA) {
        return false;
    }

    const UIAttributeDimension* const dim = &input->core.dimension;
    const f32 content = ui_input_content_height(layout, input);
    if (content <= dim->dim.y) {
        return false;
    }

    info->track = {dim->pos.x + dim->dim.x - UI_SCROLLBAR_SIZE - 1.0f, dim->pos.y - 1.0f, UI_SCROLLBAR_SIZE, dim->dim.y - 2.0f};
    info->view = dim->dim.y;
    info->content = content;
    info->scroll = input->scroll.y;

    return true;
}

inline
bool ui_input_scroll_set(UILayout* const layout, UIInput* const input, UIScrollAxis axis, f32 scroll) NO_EXCEPT
{
    f32* const value = axis == UI_SCROLL_AXIS_Y ? &input->scroll.y : &input->scroll.x;

    if (axis == UI_SCROLL_AXIS_Y) {
        scroll = ui_scroll_clamp(scroll, ui_input_content_height(layout, input), input->core.dimension.dim.y);
    } else {
        scroll = max_branched(scroll, 0.0f);
    }

    if (scroll == *value) {
        return false;
    }

    *value = scroll;
    input->core.change_type |= UI_ELEMENT_CHANGE_CONTENT;
    ui_layout_mark_dirty(layout);

    return true;
}

/**
 * Scrolls the text so that the caret is visible
 * Uses the element dimension of the last ui_cache()
 */
inline
void ui_input_scroll_to_caret(UILayout* const layout, UIInput* const input) NO_EXCEPT
{
    const UIAttributeDimension* const dim = &input->core.dimension;
    if (dim->dim.x <= 0.0f || dim->dim.y <= 0.0f) {
        return;
    }

    const char* const text = ui_input_get_text(layout, input);

    if (input->core.type == UI_ELEMENT_TYPE_TEXTAREA) {
        const f32 line_height = ui_input_line_height(layout, input);

        int32 line = 0;
        for (int32 i = 0; i < input->caret; ++i) {
            line += text[i] == '\n';
        }

        const f32 caret_top = (f32) line * line_height;
        const f32 view = dim->dim.y - 2.0f * UI_DEFAULT_TEXT_PADDING;

        f32 scroll = input->scroll.y;
        if (caret_top < scroll) {
            scroll = caret_top;
        } else if (caret_top + line_height > scroll + view) {
            scroll = caret_top + line_height - view;
        }

        ui_input_scroll_set(layout, input, UI_SCROLL_AXIS_Y, scroll);
    } else {
        const f32 caret_x = ui_text_width(layout->font, text, input->caret, input->font.size);
        const f32 text_width = ui_text_width(layout->font, text, (int32) strlen(text), input->font.size);
        const f32 view = dim->dim.x - 2.0f * UI_DEFAULT_TEXT_PADDING - UI_INPUT_CARET_WIDTH;

        f32 scroll = input->scroll.x;
        if (caret_x < scroll) {
            scroll = caret_x;
        } else if (caret_x > scroll + view) {
            scroll = caret_x - view;
        }

        // Deleting text at the end -> show as much text as possible
        scroll = min_branched(scroll, max_branched(text_width - view, 0.0f));

        ui_input_scroll_set(layout, input, UI_SCROLL_AXIS_X, scroll);
    }
}

////////////////////////////////////////////////////////////////
// Pointer selection
////////////////////////////////////////////////////////////////

/**
 * Byte offset in the text that is closest to the position (e.g. mouse click)
 * Uses the element dimension of the last ui_cache()
 */
inline
int32 ui_input_caret_at(const UILayout* const layout, const UIInput* const input, f32 x, f32 y) NO_EXCEPT
{
    const char* const text = ui_input_get_text(layout, input);
    const int32 length = (int32) strlen(text);
    const UIAttributeDimension* const dim = &input->core.dimension;

    // Line under the pointer (textarea only)
    int32 line_start = 0;
    if (input->core.type == UI_ELEMENT_TYPE_TEXTAREA) {
        const f32 line_height = ui_input_line_height(layout, input);
        int32 line = (int32) ((dim->pos.y - UI_DEFAULT_TEXT_PADDING + input->scroll.y - y) / line_height);

        while (line-- > 0) {
            const int32 line_end = ui_text_line_end(text, line_start, length);
            if (line_end >= length) {
                break;
            }

            line_start = line_end + 1;
        }
    }

    const int32 line_end = ui_text_line_end(text, line_start, length);

    // Character whose center is right of the position
    const f32 text_x = dim->pos.x + UI_DEFAULT_TEXT_PADDING - input->scroll.x;
    f32 char_x = text_x;

    int32 pos = line_start;
    while (pos < line_end) {
        const int32 next = ui_utf8_next(text, pos, length);
        const f32 width = ui_text_range_width(layout->font, text, pos, next, input->font.size);

        if (x < char_x + width * 0.5f) {
            return pos;
        }

        char_x += width;
        pos = next;
    }

    return line_end;
}

// Word boundaries around a position (@see ui_text_is_word_char)
inline
void ui_input_select_word(const UILayout* const layout, UIInput* const input, int32 pos) NO_EXCEPT
{
    const char* const text = ui_input_get_text(layout, input);
    const int32 length = (int32) strlen(text);

    int32 start = pos;
    int32 end = pos;

    if (pos < length && ui_text_is_word_char(text[pos])) {
        start = ui_text_word_prev(text, pos + 1);
        end = ui_text_word_next(text, pos, length);
    } else if (pos > 0 && ui_text_is_word_char(text[pos - 1])) {
        start = ui_text_word_prev(text, pos);
        end = pos;
    }

    input->selection_anchor = start;
    input->caret = end;
}

inline
void ui_input_select_all(const UILayout* const layout, UIInput* const input) NO_EXCEPT
{
    input->selection_anchor = 0;
    input->caret = (int32) strlen(ui_input_get_text(layout, input));
}

/**
 * Pointer selection (pressing and dragging in the text)
 *
 * @param start true = pointer pressed, false = pointer moved while pressed (extends the selection)
 * @param click_count 1 = place the caret, 2 = select the word, 3 = select everything
 *
 * @return true if the caret/selection changed
 */
inline
bool ui_input_select_at(UILayout* const layout, UIInput* const input, f32 x, f32 y, bool start, int32 click_count) NO_EXCEPT
{
    const int32 old_caret = input->caret;
    const int32 old_anchor = input->selection_anchor;

    const int32 pos = ui_input_caret_at(layout, input, x, y);

    if (!start) {
        // Dragging only extends single click selections
        if (click_count <= 1) {
            input->caret = pos;
        }
    } else if (click_count == 2) {
        ui_input_select_word(layout, input, pos);
    } else if (click_count >= 3) {
        ui_input_select_all(layout, input);
    } else {
        input->caret = pos;
        input->selection_anchor = pos;
    }

    if (old_caret == input->caret && old_anchor == input->selection_anchor) {
        return false;
    }

    input->core.change_type |= UI_ELEMENT_CHANGE_CONTENT;
    ui_layout_mark_dirty(layout);
    ui_input_scroll_to_caret(layout, input);

    return true;
}

/**
 * Applies one text input code (unicode codepoint or InputTextControl incl. modifier bits) to the input
 *
 *      shift + navigation = extend the selection
 *      ctrl + left/right/backspace/delete = word wise
 *      ctrl + a/c/x/v = select all, copy, cut, paste
 *
 * @return true if the text, caret or selection changed
 */
inline
bool ui_input_handle_code(void* app, UILayout* const layout, UIInput* const input, uint32 code) NO_EXCEPT
{
    char* const text = ui_input_get_text(layout, input);
    int32 length = (int32) strlen(text);
    const bool multiline = input->core.type == UI_ELEMENT_TYPE_TEXTAREA;

    const bool shift = (code & INPUT_TEXT_MOD_SHIFT) != 0;
    const bool ctrl = (code & INPUT_TEXT_MOD_CTRL) != 0;
    code &= ~((uint32) INPUT_TEXT_MOD_MASK);

    const int32 old_caret = input->caret;
    const int32 old_anchor = input->selection_anchor;
    bool text_changed = false;

    // Navigation keys collapse the selection unless shift is pressed
    bool is_navigation = false;

    int32 sel_start, sel_end;
    ui_input_selection(input, &sel_start, &sel_end);
    const bool has_selection = sel_start != sel_end;

    switch (code) {
        case INPUT_TEXT_BACKSPACE: {
            if (ui_input_delete_selection(input, text, &length)) {
                text_changed = true;
            } else if (input->caret > 0) {
                const int32 prev = ctrl ? ui_text_word_prev(text, input->caret) : ui_utf8_prev(text, input->caret);
                memmove(text + prev, text + input->caret, length - input->caret + 1);
                input->caret = prev;
                input->selection_anchor = prev;
                text_changed = true;
            }
        } break;
        case INPUT_TEXT_DELETE: {
            if (ui_input_delete_selection(input, text, &length)) {
                text_changed = true;
            } else if (input->caret < length) {
                const int32 next = ctrl ? ui_text_word_next(text, input->caret, length) : ui_utf8_next(text, input->caret, length);
                memmove(text + input->caret, text + next, length - next + 1);
                text_changed = true;
            }
        } break;
        case INPUT_TEXT_LEFT: {
            is_navigation = true;
            if (has_selection && !shift) {
                input->caret = sel_start;
            } else {
                input->caret = ctrl ? ui_text_word_prev(text, input->caret) : ui_utf8_prev(text, input->caret);
            }
        } break;
        case INPUT_TEXT_RIGHT: {
            is_navigation = true;
            if (has_selection && !shift) {
                input->caret = sel_end;
            } else {
                input->caret = ctrl ? ui_text_word_next(text, input->caret, length) : ui_utf8_next(text, input->caret, length);
            }
        } break;
        case INPUT_TEXT_HOME: {
            is_navigation = true;
            input->caret = multiline && !ctrl ? ui_text_line_start(text, input->caret) : 0;
        } break;
        case INPUT_TEXT_END: {
            is_navigation = true;
            input->caret = multiline && !ctrl ? ui_text_line_end(text, input->caret, length) : length;
        } break;
        case INPUT_TEXT_UP:
        case INPUT_TEXT_DOWN: {
            is_navigation = true;
            if (multiline) {
                input->caret = ui_text_caret_vertical(text, input->caret, length, code == INPUT_TEXT_UP);
            }
        } break;
        case INPUT_TEXT_SELECT_ALL: {
            input->selection_anchor = 0;
            input->caret = length;
        } break;
        case INPUT_TEXT_COPY: {
            ui_input_copy(layout, input, text);
        } break;
        case INPUT_TEXT_CUT: {
            ui_input_copy(layout, input, text);
            text_changed = ui_input_delete_selection(input, text, &length);
        } break;
        case INPUT_TEXT_PASTE: {
            const int32 old_length = length;

            // Replaces the selection if the clipboard contains text
            text_changed = ui_input_paste(layout, input, text, &length) > 0 || length != old_length;
        } break;
        case INPUT_TEXT_ENTER: {
            if (!multiline) {
                if (input->on_submit && layout->on_actions) {
                    layout->on_actions[input->on_submit - 1](app, layout, &input->core);
                }

                return false;
            }

            code = '\n';
        } [[fallthrough]];
        default: {
            // Ignore all other control characters and codes (e.g. tab, escape)
            if ((code < 0x20 && code != '\n') || code >= 0x110000 || code == 0x7F) {
                return false;
            }

            char encoded[4];
            const int32 bytes = utf8_encode(code, encoded);
            if (bytes <= 0) {
                return false;
            }

            // Characters that are not allowed don't replace the selection (e.g. letter in numeric input)
            char filtered[4];
            const int32 filtered_length = ui_input_filter(input, encoded, bytes, filtered, (int32) sizeof(filtered));
            if (filtered_length <= 0) {
                return false;
            }

            // Replaces the selection (the selection is also deleted if the input is full)
            const int32 old_length = length;
            text_changed = ui_input_insert(input, text, &length, filtered, filtered_length) > 0 || length != old_length;
        }
    }

    if (is_navigation && !shift) {
        input->selection_anchor = input->caret;
    }

    if (!text_changed && old_caret == input->caret && old_anchor == input->selection_anchor) {
        return false;
    }

    input->core.change_type |= UI_ELEMENT_CHANGE_CONTENT;
    ui_layout_mark_dirty(layout);
    ui_input_scroll_to_caret(layout, input);

    if (text_changed && input->on_change && layout->on_actions) {
        layout->on_actions[input->on_change - 1](app, layout, &input->core);
    }

    return true;
}

void ui_input_cache(
    UIInput* const input,
    GpuApiType gpu_api_type,
    UILayout* const layout,
    f32* const zindex,
    BufferMemory* const __restrict mem
) NO_EXCEPT
{
    ui_dimension_calculate(layout, &input->core);
    const UIAttributeDimension* const dim = &input->core.dimension;
    const v4_f32 rect = ui_dimension_rect(dim);

    ArrayVector<Vertex3DSamplerTextureColor>* const vertices = &layout->ui_vertex_cache;
    ArrayVector<int32>* const indices = &layout->ui_index_cache;

    ui_rect_vertices(rect, input->background_color, vertices, indices, *zindex);
    *zindex = camera_step_closer(gpu_api_type, *zindex);

    const bool is_editing = (input->core.state & UI_STATE_FOCUS) != 0;
    ui_border_solid_vertices(
        rect,
        (is_editing || (input->core.state & UI_STATE_HOVER)) ? input->border_color_focus : input->border_color,
        1,
        vertices, indices, *zindex
    );
    *zindex = camera_step_closer(gpu_api_type, *zindex);

    FontSystem* const font = layout->font;
    if (!font) {
        return;
    }

    UIScrollInfo scroll_info;
    const bool has_scrollbar = ui_input_scroll_info(layout, input, &scroll_info);

    // Everything inside of the border (and left of the scrollbar) is clipped
    const UIClipRect clip = ui_clip_from_rect({
        dim->pos.x + 1.0f,
        dim->pos.y - 1.0f,
        max_branched(dim->dim.x - 2.0f - (has_scrollbar ? UI_SCROLLBAR_SIZE : 0.0f), 0.0f),
        max_branched(dim->dim.y - 2.0f, 0.0f)
    });

    const int32 vertex_start = vertices->count;
    const int32 index_start = indices->count;

    const char* const text = ui_input_get_text(layout, input);
    const f32 line_height = ui_text_line_height(font, input->font.size);

    // Single line inputs are vertically centered, textareas start at the top
    const f32 text_x = dim->pos.x + UI_DEFAULT_TEXT_PADDING - input->scroll.x;
    const f32 text_y = input->core.type == UI_ELEMENT_TYPE_TEXTAREA
        ? dim->pos.y - UI_DEFAULT_TEXT_PADDING + input->scroll.y
        : dim->pos.y - (dim->dim.y - line_height) * 0.5f;

    // Selection background (behind the text), one rect per selected line
    if (is_editing && ui_input_has_selection(input)) {
        int32 start, end;
        ui_input_selection(input, &start, &end);
        const int32 length = (int32) strlen(text);

        int32 line = 0;
        for (int32 i = 0; i < start; ++i) {
            line += text[i] == '\n';
        }

        for (int32 pos = start; pos < end; ++line) {
            const int32 line_end = min_branched(ui_text_line_end(text, pos, length), end);

            const f32 x0 = text_x + ui_text_width(font, text, pos, input->font.size);
            f32 x1 = text_x + ui_text_width(font, text, line_end, input->font.size);

            // A selected line break is shown as a small box (like most editors)
            if (line_end < end) {
                x1 += input->font.size * 0.3f;
            }

            ui_rect_vertices(
                {x0, text_y - (f32) line * line_height, x1 - x0, line_height},
                input->selection_color,
                vertices, indices, *zindex
            );

            pos = line_end + 1;
        }

        *zindex = camera_step_closer(gpu_api_type, *zindex);
    }

    if (*text) {
        ui_text_vertices(layout, text, input->font.size, input->font.color, text_x, text_y, *zindex, mem);
    } else if (input->placeholder && !is_editing) {
        ui_text_vertices(
            layout, (const char *) (layout->ui_element_buffer.memory + input->placeholder),
            input->font.size, UI_DEFAULT_PLACEHOLDER_COLOR, text_x, text_y, *zindex, mem
        );
    }

    // Caret
    if (is_editing) {
        int32 line = 0;
        for (int32 i = 0; i < input->caret; ++i) {
            line += text[i] == '\n';
        }

        const f32 caret_x = text_x + ui_text_width(font, text, input->caret, input->font.size);
        const f32 caret_y = text_y - (f32) line * line_height;

        *zindex = camera_step_closer(gpu_api_type, *zindex);
        ui_rect_vertices({caret_x, caret_y, UI_INPUT_CARET_WIDTH, line_height}, input->font.color, vertices, indices, *zindex);
    }

    // Text that doesn't fit is cut off (@see ui_input_scroll_to_caret)
    ui_vertices_clip_or_remove(vertices, indices, vertex_start, index_start, clip);

    *zindex = camera_step_closer(gpu_api_type, *zindex);

    if (has_scrollbar) {
        ui_scrollbar_vertices(
            &scroll_info, UI_SCROLL_AXIS_Y,
            layout->interaction.scroll_dragged == ui_element_offset(layout, input),
            gpu_api_type, vertices, indices, zindex
        );
    }
}

#endif
