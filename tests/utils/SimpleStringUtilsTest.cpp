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

// Pointer-less strings: the characters are stored directly behind the struct
static void test_simple_string_inline()
{
    TEST_EQUALS(sizeof(SimpleStringInline<char>), 8);
    TEST_EQUALS(sizeof(SimpleStringInline<wchar_t>), 8);
    TEST_EQUALS(simple_string_inline_size<char>(16), 8 + 16);
    TEST_EQUALS(simple_string_inline_size<wchar_t>(16), 8 + 16 * sizeof(wchar_t));

    alignas(8) byte memory[64];
    memset(memory, 0xFF, sizeof(memory));

    SimpleStringInline<char>* const str = simple_string_inline_init<char>(memory, 16);
    TEST_TRUE((byte *) str == memory);
    TEST_EQUALS(str->length, 0);
    TEST_EQUALS(str->max_length, 16);

    // The characters start directly behind the header
    TEST_TRUE((byte *) simple_string_inline_str(str) == memory + sizeof(SimpleStringInline<char>));
    TEST_TRUE(simple_string_inline_from_str(simple_string_inline_str(str)) == str);

    const SimpleStringInline<char>* const const_str = str;
    TEST_TRUE(simple_string_inline_str(const_str) == simple_string_inline_str(str));
    TEST_TRUE(simple_string_inline_from_str(simple_string_inline_str(const_str)) == const_str);

    TEST_EQUALS(simple_string_inline_set(str, "hello", 5), 5);
    TEST_EQUALS(str->length, 5);
    TEST_EQUALS(memcmp(memory + 8, "hello", 5), 0);

    // Nothing behind the string is touched
    TEST_EQUALS(memory[8 + 5], 0xFF);

    // View for functions that take a SimpleString
    const SimpleString<char> view = simple_string_view(str);
    TEST_EQUALS(view.length, 5);
    TEST_EQUALS(view.max_length, 16);
    TEST_TRUE(view.str == (char *) memory + 8);

    const SimpleString<const char> const_view = simple_string_view(const_str);
    TEST_EQUALS(const_view.length, 5);
    TEST_TRUE(const_view.str == view.str);

    // Too long -> cut at max_length
    TEST_EQUALS(simple_string_inline_set(str, "0123456789ABCDEFGHIJ", 20), 16);
    TEST_EQUALS(str->length, 16);
    TEST_EQUALS(memcmp(memory + 8, "0123456789ABCDEF", 16), 0);
    TEST_EQUALS(memory[8 + 16], 0xFF);

    TEST_EQUALS(simple_string_inline_set(str, "", 0), 0);
    TEST_EQUALS(str->length, 0);

    // The string is position independent -> it can be copied together with its memory
    simple_string_inline_set(str, "copy me", 7);
    alignas(8) byte copy[64];
    memcpy(copy, memory, simple_string_inline_size<char>(16));

    const SimpleString<char> copy_view = simple_string_view((SimpleStringInline<char> *) copy);
    TEST_EQUALS(copy_view.length, 7);
    TEST_TRUE(copy_view.str == (char *) copy + 8);
    TEST_EQUALS(memcmp(copy_view.str, "copy me", 7), 0);
}

static void test_simple_string_inline_wchar()
{
    alignas(8) byte memory[64];

    SimpleStringInline<wchar_t>* const str = simple_string_inline_init<wchar_t>(memory, 8);
    TEST_EQUALS(simple_string_inline_set(str, L"wide", 4), 4);

    const SimpleString<wchar_t> view = simple_string_view(str);
    TEST_EQUALS(view.length, 4);
    TEST_EQUALS(view.max_length, 8);
    TEST_EQUALS(view.str[0], L'w');
    TEST_EQUALS(view.str[3], L'e');
}

static void test_utf8_string_inline()
{
    TEST_EQUALS(sizeof(Utf8StringInline), 8);
    TEST_EQUALS(utf8_string_inline_size(16), 8 + 16);

    alignas(8) byte memory[64];

    Utf8StringInline* const str = utf8_string_inline_init(memory, 16);
    TEST_EQUALS(str->length, 0);
    TEST_EQUALS(str->max_length, 16);
    TEST_TRUE((byte *) utf8_string_inline_str(str) == memory + 8);

    // "Grüße" = 5 characters, 7 bytes
    memcpy(utf8_string_inline_str(str), "Gr\xC3\xBC\xC3\x9F" "e", 8);
    str->length = 5;

    const Utf8String view = utf8_string_view(str);
    TEST_EQUALS(view.length, 5);
    TEST_EQUALS(view.max_length, 16);
    TEST_TRUE(view.str == (char *) memory + 8);

    const Utf8StringInline* const const_str = str;
    TEST_TRUE(utf8_string_inline_str(const_str) == view.str);
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
    TEST_RUN(test_simple_string_inline);
    TEST_RUN(test_simple_string_inline_wchar);
    TEST_RUN(test_utf8_string_inline);

    TEST_FINALIZE();

    return 0;
}
