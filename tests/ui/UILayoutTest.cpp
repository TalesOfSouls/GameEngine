#include "../../stdlib/Stdlib.h"
#include "../TestFramework.h"
#include "../../ui/UILayout.h"
#include "../../ui/UILayout.cpp"
#include "../../system/Allocator.h"

static const char UI_LAYOUT_TEST_THEME[] =
    "version 2\n"
    "@button\n"
    "    background_color: #111111FF\n"
    "    background_color_hover: #222222FF\n"
    "test_button\n"
    "    background_color: #333333FF\n"
    "test_window\n"
    "    background_color: #262626FF\n"
    "    $title\n"
    "        size: 100% 30\n";

static const char UI_LAYOUT_TEST_TXT[] =
    "version 2\n"
    "\n"
    "// Comment\n"
    "window test_window\n"
    "    style: test_window\n"
    "    anchor: top left\n"
    "    pos: 10 20\n"
    "    size: 300 200\n"
    "    title: Test\n"
    "    $close\n"
    "        hidden: true\n"
    "\n"
    "    panel test_scroll\n"
    "        scroll: y\n"
    "        size: 100% -30\n"
    "        button test_button\n"
    "            style: test_button\n"
    "            text: Click\n"
    "            on_click: click\n"
    "            background_color: #444444FF\n"
    "        label test_label\n"
    "            text: Line 1\\nLine 2\n"
    "            anchor: top right / top left\n"
    "            flags: +ignore_hit\n"
    "            update: label_update\n"
    "        table test_table\n"
    "            columns: A | B\n"
    "            row_capacity: 4\n"
    "            row: a1 | b1\n"
    "            row: a2\n"
    "            widths: 25% 75%\n"
    "\n"
    "label\n"
    "    text: top level\n";

static void* ui_layout_test_function(void*, UILayout*, UICore*) NO_EXCEPT { return NULL; }

static void ui_layout_test_init(UILayout* const layout) {
    memset(layout, 0, sizeof(*layout));
    buffer_alloc(&layout->ui_element_buffer, 1 * MEGABYTE, 1 * MEGABYTE, 64);
    layout->data_size = 1 * MEGABYTE;
    layout->data = (byte *) platform_alloc_aligned(layout->data_size, layout->data_size, 64);
    memset(layout->data, 0, layout->data_size);
    ui_layout_reset(layout);
    ui_root_create(layout, 1000.0f, 1000.0f);
}

