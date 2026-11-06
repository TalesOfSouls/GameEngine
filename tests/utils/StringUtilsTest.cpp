#include <stdio.h>
#include <ctype.h>
#include "../TestFramework.h"
#include "../../utils/StringUtils.h"

static void test_utf8_encode()
{
    char out[5] = {0};
    TEST_EQUALS(utf8_encode(659, out), 2);
    TEST_EQUALS(utf8_encode(67857, out), 4);
}

static void test_utf8_decode()
{
    char in[5] = {0};
    uint32 codepoint;

    *((uint16 *) in) = SWAP_ENDIAN_BIG((uint16) 0xCA93);
    TEST_EQUALS(utf8_decode(in, &codepoint), 2);
    TEST_EQUALS(codepoint, 659);

    *((uint32 *) in) = SWAP_ENDIAN_BIG(0xF090A491);
    TEST_EQUALS(utf8_decode(in, &codepoint), 4);
    TEST_EQUALS(codepoint, 67857);

    char out[5] = {0};
    TEST_EQUALS(utf8_decode(codepoint, out), 4);
    TEST_TRUE(strcmp(out, in) == 0);
}

static void test_utf8_strlen()
{
    const char in[] = "Foo © bar 𝌆 baz ☃ qux";
    TEST_EQUALS(utf8_strlen(in), 21);
}

static void test_str_is_float()
{
    TEST_TRUE(str_is_float("1.234"));
    TEST_TRUE(str_is_float("-1.234"));
    TEST_TRUE(str_is_float("+1.234"));
    TEST_FALSE(str_is_float("1.234ABC"));
    TEST_FALSE(str_is_float("B1.234"));
    TEST_FALSE(str_is_float("1.2.34"));
}

static void test_str_is_integer()
{
    TEST_TRUE(str_is_integer("1234"));
    TEST_TRUE(str_is_integer("-1234"));
    TEST_TRUE(str_is_integer("+1234"));
    TEST_FALSE(str_is_integer("1.234"));
    TEST_FALSE(str_is_integer("1.234ABC"));
    TEST_FALSE(str_is_integer("B1.234"));
    TEST_FALSE(str_is_integer("1.2.34"));
}

static void test_str_is_alpha()
{
    TEST_FALSE(__internal_isalpha('2'));
    TEST_TRUE(__internal_isalpha('s'));
    TEST_TRUE(__internal_isalpha('D'));
    TEST_FALSE(__internal_isalpha('-'));
}

static void test_str_is_num()
{
    TEST_TRUE(__internal_isdigit('2'));
    TEST_FALSE(__internal_isdigit('s'));
    TEST_FALSE(__internal_isdigit('D'));
    TEST_FALSE(__internal_isdigit('-'));
}

static void test_str_is_alphanum()
{
    TEST_TRUE(__internal_isalnum('2'));
    TEST_TRUE(__internal_isalnum('s'));
    TEST_TRUE(__internal_isalnum('D'));
    TEST_FALSE(__internal_isalnum('-'));
}

static void test_str_move_past()
{
    const char* str = "123456789";
    const char* tmp = str;

    str_move_past(&tmp, '3');
    TEST_EQUALS(*tmp, '4');
}

static void test_str_move_to()
{
    const char* str = "123456789";
    const char* tmp = str;

    str_move_to(&tmp, '3');
    TEST_EQUALS(*tmp, '3');
}

static void test_str_move_to_pos()
{
    const char* str = "123456789";
    const char* tmp = str;

    str_move_to_pos(&tmp, 3);
    TEST_EQUALS(*tmp, '4');

    tmp = str;
    str_move_to_pos(&tmp, -3);
    TEST_EQUALS(*tmp, '7');

    // -----------
    tmp = str_move_to_pos(str, 3);
    TEST_EQUALS(*tmp, '4');

    tmp = str_move_to_pos(str, -3);
    TEST_EQUALS(*tmp, '7');
}

static void test_strlen()
{
    TEST_EQUALS(strlen("2asdf dw"), 8);
}

static void test_str_length_wchar()
{
    TEST_EQUALS(wcslen(L"2asdf dw"), 8);
}

static void test_str_contains()
{
    TEST_TRUE(str_contains("2asdf dw", "asd"));
    TEST_FALSE(str_contains("2asdf dw", "asda"));
    TEST_TRUE(str_contains("2asdf dw", "asd", 4));
    TEST_FALSE(str_contains("2asdf dw", "asd", 3));
}

static void test_str_is_empty()
{
    TEST_TRUE(str_is_empty(' '));
    TEST_TRUE(str_is_empty('\t'));
    TEST_TRUE(str_is_empty('\n'));
    TEST_TRUE(str_is_empty('\r'));

    TEST_FALSE(str_is_empty('a'));
}

static void test_str_is_eol()
{
    TEST_FALSE(str_is_eol('\t'));

    TEST_TRUE(str_is_eol('\n'));
    TEST_TRUE(str_is_eol('\r'));
}

static void test_str_contains_wchar()
{
    TEST_TRUE(str_contains(L"2asdf dw", L"asd"));
    TEST_FALSE(str_contains(L"2asdf dw", L"asda"));
    //TEST_TRUE(str_contains(L"2asdf dw", L"asd", 4)); Doesn't exist for wchar
    //TEST_FALSE(str_contains(L"2asdf dw", L"asd", 3)); Doesn't exist for wchar
}

static void test_strcmp()
{
    TEST_EQUALS(strcmp("2asdf dw", "2asdf dw"), 0);
    TEST_NOT_EQUALS(__internal_strcmp("2asdf dw", "2asdf"), 0);
}

static void test_strcmp_wchar()
{
    TEST_EQUALS(wcscmp(L"2asdf dw", L"2asdf dw"), 0);
    TEST_NOT_EQUALS(__internal_wcscmp(L"2asdf dw", L"2asdf"), 0);
}

#if PERFORMANCE_TEST
static void _strlen(volatile void* val) {
    volatile int64* res = (volatile int64 *) val;

    char buffer[32];
    memcpy(buffer, "This %d is a %s with %f values", sizeof("This %d is a %s with %f values"));
    buffer[30] = (byte) *res;

    *res += (int64) strlen(buffer);
}

static void _strlen_internal(volatile void* val) {
    volatile int64* res = (volatile int64 *) val;

    char buffer[32];
    memcpy(buffer, "This %d is a %s with %f values", sizeof("This %d is a %s with %f values"));
    buffer[30] = (byte) *res;

    *res += (int64) __internal_strlen(buffer);
}

