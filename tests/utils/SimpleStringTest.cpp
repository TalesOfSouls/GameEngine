#include "../TestFramework.h"
#include "../../utils/SimpleString.h"

/**
 * Tests for the length aware string views in SimpleString.h
 *
 * The header only contains aggregate structs, so the tests cover the field
 * layout, the buffer conversion operators and the value semantics.
 */

static void test_simple_string_struct_char_fields()
{
    char buffer[16] = "hello";

    SimpleString<char> str = { 5, 16, buffer };

    TEST_EQUALS(str.length, 5);
    TEST_EQUALS(str.max_length, 16);
    TEST_TRUE(str.str == buffer);
    TEST_EQUALS(str.str[0], 'h');
    TEST_EQUALS(str.str[4], 'o');
    TEST_EQUALS(str.str[5], '\0');
}

static void test_simple_string_struct_wchar_fields()
{
    wchar_t buffer[16] = L"wide";

    SimpleString<wchar_t> str = { 4, 16, buffer };

    TEST_EQUALS(str.length, 4);
    TEST_EQUALS(str.max_length, 16);
    TEST_TRUE(str.str == buffer);
    TEST_EQUALS(str.str[0], L'w');
    TEST_EQUALS(str.str[3], L'e');
    TEST_EQUALS(str.str[4], L'\0');
}

static void test_simple_string_struct_utf8_fields()
{
    char buffer[16] = "utf8";

    Utf8String str = { 4, 8, buffer };

    TEST_EQUALS(str.length, 4);
    TEST_EQUALS(str.max_length, 8);
    TEST_TRUE(str.str == buffer);
    TEST_EQUALS(str.str[0], 'u');
    TEST_EQUALS(str.str[3], '8');
    TEST_EQUALS(str.str[4], '\0');
}

static void test_simple_string_struct_empty()
{
    SimpleString<char> str = { 0, 0, NULL };
    SimpleString<wchar_t> wstr = { 0, 0, NULL };
    Utf8String ustr = { 0, 0, NULL };

    TEST_EQUALS(str.length, 0);
    TEST_EQUALS(str.max_length, 0);
    TEST_TRUE(str.str == NULL);

    TEST_EQUALS(wstr.length, 0);
    TEST_EQUALS(wstr.max_length, 0);
    TEST_TRUE(wstr.str == NULL);

    TEST_EQUALS(ustr.length, 0);
    TEST_EQUALS(ustr.max_length, 0);
    TEST_TRUE(ustr.str == NULL);
}

static void test_simple_string_struct_value_semantics()
{
    char buffer[] = "abc";

    SimpleString<char> first = { 3, 3, buffer };
    SimpleString<char> second = first;

    // Copying the struct copies the pointer (shallow) but not the length
    TEST_TRUE(second.str == first.str);
    TEST_EQUALS(second.length, 3);
    TEST_EQUALS(second.max_length, 3);

    second.length = 1;
    TEST_EQUALS(first.length, 3);
    TEST_EQUALS(second.length, 1);
}

static void test_simple_string_struct_buffer_conversion()
{
    SimpleStringBuffer<char, 32> buffer = {};
    buffer.length = 3;
    buffer.str[0] = 'a';
    buffer.str[1] = 'b';
    buffer.str[2] = 'c';

    SimpleString<char> view = buffer;

    TEST_EQUALS(view.length, 3);
    // The converted view is limited by the current length, not by the capacity
    TEST_EQUALS(view.max_length, 3);
    TEST_TRUE(view.str == buffer.str);
    TEST_EQUALS(strcmp(view.str, "abc"), 0);

    // The conversion copies the value at conversion time
    buffer.length = 1;
    TEST_EQUALS(view.length, 3);
    TEST_EQUALS(buffer.length, 1);
}

static void test_simple_string_struct_utf8_buffer_conversion()
{
    Utf8StringBuffer<32> buffer = {};
    buffer.length = 2;
    buffer.str[0] = (char) (byte) 0xC3;
    buffer.str[1] = (char) (byte) 0xA4;
    buffer.str[2] = '\0';

    Utf8String view = buffer;

    TEST_EQUALS(view.length, 2);
    TEST_EQUALS(view.max_length, 2);
    TEST_TRUE(view.str == buffer.str);
    TEST_EQUALS((byte) view.str[0], 0xC3);
    TEST_EQUALS((byte) view.str[1], 0xA4);
    TEST_EQUALS(view.str[2], '\0');

    buffer.length = 0;
    TEST_EQUALS(view.length, 2);
}

#ifdef UBER_TEST
    #ifdef main
        #undef main
    #endif
    #define main UtilsSimpleStringTest
#endif

int main() {
    TEST_INIT(64);

    TEST_RUN(test_simple_string_struct_char_fields);
    TEST_RUN(test_simple_string_struct_wchar_fields);
    TEST_RUN(test_simple_string_struct_utf8_fields);
    TEST_RUN(test_simple_string_struct_empty);
    TEST_RUN(test_simple_string_struct_value_semantics);
    TEST_RUN(test_simple_string_struct_buffer_conversion);
    TEST_RUN(test_simple_string_struct_utf8_buffer_conversion);

    TEST_FINALIZE();

    return 0;
}
