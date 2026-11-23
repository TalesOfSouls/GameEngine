#include "../../stdlib/Stdlib.h"
#include "../TestFramework.h"
#include "../../localization/Language.cpp"
#include "../../system/Allocator.h"

static const char LANGUAGE_TEST_TXT[] =
    "version 2\n"
    "\n"
    "// Comment: with a colon\n"
    "menu_quit: Quit\n"
    "status_prefix: \"Status: \"\n"
    "  indented: value   \n"
    "multi_line: Line 1\\nLine 2\\t\\\"quoted\\\" \\\\\n"
    "empty:\n"
    "format: Ping: %l\r\n"
    "menu_quit: Quit game\n"
    "invalid line without colon\n";

static void test_language_from_txt() {
    const int32 length = (int32) sizeof(LANGUAGE_TEST_TXT) - 1;
    const int32 capacity = language_txt_memory_size(LANGUAGE_TEST_TXT, length);

    Language lang = {};
    lang.data = (byte *) platform_alloc_aligned(capacity, capacity, 64);

    TEST_TRUE(language_from_txt(&lang, LANGUAGE_TEST_TXT, length, capacity) > 0);
    TEST_TRUE(lang.size <= capacity);

    // Duplicate keys are overwritten -> 6 strings
    TEST_EQUALS(lang.count, 6);

    TEST_EQUALS(strcmp(language_get(&lang, "menu_quit"), "Quit game"), 0);
    TEST_EQUALS(strcmp(language_get(&lang, "status_prefix"), "Status: "), 0);
    TEST_EQUALS(strcmp(language_get(&lang, "indented"), "value"), 0);
    TEST_EQUALS(strcmp(language_get(&lang, "multi_line"), "Line 1\nLine 2\t\"quoted\" \\"), 0);
    TEST_EQUALS(strcmp(language_get(&lang, "empty"), ""), 0);
    TEST_EQUALS(strcmp(language_get(&lang, "format"), "Ping: %l"), 0);

    // Missing keys
    TEST_EQUALS(language_get(&lang, "missing"), NULL);
    TEST_EQUALS(language_get(&lang, "menu"), NULL);

    // Strings incl. their length (stored in the slot -> no strlen)
    const char* const length_keys[] = {"menu_quit", "status_prefix", "indented", "multi_line", "empty", "format"};
    for (int32 i = 0; i < (int32) ARRAY_COUNT(length_keys); ++i) {
        const SimpleString<const char> str = language_get_string(&lang, length_keys[i]);
        TEST_TRUE(str.str == language_get(&lang, length_keys[i]));
        TEST_EQUALS(str.length, (int32) strlen(str.str));
        TEST_EQUALS(str.max_length, str.length);
    }

    // The overwritten duplicate has the length of the new value
    TEST_EQUALS(language_get_string(&lang, "menu_quit").length, 9);
    TEST_EQUALS(language_get_string(&lang, "empty").length, 0);
    TEST_NOT_EQUALS(language_get_string(&lang, "empty").str, NULL);

    TEST_EQUALS(language_get_string(&lang, "missing").str, NULL);
    TEST_EQUALS(language_get_string(&lang, "missing").length, 0);

    // Cached lookup
    SimpleString<const char> cache = {};
    TEST_TRUE(language_get_string(&lang, &cache, "status_prefix").str == language_get(&lang, "status_prefix"));
    TEST_EQUALS(cache.length, 8);

    // The cache is used -> the key doesn't matter anymore
    TEST_TRUE(language_get_string(&lang, &cache, "menu_quit").str == language_get(&lang, "status_prefix"));

    // Binary layout
    TEST_EQUALS(sizeof(LanguageSlot), 16);
    TEST_EQUALS(((const LanguageHeader *) lang.data)->version, LANGUAGE_VERSION);

    // Too small
    Language too_small = {};
    too_small.data = (byte *) platform_alloc_aligned(64, 64, 64);
    TEST_EQUALS(language_from_txt(&too_small, LANGUAGE_TEST_TXT, length, 64), -1);

    // Binary
    byte* const out = (byte *) platform_alloc_aligned(capacity, capacity, 64);
    TEST_EQUALS(language_to_data(&lang, out), lang.size);

    Language loaded = {};
    loaded.data = (byte *) platform_alloc_aligned(capacity, capacity, 64);
    TEST_EQUALS(language_from_data(out, &loaded), lang.size);
    TEST_EQUALS(loaded.count, lang.count);
    TEST_EQUALS(strcmp(language_get(&loaded, "status_prefix"), "Status: "), 0);
    TEST_EQUALS(language_get_string(&loaded, "status_prefix").length, 8);

    // Incompatible data
    ((LanguageHeader *) out)->version = 1;
    TEST_EQUALS(language_from_data(out, &loaded), -1);

    // The previous binary version had no value lengths
    ((LanguageHeader *) out)->version = 2;
    TEST_EQUALS(language_from_data(out, &loaded), -1);

    platform_aligned_free((void **) &out);
    platform_aligned_free((void **) &loaded.data);
    platform_aligned_free((void **) &too_small.data);
    platform_aligned_free((void **) &lang.data);
}