static void test_str_length_performance() {
    COMPARE_FUNCTION_TEST_TIME(_strlen_internal, _strlen, 5.0);
    COMPARE_FUNCTION_TEST_CYCLE(_strlen_internal, _strlen, 5.0);
}
#endif

#if PERFORMANCE_TEST
static void _str_is_alphanum(volatile void* val) {
    bool* res = (bool *) val;
    srand(0);

    int32 a = 0;
    for (int32 i = 0; i < 1000; ++i) {
        a += __internal_isalnum((char) rand());
    }

    *res |= (bool) a;
}

static void _isalnum(volatile void* val) {
    bool* res = (bool *) val;
    srand(0);

    int32 a = 0;
    for (int32 i = 0; i < 1000; ++i) {
        a += isalnum((char) rand());
    }

    *res |= (bool) a;
}

static void test_str_is_alphanum_performance() {
    COMPARE_FUNCTION_TEST_TIME(_str_is_alphanum, _isalnum, 5.0);
    COMPARE_FUNCTION_TEST_CYCLE(_str_is_alphanum, _isalnum, 5.0);
}
#endif

static void test_sprintf_fast()
{
    char buffer[256];
    sprintf_fast(buffer, "This %d is a %s with %f values", 1337, "test", 3.0f);
    TEST_TRUE(strcmp(buffer, "This 1337 is a test with 3.00000 values") == 0);
}

#if PERFORMANCE_TEST
static void _sprintf_fast(volatile void* val) {
    volatile bool* res = (volatile bool *) val;

    char buffer[256];
    sprintf_fast(buffer, "This %d is a %s with %f values", 1337, "test", 3.0);
    *res |= (bool) (strcmp(buffer, "This 1337 is a test with 3.00000 values") == 0);
}

static void _sprintf(volatile void* val) {
    volatile bool* res = (volatile bool *) val;

    char buffer[256];
    sprintf(buffer, "This %d is a %s with %f values", 1337, "test", 3.0);
    *res |= (bool) (strcmp(buffer, "This 1337 is a test with 3.000000 values") == 0);
}

static void test_sprintf_fast_performance() {
    COMPARE_FUNCTION_TEST_TIME(_sprintf_fast, _sprintf, 5.0);
    COMPARE_FUNCTION_TEST_CYCLE(_sprintf_fast, _sprintf, 5.0);
}
#endif

static void test_str_to_float()
{
    TEST_EQUALS(str_to_float("1.000000"), 1.0f);
    TEST_EQUALS(str_to_float("+1.000000"), 1.0f);
    TEST_EQUALS(str_to_float("-1.000000"), -1.0f);
}

static void test_str_to_int_and_int_length()
{
    TEST_EQUALS(str_to_int("0"), 0);
    TEST_EQUALS(str_to_int("123"), 123);
    TEST_EQUALS(str_to_int("-123"), -123);
    TEST_EQUALS(str_to_int("007"), 7);
    TEST_EQUALS(str_to_int("2147483647"), 2147483647);
    TEST_EQUALS(str_to_int("9223372036854775807"), 9223372036854775807LL);

    // Parsing stops at the first non digit
    TEST_EQUALS(str_to_int("123abc"), 123);

    const char* pos = NULL;
    TEST_EQUALS(str_to_int("45xyz", &pos), 45);
    TEST_EQUALS(*pos, 'x');

    pos = NULL;
    TEST_EQUALS(str_to_int("-9,8", &pos), -9);
    TEST_EQUALS(*pos, ',');

    TEST_EQUALS(str_to_int("abc"), 0);
    TEST_EQUALS(str_to_int(""), 0);

    // Reference comparison against the C library for valid inputs
    TEST_EQUALS(str_to_int("123"), strtol("123", NULL, 10));
    TEST_EQUALS(str_to_int("-987654321"), strtol("-987654321", NULL, 10));

    char mutable_str[16] = "77rest";
    char* mutable_pos = NULL;
    TEST_EQUALS(str_to_int(mutable_str, &mutable_pos), 77);
    TEST_EQUALS(*mutable_pos, 'r');

    TEST_EQUALS(int_length(0), 1);
    TEST_EQUALS(int_length(9), 1);
    TEST_EQUALS(int_length(10), 2);
    TEST_EQUALS(int_length(-1), 2);
    TEST_EQUALS(int_length(1234567890123LL), 13);
    TEST_EQUALS(int_length(-1234567890123LL), 14);
    TEST_EQUALS(int_length(9223372036854775807LL), 19);
}

static void test_int_to_str_signed_unsigned()
{
    char buffer[32];

    TEST_EQUALS(int_to_str((int64) 0, buffer), 1);
    TEST_TRUE(strcmp(buffer, "0") == 0);

    TEST_EQUALS(int_to_str((int64) 123, buffer), 3);
    TEST_TRUE(strcmp(buffer, "123") == 0);

    TEST_EQUALS(int_to_str((int64) -123, buffer), 4);
    TEST_TRUE(strcmp(buffer, "-123") == 0);

    TEST_EQUALS(int_to_str((int32) 1000, buffer), 4);
    TEST_TRUE(strcmp(buffer, "1000") == 0);

    TEST_EQUALS(int_to_str((int32) -2147483647, buffer), 11);
    TEST_TRUE(strcmp(buffer, "-2147483647") == 0);

    TEST_EQUALS(int_to_str((int64) 1234567890123LL, buffer), 13);
    TEST_TRUE(strcmp(buffer, "1234567890123") == 0);

    TEST_EQUALS(int_to_str((int64) -1234567890123LL, buffer), 14);
    TEST_TRUE(strcmp(buffer, "-1234567890123") == 0);

    TEST_EQUALS(int_to_str((uint32) 4000000000U, buffer), 10);
    TEST_TRUE(strcmp(buffer, "4000000000") == 0);

    TEST_EQUALS(int_to_str((uint64) 0xFFFFFFFFFFFFFFFFULL, buffer), 20);
    TEST_TRUE(strcmp(buffer, "18446744073709551615") == 0);

    // The return value always matches the written length
    int32 written = int_to_str((int64) 98765, buffer);
    TEST_EQUALS(written, (int32) strlen(buffer));
    TEST_EQUALS(written, int_length(98765));
}

