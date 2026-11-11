#include "../../stdlib/Stdlib.h"
#include "../TestFramework.h"
#include "../../ui/UILayout.h"
#include "../../ui/UILayout.cpp"
#include "../../input/KeyNames.h"
#include "../../system/Allocator.h"

/**
 * Tests for the texts with a stored length (@see ui_string_reserve)
 *
 * Labels and inputs store their text as C string with a SimpleStringInline header in front of it.
 * Every function that changes the text must keep the stored length identical to strlen().
 */

static void ui_text_test_alloc(UILayout* const layout) {
    memset(layout, 0, sizeof(*layout));
    buffer_alloc(&layout->ui_element_buffer, 1 * MEGABYTE, 1 * MEGABYTE, 64);
    layout->data_size = 1 * MEGABYTE;
    layout->data = (byte *) platform_alloc_aligned(layout->data_size, layout->data_size, 64);
    memset(layout->data, 0, layout->data_size);
    ui_layout_reset(layout);
    ui_root_create(layout, 800.0f, 600.0f);
}

static void ui_text_test_free(UILayout* const layout) {
    buffer_free(&layout->ui_element_buffer);
    platform_aligned_free((void **) &layout->data);
}

// The stored length must always match the C string
#define TEST_UI_STRING(layout, offset, expected) do {                                                   \
    const SimpleString<const char> _str = ui_string_get((layout), (offset));                           \
    TEST_EQUALS(strcmp(_str.str, (expected)), 0);                                                       \
    TEST_EQUALS(_str.length, (int32) strlen(expected));                                                 \
    TEST_EQUALS(_str.length, (int32) strlen(_str.str));                                                 \
} while (0)

static void test_ui_string_reserve() {
    UILayout layout;
    ui_text_test_alloc(&layout);

    const int32 offset = ui_string_reserve(&layout, CHAR_TYPE_CHAR, 16);
    TEST_TRUE(offset > 0);

    // The header is stored directly in front of the characters
    const SimpleStringInline<char>* const header = ui_string_header(&layout, offset);
    TEST_TRUE((const byte *) (header + 1) == layout.ui_element_buffer.memory + offset);
    TEST_EQUALS(((uintptr_t) header) % alignof(int32), 0);
    TEST_EQUALS(header->length, 0);
    TEST_EQUALS(header->max_length, 15);
    TEST_UI_STRING(&layout, offset, "");

    memcpy(layout.ui_element_buffer.memory + offset, "abc", 4);
    ui_string_length_set(&layout, offset, 3);
    TEST_EQUALS(ui_string_length(&layout, offset), 3);
    TEST_UI_STRING(&layout, offset, "abc");

    // No text
    TEST_EQUALS(ui_string_length(&layout, 0), 0);
    TEST_EQUALS(ui_string_get(&layout, 0).length, 0);
    TEST_EQUALS(strcmp(ui_string_get(&layout, 0).str, ""), 0);

    // wchar_t texts use the same header (length in characters)
    const int32 wide = ui_string_reserve(&layout, CHAR_TYPE_WCHAR, 8);
    TEST_EQUALS(ui_string_header(&layout, wide)->max_length, 7);
    TEST_EQUALS(((uintptr_t) (layout.ui_element_buffer.memory + wide)) % alignof(wchar_t), 0);

    ui_text_test_free(&layout);
}

