#include "../../stdlib/Stdlib.h"
#include "../TestFramework.h"
#include "../../ui/UITheme.cpp"
#include "../../system/Allocator.h"

static const char UI_THEME_TEST_TXT[] =
    "version 2\n"
    "\n"
    "// Type default\n"
    "@button\n"
    "    background_color: #444444FF\n"
    "\n"
    "window_default\n"
    "    background_color: #262626FF\n"
    "    $title\n"
    "        size: 100% 38\n"
    "\n"
    "\t// tab indentation\n"
    "big_text\n"
    "\tfont_size: 20\n";

static void test_theme_from_txt() {
    UITheme theme = {};
    theme.data_size = 1 * MEGABYTE;
    theme.data = (byte *) platform_alloc_aligned(theme.data_size, theme.data_size, 64);

    TEST_TRUE(ui_theme_from_txt(&theme, UI_THEME_TEST_TXT, (int32) sizeof(UI_THEME_TEST_TXT) - 1));
    TEST_EQUALS(theme.document.version, UI_THEME_VERSION);

    const int32 button = ui_theme_style(&theme, "@button");
    const int32 window = ui_theme_style(&theme, "window_default");
    const int32 text = ui_theme_style(&theme, "big_text");
    TEST_TRUE(button >= 0);
    TEST_TRUE(window >= 0);
    TEST_TRUE(text >= 0);
    TEST_EQUALS(ui_theme_style(&theme, "unknown"), -1);

    // Properties
    char value[64];
    TEST_TRUE(ui_document_property_get(&theme.document, &theme.document.nodes[button], "background_color", value, sizeof(value)) != NULL);
    TEST_EQUALS(strcmp(value, "#444444FF"), 0);
    TEST_TRUE(ui_document_property_get(&theme.document, &theme.document.nodes[text], "font_size", value, sizeof(value)) != NULL);
    TEST_EQUALS(strcmp(value, "20"), 0);

    // Part
    const UIDocumentNode* const part = &theme.document.nodes[theme.document.nodes[window].first_child];
    TEST_EQUALS(part->kind, UI_DOCUMENT_NODE_PART);
    TEST_TRUE(ui_document_equals(&theme.document, part->type, "title"));
    TEST_TRUE(ui_document_property_get(&theme.document, part, "size", value, sizeof(value)) != NULL);
    TEST_EQUALS(strcmp(value, "100% 38"), 0);

    platform_aligned_free((void **) &theme.data);
}

// Assets of the theme ("@theme" block, hex asset ids)
static void test_theme_assets() {
    static const char txt[] =
        "version 2\n"
        "@theme\n"
        "    font: 04000000\n"
        "    atlas: 0x0400000A\n"
        "    sound: 01\n"
        "big_text\n"
        "    font_size: 20\n";

    static const char invalid_txt[] =
        "version 2\n"
        "@theme\n"
        "    font: xyz\n"
        "    atlas: 123456789\n";

    UITheme theme = {};
    theme.data_size = 1 * MEGABYTE;
    theme.data = (byte *) platform_alloc_aligned(theme.data_size, theme.data_size, 64);

    // Not defined
    TEST_TRUE(ui_theme_from_txt(&theme, UI_THEME_TEST_TXT, (int32) sizeof(UI_THEME_TEST_TXT) - 1));
    TEST_EQUALS(theme.font_asset_id, UI_THEME_ASSET_NONE);
    TEST_EQUALS(theme.atlas_asset_id, UI_THEME_ASSET_NONE);

    TEST_TRUE(ui_theme_from_txt(&theme, txt, (int32) sizeof(txt) - 1));
    TEST_EQUALS(theme.font_asset_id, 0x04000000);
    TEST_EQUALS(theme.atlas_asset_id, 0x0400000A);
    TEST_EQUALS(theme.font, NULL);
    TEST_TRUE(ui_theme_style(&theme, "big_text") >= 0);

    // The asset ids are part of the binary theme
    byte* out = (byte *) platform_alloc_aligned(1 * MEGABYTE, 1 * MEGABYTE, 64);
    const int32 size = theme_to_data(&theme, out);

    UITheme loaded = {};
    loaded.data_size = 1 * MEGABYTE;
    loaded.data = (byte *) platform_alloc_aligned(loaded.data_size, loaded.data_size, 64);
    TEST_EQUALS(theme_from_data(out, &loaded), size);
    TEST_EQUALS(loaded.font_asset_id, 0x04000000);
    TEST_EQUALS(loaded.atlas_asset_id, 0x0400000A);

    // Invalid ids are ignored
    TEST_TRUE(ui_theme_from_txt(&theme, invalid_txt, (int32) sizeof(invalid_txt) - 1));
    TEST_EQUALS(theme.font_asset_id, UI_THEME_ASSET_NONE);
    TEST_EQUALS(theme.atlas_asset_id, UI_THEME_ASSET_NONE);

    platform_aligned_free((void **) &out);
    platform_aligned_free((void **) &loaded.data);
    platform_aligned_free((void **) &theme.data);
}

static void test_theme_to_from_data() {
    UITheme theme_dump = {};
    theme_dump.data_size = 1 * MEGABYTE;
    theme_dump.data = (byte *) platform_alloc_aligned(theme_dump.data_size, theme_dump.data_size, 64);
    ui_theme_from_txt(&theme_dump, UI_THEME_TEST_TXT, (int32) sizeof(UI_THEME_TEST_TXT) - 1);

    byte* out = (byte *) platform_alloc_aligned(1 * MEGABYTE, 1 * MEGABYTE, 64);
    const int32 size = theme_to_data(&theme_dump, out);

    UITheme theme_load = {};
    theme_load.data_size = 1 * MEGABYTE;
    theme_load.data = (byte *) platform_alloc_aligned(theme_load.data_size, theme_load.data_size, 64);
    TEST_EQUALS(theme_from_data(out, &theme_load), size);

    TEST_EQUALS(theme_load.used_data_size, theme_dump.used_data_size);
    TEST_EQUALS(theme_load.document.node_count, theme_dump.document.node_count);
    TEST_EQUALS(ui_theme_style(&theme_load, "window_default"), ui_theme_style(&theme_dump, "window_default"));

    char value[64];
    const int32 button = ui_theme_style(&theme_load, "@button");
    TEST_TRUE(button >= 0);
    TEST_TRUE(ui_document_property_get(&theme_load.document, &theme_load.document.nodes[button], "background_color", value, sizeof(value)) != NULL);
    TEST_EQUALS(strcmp(value, "#444444FF"), 0);

    platform_aligned_free((void **) &out);
    platform_aligned_free((void **) &theme_load.data);
    platform_aligned_free((void **) &theme_dump.data);
}

#ifdef UBER_TEST
    #ifdef main
        #undef main
    #endif
    #define main UIUIThemeTest
#endif

int main() {
    TEST_INIT(100);

    TEST_RUN(test_theme_from_txt);
    TEST_RUN(test_theme_assets);
    TEST_RUN(test_theme_to_from_data);

    TEST_FINALIZE();

    return 0;
}