static void test_int_to_str_wchar_and_thousands()
{
    wchar_t wbuffer[32];

    TEST_EQUALS(int_to_str((int64) 1234, wbuffer), 4);
    TEST_TRUE(wcscmp(wbuffer, L"1234") == 0);

    TEST_EQUALS(int_to_str((int64) -42, wbuffer), 3);
    TEST_TRUE(wcscmp(wbuffer, L"-42") == 0);

    char buffer[32];

    TEST_EQUALS(int_to_str((int64) 0, buffer, ','), 1);
    TEST_TRUE(strcmp(buffer, "0") == 0);

    TEST_EQUALS(int_to_str((int64) 999, buffer, ','), 3);
    TEST_TRUE(strcmp(buffer, "999") == 0);

    TEST_EQUALS(int_to_str((int64) 1000, buffer, ','), 5);
    TEST_TRUE(strcmp(buffer, "1,000") == 0);

    TEST_EQUALS(int_to_str((int64) 1234567, buffer, ','), 9);
    TEST_TRUE(strcmp(buffer, "1,234,567") == 0);

    TEST_EQUALS(int_to_str((int64) -1234567, buffer, ','), 10);
    TEST_TRUE(strcmp(buffer, "-1,234,567") == 0);
}

static void test_str_length_and_str_right()
{
    TEST_EQUALS(str_length(""), 0);
    TEST_EQUALS(str_length("abc"), 3);
    TEST_EQUALS(str_length(L"wide"), 4);

    const char* path = "a/b/c";
    char* last_slash = str_right(path, '/');
    TEST_TRUE(last_slash != NULL);
    TEST_EQUALS(*last_slash, '/');
    TEST_TRUE(strcmp(last_slash, "/c") == 0);

    TEST_TRUE(str_right("abc", '/') == NULL);
    TEST_TRUE(str_right("", '/') == NULL);

    const wchar_t* wpath = L"a/b/c";
    wchar_t* wlast_slash = str_right(wpath, L'/');
    TEST_TRUE(wlast_slash != NULL);
    TEST_EQUALS(*wlast_slash, L'/');
    TEST_TRUE(wcscmp(wlast_slash, L"/c") == 0);
}

static void test_str_copy_and_copy_until()
{
    alignas(size_t) char aligned_src[64] = "hello";
    char dest[64] = {};

    // Aligned copy returns the length including the terminator
    TEST_EQUALS(str_copy(dest, aligned_src), 6);
    TEST_TRUE(strcmp(dest, "hello") == 0);

    alignas(size_t) char unaligned_storage[64] = {};
    memcpy(unaligned_storage + 1, "0123456789012345678", 20);

    char long_dest[64] = {};
    TEST_EQUALS(str_copy(long_dest, unaligned_storage + 1), 20);
    TEST_TRUE(memcmp(long_dest, "0123456789012345678", 20) == 0);

    // Short unaligned source: the terminator is hit while aligning, the return value
    // is the same as for every other copy (length including the terminator)
    alignas(size_t) char short_storage[64] = {};
    memcpy(short_storage + 1, "hello", 6);

    char short_dest[16] = {};
    TEST_EQUALS(str_copy(short_dest, short_storage + 1), 6);
    TEST_TRUE(strcmp(short_dest, "hello") == 0);

    char length_src[16] = "abcdef";
    char length_dest[16] = {};
    TEST_EQUALS(str_copy(length_dest, length_src, 3), 3);
    TEST_TRUE(strcmp(length_dest, "abc") == 0);

    alignas(size_t) wchar_t wide_src[32] = L"hello";
    wchar_t wide_dest[32] = {};
    TEST_EQUALS(str_copy(wide_dest, wide_src), 6);
    TEST_TRUE(wcscmp(wide_dest, L"hello") == 0);

    alignas(size_t) wchar_t wide_short_storage[32] = {};
    wchar_t* wide_short_src = wide_short_storage + 1;
    wcscpy(wide_short_src, L"hi");

    wchar_t wide_short_dest[32] = {};
    TEST_EQUALS(str_copy(wide_short_dest, wide_short_src), -1);
    TEST_TRUE(wcscmp(wide_short_dest, L"hi") == 0);

    // str_copy_until stops at the first delimiter or terminator
    char until_src[64] = "abc,def";
    char until_dest[32] = {};

    TEST_EQUALS(str_copy_until(until_dest, until_src, ','), 3);
    TEST_TRUE(strcmp(until_dest, "abc") == 0);

    TEST_EQUALS(str_copy_until(until_dest, until_src, ';'), 7);
    TEST_TRUE(strcmp(until_dest, "abc,def") == 0);

    TEST_EQUALS(str_copy_until(until_dest, until_src, 'a'), 0);
    TEST_TRUE(strcmp(until_dest, "") == 0);

    char empty_src[8] = "";
    TEST_EQUALS(str_copy_until(until_dest, empty_src, ','), 0);

    char delim_set_src[64] = "abc;def=ghi";
    str_copy_until(until_dest, delim_set_src, ",;=");
    TEST_TRUE(strcmp(until_dest, "abc") == 0);

    wchar_t wide_until_src[32] = L"abc,def";
    wchar_t wide_until_dest[32] = {};
    TEST_EQUALS(str_copy_until(wide_until_dest, wide_until_src, L','), 3);
    TEST_TRUE(wcscmp(wide_until_dest, L"abc") == 0);
}

