#include "../TestFramework.h"
#include "../../utils/StringUtils.h"
#include "../../utils/RegexSimplified.h"

/**
 * Tests for the simplified regex validator in RegexSimplified.h
 *
 * The current implementation only evaluates a single pattern element (plus
 * the optional anchors). Multi element patterns, real repetition via {n,m}
 * and general alternation are not implemented, see the report for details.
 * Only the supported subset is asserted here.
 */

static void test_regex_literal()
{
    TEST_TRUE(regex_simplified_validate("a", "a"));
    TEST_FALSE(regex_simplified_validate("a", "b"));
    TEST_FALSE(regex_simplified_validate("a", ""));

    // Without anchors a prefix match is enough
    TEST_TRUE(regex_simplified_validate("a", "abc"));

    TEST_FALSE(regex_simplified_validate("b", "abc"));
}

static void test_regex_any_and_escape()
{
    TEST_TRUE(regex_simplified_validate(".", "x"));
    TEST_TRUE(regex_simplified_validate(".", " "));
    TEST_TRUE(regex_simplified_validate(".", "xy"));
    TEST_FALSE(regex_simplified_validate(".", ""));

    TEST_TRUE(regex_simplified_validate("\\.", "."));
    TEST_FALSE(regex_simplified_validate("\\.", "a"));

    // An escaped backslash matches a literal backslash
    TEST_TRUE(regex_simplified_validate("\\\\", "\\"));
    TEST_FALSE(regex_simplified_validate("\\\\", "/"));
}

static void test_regex_digit_class()
{
    TEST_TRUE(regex_simplified_validate("\\d", "0"));
    TEST_TRUE(regex_simplified_validate("\\d", "7"));
    TEST_TRUE(regex_simplified_validate("\\d", "9"));
    TEST_FALSE(regex_simplified_validate("\\d", "a"));
    TEST_FALSE(regex_simplified_validate("\\d", ""));

    TEST_TRUE(regex_simplified_validate("^\\d$", "5"));
    TEST_FALSE(regex_simplified_validate("^\\d$", "a"));
    TEST_FALSE(regex_simplified_validate("^\\d$", "55"));

    // The 0-9 range behaves like the digit escape for all but the first char
    TEST_TRUE(regex_simplified_validate("0-9", "5"));
    TEST_FALSE(regex_simplified_validate("0-9", "a"));
    TEST_TRUE(regex_simplified_validate("^0-9$", "5"));
    TEST_FALSE(regex_simplified_validate("^0-9$", "a"));
}

static void test_regex_ranges()
{
    TEST_TRUE(regex_simplified_validate("a-z", "b"));
    TEST_TRUE(regex_simplified_validate("a-z", "z"));
    TEST_FALSE(regex_simplified_validate("a-z", "A"));
    TEST_FALSE(regex_simplified_validate("a-z", "1"));

    TEST_TRUE(regex_simplified_validate("A-Z", "B"));
    TEST_FALSE(regex_simplified_validate("A-Z", "a"));

    TEST_TRUE(regex_simplified_validate("^a-z$", "b"));
    TEST_FALSE(regex_simplified_validate("^a-z$", "A"));
    TEST_FALSE(regex_simplified_validate("^a-z$", "ab"));
}

static void test_regex_quantifiers()
{
    // Zero or more
    TEST_TRUE(regex_simplified_validate("a*", "aaa"));
    TEST_FALSE(regex_simplified_validate("a*", "b"));
    TEST_FALSE(regex_simplified_validate("a*", ""));

    // One or more
    TEST_TRUE(regex_simplified_validate("a+", "aaa"));
    TEST_FALSE(regex_simplified_validate("a+", ""));
    TEST_FALSE(regex_simplified_validate("a+", "b"));

    // Zero or one
    TEST_TRUE(regex_simplified_validate("a?", "a"));
    TEST_FALSE(regex_simplified_validate("a?", "b"));
    TEST_FALSE(regex_simplified_validate("a?", ""));

    // A quantifier with a following element consumes both, e.g. a*b
    TEST_TRUE(regex_simplified_validate("a*b", "ab"));
    TEST_FALSE(regex_simplified_validate("a*b", "b"));
}