static void test_layout_from_txt() {
    UITheme theme = {};
    theme.data_size = 1 * MEGABYTE;
    theme.data = (byte *) platform_alloc_aligned(theme.data_size, theme.data_size, 64);
    ui_theme_from_txt(&theme, UI_LAYOUT_TEST_THEME, (int32) sizeof(UI_LAYOUT_TEST_THEME) - 1);

    const UINameId update_names[] = { {"label_update", 1} };
    const UINameId action_names[] = { {"click", 3} };

    UILayoutTxtContext ctx = {};
    ctx.themes[ctx.theme_count++] = &theme;
    ctx.update_functions = update_names;
    ctx.update_function_count = 1;
    ctx.actions = action_names;
    ctx.action_count = 1;

    UILayout layout;
    ui_layout_test_init(&layout);

    BufferMemory mem;
    buffer_alloc(&mem, 1 * MEGABYTE, 1 * MEGABYTE, 64);

    TEST_EQUALS(ui_layout_from_txt(&layout, UI_LAYOUT_TEST_TXT, (int32) sizeof(UI_LAYOUT_TEST_TXT) - 1, &ctx, &mem), 2);

    // Window: style + own properties + part
    const UIWindow* const window = (const UIWindow *) ui_get_element(&layout, "test_window");
    TEST_NOT_EQUALS(window, NULL);
    TEST_EQUALS(window->core.type, UI_ELEMENT_TYPE_VIEW_WINDOW);
    TEST_EQUALS(window->background_color, 0x262626FFU);
    TEST_EQUALS(window->core.dimension.align_anchor, (byte) UI_ANCHOR_ALIGN_TOP_LEFT);
    TEST_EQUALS_WITH_DELTA(window->core.dimension.pos_raw.y, 20.0f, 0.001f);
    TEST_EQUALS_WITH_DELTA(ui_window_title(&layout, window)->core.dimension.dim_raw.y, 30.0f, 0.001f);
    TEST_TRUE(ui_window_button_close(&layout, window)->core.state & UI_STATE_HIDDEN);
    TEST_EQUALS(strcmp((const char *) (layout.ui_element_buffer.memory + ui_window_title_label(&layout, window)->content), "Test"), 0);

    // Children of a scroll panel are in its content panel
    UIPanel* const scroll = (UIPanel *) ui_get_element(&layout, "test_scroll");
    TEST_NOT_EQUALS(scroll, NULL);
    TEST_TRUE(scroll->panel_flags & UI_PANEL_FLAG_SCROLL_Y);
    TEST_TRUE(scroll->core.dimension.flag & UI_DIMENSION_DIM_X_RELATIVE);
    TEST_EQUALS_WITH_DELTA(scroll->core.dimension.dim_raw.y, -30.0f, 0.001f);

    // Style order: type default < style < own properties
    const UIButton* const button = (const UIButton *) ui_get_element(&layout, "test_button");
    TEST_NOT_EQUALS(button, NULL);
    TEST_EQUALS(button->core.parent_offset, ui_element_offset(&layout, ui_panel_content(&layout, scroll)));
    TEST_EQUALS(button->background_color, 0x444444FFU);
    TEST_EQUALS(button->background_color_hover, 0x222222FFU);
    TEST_EQUALS(button->on_click, 3);
    TEST_TRUE(button->label != 0);

    // Escape sequences, anchor + align, flags, functions
    const UILabel* const label = (const UILabel *) ui_get_element(&layout, "test_label");
    TEST_NOT_EQUALS(label, NULL);
    TEST_EQUALS(strcmp((const char *) (layout.ui_element_buffer.memory + label->content), "Line 1\nLine 2"), 0);
    TEST_EQUALS(label->core.dimension.align_anchor, (byte) (UI_ANCHOR_H_RIGHT | UI_ANCHOR_V_TOP | UI_ALIGN_V_TOP));
    TEST_TRUE(label->core.flags & UI_ELEMENT_FLAG_IGNORE_HIT);
    TEST_EQUALS(label->core.update_func, 1);

    // Table rows (missing cells are empty)
    const UITable* const table = (const UITable *) ui_get_element(&layout, "test_table");
    TEST_NOT_EQUALS(table, NULL);
    TEST_EQUALS(table->column_count, 2);
    TEST_EQUALS(table->row_count, 2);
    TEST_EQUALS(strcmp(ui_table_cell(&layout, table, 0, 1), "b1"), 0);
    TEST_EQUALS(strcmp(ui_table_cell(&layout, table, 1, 1), ""), 0);
    TEST_EQUALS_WITH_DELTA(((const f32 *) (layout.ui_element_buffer.memory + table->column_widths))[1], 0.75f, 0.001f);

    // Top level elements in file order: window, label
    const UICore* const first = ui_element_get(&layout, layout.ui_root->next_offset);
    TEST_EQUALS(first, &window->core);
    const UICore* const second = ui_element_get(&layout, window->core.skip_offset);
    TEST_EQUALS(second->type, UI_ELEMENT_TYPE_LABEL);

    buffer_free(&mem);
    buffer_free(&layout.ui_element_buffer);
    platform_aligned_free((void **) &layout.data);
    platform_aligned_free((void **) &theme.data);

    PSEUDO_USE(ui_layout_test_function);
}