static void test_str_append_concat_insert_remove()
{
    char append_buffer[32] = "abc";
    TEST_EQUALS(str_append(append_buffer, 3, "def", 3), 6);
    TEST_TRUE(strcmp(append_buffer, "abcdef") == 0);

    char concat1[32] = {};
    str_concat_new(concat1, (const char*) "foo", (const char*) "bar");
    TEST_TRUE(strcmp(concat1, "foobar") == 0);

    char concat2[32] = {};
    TEST_EQUALS(str_concat_new(concat2, "foo", 3, "bar", 3), 6);
    TEST_TRUE(strcmp(concat2, "foobar") == 0);

    char concat3[32] = {};
    str_concat_new(concat3, "a", "b", "c");
    TEST_TRUE(strcmp(concat3, "abc") == 0);

    wchar_t wconcat3[32] = {};
    str_concat_new(wconcat3, L"a", L"b", L"c");
    TEST_TRUE(wcscmp(wconcat3, L"abc") == 0);

    char concat4[32] = {};
    str_concat_new(concat4, "x=", 2, (int64) 42);
    TEST_TRUE(strcmp(concat4, "x=42") == 0);

    char concat5[32] = {};
    str_concat_new(concat5, "x=", (int64) 42);
    TEST_TRUE(strcmp(concat5, "x=42") == 0);

    wchar_t wconcat_lengths[32] = {};
    TEST_EQUALS(str_concat_new(wconcat_lengths, L"ab", 2, L"cd", 2), 4);
    TEST_TRUE(wcscmp(wconcat_lengths, L"abcd") == 0);

    char cat4[32] = "hello";
    TEST_EQUALS(strcat(cat4, (size_t) 5, "abc", (size_t) 3), 8);
    TEST_TRUE(strcmp(cat4, "helloabc") == 0);

    char cat3[32] = "abc";
    strcat(cat3, (size_t) 3, "!");
    TEST_TRUE(strcmp(cat3, "abc!") == 0);

    char cat_int[32] = "n=";
    strcat(cat_int, (int64) 7);
    TEST_TRUE(strcmp(cat_int, "n=7") == 0);

    char insert_front[32] = "hello";
    str_insert(insert_front, 0, "X");
    TEST_TRUE(strcmp(insert_front, "Xhello") == 0);

    char insert_back[32] = "hello";
    str_insert(insert_back, 5, "!");
    TEST_TRUE(strcmp(insert_back, "hello!") == 0);

    wchar_t wide_insert[32] = L"hello";
    str_insert(wide_insert, 2, L"XX");
    TEST_TRUE(wcscmp(wide_insert, L"heXXllo") == 0);

    char insert_lengths[32] = "hello";
    TEST_EQUALS(str_insert(insert_lengths, 5, 1, "XY", 2), 7);
    TEST_TRUE(strcmp(insert_lengths, "hXYello") == 0);

    char remove_buffer[32] = "hello";
    str_remove(remove_buffer, 1, 2);
    TEST_TRUE(strcmp(remove_buffer, "hlo") == 0);

    char remove_none[32] = "hello";
    str_remove(remove_none, 5, 0);
    TEST_TRUE(strcmp(remove_none, "hello") == 0);

    // Tokenizing keeps empty fields collapsed and modifies the buffer in place
    char tokens[32] = "a,b,,c";
    char* token_key = NULL;
    TEST_TRUE(strcmp(strtok(tokens, ",", &token_key), "a") == 0);
    TEST_TRUE(strcmp(strtok(NULL, ",", &token_key), "b") == 0);
    TEST_TRUE(strcmp(strtok(NULL, ",", &token_key), "c") == 0);
    TEST_TRUE(strtok(NULL, ",", &token_key) == NULL);

    char sep_tokens[32] = "one;two";
    const char* sep_state = sep_tokens;
    char* sep_part = strsep(&sep_state, ";");
    TEST_TRUE(sep_part != NULL);
    TEST_TRUE(strcmp(sep_part, "one") == 0);
    TEST_EQUALS(*sep_state, 't');
    sep_part = strsep(&sep_state, ";");
    TEST_TRUE(sep_part != NULL);
    TEST_TRUE(strcmp(sep_part, "two") == 0);
    TEST_TRUE(strsep(&sep_state, ";") == NULL);
}

static void test_str_count_and_contains()
{
    TEST_EQUALS(str_count("abababa", "aba"), 2);
    TEST_EQUALS(str_count("aaaa", "aa"), 2);
    TEST_EQUALS(str_count("aaaaa", "aa"), 2);
    TEST_EQUALS(str_count("hello", "l"), 2);
    TEST_EQUALS(str_count("hello", "xyz"), 0);
    TEST_EQUALS(str_count("abc", ""), 0);

    char haystack[64] = "hello world";
    TEST_TRUE(str_contains(haystack, "world"));
    TEST_TRUE(str_contains(haystack, "hello"));
    TEST_TRUE(str_contains("", "x") == false);

    // The length limited variant does not perform word wise scanning
    TEST_TRUE(str_contains(haystack, "ell", 5));
    TEST_TRUE(str_contains(haystack, "ell", 4));
    TEST_FALSE(str_contains(haystack, "ell", 2));
    TEST_FALSE(str_contains(haystack, "xyz", 5));

    TEST_TRUE(str_contains(L"hello", L"ell"));
    TEST_FALSE(str_contains(L"hello", L"xyz"));
    TEST_TRUE(str_contains(L"hello", L""));

    byte byte_haystack[] = {1, 2, 3, 4, 5, 6};
    byte byte_needle[] = {3, 4};
    byte byte_reversed[] = {4, 3};
    byte byte_last[] = {6};

    TEST_TRUE(byte_contains(byte_haystack, sizeof(byte_haystack), byte_needle, sizeof(byte_needle)));
    TEST_FALSE(byte_contains(byte_haystack, sizeof(byte_haystack), byte_reversed, sizeof(byte_reversed)));
    TEST_TRUE(byte_contains(byte_haystack, sizeof(byte_haystack), byte_last, sizeof(byte_last)));
}

static void test_str_compare_caseless_and_ends_with()
{
    TEST_EQUALS(str_compare_caseless("Hello", "hello"), 0);
    TEST_EQUALS(str_compare_caseless("ABC", "abc"), 0);
    TEST_TRUE(str_compare_caseless("abc", "abd") < 0);
    TEST_TRUE(str_compare_caseless("abd", "abc") > 0);

    TEST_EQUALS(str_compare_caseless("Hello", "HELP", 3), 0);
    TEST_TRUE(str_compare_caseless("Hello", "HELP", 4) != 0);
    TEST_EQUALS(str_compare_caseless("abc", "xyz", 0), 0);

    TEST_TRUE(str_ends_with("filename.cpp", ".cpp"));
    TEST_FALSE(str_ends_with("filename.cpp", ".h"));
    TEST_TRUE(str_ends_with("abc", ""));
    TEST_TRUE(str_ends_with("", ""));
    TEST_FALSE(str_ends_with("abc", "abcd"));
    TEST_FALSE(str_ends_with((const char*) NULL, "a"));
    TEST_FALSE(str_ends_with("a", (const char*) NULL));

    TEST_TRUE(str_ends_with(L"filename.cpp", L".cpp"));
    TEST_FALSE(str_ends_with(L"filename.cpp", L".h"));
    TEST_TRUE(str_ends_with(L"abc", L""));

    TEST_TRUE(str_is_alpha("abc"));
    TEST_TRUE(str_is_alpha("AbC"));
    TEST_FALSE(str_is_alpha("ab1"));
    TEST_TRUE(str_is_num("123"));
    TEST_FALSE(str_is_num("12a"));
    TEST_TRUE(str_is_alphanum("a1B"));
    TEST_FALSE(str_is_alphanum("a-1"));

    TEST_TRUE(str_starts_with_num("123"));
    TEST_TRUE(str_starts_with_num("-42"));
    TEST_TRUE(str_starts_with_num("+7"));
    TEST_TRUE(str_starts_with_num("12 34"));
    TEST_FALSE(str_starts_with_num("abc"));
    TEST_FALSE(str_starts_with_num("12abc"));
}

