#include "../TestFramework.h"
#include "../../utils/SimpleStringUtils.h"

/**
 * Tests for the constructors/helpers in SimpleStringUtils.h
 *
 * simple_string_literal() and utf8_string_literal() are CONSTEVAL in the
 * current code base, so they are only ever called with objects that have
 * static storage duration here.
 */

static constexpr char SIMPLE_STRING_UTILS_HELLO[] = "hello";
static constexpr wchar_t SIMPLE_STRING_UTILS_WIDE[] = L"wide";
static constexpr char SIMPLE_STRING_UTILS_UTF8_HELLO[] = "hello";
static constexpr char SIMPLE_STRING_UTILS_UTF8_UMLAUT[] = "\xC3\xA4\xC3\xB6\xC3\xBC";
static constexpr char SIMPLE_STRING_UTILS_UTF8_EMOJI[] = "\xF0\x9F\x98\x80";
static constexpr char SIMPLE_STRING_UTILS_UTF8_EMPTY[] = "";

static void test_simple_string_utils_literal()
{
    SimpleString<char> hello = simple_string_literal(SIMPLE_STRING_UTILS_HELLO);
    TEST_EQUALS(hello.length, 5);
    TEST_EQUALS(hello.max_length, 5);
    TEST_TRUE(hello.str == SIMPLE_STRING_UTILS_HELLO);
    TEST_EQUALS(strcmp(hello.str, "hello"), 0);

    SimpleString<wchar_t> wide = simple_string_literal(SIMPLE_STRING_UTILS_WIDE);
    TEST_EQUALS(wide.length, 4);
    TEST_EQUALS(wide.max_length, 4);
    TEST_TRUE(wide.str == SIMPLE_STRING_UTILS_WIDE);
    TEST_EQUALS(wcscmp(wide.str, L"wide"), 0);
}

static void test_simple_string_utils_literal_length()
{
    SimpleString<char> part = simple_string_literal(SIMPLE_STRING_UTILS_HELLO, 3);

    // The length is explicit, the maximum is derived from the array size
    TEST_EQUALS(part.length, 3);
    TEST_EQUALS(part.max_length, 5);
    TEST_TRUE(part.str == SIMPLE_STRING_UTILS_HELLO);
    TEST_TRUE(strncmp(part.str, "hel", 3) == 0);

    SimpleString<char> empty = simple_string_literal(SIMPLE_STRING_UTILS_HELLO, 0);
    TEST_EQUALS(empty.length, 0);
    TEST_EQUALS(empty.max_length, 5);
    TEST_TRUE(empty.str == SIMPLE_STRING_UTILS_HELLO);
}

static void test_simple_string_utils_init()
{
    char buffer[16] = "hello";

    SimpleString<char> sized = simple_string_init(buffer, 5);
    TEST_EQUALS(sized.length, 5);
    TEST_EQUALS(sized.max_length, 5);
    TEST_TRUE(sized.str == buffer);

    SimpleString<char> bounded = simple_string_init(buffer, 3, 16);
    TEST_EQUALS(bounded.length, 3);
    TEST_EQUALS(bounded.max_length, 16);
    TEST_TRUE(bounded.str == buffer);

    SimpleString<char> measured = simple_string_init(buffer);
    TEST_EQUALS(measured.length, 5);
    TEST_EQUALS(measured.max_length, 5);
    TEST_TRUE(measured.str == buffer);

    wchar_t wbuffer[16] = L"wide";
    SimpleString<wchar_t> wsized = simple_string_init(wbuffer, 4);
    TEST_EQUALS(wsized.length, 4);
    TEST_EQUALS(wsized.max_length, 4);
    TEST_TRUE(wsized.str == wbuffer);
}

static void test_simple_string_utils_empty_init()
{
    SimpleString<char> str = simple_string_empty_init<char>();
    TEST_EQUALS(str.length, 0);
    TEST_EQUALS(str.max_length, 0);
    TEST_TRUE(str.str == NULL);

    SimpleString<wchar_t> wstr = simple_string_empty_init<wchar_t>();
    TEST_EQUALS(wstr.length, 0);
    TEST_EQUALS(wstr.max_length, 0);
    TEST_TRUE(wstr.str == NULL);
}