// Strings: "@key" = language of the file (layout or theme), "@@key" = global language
static void test_layout_language() {
    static const char theme_txt[] =
        "version 2\n"
        "lang_window\n"
        "    $title_label\n"
        "        text: @title\n";

    static const char theme_lang_txt[] =
        "version 2\n"
        "title: Theme title\n";

    static const char layout_txt[] =
        "version 2\n"
        "window lang_window_element\n"
        "    style: lang_window\n"
        "    label lang_local\n"
        "        text: @hello\n"
        "    label lang_global\n"
        "        text: @@quit\n"
        "    label lang_literal\n"
        "        text: \\@hello\n"
        "    label lang_missing\n"
        "        text: @missing\n"
        "    label lang_pattern\n"
        "        pattern: @fps\n"
        "    select lang_select\n"
        "        options: @fruits\n";

    static const char layout_lang_txt[] =
        "version 2\n"
        "hello: Hello world\n"
        "fps: FPS: %n\n"
        "fruits: Apple | Banana | Cherry\n"
        // The theme language is separate (the theme key is not visible in the layout)
        "title: Layout title\n";

    static const char global_lang_txt[] =
        "version 2\n"
        "quit: Quit\n";

    UITheme theme = {};
    theme.data_size = 1 * MEGABYTE;
    theme.data = (byte *) platform_alloc_aligned(theme.data_size, theme.data_size, 64);
    TEST_TRUE(ui_theme_from_txt(&theme, theme_txt, (int32) sizeof(theme_txt) - 1));
    TEST_TRUE(ui_theme_language_from_txt(&theme, theme_lang_txt, (int32) sizeof(theme_lang_txt) - 1));
    TEST_EQUALS(theme.language.count, 1);

    byte* lang_memory = (byte *) platform_alloc_aligned(64 * KILOBYTE, 64 * KILOBYTE, 64);
    Language layout_lang = {};
    layout_lang.data = lang_memory;
    TEST_TRUE(language_from_txt(&layout_lang, layout_lang_txt, (int32) sizeof(layout_lang_txt) - 1, 32 * KILOBYTE) > 0);

    Language global_lang = {};
    global_lang.data = lang_memory + 32 * KILOBYTE;
    TEST_TRUE(language_from_txt(&global_lang, global_lang_txt, (int32) sizeof(global_lang_txt) - 1, 32 * KILOBYTE) > 0);

    UILayoutTxtContext ctx = {};
    ctx.themes[ctx.theme_count++] = &theme;
    ctx.language = &layout_lang;
    ctx.language_global = &global_lang;

    UILayout layout;
    ui_layout_test_init(&layout);

    BufferMemory mem;
    buffer_alloc(&mem, 1 * MEGABYTE, 1 * MEGABYTE, 64);
    TEST_EQUALS(ui_layout_from_txt(&layout, layout_txt, (int32) sizeof(layout_txt) - 1, &ctx, &mem), 1);

    const byte* const buffer = layout.ui_element_buffer.memory;
    const UIWindow* const window = (const UIWindow *) ui_get_element(&layout, "lang_window_element");
    TEST_NOT_EQUALS(window, NULL);
    TEST_EQUALS(strcmp((const char *) (buffer + ui_window_title_label(&layout, window)->content), "Theme title"), 0);

    TEST_EQUALS(strcmp((const char *) (buffer + ((const UILabel *) ui_get_element(&layout, "lang_local"))->content), "Hello world"), 0);
    TEST_EQUALS(strcmp((const char *) (buffer + ((const UILabel *) ui_get_element(&layout, "lang_global"))->content), "Quit"), 0);
    TEST_EQUALS(strcmp((const char *) (buffer + ((const UILabel *) ui_get_element(&layout, "lang_literal"))->content), "@hello"), 0);
    TEST_EQUALS(strcmp((const char *) (buffer + ((const UILabel *) ui_get_element(&layout, "lang_missing"))->content), "@missing"), 0);
    TEST_EQUALS(strcmp((const char *) (buffer + ((const UILabel *) ui_get_element(&layout, "lang_pattern"))->pattern), "FPS: %n"), 0);

    const UISelect* const select = (const UISelect *) ui_get_element(&layout, "lang_select");
    TEST_NOT_EQUALS(select, NULL);
    TEST_EQUALS(select->option_count, 3);
    TEST_EQUALS(strcmp(ui_select_option(&layout, select, 1), "Banana"), 0);

    // The theme language is part of the binary theme
    byte* const out = (byte *) platform_alloc_aligned(2 * MEGABYTE, 2 * MEGABYTE, 64);
    const int32 size = theme_to_data(&theme, out);

    UITheme loaded = {};
    loaded.data_size = 1 * MEGABYTE;
    loaded.data = (byte *) platform_alloc_aligned(loaded.data_size, loaded.data_size, 64);
    TEST_EQUALS(theme_from_data(out, &loaded), size);
    TEST_EQUALS(loaded.language.count, 1);
    TEST_EQUALS(strcmp(language_get(&loaded.language, "title"), "Theme title"), 0);

    platform_aligned_free((void **) &out);
    platform_aligned_free((void **) &loaded.data);
    buffer_free(&mem);
    buffer_free(&layout.ui_element_buffer);
    platform_aligned_free((void **) &layout.data);
    platform_aligned_free((void **) &lang_memory);
    platform_aligned_free((void **) &theme.data);
}

static void test_layout_to_from_data() {
    UILayoutTxtContext ctx = {};

    UILayout layout_dump;
    ui_layout_test_init(&layout_dump);

    BufferMemory mem;
    buffer_alloc(&mem, 1 * MEGABYTE, 1 * MEGABYTE, 64);
    ui_layout_from_txt(&layout_dump, UI_LAYOUT_TEST_TXT, (int32) sizeof(UI_LAYOUT_TEST_TXT) - 1, &ctx, &mem);

    byte* out = (byte *) platform_alloc_aligned(2 * MEGABYTE, 2 * MEGABYTE, 64);
    const int32 size = layout_to_data(&layout_dump, out);

    UILayout layout_load;
    ui_layout_test_init(&layout_load);
    TEST_EQUALS(layout_from_data(out, &layout_load), size);

    const int64 used = layout_dump.ui_element_buffer.head - layout_dump.ui_element_buffer.memory;
    TEST_EQUALS(layout_load.ui_element_buffer.head - layout_load.ui_element_buffer.memory, used);

    const UICore* const window = ui_get_element(&layout_load, "test_window");
    TEST_NOT_EQUALS(window, NULL);
    TEST_EQUALS(ui_element_offset(&layout_load, window), ui_element_offset(&layout_dump, ui_get_element(&layout_dump, "test_window")));

    platform_aligned_free((void **) &out);
    buffer_free(&mem);
    buffer_free(&layout_dump.ui_element_buffer);
    buffer_free(&layout_load.ui_element_buffer);
    platform_aligned_free((void **) &layout_dump.data);
    platform_aligned_free((void **) &layout_load.data);
}

#ifdef UBER_TEST
    #ifdef main
        #undef main
    #endif
    #define main UIUILayoutTest
#endif

int main() {
    TEST_INIT(100);

    TEST_RUN(test_layout_from_txt);
    TEST_RUN(test_layout_language);
    TEST_RUN(test_layout_to_from_data);

    TEST_FINALIZE();

    return 0;
}