static void test_str_toupper_tolower_and_pad()
{
    char upper[32] = "Hello, World! 123";
    str_toupper(upper);
    TEST_TRUE(strcmp(upper, "HELLO, WORLD! 123") == 0);

    char lower[32] = "Hello, World! 123";
    str_tolower(lower);
    TEST_TRUE(strcmp(lower, "hello, world! 123") == 0);

    char digits[16] = "123456";
    str_toupper(digits);
    TEST_TRUE(strcmp(digits, "123456") == 0);

    char padded_right[16] = {};
    str_pad_right("abc", padded_right, '-', 6);
    TEST_TRUE(strcmp(padded_right, "abc---") == 0);
    TEST_EQUALS(padded_right[6], '\0');

    char truncated_right[16] = {};
    str_pad_right("abc", truncated_right, '-', 2);
    TEST_TRUE(strncmp(truncated_right, "ab", 2) == 0);
}

static void test_hex_helpers()
{
    char hex_buffer[32] = {};

    byte_to_hex((byte) 0xAB, hex_buffer);
    TEST_TRUE(strcmp(hex_buffer, "AB") == 0);

    memset(hex_buffer, 0, sizeof(hex_buffer));
    byte_to_hex((byte) 0x0F, hex_buffer);
    TEST_TRUE(strcmp(hex_buffer, "0F") == 0);

    byte hex_data[] = {0x00, 0x1F, 0x2A};
    memset(hex_buffer, 0, sizeof(hex_buffer));
    bytes_to_hex(hex_data, sizeof(hex_data), hex_buffer);
    TEST_TRUE(strcmp(hex_buffer, "001F2A") == 0);

    wchar_t wide_hex[8] = {};
    byte_to_hex((byte) 0x0F, wide_hex);
    TEST_TRUE(wcscmp(wide_hex, L"0F") == 0);

    memset(hex_buffer, 0, sizeof(hex_buffer));
    TEST_EQUALS(int_to_hex((uint32) 0xDEADBEEF, hex_buffer), 8);
    TEST_TRUE(strcmp(hex_buffer, "DEADBEEF") == 0);

    memset(hex_buffer, 0, sizeof(hex_buffer));
    TEST_EQUALS(int_to_hex((uint64) 0, hex_buffer), 1);
    TEST_TRUE(strcmp(hex_buffer, "0") == 0);

    memset(hex_buffer, 0, sizeof(hex_buffer));
    TEST_EQUALS(int_to_hex((byte) 0x0F, hex_buffer), 1);
    TEST_TRUE(strcmp(hex_buffer, "F") == 0);

    TEST_EQUALS(hex_to_int("1A2B"), 0x1A2B);
    TEST_EQUALS(hex_to_int("ff"), 0xFF);
    TEST_EQUALS(hex_to_int("0"), 0);
    TEST_EQUALS(hex_to_int("xyz"), 0);
    TEST_EQUALS(hex_to_int("FF8000FF"), (int64) 0xFF8000FF);

    TEST_TRUE(str_is_hex_color("#A1B2C3"));
    TEST_TRUE(str_is_hex_color("#a1b2c3"));
    TEST_FALSE(str_is_hex_color("A1B2C3"));
    TEST_FALSE(str_is_hex_color("#GG0000"));

    v4_f32 color = {};
    hexstr_to_rgba(&color, "#FF8000FF");
    TEST_EQUALS_WITH_DELTA(color.r, 1.0f, 0.0001);
    TEST_EQUALS_WITH_DELTA(color.g, 128.0f / 255.0f, 0.0001);
    TEST_EQUALS_WITH_DELTA(color.b, 0.0f, 0.0001);
    TEST_EQUALS_WITH_DELTA(color.a, 1.0f, 0.0001);
}

static void test_str_to_float_and_float_to_str()
{
    TEST_EQUALS_WITH_DELTA(str_to_float("1.5"), 1.5f, 0.0001f);
    TEST_EQUALS_WITH_DELTA(str_to_float("-2.25"), -2.25f, 0.0001f);
    TEST_EQUALS_WITH_DELTA(str_to_float("+3.125"), 3.125f, 0.0001f);
    TEST_EQUALS_WITH_DELTA(str_to_float("42"), 42.0f, 0.0001f);
    TEST_EQUALS_WITH_DELTA(str_to_float(".5"), 0.5f, 0.0001f);
    TEST_EQUALS_WITH_DELTA(str_to_float("  7.5"), 7.5f, 0.0001f);
    TEST_EQUALS_WITH_DELTA(str_to_float("-0.5"), -0.5f, 0.0001f);

    const char* pos = NULL;
    TEST_EQUALS_WITH_DELTA(str_to_float("1.5x", &pos), 1.5f, 0.0001f);
    TEST_EQUALS(*pos, 'x');

    TEST_EQUALS_WITH_DELTA(str_to_float("123.456"), strtof("123.456", NULL), 0.0001f);

    char buffer[32];

    TEST_EQUALS(float_to_str((f64) 1.5, buffer, 2), 4);
    TEST_TRUE(strcmp(buffer, "1.50") == 0);

    TEST_EQUALS(float_to_str((f64) 1.5, buffer), 7);
    TEST_TRUE(strcmp(buffer, "1.50000") == 0);

    TEST_EQUALS(float_to_str((f64) 0.0, buffer, 3), 5);
    TEST_TRUE(strcmp(buffer, "0.000") == 0);

    TEST_EQUALS(float_to_str((f64) -1.25, buffer, 2), 5);
    TEST_TRUE(strcmp(buffer, "-1.25") == 0);

    TEST_EQUALS(float_to_str((f64) 1.5, buffer, 0), 1);
    TEST_TRUE(strcmp(buffer, "2") == 0);

    TEST_EQUALS(float_to_str((f64) 100.0, buffer, 0), 3);
    TEST_TRUE(strcmp(buffer, "100") == 0);

    wchar_t wide_buffer[32] = {};
    TEST_EQUALS(float_to_str((f64) 2.5, wide_buffer, 2), 4);
    TEST_TRUE(wcscmp(wide_buffer, L"2.50") == 0);
}