static void test_ui_label_text_length() {
    UILayout layout;
    ui_text_test_alloc(&layout);

    UILabel* const label = ui_label_create(&layout, CHAR_TYPE_CHAR, 8, 8);
    TEST_UI_STRING(&layout, label->content, "");
    TEST_UI_STRING(&layout, label->pattern, "");

    // Fits
    TEST_TRUE(ui_label_set_text(&layout, label, "Hello"));
    TEST_UI_STRING(&layout, label->content, "Hello");
    TEST_EQUALS(ui_label_string(&layout, label).length, 5);
    TEST_EQUALS(strcmp(ui_label_text(&layout, label), "Hello"), 0);

    // Unchanged
    TEST_FALSE(ui_label_set_text(&layout, label, "Hello"));
    TEST_UI_STRING(&layout, label->content, "Hello");

    // Shorter
    TEST_TRUE(ui_label_set_text(&layout, label, "Hi"));
    TEST_UI_STRING(&layout, label->content, "Hi");

    // Too long -> new memory
    const int32 old_content = label->content;
    TEST_TRUE(ui_label_set_text(&layout, label, "A much longer text"));
    TEST_TRUE(label->content != old_content);
    TEST_EQUALS(label->content_length, 19);
    TEST_UI_STRING(&layout, label->content, "A much longer text");

    // Known length
    TEST_TRUE(ui_label_set_text(&layout, label, "12345678", 4));
    TEST_UI_STRING(&layout, label->content, "1234");
    TEST_FALSE(ui_label_set_text(&layout, label, "1234", 4));

    // Known length, too long -> new memory
    TEST_TRUE(ui_label_set_text(&layout, label, "This text needs even more memory", 32));
    TEST_UI_STRING(&layout, label->content, "This text needs even more memory");

    // NULL = empty
    TEST_TRUE(ui_label_set_text(&layout, label, NULL));
    TEST_UI_STRING(&layout, label->content, "");

    // Pattern
    ui_label_set_pattern(&layout, label, "FPS: %n");
    TEST_UI_STRING(&layout, label->pattern, "FPS: %n");
    TEST_EQUALS(ui_label_pattern_string(&layout, label).length, 7);
    ui_label_set_pattern(&layout, label, "A pattern that doesn't fit", 26);
    TEST_UI_STRING(&layout, label->pattern, "A pattern that doesn't fit");

    // Label without reserved memory
    UILabel* const empty = ui_label_create(&layout, CHAR_TYPE_CHAR);
    TEST_EQUALS(empty->content, 0);
    TEST_EQUALS(ui_label_string(&layout, empty).length, 0);
    TEST_FALSE(ui_label_set_text_fixed(&layout, empty, "ignored"));
    TEST_TRUE(ui_label_set_text(&layout, empty, "Now it has text"));
    TEST_UI_STRING(&layout, empty->content, "Now it has text");

    // NULL for a label without memory reserves an empty text
    UILabel* const empty2 = ui_label_create(&layout, CHAR_TYPE_CHAR);
    TEST_FALSE(ui_label_set_text(&layout, empty2, NULL));
    TEST_TRUE(empty2->content != 0);
    TEST_UI_STRING(&layout, empty2->content, "");

    ui_text_test_free(&layout);
}

static void test_ui_label_text_fixed() {
    UILayout layout;
    ui_text_test_alloc(&layout);

    UILabel* const label = ui_label_create(&layout, CHAR_TYPE_CHAR, 0, 8);
    const int32 content = label->content;

    TEST_TRUE(ui_label_set_text_fixed(&layout, label, "abc"));
    TEST_UI_STRING(&layout, label->content, "abc");
    TEST_FALSE(ui_label_set_text_fixed(&layout, label, "abc"));

    // Cut, never new memory
    TEST_TRUE(ui_label_set_text_fixed(&layout, label, "0123456789"));
    TEST_EQUALS(label->content, content);
    TEST_UI_STRING(&layout, label->content, "0123456");

    TEST_TRUE(ui_label_set_text_fixed(&layout, label, ""));
    TEST_UI_STRING(&layout, label->content, "");

    ui_text_test_free(&layout);
}

static void test_ui_tooltip_text_length() {
    UILayout layout;
    ui_text_test_alloc(&layout);

    UIPanel* const tooltip = ui_tooltip_create(&layout);
    UILabel* const label = ui_tooltip_label(&layout, tooltip);

    ui_tooltip_show(&layout, tooltip, "Line 1\nLine 2", 10.0f, 10.0f);
    TEST_UI_STRING(&layout, label->content, "Line 1\nLine 2");

    ui_tooltip_show(&layout, tooltip, "Short", 5, 10.0f, 10.0f);
    TEST_UI_STRING(&layout, label->content, "Short");

    ui_tooltip_show(&layout, tooltip, "Known length text", 5, 10.0f, 10.0f);
    TEST_UI_STRING(&layout, label->content, "Known");

    ui_text_test_free(&layout);
}

