#include "../TestFramework.h"
#include "../../stdlib/stdlib_internal.h"

/**
 * Tests for the stdlib replacement primitives (@see stdlib_internal.h)
 *
 * They are used for NO_STDLIB builds, so they have to behave like the C library functions.
 */

static void test_stdlib_internal_memset() {
    byte buffer[64];

    memset(buffer, 0xCC, sizeof(buffer));
    __internal_memset(buffer, 0xAA, sizeof(buffer));

    for (int32 i = 0; i < (int32) sizeof(buffer); ++i) {
        TEST_EQUALS(buffer[i], 0xAA);
    }

    // Non word sized length
    memset(buffer, 0xCC, sizeof(buffer));
    __internal_memset(buffer, 0x55, 13);
    TEST_EQUALS(buffer[0], 0x55);
    TEST_EQUALS(buffer[12], 0x55);
    TEST_EQUALS(buffer[13], 0xCC);

    // Zero length doesn't change anything
    __internal_memset(buffer, 0x00, 0);
    TEST_EQUALS(buffer[0], 0x55);
}

static void test_stdlib_internal_memcpy() {
    // Unaligned source and destination, sizes around the size_t boundary
    const int32 lengths[] = {0, 1, 7, 8, 9, 15, 16, 17, 63};

    for (int32 l = 0; l < (int32) (sizeof(lengths) / sizeof(lengths[0])); ++l) {
        byte source[64];
        byte expected[64];
        byte result[64];

        for (int32 i = 0; i < 64; ++i) {
            source[i] = (byte) (i * 3 + 1);
            expected[i] = 0xEE;
            result[i] = 0xEE;
        }

        memcpy(expected, source, lengths[l]);
        __internal_memcpy(result, source, lengths[l]);

        TEST_MEMORY_EQUALS(result, expected, sizeof(result));
    }

    // Unaligned pointers
    byte source[80];
    byte expected[80];
    byte result[80];

    for (int32 i = 0; i < 80; ++i) {
        source[i] = (byte) i;
        expected[i] = 0;
        result[i] = 0;
    }

    memcpy(expected + 1, source + 3, 40);
    __internal_memcpy(result + 1, source + 3, 40);
    TEST_MEMORY_EQUALS(result, expected, sizeof(result));
}

static void test_stdlib_internal_memmove() {
    // Forward overlap (destination before source)
    byte forward_expected[32];
    byte forward_result[32];

    for (int32 i = 0; i < 32; ++i) {
        forward_expected[i] = (byte) i;
        forward_result[i] = (byte) i;
    }

    memmove(forward_expected + 2, forward_expected, 20);
    __internal_memmove(forward_result + 2, forward_result, 20);
    TEST_MEMORY_EQUALS(forward_result, forward_expected, sizeof(forward_result));

    // Backward overlap (destination after source)
    byte backward_expected[32];
    byte backward_result[32];

    for (int32 i = 0; i < 32; ++i) {
        backward_expected[i] = (byte) (i * 2);
        backward_result[i] = (byte) (i * 2);
    }

    memmove(backward_expected, backward_expected + 3, 20);
    __internal_memmove(backward_result, backward_result + 3, 20);
    TEST_MEMORY_EQUALS(backward_result, backward_expected, sizeof(backward_result));

    // Same pointer and zero length are no-ops
    __internal_memmove(backward_result, backward_result, 16);
    __internal_memmove(backward_result, backward_result + 1, 0);
    TEST_MEMORY_EQUALS(backward_result, backward_expected, sizeof(backward_result));
}

static void test_stdlib_internal_memcmp() {
    const byte a[] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
    const byte b[] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
    const byte c[] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x09};
    const byte d[] = {0x01, 0x09, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};

    TEST_EQUALS(__internal_memcmp(a, b, sizeof(a)), 0);
    TEST_EQUALS(__internal_memcmp(a, c, sizeof(a)) < 0, true);
    TEST_EQUALS(__internal_memcmp(c, a, sizeof(a)) > 0, true);
    TEST_EQUALS(__internal_memcmp(a, d, sizeof(a)) < 0, true);

    // The first difference decides
    TEST_EQUALS(__internal_memcmp(a, d, 1), 0);
    TEST_EQUALS(__internal_memcmp(a, d, 2) < 0, true);

    // Zero length is always equal
    TEST_EQUALS(__internal_memcmp(a, c, 0), 0);
}