static void test_format_time_helpers()
{
    char time_buffer[32] = {};

    format_time_hh_mm_ss_ms(time_buffer, (uint64) 3661123);
    TEST_TRUE(strcmp(time_buffer, "01:01:01.123") == 0);

    format_time_hh_mm_ss_ms(time_buffer, (uint64) 0);
    TEST_TRUE(strcmp(time_buffer, "00:00:00.000") == 0);

    format_time_hh_mm_ss_ms(time_buffer, 23, 59, 58, 7);
    TEST_TRUE(strcmp(time_buffer, "23:59:58.007") == 0);

    format_time_hh_mm_ss(time_buffer, (uint64) 3661);
    TEST_TRUE(strcmp(time_buffer, "01:01:01") == 0);

    format_time_hh_mm_ss(time_buffer, 0, 0, 0);
    TEST_TRUE(strcmp(time_buffer, "00:00:00") == 0);

    format_time_hh_mm(time_buffer, (uint64) 3661);
    TEST_TRUE(strcmp(time_buffer, "01:01") == 0);

    format_time_hh_mm(time_buffer, 12, 5);
    TEST_TRUE(strcmp(time_buffer, "12:05") == 0);
}

static void test_sprintf_fast_and_iter()
{
    char buffer[128] = {};

    TEST_EQUALS(sprintf_fast(buffer, "val=%d", (int32) -42), 7);
    TEST_TRUE(strcmp(buffer, "val=-42") == 0);

    sprintf_fast(buffer, "%u", (uint32) 4000000000U);
    TEST_TRUE(strcmp(buffer, "4000000000") == 0);

    sprintf_fast(buffer, "%l", (int64) -1234567890123LL);
    TEST_TRUE(strcmp(buffer, "-1234567890123") == 0);

    sprintf_fast(buffer, "%n", (int64) 1234567);
    TEST_TRUE(strcmp(buffer, "1,234,567") == 0);

    sprintf_fast(buffer, "%f", (f64) 1.5);
    TEST_TRUE(strcmp(buffer, "1.50000") == 0);

    sprintf_fast(buffer, "%f2", (f64) 1.25);
    TEST_TRUE(strcmp(buffer, "1.25") == 0);

    sprintf_fast(buffer, "%s", "abc");
    TEST_TRUE(strcmp(buffer, "abc") == 0);

    sprintf_fast(buffer, "%s2", "abcdef");
    TEST_TRUE(strcmp(buffer, "ab") == 0);

    sprintf_fast(buffer, "%c", (int32) 'Q');
    TEST_TRUE(strcmp(buffer, "Q") == 0);

    byte hex_data[] = {0xDE, 0xAD, 0x00};
    sprintf_fast(buffer, "%h", (const byte*) hex_data);
    TEST_TRUE(strcmp(buffer, "DEAD") == 0);

    sprintf_fast(buffer, "%T", (int64) 3661);
    TEST_TRUE(strcmp(buffer, "01:01:01") == 0);

    sprintf_fast(buffer, "100\\% sure");
    TEST_TRUE(strcmp(buffer, "100% sure") == 0);

    sprintf_fast(buffer, "x=%d y=%s", (int32) 1, "z");
    TEST_TRUE(strcmp(buffer, "x=1 y=z") == 0);

    TEST_EQUALS(sprintf_fast(buffer, 128, "v=%d", (int32) 7), 3);
    TEST_TRUE(strcmp(buffer, "v=7") == 0);

    // The iter variant only replaces the first % specifier
    sprintf_fast_iter(buffer, "a=%d b=%d", (int32) 1, (int32) 2);
    TEST_TRUE(strcmp(buffer, "a=1 b=%d") == 0);

    TEST_EQUALS(sprintf_fast_iter(buffer, 6, "%s", "abcdefgh"), 5);
    TEST_TRUE(strcmp(buffer, "abcde") == 0);

    TEST_EQUALS(sprintf_fast_iter(buffer, 3, "%d", (int32) 12345), 0);
    TEST_TRUE(strcmp(buffer, "") == 0);

    sprintf_fast_iter(buffer, "w=%w", L"AB");
    TEST_TRUE(strcmp(buffer, "w=AB") == 0);

    int64 first = 0;
    int64 second = 0;
    TEST_TRUE(sscanf_fast("12 34", "%d %d", &first, &second));
    TEST_EQUALS(first, 12);
    TEST_EQUALS(second, 34);

    int64 negative = 0;
    TEST_TRUE(sscanf_fast("  -45xyz", "%d", &negative));
    TEST_EQUALS(negative, -45);

    f32 float_value = 0.0f;
    TEST_TRUE(sscanf_fast("3.5", "%f", &float_value));
    TEST_EQUALS_WITH_DELTA(float_value, 3.5f, 0.0001f);
}