static void test_ui_input_text_length() {
    UILayout layout;
    ui_text_test_alloc(&layout);

    UIInput* const input = ui_input_create(&layout, 8);
    TEST_EQUALS(input->content_length, 9);
    TEST_EQUALS(ui_input_text_length(&layout, input), 0);
    TEST_UI_STRING(&layout, input->content, "");

    ui_input_set_text(&layout, input, "abc");
    TEST_UI_STRING(&layout, input->content, "abc");
    TEST_EQUALS(ui_input_text_length(&layout, input), 3);
    TEST_EQUALS(ui_input_string(&layout, input).length, 3);
    TEST_EQUALS(input->caret, 3);

    // Cut at the reserved memory
    ui_input_set_text(&layout, input, "0123456789");
    TEST_UI_STRING(&layout, input->content, "01234567");
    TEST_EQUALS(input->caret, 8);

    // Known length
    ui_input_set_text(&layout, input, "abcdef", 2);
    TEST_UI_STRING(&layout, input->content, "ab");

    // An utf8 character is never cut in half (ä = 2 bytes)
    ui_input_set_text(&layout, input, "1234567\xC3\xA4");
    TEST_UI_STRING(&layout, input->content, "1234567");

    ui_input_set_text(&layout, input, NULL);
    TEST_UI_STRING(&layout, input->content, "");

    ui_input_set_placeholder(&layout, input, "Search...");
    TEST_UI_STRING(&layout, input->placeholder, "Search...");

    ui_text_test_free(&layout);
}

static void ui_text_test_type(UILayout* const layout, UIInput* const input, const char* text) {
    while (*text) {
        ui_input_handle_code(NULL, layout, input, (uint32) (byte) *text++);
    }
}