static void test_stdlib_internal_strlen() {
    TEST_EQUALS(__internal_strlen(""), 0);
    TEST_EQUALS(__internal_strlen("a"), 1);
    TEST_EQUALS(__internal_strlen("1234567"), 7);
    TEST_EQUALS(__internal_strlen("12345678"), 8);
    TEST_EQUALS(__internal_strlen("123456789"), 9);
    TEST_EQUALS(__internal_strlen("The quick brown fox jumps over the lazy dog"), 43);

    // Unaligned start
    const char* const str = "0123456789";
    TEST_EQUALS(__internal_strlen(str + 3), 7);

    TEST_EQUALS(__internal_wcslen(L""), 0);
    TEST_EQUALS(__internal_wcslen(L"123456789"), 9);
}

static void test_stdlib_internal_strstr() {
    const char* const haystack = "The quick brown fox jumps over the lazy dog";

    TEST_EQUALS(strcmp(__internal_strstr(haystack, "brown"), "brown fox jumps over the lazy dog"), 0);
    TEST_EQUALS(strcmp(__internal_strstr(haystack, "The"), "The quick brown fox jumps over the lazy dog"), 0);
    TEST_EQUALS(strcmp(__internal_strstr(haystack, "dog"), "dog"), 0);
    TEST_EQUALS(strcmp(__internal_strstr(haystack, ""), haystack), 0);

    TEST_EQUALS(__internal_strstr(haystack, "cat"), NULL);
    TEST_EQUALS(__internal_strstr(haystack, "THE"), NULL); // case sensitive

    const char* const repeated = "aaaaaaaaaaaaaaaaaaaaX";
    TEST_EQUALS(strcmp(__internal_strstr(repeated, "X"), "X"), 0);
}

static void test_stdlib_internal_strchr() {
    const char* const str = "abcdefghij";

    TEST_EQUALS(strcmp(__internal_strchr(str, 'a'), "abcdefghij"), 0);
    TEST_EQUALS(strcmp(__internal_strchr(str, 'e'), "efghij"), 0);
    TEST_EQUALS(strcmp(__internal_strchr(str, 'j'), "j"), 0);
    TEST_EQUALS(strcmp(__internal_strchr(str, '\0'), ""), 0);
    TEST_EQUALS(__internal_strchr(str, 'z'), NULL);
}

static void test_stdlib_internal_strncpy() {
    char buffer[32];

    memset(buffer, 'X', sizeof(buffer));
    __internal_strncpy(buffer, "hello", 8);
    TEST_EQUALS(strcmp(buffer, "hello"), 0);

    // Truncation always keeps a terminator
    memset(buffer, 'X', sizeof(buffer));
    __internal_strncpy(buffer, "hello", 4);
    TEST_EQUALS(strcmp(buffer, "hel"), 0);
    TEST_EQUALS(buffer[3], '\0');

    // Source shorter than length
    memset(buffer, 'X', sizeof(buffer));
    __internal_strncpy(buffer, "ab", 16);
    TEST_EQUALS(strcmp(buffer, "ab"), 0);

    // Long source
    memset(buffer, 'X', sizeof(buffer));
    __internal_strncpy(buffer, "0123456789ABCDEFGHIJKLMNOP", 31);
    TEST_EQUALS(strlen(buffer), 26);
    TEST_EQUALS(strcmp(buffer, "0123456789ABCDEFGHIJKLMNOP"), 0);

    // wchar version
    wchar_t wbuffer[16];
    __internal_wcsncpy(wbuffer, L"hello", 16);
    TEST_EQUALS(wcscmp(wbuffer, L"hello"), 0);

    __internal_wcsncpy(wbuffer, L"hello", 3);
    TEST_EQUALS(wcscmp(wbuffer, L"he"), 0);
}