static void test_utf8_and_wchar_conversions()
{
    char encoded[8] = {};
    uint32 codepoint = 0;

    TEST_EQUALS(utf8_encode(0x7F, encoded), 1);
    TEST_EQUALS(utf8_encode(0x80, encoded), 2);
    TEST_EQUALS(utf8_encode(0x7FF, encoded), 2);
    TEST_EQUALS(utf8_encode(0x800, encoded), 3);
    TEST_EQUALS(utf8_encode(0xFFFF, encoded), 3);
    TEST_EQUALS(utf8_encode(0x10000, encoded), 4);
    TEST_EQUALS(utf8_encode(0x10FFFF, encoded), 4);
    TEST_EQUALS(utf8_encode(0x110000, encoded), -1);

    const uint32 codepoints[] = {0x41, 0xA9, 0x20AC, 0x1F600};
    const int32 expected_bytes[] = {1, 2, 3, 4};

    for (int32 i = 0; i < 4; ++i) {
        memset(encoded, 0, sizeof(encoded));
        TEST_EQUALS(utf8_encode(codepoints[i], encoded), expected_bytes[i]);
        codepoint = 0;
        TEST_EQUALS(utf8_decode(encoded, &codepoint), expected_bytes[i]);
        TEST_EQUALS(codepoint, codepoints[i]);
    }

    char text[32] = "Foo \xC2\xA9 bar";
    TEST_EQUALS(utf8_get_char_at(text, 0), 'F');
    TEST_EQUALS(utf8_get_char_at(text, 4), 0xA9);
    TEST_EQUALS(utf8_get_char_at(text, 8), 'r');
    TEST_EQUALS(utf8_get_char_at(text, 9), -1);

    const uint32 utf32[] = {0x46, 0x6F, 0x6F, 0x20, 0xA9, 0x20, 0x62, 0x61, 0x72, 0};
    char from_utf32[32] = {};
    string_to_utf8(utf32, from_utf32);
    TEST_TRUE(strcmp(from_utf32, "Foo \xC2\xA9 bar") == 0);

    wchar_t wide[32] = {};
    TEST_EQUALS(char_to_wchar(wide, "abc", 32), 3);
    TEST_TRUE(wcscmp(wide, L"abc") == 0);

    memset(wide, 0, sizeof(wide));
    TEST_EQUALS(char_to_wchar(wide, "abcdef", 3), 2);
    TEST_TRUE(wcscmp(wide, L"ab") == 0);

    char narrow[8] = {};
    TEST_EQUALS((int32) wchar_to_char(narrow, L'A'), 1);
    TEST_EQUALS(narrow[0], 'A');

    memset(narrow, 0, sizeof(narrow));
    TEST_EQUALS((int32) wchar_to_char(narrow, (wchar_t) 0x20AC), 2);
    TEST_EQUALS((byte) narrow[0], 0xAC);
    TEST_EQUALS((byte) narrow[1], 0x20);

    wchar_t in_place[16] = L"abc";
    wchar_to_char(in_place);
    TEST_TRUE(strcmp((char*) in_place, "abc") == 0);

    char wide_to_narrow[16] = {};
    wchar_to_char(wide_to_narrow, L"abc");
    TEST_TRUE(strcmp(wide_to_narrow, "abc") == 0);

    char wide_to_narrow_limited[16] = {};
    wchar_to_char(wide_to_narrow_limited, L"abcdef", 3);
    TEST_TRUE(strcmp(wide_to_narrow_limited, "abc") == 0);

    wchar_t utf8_to_wide[16] = {};
    TEST_EQUALS(utf8_to_wchar("abc", utf8_to_wide, 16), 4);
    TEST_TRUE(wcscmp(utf8_to_wide, L"abc") == 0);

    memset(utf8_to_wide, 0, sizeof(utf8_to_wide));
    TEST_EQUALS(utf8_to_wchar("\xC3\xA4", utf8_to_wide, 16), 2);
    TEST_EQUALS(utf8_to_wide[0], 0xE4);

    memset(utf8_to_wide, 0, sizeof(utf8_to_wide));
    TEST_EQUALS(utf8_to_wchar("\xE2\x82\xAC", utf8_to_wide, 16), 2);
    TEST_EQUALS(utf8_to_wide[0], 0x20AC);

    memset(utf8_to_wide, 0, sizeof(utf8_to_wide));
    if constexpr (sizeof(wchar_t) == 2) {
        TEST_EQUALS(utf8_to_wchar("\xF0\x9F\x98\x80", utf8_to_wide, 16), 3);
        TEST_EQUALS(utf8_to_wide[0], 0xD83D);
        TEST_EQUALS(utf8_to_wide[1], 0xDE00);
    } else {
        TEST_EQUALS(utf8_to_wchar("\xF0\x9F\x98\x80", utf8_to_wide, 16), 2);
        TEST_EQUALS(utf8_to_wide[0], 0x1F600);
    }

    memset(utf8_to_wide, 0, sizeof(utf8_to_wide));
    TEST_EQUALS(utf8_to_wchar("abc", utf8_to_wide, 2), 2);
    TEST_TRUE(wcscmp(utf8_to_wide, L"a") == 0);

    memset(utf8_to_wide, 0, sizeof(utf8_to_wide));
    TEST_EQUALS(utf8_to_wchar("\xF0\x9F\x98\x80", utf8_to_wide, 2), 0);

    memset(utf8_to_wide, 0, sizeof(utf8_to_wide));
    TEST_EQUALS(utf8_to_wchar("\xFF", utf8_to_wide, 16), 0);
}