static void test_utf8_string_utils_literal()
{
    Utf8String hello = utf8_string_literal(SIMPLE_STRING_UTILS_UTF8_HELLO);
    TEST_EQUALS(hello.length, 5);
    TEST_EQUALS(hello.max_length, 5);
    TEST_TRUE(hello.str == SIMPLE_STRING_UTILS_UTF8_HELLO);
    TEST_TRUE(memcmp(hello.str, "hello", 5) == 0);

    // 3 codepoints, 6 bytes
    Utf8String umlaut = utf8_string_literal(SIMPLE_STRING_UTILS_UTF8_UMLAUT);
    TEST_EQUALS(umlaut.length, 3);
    TEST_EQUALS(umlaut.max_length, 6);
    TEST_TRUE(memcmp(umlaut.str, SIMPLE_STRING_UTILS_UTF8_UMLAUT, 6) == 0);

    // 1 codepoint, 4 bytes
    Utf8String emoji = utf8_string_literal(SIMPLE_STRING_UTILS_UTF8_EMOJI);
    TEST_EQUALS(emoji.length, 1);
    TEST_EQUALS(emoji.max_length, 4);
    TEST_TRUE(memcmp(emoji.str, SIMPLE_STRING_UTILS_UTF8_EMOJI, 4) == 0);

    Utf8String empty = utf8_string_literal(SIMPLE_STRING_UTILS_UTF8_EMPTY);
    TEST_EQUALS(empty.length, 0);
    TEST_EQUALS(empty.max_length, 0);
    TEST_TRUE(empty.str != NULL);
    TEST_EQUALS(empty.str[0], '\0');
}

static void test_utf8_string_utils_init()
{
    char buffer[16] = "hello";

    Utf8String sized = utf8_string_init(buffer, 5);
    TEST_EQUALS(sized.length, 5);
    TEST_EQUALS(sized.max_length, 5);
    TEST_TRUE(sized.str == buffer);

    Utf8String bounded = utf8_string_init(buffer, 3, 16);
    TEST_EQUALS(bounded.length, 3);
    TEST_EQUALS(bounded.max_length, 16);
    TEST_TRUE(bounded.str == buffer);

    Utf8String measured = utf8_string_init(buffer);
    TEST_EQUALS(measured.length, 5);
    TEST_EQUALS(measured.max_length, 5);
    TEST_TRUE(measured.str == buffer);

    char umlaut[16];
    memcpy(umlaut, SIMPLE_STRING_UTILS_UTF8_UMLAUT, sizeof(SIMPLE_STRING_UTILS_UTF8_UMLAUT));

    Utf8String measured_umlaut = utf8_string_init(umlaut);
    // The helper counts codepoints, not bytes
    TEST_EQUALS(measured_umlaut.length, 3);
    TEST_EQUALS(measured_umlaut.max_length, 3);
    TEST_TRUE(measured_umlaut.str == umlaut);
}

static void test_utf8_string_utils_empty_init()
{
    Utf8String str = utf8_string_empty_init();
    TEST_EQUALS(str.length, 0);
    TEST_EQUALS(str.max_length, 0);
    TEST_TRUE(str.str == NULL);
}

static void test_simple_string_utils_identity_overloads()
{
    SimpleString<char> hello = simple_string_literal(SIMPLE_STRING_UTILS_HELLO);
    SimpleString<char> hello_again = simple_string_literal(hello);
    TEST_TRUE(hello_again.str == hello.str);
    TEST_EQUALS(hello_again.length, hello.length);
    TEST_EQUALS(hello_again.max_length, hello.max_length);

    Utf8String utf8 = utf8_string_literal(SIMPLE_STRING_UTILS_UTF8_HELLO);
    Utf8String utf8_again = utf8_string_literal(utf8);
    TEST_TRUE(utf8_again.str == utf8.str);
    TEST_EQUALS(utf8_again.length, utf8.length);
    TEST_EQUALS(utf8_again.max_length, utf8.max_length);
}

#ifdef UBER_TEST
    #ifdef main
        #undef main
    #endif
    #define main UtilsSimpleStringUtilsTest
#endif

int main() {
    TEST_INIT(96);

    TEST_RUN(test_simple_string_utils_literal);
    TEST_RUN(test_simple_string_utils_literal_length);
    TEST_RUN(test_simple_string_utils_init);
    TEST_RUN(test_simple_string_utils_empty_init);
    TEST_RUN(test_utf8_string_utils_literal);
    TEST_RUN(test_utf8_string_utils_init);
    TEST_RUN(test_utf8_string_utils_empty_init);
    TEST_RUN(test_simple_string_utils_identity_overloads);

    TEST_FINALIZE();

    return 0;
}