static void test_stdlib_internal_strcmp() {
    TEST_EQUALS(__internal_strcmp("", ""), 0);
    TEST_EQUALS(__internal_strcmp("abc", "abc"), 0);
    TEST_EQUALS(__internal_strcmp("abc", "abd") < 0, true);
    TEST_EQUALS(__internal_strcmp("abd", "abc") > 0, true);
    TEST_EQUALS(__internal_strcmp("abc", "abcd") < 0, true);
    TEST_EQUALS(__internal_strcmp("abcd", "abc") > 0, true);

    TEST_EQUALS(__internal_strncmp("abc", "abc", 3), 0);
    TEST_EQUALS(__internal_strncmp("abcX", "abcY", 3), 0);
    TEST_EQUALS(__internal_strncmp("abcX", "abcY", 4) < 0, true);
    TEST_EQUALS(__internal_strncmp("abc", "abd", 8) < 0, true);
    TEST_EQUALS(__internal_strncmp("abc", "abc", 0), 0);

    TEST_EQUALS(__internal_wcscmp(L"", L""), 0);
    TEST_EQUALS(__internal_wcscmp(L"abc", L"abc"), 0);
    TEST_EQUALS(__internal_wcscmp(L"abc", L"abd") < 0, true);
}

static void test_stdlib_internal_case() {
    TEST_EQUALS(__internal_toupper('a'), 'A');
    TEST_EQUALS(__internal_toupper('z'), 'Z');
    TEST_EQUALS(__internal_toupper('A'), 'A');
    TEST_EQUALS(__internal_toupper('0'), '0');
    TEST_EQUALS(__internal_toupper('-'), '-');

    TEST_EQUALS(__internal_tolower('A'), 'a');
    TEST_EQUALS(__internal_tolower('Z'), 'z');
    TEST_EQUALS(__internal_tolower('a'), 'a');
    TEST_EQUALS(__internal_tolower('0'), '0');

    char upper[] = "Hello World 123!";
    __internal__toupper(upper);
    TEST_EQUALS(strcmp(upper, "HELLO WORLD 123!"), 0);

    char lower[] = "Hello World 123!";
    __internal_tolower(lower);
    TEST_EQUALS(strcmp(lower, "hello world 123!"), 0);
}

static void test_stdlib_internal_char_classes() {
    // Compare against the C library for all possible char values
    for (int32 i = 0; i < 256; ++i) {
        const char c = (char) (byte) i;

        TEST_EQUALS(__internal_isdigit(c), isdigit((unsigned char) c) != 0);
        TEST_EQUALS(__internal_isalpha(c), isalpha((unsigned char) c) != 0);
        TEST_EQUALS(__internal_isalnum(c), isalnum((unsigned char) c) != 0);
    }
}

static void test_stdlib_internal_has_zero() {
    // OMS_HAS_ZERO detects a zero byte inside a size_t word (the result is a mask, not a bool)
    TEST_FALSE(OMS_HAS_ZERO((size_t) 0x0102030405060708ULL));
    TEST_TRUE(OMS_HAS_ZERO((size_t) 0x0102030405060008ULL));
    TEST_TRUE(OMS_HAS_ZERO((size_t) 0x0002030405060708ULL));
    TEST_TRUE(OMS_HAS_ZERO((size_t) 0x0102030405060700ULL));
    TEST_TRUE(OMS_HAS_ZERO((size_t) 0));

    // OMS_HAS_CHAR detects a specific byte inside a word
    TEST_TRUE(OMS_HAS_CHAR((size_t) 0x0102030405060708ULL, 0x04));
    TEST_FALSE(OMS_HAS_CHAR((size_t) 0x0102030405060708ULL, 0x09));
    TEST_FALSE(OMS_HAS_CHAR((size_t) 0x0102030405060708ULL, 0x00));
}

#ifdef UBER_TEST
    #ifdef main
        #undef main
    #endif
    #define main StdlibStdlibInternalTest
#endif

int main() {
    TEST_INIT(128);

    TEST_RUN(test_stdlib_internal_memset);
    TEST_RUN(test_stdlib_internal_memcpy);
    TEST_RUN(test_stdlib_internal_memmove);
    TEST_RUN(test_stdlib_internal_memcmp);
    TEST_RUN(test_stdlib_internal_strlen);
    TEST_RUN(test_stdlib_internal_strstr);
    TEST_RUN(test_stdlib_internal_strchr);
    TEST_RUN(test_stdlib_internal_strncpy);
    TEST_RUN(test_stdlib_internal_strcmp);
    TEST_RUN(test_stdlib_internal_case);
    TEST_RUN(test_stdlib_internal_char_classes);
    TEST_RUN(test_stdlib_internal_has_zero);

    TEST_FINALIZE();

    return 0;
}