static void test_str_line_and_skip_helpers()
{
    TEST_EQUALS(is_eol("\n"), 1);
    TEST_EQUALS(is_eol("\r\n"), 2);
    TEST_EQUALS(is_eol("abc"), 0);
    TEST_EQUALS(is_eol(""), 0);

    char eol_text[32] = "abc\ndef";
    TEST_EQUALS(str_to_eol(eol_text), 3);

    char eol_crlf[32] = "abc\r\ndef";
    TEST_EQUALS(str_to_eol(eol_crlf), 3);

    char no_eol[32] = "abc";
    TEST_EQUALS(str_to_eol(no_eol), 3);
    TEST_EQUALS(str_to_eol((const char*) ""), 0);

    char copy_eol[32] = {};
    TEST_EQUALS(str_copy_to_eol(eol_text, copy_eol), 3);
    TEST_TRUE(strcmp(copy_eol, "abc") == 0);
    TEST_EQUALS(str_copy_to_eol(eol_text, copy_eol, 2), 2);
    TEST_TRUE(strcmp(copy_eol, "ab") == 0);
    TEST_EQUALS(str_copy_to_eol((const char*) "", copy_eol), 0);
    TEST_TRUE(strcmp(copy_eol, "") == 0);

    char empty_text[32] = "abc def";
    char empty_copy[32] = {};
    TEST_EQUALS(str_copy_to_empty(empty_text, empty_copy), 3);
    TEST_TRUE(strcmp(empty_copy, "abc") == 0);
    TEST_EQUALS(str_copy_to_empty(empty_text, empty_copy, 2), 2);
    TEST_TRUE(strcmp(empty_copy, "ab") == 0);

    char move_text[32] = "abc,def";
    const char* move_src = move_text;
    char move_dest[32] = {};
    str_copy_move_until(move_dest, &move_src, ',');
    TEST_TRUE(strcmp(move_dest, "abc") == 0);
    TEST_EQUALS(*move_src, ',');
    ++move_src;
    str_copy_move_until(move_dest, &move_src, ',');
    TEST_TRUE(strcmp(move_dest, "def") == 0);
    TEST_EQUALS(*move_src, '\0');

    char move_set_text[32] = "abc;def=ghi";
    const char* move_set_src = move_set_text;
    str_copy_move_until(move_dest, &move_set_src, ";=");
    TEST_TRUE(strcmp(move_dest, "abc") == 0);
    TEST_EQUALS(*move_set_src, ';');

    char hyphens[32] = "--abc";
    const char* skip_pos = hyphens;
    str_skip(&skip_pos, '-');
    TEST_EQUALS(*skip_pos, 'a');

    char spaces[32] = "  \tabc";
    skip_pos = spaces;
    str_skip_whitespace(&skip_pos);
    TEST_EQUALS(*skip_pos, 'a');

    char empties[32] = " \t\r\nabc";
    skip_pos = empties;
    str_skip_empty(&skip_pos);
    TEST_EQUALS(*skip_pos, 'a');
    TEST_EQUALS(*str_skip_empty((const char*) empties), 'a');

    char eols[32] = "\r\n\nabc";
    skip_pos = eols;
    str_skip_eol(&skip_pos);
    TEST_EQUALS(*skip_pos, 'a');
    TEST_EQUALS(*str_skip_eol((const char*) eols), 'a');

    char non_empty[32] = "abc def";
    skip_pos = non_empty;
    str_skip_non_empty(&skip_pos);
    TEST_EQUALS(*skip_pos, ' ');
    TEST_EQUALS(*str_skip_non_empty((const char*) non_empty), ' ');

    char line[32] = "abc\ndef";
    skip_pos = line;
    str_skip_line(&skip_pos);
    TEST_EQUALS(*skip_pos, 'd');
    TEST_EQUALS(*str_skip_line((const char*) "abc"), '\0');

    char skip_list_text[32] = ";;,,abc";
    skip_pos = skip_list_text;
    str_skip_list(&skip_pos, ",;", 2);
    TEST_EQUALS(*skip_pos, 'a');

    char alpha_num[32] = "abc-123_def ghi";
    skip_pos = alpha_num;
    str_move_past_alpha_num(&skip_pos);
    TEST_EQUALS(*skip_pos, ' ');

    TEST_TRUE(str_is_comment("//x"));
    TEST_TRUE(str_is_comment("/*x"));
    TEST_FALSE(str_is_comment("*/"));
    TEST_FALSE(str_is_comment("/x"));

    TEST_TRUE(str_is_space(' '));
    TEST_TRUE(str_is_space('\t'));
    TEST_TRUE(str_is_space('\r'));
    TEST_FALSE(str_is_space('\n'));
    TEST_FALSE(str_is_space('a'));

    TEST_TRUE(str_is_eol('\n'));
    TEST_TRUE(str_is_eol('\r'));
    TEST_FALSE(str_is_eol(' '));

    TEST_TRUE(is_whitespace(' '));
    TEST_TRUE(is_whitespace('\t'));
    TEST_FALSE(is_whitespace('\n'));

    char line_buffer[32] = {};
    int32 line_length = 0;

    str_line_append(line_buffer, &line_length, 32, "abc");
    TEST_EQUALS(line_length, 3);
    TEST_TRUE(strcmp(line_buffer, "abc") == 0);

    str_line_append(line_buffer, &line_length, 32, "def");
    TEST_EQUALS(line_length, 7);
    TEST_TRUE(strcmp(line_buffer, "abc\ndef") == 0);

    char small_line_buffer[8] = {};
    int32 small_line_length = 0;
    str_line_append(small_line_buffer, &small_line_length, 8, "abcdefghij");
    TEST_EQUALS(small_line_length, 7);
    TEST_TRUE(strcmp(small_line_buffer, "abcdefg") == 0);
}

#ifdef UBER_TEST
    #ifdef main
        #undef main
    #endif
    #define main UtilsStringUtilsTest
#endif

int main() {
    TEST_INIT(512);

    TEST_RUN(test_utf8_encode);
    TEST_RUN(test_utf8_decode);
    TEST_RUN(test_utf8_strlen);
    TEST_RUN(test_str_is_float);
    TEST_RUN(test_str_is_integer);
    TEST_RUN(test_sprintf_fast);
    TEST_RUN(test_str_is_alpha);
    TEST_RUN(test_str_is_num);
    TEST_RUN(test_str_is_alphanum);
    TEST_RUN(test_str_to_float);
    TEST_RUN(test_str_move_past);
    TEST_RUN(test_str_move_to);
    TEST_RUN(test_str_move_to_pos);
    TEST_RUN(test_strlen);
    TEST_RUN(test_str_contains);
    TEST_RUN(test_strcmp);
    TEST_RUN(test_str_is_empty);
    TEST_RUN(test_str_is_eol);

    // Extended coverage of the current StringUtils.h surface
    TEST_RUN(test_str_to_int_and_int_length);
    TEST_RUN(test_int_to_str_signed_unsigned);
    TEST_RUN(test_int_to_str_wchar_and_thousands);
    TEST_RUN(test_str_length_and_str_right);
    TEST_RUN(test_str_copy_and_copy_until);
    TEST_RUN(test_str_append_concat_insert_remove);
    TEST_RUN(test_str_count_and_contains);
    TEST_RUN(test_str_compare_caseless_and_ends_with);
    TEST_RUN(test_str_toupper_tolower_and_pad);
    TEST_RUN(test_hex_helpers);
    TEST_RUN(test_str_to_float_and_float_to_str);
    TEST_RUN(test_format_time_helpers);
    TEST_RUN(test_sprintf_fast_and_iter);
    TEST_RUN(test_utf8_and_wchar_conversions);
    TEST_RUN(test_str_line_and_skip_helpers);

    // Wchar functions
    TEST_RUN(test_str_length_wchar);
    TEST_RUN(test_str_contains_wchar);
    TEST_RUN(test_strcmp_wchar);

    #if PERFORMANCE_TEST
        TEST_RUN(test_str_length_performance);
        TEST_RUN(test_str_is_alphanum_performance);
        TEST_RUN(test_sprintf_fast_performance);
    #endif

    TEST_FINALIZE();

    return 0;
}