static void test_ui_input_edit_length() {
    UILayout layout;
    ui_text_test_alloc(&layout);

    UIInput* const input = ui_input_create(&layout, 16);

    // Typing
    ui_text_test_type(&layout, input, "hello");
    TEST_UI_STRING(&layout, input->content, "hello");
    TEST_EQUALS(input->caret, 5);

    // Backspace
    TEST_TRUE(ui_input_handle_code(NULL, &layout, input, INPUT_TEXT_BACKSPACE));
    TEST_UI_STRING(&layout, input->content, "hell");

    // Navigation doesn't change the length
    TEST_TRUE(ui_input_handle_code(NULL, &layout, input, INPUT_TEXT_HOME));
    TEST_UI_STRING(&layout, input->content, "hell");
    TEST_EQUALS(input->caret, 0);

    // Backspace at the start does nothing
    TEST_FALSE(ui_input_handle_code(NULL, &layout, input, INPUT_TEXT_BACKSPACE));
    TEST_UI_STRING(&layout, input->content, "hell");

    // Delete
    TEST_TRUE(ui_input_handle_code(NULL, &layout, input, INPUT_TEXT_DELETE));
    TEST_UI_STRING(&layout, input->content, "ell");

    // Insert in the middle
    TEST_TRUE(ui_input_handle_code(NULL, &layout, input, INPUT_TEXT_RIGHT));
    ui_text_test_type(&layout, input, "XY");
    TEST_UI_STRING(&layout, input->content, "eXYll");

    // End + delete at the end does nothing
    TEST_TRUE(ui_input_handle_code(NULL, &layout, input, INPUT_TEXT_END));
    TEST_EQUALS(input->caret, 5);
    TEST_FALSE(ui_input_handle_code(NULL, &layout, input, INPUT_TEXT_DELETE));
    TEST_UI_STRING(&layout, input->content, "eXYll");

    // Multi byte character (ä = 2 bytes)
    TEST_TRUE(ui_input_handle_code(NULL, &layout, input, 0xE4));
    TEST_UI_STRING(&layout, input->content, "eXYll\xC3\xA4");
    TEST_TRUE(ui_input_handle_code(NULL, &layout, input, INPUT_TEXT_BACKSPACE));
    TEST_UI_STRING(&layout, input->content, "eXYll");

    // Word wise delete (ctrl + backspace/delete)
    ui_input_set_text(&layout, input, "one two three");
    TEST_TRUE(ui_input_handle_code(NULL, &layout, input, INPUT_TEXT_BACKSPACE | INPUT_TEXT_MOD_CTRL));
    TEST_UI_STRING(&layout, input->content, "one two ");
    TEST_TRUE(ui_input_handle_code(NULL, &layout, input, INPUT_TEXT_HOME));
    TEST_TRUE(ui_input_handle_code(NULL, &layout, input, INPUT_TEXT_DELETE | INPUT_TEXT_MOD_CTRL));
    TEST_UI_STRING(&layout, input->content, " two ");

    // Select all + typing replaces the selection
    TEST_TRUE(ui_input_handle_code(NULL, &layout, input, INPUT_TEXT_SELECT_ALL));
    TEST_EQUALS(input->selection_anchor, 0);
    TEST_EQUALS(input->caret, 5);
    ui_text_test_type(&layout, input, "Z");
    TEST_UI_STRING(&layout, input->content, "Z");

    // Shift selection + backspace deletes the selection
    ui_input_set_text(&layout, input, "abcdef");
    TEST_TRUE(ui_input_handle_code(NULL, &layout, input, INPUT_TEXT_LEFT | INPUT_TEXT_MOD_SHIFT));
    TEST_TRUE(ui_input_handle_code(NULL, &layout, input, INPUT_TEXT_LEFT | INPUT_TEXT_MOD_SHIFT));
    TEST_TRUE(ui_input_handle_code(NULL, &layout, input, INPUT_TEXT_BACKSPACE));
    TEST_UI_STRING(&layout, input->content, "abcd");

    // Full input: additional characters are ignored
    ui_input_set_text(&layout, input, "0123456789ABCDEF");
    TEST_EQUALS(ui_input_text_length(&layout, input), 16);
    ui_text_test_type(&layout, input, "G");
    TEST_UI_STRING(&layout, input->content, "0123456789ABCDEF");

    // The selection helpers use the stored length
    ui_input_select_all(&layout, input);
    TEST_EQUALS(input->caret, 16);
    ui_input_set_text(&layout, input, "foo bar");
    ui_input_select_word(&layout, input, 5);
    TEST_EQUALS(input->selection_anchor, 4);
    TEST_EQUALS(input->caret, 7);

    // Textarea: enter inserts a line break
    UIInput* const textarea = ui_textarea_create(&layout, 32);
    ui_text_test_type(&layout, textarea, "a");
    TEST_TRUE(ui_input_handle_code(NULL, &layout, textarea, INPUT_TEXT_ENTER));
    ui_text_test_type(&layout, textarea, "b");
    TEST_UI_STRING(&layout, textarea->content, "a\nb");

    ui_text_test_free(&layout);
}

// The stored lengths are part of the element buffer -> they survive a dump/load
static void test_ui_text_length_to_from_data() {
    UILayout layout;
    ui_text_test_alloc(&layout);
    ui_layout_hashmap_init(&layout, 16);

    UILabel* const label = ui_label_create(&layout, CHAR_TYPE_CHAR, 0, 16);
    ui_label_set_text(&layout, label, "Label text");
    const int32 label_offset = ui_element_offset(&layout, label);

    UIInput* const input = ui_input_create(&layout, 16);
    ui_input_set_text(&layout, input, "Input");
    const int32 input_offset = ui_element_offset(&layout, input);

    byte* const out = (byte *) platform_alloc_aligned(2 * MEGABYTE, 2 * MEGABYTE, 64);
    const int32 size = layout_to_data(&layout, out);
    TEST_TRUE(size > 0);

    UILayout loaded;
    ui_text_test_alloc(&loaded);
    TEST_EQUALS(layout_from_data(out, &loaded), size);

    const UILabel* const loaded_label = (const UILabel *) ui_element_get(&loaded, label_offset);
    TEST_UI_STRING(&loaded, loaded_label->content, "Label text");

    const UIInput* const loaded_input = (const UIInput *) ui_element_get(&loaded, input_offset);
    TEST_UI_STRING(&loaded, loaded_input->content, "Input");
    TEST_EQUALS(ui_input_text_length(&loaded, loaded_input), 5);

    // Old layouts don't have the stored lengths -> they must be rejected
    *((int32 *) out) = UI_LAYOUT_VERSION - 1;
    TEST_EQUALS(layout_from_data(out, &loaded), -1);

    platform_aligned_free((void **) &out);
    ui_text_test_free(&loaded);
    ui_text_test_free(&layout);
}

