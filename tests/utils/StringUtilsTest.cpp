#include <stdio.h>
#include <ctype.h>
#include "../TestFramework.h"
#include "../../utils/StringUtils.h"
#include "../../utils/EndianUtils.h"

static void test_utf8_encode()
{
    char out[5] = {};
    ASSERT_EQUALS(utf8_encode(659, out), 2);
    ASSERT_EQUALS(utf8_encode(67857, out), 4);
}

static void test_utf8_decode()
{
    char in[5] = {};
    uint32 codepoint;

    *((uint16 *) in) = SWAP_ENDIAN_BIG((uint16) 0xCA93);
    ASSERT_EQUALS(utf8_decode(in, &codepoint), 2);
    ASSERT_EQUALS(codepoint, 659);

    *((uint32 *) in) = SWAP_ENDIAN_BIG(0xF090A491);
    ASSERT_EQUALS(utf8_decode(in, &codepoint), 4);
    ASSERT_EQUALS(codepoint, 67857);

    char out[5] = {};
    ASSERT_EQUALS(utf8_decode(codepoint, out), 4);
    ASSERT_TRUE(strcmp(out, in) == 0);
}

static void test_utf8_str_length()
{
    char in[] = "Foo © bar 𝌆 baz ☃ qux";
    ASSERT_EQUALS(utf8_str_length(in), 21);
}

static void test_str_is_float()
{
    ASSERT_TRUE(str_is_float("1.234"));
    ASSERT_TRUE(str_is_float("-1.234"));
    ASSERT_TRUE(str_is_float("+1.234"));
    ASSERT_FALSE(str_is_float("1.234ABC"));
    ASSERT_FALSE(str_is_float("B1.234"));
    ASSERT_FALSE(str_is_float("1.2.34"));
}

static void test_str_is_integer()
{
    ASSERT_TRUE(str_is_integer("1234"));
    ASSERT_TRUE(str_is_integer("-1234"));
    ASSERT_TRUE(str_is_integer("+1234"));
    ASSERT_FALSE(str_is_integer("1.234"));
    ASSERT_FALSE(str_is_integer("1.234ABC"));
    ASSERT_FALSE(str_is_integer("B1.234"));
    ASSERT_FALSE(str_is_integer("1.2.34"));
}

static void test_str_is_alpha()
{
    ASSERT_FALSE(str_is_alpha('2'));
    ASSERT_TRUE(str_is_alpha('s'));
    ASSERT_TRUE(str_is_alpha('D'));
    ASSERT_FALSE(str_is_alpha('-'));
}

static void test_str_is_num()
{
    ASSERT_TRUE(str_is_num('2'));
    ASSERT_FALSE(str_is_num('s'));
    ASSERT_FALSE(str_is_num('D'));
    ASSERT_FALSE(str_is_num('-'));
}

static void test_str_is_alphanum()
{
    ASSERT_TRUE(str_is_alphanum('2'));
    ASSERT_TRUE(str_is_alphanum('s'));
    ASSERT_TRUE(str_is_alphanum('D'));
    ASSERT_FALSE(str_is_alphanum('-'));
}

static void test_str_length()
{
    ASSERT_EQUALS(str_length("2asdf dw"), 8);
}

#if PERFORMANCE_TEST
static void _str_length(volatile void* val) {
    volatile int64* res = (volatile int64 *) val;

    char buffer[32];
    memcpy(buffer, "This %d is a %s with %f values", sizeof("This %d is a %s with %f values"));
    buffer[30] = (byte) *res;

    *res += (int64) str_length(buffer);
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
    ASSERT_TRUE(strcmp(buffer, "This 1337 is a test with 3.00000 values") == 0);
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
    ASSERT_EQUALS(str_to_float("1.000000"), 1.0f);
    ASSERT_EQUALS(str_to_float("+1.000000"), 1.0f);
    ASSERT_EQUALS(str_to_float("-1.000000"), -1.0f);
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
    TEST_RUN(test_str_length);
    TEST_RUN(test_str_to_float);

    #if PERFORMANCE_TEST
        TEST_RUN(test_str_length_performance);
        TEST_RUN(test_str_is_alphanum_performance);
        TEST_RUN(test_sprintf_fast_performance);
    #endif

    TEST_FINALIZE();

    return 0;
}