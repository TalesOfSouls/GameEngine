#include "../TestFramework.h"
#include "../../ui/UITheme.h"
#include "../../system/Allocator.h"

static void test_theme_from_file_txt() {
    RingMemory ring;
    ring_alloc(&ring, 10 * MEGABYTE, 64);

    UIThemeStyle theme;
    theme.data = (byte *) platform_alloc(2 * MEGABYTE);
    theme_from_file_txt(&theme, "./../../GameEditor/assets/themes/default/scene1.themetxt", &ring);

    UIAttributeGroup* group = theme_style_group(&theme, "#cmd_window");
    ASSERT_NOT_EQUALS(group, NULL);
    ASSERT_TRUE(group->attribute_count > 0);

    UIAttribute* attr = ui_attribute_from_group(group, UI_ATTRIBUTE_TYPE_POSITION_X);
    ASSERT_NOT_EQUALS(attr, NULL);
    ASSERT_EQUALS(attr->datatype, UI_ATTRIBUTE_DATA_TYPE_F32);
    ASSERT_EQUALS_WITH_DELTA(attr->value_float, 0.0f, 0.001f);

    platform_free((void **) &theme.data);
    ring_free(&ring);
}

static void test_theme_to_from_data() {
    RingMemory ring;
    ring_alloc(&ring, 10 * MEGABYTE, 64);

    UIThemeStyle theme_dump;
    theme_dump.data = (byte *) platform_alloc(2 * MEGABYTE);
    theme_from_file_txt(&theme_dump, "./../../GameEditor/assets/themes/default/scene1.themetxt", &ring);

    UIThemeStyle theme_load;
    theme_load.data = (byte *) platform_alloc(2 * MEGABYTE);

    byte* out = ring_get_memory(&ring, 1024 * 1024);

    int64 dump_size = theme_to_data(&theme_dump, out);
    int64 load_size = theme_from_data(out, &theme_load);
    ASSERT_EQUALS(dump_size, load_size);
    ASSERT_MEMORY_EQUALS(theme_dump.data, theme_load.data, (uint32) (load_size * 0.8));

    UIAttributeGroup* group = theme_style_group(&theme_load, "#cmd_window");
    ASSERT_NOT_EQUALS(group, NULL);
    ASSERT_TRUE(group->attribute_count > 0);

    UIAttribute* attr = ui_attribute_from_group(group, UI_ATTRIBUTE_TYPE_POSITION_X);
    ASSERT_NOT_EQUALS(attr, NULL);
    ASSERT_EQUALS(attr->datatype, UI_ATTRIBUTE_DATA_TYPE_F32);
    ASSERT_EQUALS_WITH_DELTA(attr->value_float, 0.0f, 0.001f);

    platform_free((void **) &theme_load.data);
    platform_free((void **) &theme_dump.data);
    ring_free(&ring);
}

#ifdef UBER_TEST
    #ifdef main
        #undef main
    #endif
    #define main UIUIThemeTest
#endif

int main() {
    TEST_INIT(100);

    TEST_RUN(test_theme_from_file_txt);
    TEST_RUN(test_theme_to_from_data);

    TEST_FINALIZE();

    return 0;
}