static void test_input_key_names() {
    // The names are stored with their length
    const SimpleString<char> w = input_key_name_string(INPUT_KEYBOARD_PREFIX | 0x11);
    TEST_EQUALS(strcmp(w.str, "W"), 0);
    TEST_EQUALS(w.length, 1);

    const SimpleString<char> shift = input_key_name_string(INPUT_KEYBOARD_PREFIX | 0x2A);
    TEST_EQUALS(strcmp(shift.str, "Left Shift"), 0);
    TEST_EQUALS(shift.length, 10);

    for (int32 i = 0; i < (int32) ARRAY_COUNT(_input_keyboard_names); ++i) {
        if (_input_keyboard_names[i].str) {
            TEST_EQUALS(_input_keyboard_names[i].length, (int32) strlen(_input_keyboard_names[i].str));
        } else {
            TEST_EQUALS(_input_keyboard_names[i].length, 0);
        }
    }

    for (int32 i = 1; i < (int32) ARRAY_COUNT(_input_mouse_names); ++i) {
        TEST_EQUALS(_input_mouse_names[i].length, (int32) strlen(_input_mouse_names[i].str));
    }

    for (int32 i = 0; i < (int32) CONTROLLER_BUTTON_SIZE; ++i) {
        TEST_EQUALS(_input_controller_names[i].length, (int32) strlen(_input_controller_names[i].str));
    }

    // Unknown keys
    TEST_EQUALS(input_key_name(INPUT_KEYBOARD_PREFIX | 0x00), NULL);
    TEST_EQUALS(input_key_name(INPUT_KEYBOARD_PREFIX | 0xFF), NULL);
    TEST_EQUALS(strcmp(input_key_name(INPUT_KEYBOARD_PREFIX | 0x01), "Esc"), 0);

    char out[64];
    const int16 keys[] = {(int16) (INPUT_KEYBOARD_PREFIX | 0x1D), 0, (int16) (INPUT_KEYBOARD_PREFIX | 0x1F)};
    int32 length = input_key_combination_name(keys, 3, out, (int32) sizeof(out));
    TEST_EQUALS(strcmp(out, "Ctrl + S"), 0);
    TEST_EQUALS(length, 8);

    // Unknown keys show their code
    const int16 unknown[] = {(int16) (INPUT_KEYBOARD_PREFIX | 0x1D), (int16) (INPUT_KEYBOARD_PREFIX | 0xFF)};
    length = input_key_combination_name(unknown, 2, out, (int32) sizeof(out));
    TEST_EQUALS(length, (int32) strlen(out));
    TEST_TRUE(strncmp(out, "Ctrl + #", 8) == 0);

    // Too small output buffer -> the name that doesn't fit is skipped
    length = input_key_combination_name(keys, 3, out, 8);
    TEST_EQUALS(strcmp(out, "Ctrl"), 0);
    TEST_EQUALS(length, 4);
}

#ifdef UBER_TEST
    #ifdef main
        #undef main
    #endif
    #define main UIUITextTest
#endif

int main() {
    TEST_INIT(100);

    TEST_RUN(test_ui_string_reserve);
    TEST_RUN(test_ui_label_text_length);
    TEST_RUN(test_ui_label_text_fixed);
    TEST_RUN(test_ui_tooltip_text_length);
    TEST_RUN(test_ui_input_text_length);
    TEST_RUN(test_ui_input_edit_length);
    TEST_RUN(test_ui_text_length_to_from_data);
    TEST_RUN(test_input_key_names);

    TEST_FINALIZE();

    return 0;
}