// Many keys -> collisions in the hash map
// The hash + length of a literal key are calculated at compile time
static_assert(language_key("menu_quit").hash == language_hash("menu_quit"), "language_key must be evaluated at compile time");
static_assert(language_key("menu_quit").length == 9, "language_key must be evaluated at compile time");

static void test_language_key() {
    const int32 length = (int32) sizeof(LANGUAGE_TEST_TXT) - 1;
    const int32 capacity = language_txt_memory_size(LANGUAGE_TEST_TXT, length);

    Language lang = {};
    lang.data = (byte *) platform_alloc_aligned(capacity, capacity, 64);
    TEST_TRUE(language_from_txt(&lang, LANGUAGE_TEST_TXT, length, capacity) > 0);

    // Same result as the \0 terminated key
    TEST_TRUE(language_get(&lang, language_key("menu_quit")) == language_get(&lang, "menu_quit"));
    TEST_TRUE(language_get(&lang, language_key("status_prefix")) == language_get(&lang, "status_prefix"));
    TEST_TRUE(language_get(&lang, language_key("empty")) == language_get(&lang, "empty"));
    TEST_EQUALS(strcmp(language_get(&lang, language_key("format")), "Ping: %l"), 0);

    // Missing keys, prefixes and longer keys
    TEST_EQUALS(language_get(&lang, language_key("missing")), NULL);
    TEST_EQUALS(language_get(&lang, language_key("menu")), NULL);
    TEST_EQUALS(language_get(&lang, language_key("menu_quit_")), NULL);

    // Runtime key with a known length (not \0 terminated)
    const char buffer[] = "status_prefix: rest";
    TEST_TRUE(language_get(&lang, language_key(buffer, 13)) == language_get(&lang, "status_prefix"));
    TEST_EQUALS(language_get(&lang, language_key(buffer, 6)), NULL);

    TEST_EQUALS(language_get_string(&lang, language_key("status_prefix")).length, 8);

    const char* cache = NULL;
    TEST_TRUE(language_get(&lang, &cache, language_key("format")) == language_get(&lang, "format"));
    TEST_TRUE(cache == language_get(&lang, "format"));

    platform_aligned_free((void **) &lang.data);
}

static void test_language_many() {
    char text[64 * 1024];
    int32 length = 0;
    for (int32 i = 0; i < 1000; ++i) {
        length += sprintf_fast(text + length, (int32) sizeof(text) - length, "key_%d: value %d\n", i, i);
    }

    const int32 capacity = language_txt_memory_size(text, length);
    Language lang = {};
    lang.data = (byte *) platform_alloc_aligned(capacity, capacity, 64);
    TEST_TRUE(language_from_txt(&lang, text, length, capacity) > 0);
    TEST_EQUALS(lang.count, 1000);
    TEST_TRUE(((const LanguageHeader *) lang.data)->slot_count >= 2000);

    bool all = true;
    for (int32 i = 0; i < 1000; ++i) {
        char key[32];
        char value[32];
        sprintf_fast(key, (int32) sizeof(key), "key_%d", i);
        sprintf_fast(value, (int32) sizeof(value), "value %d", i);

        const char* const str = language_get(&lang, key);
        all = all && str && strcmp(str, value) == 0;

        const SimpleString<const char> string = language_get_string(&lang, key);
        all = all && string.str == str && string.length == (int32) strlen(value);
    }

    TEST_TRUE(all);
    TEST_EQUALS(language_get(&lang, "key_1000"), NULL);

    platform_aligned_free((void **) &lang.data);
}

#ifdef UBER_TEST
    #ifdef main
        #undef main
    #endif
    #define main LocalizationLanguageTest
#endif

int main() {
    TEST_INIT(100);

    TEST_RUN(test_language_from_txt);
    TEST_RUN(test_language_many);
    TEST_RUN(test_language_key);

    TEST_FINALIZE();

    return 0;
}

#ifdef UBER_TEST
    #ifdef main
        #undef main
    #endif
#endif