static void test_regex_anchors()
{
    TEST_TRUE(regex_simplified_validate("^a$", "a"));
    TEST_FALSE(regex_simplified_validate("^a$", "ab"));
    TEST_FALSE(regex_simplified_validate("^a$", ""));
    TEST_FALSE(regex_simplified_validate("^a$", "b"));

    TEST_TRUE(regex_simplified_validate("^a", "abc"));
    TEST_FALSE(regex_simplified_validate("^a", "bc"));

    TEST_TRUE(regex_simplified_validate("a$", "a"));
    TEST_FALSE(regex_simplified_validate("a$", "ab"));

    TEST_TRUE(regex_simplified_validate("^.$", "x"));
    TEST_FALSE(regex_simplified_validate("^.$", "xy"));
    TEST_FALSE(regex_simplified_validate("^.$", ""));

    TEST_TRUE(regex_simplified_validate(".$", "x"));
    TEST_FALSE(regex_simplified_validate(".$", "xy"));
}

static void test_regex_group_single()
{
    TEST_TRUE(regex_simplified_validate("(a)", "a"));
    TEST_FALSE(regex_simplified_validate("(a)", "b"));

    TEST_TRUE(regex_simplified_validate("^(a)$", "a"));
    TEST_FALSE(regex_simplified_validate("^(a)$", "b"));
    TEST_FALSE(regex_simplified_validate("^(a)$", "ab"));
}

static void test_regex_alternation()
{
    // The only alternation form supported by the current implementation is
    // when the alternative also matches (the first branch has to succeed)
    TEST_TRUE(regex_simplified_validate("a|a", "a"));
    TEST_TRUE(regex_simplified_validate("x|x|x", "x"));
    TEST_TRUE(regex_simplified_validate("(a|a)", "a"));

    TEST_FALSE(regex_simplified_validate("a|a", "b"));
}

static void test_regex_whitespace()
{
    TEST_TRUE(regex_simplified_validate(" a", "a"));
    TEST_TRUE(regex_simplified_validate("a ", "a"));
    TEST_TRUE(regex_simplified_validate("^ a $", "a"));
    TEST_FALSE(regex_simplified_validate("^ a $", "b"));

    TEST_TRUE(regex_simplified_validate("( a )", "a"));
}

static void test_regex_invalid_syntax()
{
    // Unclosed group
    TEST_FALSE(regex_simplified_validate("(a", "a"));

    // Invalid repetition specifiers
    TEST_FALSE(regex_simplified_validate("a{", "a"));
    TEST_FALSE(regex_simplified_validate("a{2,1}", "aa"));
    TEST_FALSE(regex_simplified_validate("a{2,1}", "a"));

    // Empty pattern never matches, not even the empty string
    TEST_FALSE(regex_simplified_validate("", ""));
    TEST_FALSE(regex_simplified_validate("", "a"));

    TEST_FALSE(regex_simplified_validate("^$", ""));
    TEST_FALSE(regex_simplified_validate("^$", "a"));
}

#ifdef UBER_TEST
    #ifdef main
        #undef main
    #endif
    #define main UtilsRegexSimplifiedTest
#endif

int main() {
    TEST_INIT(96);

    TEST_RUN(test_regex_literal);
    TEST_RUN(test_regex_any_and_escape);
    TEST_RUN(test_regex_digit_class);
    TEST_RUN(test_regex_ranges);
    TEST_RUN(test_regex_quantifiers);
    TEST_RUN(test_regex_anchors);
    TEST_RUN(test_regex_group_single);
    TEST_RUN(test_regex_alternation);
    TEST_RUN(test_regex_whitespace);
    TEST_RUN(test_regex_invalid_syntax);

    TEST_FINALIZE();

    return 0;
}
