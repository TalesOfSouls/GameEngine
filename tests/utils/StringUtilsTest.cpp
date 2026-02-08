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
    char in[] = "Foo © bar 𝌆 baz ☃ qux";
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
    TEST_FALSE(str_is_alpha('2'));
    TEST_TRUE(str_is_alpha('s'));
    TEST_TRUE(str_is_alpha('D'));
    TEST_FALSE(str_is_alpha('-'));
}

static void test_str_is_num()
{
    TEST_TRUE(str_is_num('2'));
    TEST_FALSE(str_is_num('s'));
    TEST_FALSE(str_is_num('D'));
    TEST_FALSE(str_is_num('-'));
}

static void test_str_is_alphanum()
{
    TEST_TRUE(str_is_alphanum('2'));
    TEST_TRUE(str_is_alphanum('s'));
    TEST_TRUE(str_is_alphanum('D'));
    TEST_FALSE(str_is_alphanum('-'));
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
    TEST_EQUALS(strlen(L"2asdf dw"), 8);
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
    TEST_NOT_EQUALS(strcmp("2asdf dw", "2asdf"), 0);
}

static void test_strcmp_wchar()
{
    TEST_EQUALS(strcmp(L"2asdf dw", L"2asdf dw"), 0);
    TEST_NOT_EQUALS(strcmp(L"2asdf dw", L"2asdf"), 0);
}

#if PERFORMANCE_TEST
static void _strlen(volatile void* val) {
    volatile int64* res = (volatile int64 *) val;

    char buffer[32];
    memcpy(buffer, "This %d is a %s with %f values", sizeof("This %d is a %s with %f values"));
    buffer[30] = (byte) *res;

    *res += (int64) strlen(buffer);
}

static void _strlen(volatile void* val) {
    volatile int64* res = (volatile int64 *) val;

    char buffer[32];
    memcpy(buffer, "This %d is a %s with %f values", sizeof("This %d is a %s with %f values"));
    buffer[30] = (byte) *res;

    *res += (int64) strlen(buffer);
}

static void test_str_length_performance() {
    COMPARE_FUNCTION_TEST_TIME(_str_length, _strlen, 5.0);
    COMPARE_FUNCTION_TEST_CYCLE(_str_length, _strlen, 5.0);
}
#endif

#if PERFORMANCE_TEST
static void _str_is_alphanum(volatile void* val) {
    bool* res = (bool *) val;
    srand(0);

    int32 a = 0;
    for (int32 i = 0; i < 1000; ++i) {
        a += str_is_alphanum((byte) rand());
    }

    *res |= (bool) a;
}

static void _isalnum(volatile void* val) {
    bool* res = (bool *) val;
    srand(0);

    int32 a = 0;
    for (int32 i = 0; i < 1000; ++i) {
        a += isalnum((byte) rand());
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

#ifdef UBER_TEST
    #ifdef main
        #undef main
    #endif
    #define main UtilsStringUtilsTest
#endif

int main() {
    TEST_INIT(100);

    TEST_RUN(test_utf8_encode);
    TEST_RUN(test_utf8_decode);
    TEST_RUN(test_utf8_str_length);
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
    TEST_RUN(test_str_length);
    TEST_RUN(test_str_contains);
    TEST_RUN(test_strcmp);
    TEST_RUN(test_str_is_empty);
    TEST_RUN(test_str_is_eol);

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