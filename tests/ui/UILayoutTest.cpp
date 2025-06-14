#include "../TestFramework.h"
#include "../../ui/UILayout.h"
#include "../../ui/UILayout.cpp"
#include "../../system/Allocator.h"

static void test_layout_from_file_txt() {
    RingMemory ring;
    ring_alloc(&ring, 10 * MEGABYTE, 64);

    UILayout layout;
    layout.data = (byte *) platform_alloc(2 * MEGABYTE);
    layout_from_file_txt(&layout, "./../../GameEditor/assets/themes/default/scene1.layouttxt", &ring);

    UIElement* element = layout_get_element(&layout, "cmd_window");
    ASSERT_NOT_EQUALS(element, NULL);

    platform_free((void **) &layout.data);
    ring_free(&ring);
}

static void test_layout_to_from_data() {
    RingMemory ring;
    ring_alloc(&ring, 10 * MEGABYTE, 64);

    UILayout layout_dump;
    layout_dump.data = (byte *) platform_alloc(2 * MEGABYTE);
    layout_from_file_txt(&layout_dump, "./../../GameEditor/assets/themes/default/scene1.layouttxt", &ring);

    UILayout layout_load;
    layout_load.data = (byte *) platform_alloc(2 * MEGABYTE);

    byte* out = ring_get_memory(&ring, 1024 * 1024);

    int64 dump_size = layout_to_data(&layout_dump, out);
    int64 load_size = layout_from_data(out, &layout_load);
    ASSERT_EQUALS(dump_size, load_size);
    ASSERT_MEMORY_EQUALS(layout_dump.data, layout_load.data, (uint32) (load_size * 0.8));

    UIElement* element = layout_get_element(&layout_load, "cmd_window");
    ASSERT_NOT_EQUALS(element, NULL);

    platform_free((void **) &layout_load.data);
    platform_free((void **) &layout_dump.data);
    ring_free(&ring);
}

static void test_layout_from_theme() {
    RingMemory ring;
    ring_alloc(&ring, 10 * MEGABYTE, 64);

    UILayout layout;
    layout.data = (byte *) platform_alloc(10 * MEGABYTE);
    layout_from_file_txt(&layout, "./../../GameEditor/assets/themes/default/scene1.layouttxt", &ring);

    UIThemeStyle theme1;
    theme1.data = (byte *) platform_alloc(2 * MEGABYTE);
    theme_from_file_txt(&theme1, "./../../GameEditor/assets/themes/default/general.themetxt", &ring);

    UIThemeStyle theme2;
    theme2.data = (byte *) platform_alloc(2 * MEGABYTE);
    theme_from_file_txt(&theme2, "./../../GameEditor/assets/themes/default/scene1.themetxt", &ring);

    layout_from_theme(&layout, &theme1);
    layout_from_theme(&layout, &theme2);

    UIElement* element = layout_get_element(&layout, "cmd_window");
    ASSERT_NOT_EQUALS(element, NULL);
    ASSERT_TRUE(element->children_count > 0);

    UIWindow* default_style = (UIWindow *) layout_get_element_style(&layout, element, UI_STYLE_TYPE_DEFAULT);
    ASSERT_EQUALS_WITH_DELTA(default_style->dimension.dimension.width, 1.0f, 0.001f);
    ASSERT_EQUALS_WITH_DELTA(default_style->dimension.dimension.height, 0.25f, 0.001f);

    platform_free((void **) &layout.data);
    platform_free((void **) &theme1.data);
    platform_free((void **) &theme2.data);
    ring_free(&ring);
}

#ifdef UBER_TEST
    #ifdef main
        #undef main
    #endif
    #define main UIUILayoutTest
#endif

int main() {
    TEST_INIT(100);

    TEST_RUN(test_layout_from_file_txt);
    TEST_RUN(test_layout_to_from_data);
    TEST_RUN(test_layout_from_theme);

    TEST_FINALIZE();

    return 0;
}
