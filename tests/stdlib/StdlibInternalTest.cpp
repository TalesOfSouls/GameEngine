#include "../TestFramework.h"
#include "../../stdlib/stdlib_internal.h"

/**
 * Tests for the stdlib replacement primitives
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

    // Every alignment, every position of the needle (alignment part, words, tail) + not found
    alignas(size_t) char storage[64];
    const char* const text = "0123456789ABCDEFGHIJKLMNOP";

    int32 mismatches = 0;
    for (int32 offset = 0; offset < 8; ++offset) {
        char* const s = storage + offset;
        strcpy(s, text);

        for (const char* c = text; ; ++c) {
            mismatches += __internal_strchr(s, *c) != strchr(s, *c);
            if (*c == '\0') {
                break;
            }
        }

        mismatches += __internal_strchr(s, 'z') != strchr(s, 'z');
    }

    TEST_EQUALS(mismatches, 0);
}

static void test_stdlib_internal_wcschr() {
    alignas(size_t) wchar_t storage[32];
    const wchar_t* const text = L"hello world";

    int32 mismatches = 0;
    for (int32 offset = 0; offset < 4; ++offset) {
        wchar_t* const s = storage + offset;
        wcscpy(s, text);

        for (const wchar_t* c = text; ; ++c) {
            mismatches += __internal_wcschr(s, *c) != wcschr(s, *c);
            if (*c == L'\0') {
                break;
            }
        }

        mismatches += __internal_wcschr(s, L'z') != wcschr(s, L'z');
        mismatches += __internal_wcschr(s, (wchar_t) 0x6F00) != wcschr(s, (wchar_t) 0x6F00);
    }

    TEST_EQUALS(mismatches, 0);
}

static void test_stdlib_internal_wcsstr() {
    const wchar_t* const haystack = L"The quick brown fox jumps over the lazy dog";

    TEST_TRUE(__internal_wcsstr(haystack, L"brown") == wcsstr(haystack, L"brown"));
    TEST_TRUE(__internal_wcsstr(haystack, L"The") == haystack);
    TEST_TRUE(__internal_wcsstr(haystack, L"dog") == wcsstr(haystack, L"dog"));
    TEST_TRUE(__internal_wcsstr(haystack, L"") == haystack);
    TEST_TRUE(__internal_wcsstr(haystack, L"cat") == NULL);
}

static int32 test_sign(int32 value)
{
    return (value > 0) - (value < 0);
}

static void test_stdlib_internal_strcpy() {
    // Every source alignment and length (terminator inside of the alignment part, the words and the tail)
    alignas(size_t) char src_storage[64];
    const char* const text = "0123456789ABCDEFGHIJKLMNOP";
    const int32 text_length = (int32) strlen(text);

    int32 mismatches = 0;
    for (int32 offset = 0; offset < 8; ++offset) {
        for (int32 length = 0; length <= text_length; ++length) {
            char* const src = src_storage + offset;
            memcpy(src, text, length);
            src[length] = '\0';

            char expected[48];
            char buffer[48];
            memset(expected, 'X', sizeof(expected));
            memset(buffer, 'X', sizeof(buffer));

            strcpy(expected, src);
            mismatches += __internal_strcpy(buffer, src) != buffer;
            mismatches += memcmp(expected, buffer, sizeof(buffer)) != 0;
        }
    }

    TEST_EQUALS(mismatches, 0);

    alignas(size_t) wchar_t wsrc_storage[32];
    const wchar_t* const wtext = L"hello world";
    const int32 wtext_length = (int32) wcslen(wtext);

    int32 wmismatches = 0;
    for (int32 offset = 0; offset < 4; ++offset) {
        for (int32 length = 0; length <= wtext_length; ++length) {
            wchar_t* const wsrc = wsrc_storage + offset;
            memcpy(wsrc, wtext, length * sizeof(wchar_t));
            wsrc[length] = L'\0';

            wchar_t expected[20];
            wchar_t buffer[20];
            memset(expected, 0x7F, sizeof(expected));
            memset(buffer, 0x7F, sizeof(buffer));

            wcscpy(expected, wsrc);
            wmismatches += __internal_wcscpy(buffer, wsrc) != buffer;
            wmismatches += memcmp(expected, buffer, sizeof(buffer)) != 0;
        }
    }

    TEST_EQUALS(wmismatches, 0);
}

static void test_stdlib_internal_strncpy() {
    // Every source alignment, n below/at/above the string length (incl. the '\0' padding of the C library)
    alignas(size_t) char src_storage[64];
    const char* const text = "0123456789ABCDEFGHIJKLMNOP";
    const int32 text_length = (int32) strlen(text);

    int32 mismatches = 0;
    for (int32 offset = 0; offset < 8; ++offset) {
        char* const src = src_storage + offset;
        strcpy(src, text);

        for (size_t n = 0; n <= (size_t) text_length + 6; ++n) {
            char expected[48];
            char buffer[48];
            memset(expected, 'X', sizeof(expected));
            memset(buffer, 'X', sizeof(buffer));

            strncpy(expected, src, n);
            mismatches += __internal_strncpy(buffer, src, n) != buffer;
            mismatches += memcmp(expected, buffer, sizeof(buffer)) != 0;
        }
    }

    TEST_EQUALS(mismatches, 0);

    // wchar version
    alignas(size_t) wchar_t wsrc_storage[32];
    const wchar_t* const wtext = L"hello world";

    int32 wmismatches = 0;
    for (int32 offset = 0; offset < 4; ++offset) {
        wchar_t* const wsrc = wsrc_storage + offset;
        wcscpy(wsrc, wtext);

        for (size_t n = 0; n <= 16; ++n) {
            wchar_t expected[20];
            wchar_t buffer[20];
            memset(expected, 0x7F, sizeof(expected));
            memset(buffer, 0x7F, sizeof(buffer));

            wcsncpy(expected, wsrc, n);
            wmismatches += __internal_wcsncpy(buffer, wsrc, n) != buffer;
            wmismatches += memcmp(expected, buffer, sizeof(buffer)) != 0;
        }
    }

    TEST_EQUALS(wmismatches, 0);
}

static void test_stdlib_internal_strcmp() {
    // Every alignment combination (same offset = word-wise path, different offset = byte path)
    // with differences before, inside and after the first word + prefixes of each other
    alignas(size_t) char a_storage[64];
    alignas(size_t) char b_storage[64];

    const char* const texts[] = {
        "", "a", "abc", "abcdefg", "abcdefgh", "abcdefghijklmnopqrstuvwxyz",
        "abcdefghijklmnopqrstuvwxyZ", "abcdefghXjklmnop", "abcdefghijklmnopq", "\xFF" "bc"
    };
    const int32 text_count = ARRAY_COUNT(texts);

    int32 mismatches = 0;
    int32 n_mismatches = 0;
    for (int32 off_a = 0; off_a < 8; ++off_a) {
        for (int32 off_b = 0; off_b < 8; ++off_b) {
            for (int32 i = 0; i < text_count; ++i) {
                for (int32 j = 0; j < text_count; ++j) {
                    char* const a = a_storage + off_a;
                    char* const b = b_storage + off_b;
                    strcpy(a, texts[i]);
                    strcpy(b, texts[j]);

                    mismatches += test_sign(__internal_strcmp(a, b)) != test_sign(strcmp(a, b));

                    for (size_t n = 0; n < 30; ++n) {
                        n_mismatches += test_sign(__internal_strncmp(a, b, n)) != test_sign(strncmp(a, b, n));
                    }
                }
            }
        }
    }

    TEST_EQUALS(mismatches, 0);
    TEST_EQUALS(n_mismatches, 0);

    TEST_EQUALS(__internal_wcscmp(L"", L""), 0);
    TEST_EQUALS(__internal_wcscmp(L"abc", L"abc"), 0);
    TEST_EQUALS(__internal_wcscmp(L"abc", L"abd") < 0, true);
    TEST_EQUALS(__internal_wcscmp(L"abd", L"abc") > 0, true);
    TEST_EQUALS(__internal_wcscmp(L"ab", L"abc") < 0, true);

    TEST_EQUALS(__internal_wcsncmp(L"abcX", L"abcY", 3), 0);
    TEST_EQUALS(__internal_wcsncmp(L"abcX", L"abcY", 4) < 0, true);
    TEST_EQUALS(__internal_wcsncmp(L"ab", L"abc", 8) < 0, true);
    TEST_EQUALS(__internal_wcsncmp(L"a", L"b", 0), 0);
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
    TEST_RUN(test_stdlib_internal_wcschr);
    TEST_RUN(test_stdlib_internal_wcsstr);
    TEST_RUN(test_stdlib_internal_strcpy);
    TEST_RUN(test_stdlib_internal_strncpy);
    TEST_RUN(test_stdlib_internal_strcmp);
    TEST_RUN(test_stdlib_internal_case);
    TEST_RUN(test_stdlib_internal_char_classes);
    TEST_RUN(test_stdlib_internal_has_zero);

    TEST_FINALIZE();

    return 0;
}
