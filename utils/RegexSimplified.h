/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_UTILS_REGEX_SIMPLIFIED_H
#define COMS_UTILS_REGEX_SIMPLIFIED_H

#include "../stdlib/Types.h"
#include "StringUtils.h"

struct SimplifiedRegexParser {
    const char *pattern;
    int32 pos;
};

struct MatchResult {
    bool matched;
    int32 length;
};

static
void regex_skip_whitespace(SimplifiedRegexParser *parser) {
    while (parser->pattern[parser->pos] == ' ') {
        parser->pos++;
    }
}

MatchResult regex_match_char(SimplifiedRegexParser *parser, const char *text) {
    regex_skip_whitespace(parser);
    MatchResult result = {false, 0};

    if (parser->pattern[parser->pos] == '\0' || *text == '\0') {
        return result;
    }

    if (parser->pattern[parser->pos] == '\\') {
        // Handle escape sequences
        parser->pos++;
        if (parser->pattern[parser->pos] == 'd') {
            if (str_is_num(*text)) {
                result.matched = true;
                result.length = 1;
                parser->pos++;
            }
        } else if (parser->pattern[parser->pos] == *text) {
            result.matched = true;
            result.length = 1;
            parser->pos++;
        }
    } else if (parser->pattern[parser->pos] == '.') {
        // Match any character
        result.matched = true;
        result.length = 1;
        parser->pos++;
    } else if (parser->pattern[parser->pos] == *text) {
        // Match literal character
        result.matched = true;
        result.length = 1;
        parser->pos++;
    } else if (parser->pattern[parser->pos] == 'a' &&
               parser->pattern[parser->pos+1] == '-' &&
               parser->pattern[parser->pos+2] == 'z') {
        // Match a-z range
        if (*text >= 'a' && *text <= 'z') {
            result.matched = true;
            result.length = 1;
            parser->pos += 3;
        }
    } else if (parser->pattern[parser->pos] == 'A' &&
               parser->pattern[parser->pos+1] == '-' &&
               parser->pattern[parser->pos+2] == 'Z') {
        // Match A-Z range
        if (*text >= 'A' && *text <= 'Z') {
            result.matched = true;
            result.length = 1;
            parser->pos += 3;
        }
    } else if (parser->pattern[parser->pos] == '0' &&
               parser->pattern[parser->pos+1] == '-' &&
               parser->pattern[parser->pos+2] == '9') {
        // Match 0-9 range
        if (str_is_num(*text)) {
            result.matched = true;
            result.length = 1;
            parser->pos += 3;
        }
    }

    return result;
}

int regex_parse_number(SimplifiedRegexParser *parser) {
    int32 num = 0;
    while (str_is_num(parser->pattern[parser->pos])) {
        num = num * 10 + (parser->pattern[parser->pos] - '0');
        parser->pos++;
    }
    return num;
}

MatchResult regex_match_pattern(SimplifiedRegexParser *parser, const char *text);
MatchResult regex_match_atom(SimplifiedRegexParser *parser, const char *text) {
    regex_skip_whitespace(parser);
    MatchResult result = {false, 0};

    if (parser->pattern[parser->pos] == '(') {
        // Handle group
        int32 saved_pos = parser->pos;
        parser->pos++;
        result = regex_match_pattern(parser, text);
        if (parser->pattern[parser->pos] == ')') {
            parser->pos++;
        } else {
            // Group not properly closed, backtrack
            parser->pos = saved_pos;
            result = regex_match_char(parser, text);
        }
    } else {
        // Handle single character
        result = regex_match_char(parser, text);
    }

    return result;
}

MatchResult regex_match_repetition(SimplifiedRegexParser *parser, const char *text, MatchResult atom_result) {
    MatchResult result = {false, 0};

    parser->pos++; // Skip '{'
    regex_skip_whitespace(parser);

    int32 min = regex_parse_number(parser);
    regex_skip_whitespace(parser);

    int32 max = min;
    if (parser->pattern[parser->pos] == ',') {
        parser->pos++;
        regex_skip_whitespace(parser);
        if (parser->pattern[parser->pos] == '}') {
            // {x,} means x or more (no max)
            max = -1;
        } else {
            max = regex_parse_number(parser);
        }
    }

    regex_skip_whitespace(parser);
    if (parser->pattern[parser->pos] != '}') {
        // Invalid repetition syntax
        return result;
    }
    parser->pos++; // Skip '}'

    if (min < 0 || (max != -1 && max < min)) {
        // Invalid range
        return result;
    }

    // Try to match exactly min times first
    int32 count = 0;
    int32 total_length = 0;
    const char *current_text = text;

    while (true) {
        if (max != -1 && count >= max) break;

        MatchResult next_result = regex_match_atom(parser, current_text);
        if (!next_result.matched) break;

        count++;
        total_length += next_result.length;
        current_text += next_result.length;
    }

    if (count >= min && (max == -1 || count <= max)) {
        result.matched = true;
        result.length = total_length;
    }

    return result;
}

MatchResult regex_match_element(SimplifiedRegexParser *parser, const char *text) {
    MatchResult atom_result = regex_match_atom(parser, text);

    if (!atom_result.matched) {
        return atom_result;
    }

    regex_skip_whitespace(parser);
    char quantifier = parser->pattern[parser->pos];

    if (quantifier == '*') {
        // Zero or more
        parser->pos++;
        int32 consumed = atom_result.length;
        const char *remaining_text = text + consumed;
        MatchResult star_result = {true, consumed};

        while (true) {
            MatchResult next_result = regex_match_atom(parser, remaining_text);
            if (!next_result.matched) break;
            consumed += next_result.length;
            remaining_text += next_result.length;
            star_result.length = consumed;
        }

        return star_result;
    } else if (quantifier == '+') {
        // One or more
        parser->pos++;
        int32 consumed = atom_result.length;
        const char *remaining_text = text + consumed;
        MatchResult plus_result = {true, consumed};

        while (true) {
            MatchResult next_result = regex_match_atom(parser, remaining_text);
            if (!next_result.matched) break;
            consumed += next_result.length;
            remaining_text += next_result.length;
            plus_result.length = consumed;
        }

        return plus_result;
    } else if (quantifier == '?') {
        // Zero or one
        parser->pos++;
        return atom_result;
    } else if (quantifier == '{') {
        // Min/max repetition {x,y}
        return regex_match_repetition(parser, text, atom_result);
    } else {
        // No quantifier
        return atom_result;
    }
}

MatchResult regex_match_pattern(SimplifiedRegexParser *parser, const char *text) {
    MatchResult result = regex_match_element(parser, text);

    regex_skip_whitespace(parser);
    if (parser->pattern[parser->pos] == '|') {
        parser->pos++;
        MatchResult alternative = regex_match_pattern(parser, text);
        if (alternative.matched) {
            return alternative;
        }
    }

    return result;
}

bool regex_simplified_validate(const char* pattern, const char* text) {
    SimplifiedRegexParser parser = {pattern, 0};
    bool starts_with = false;
    bool ends_with = false;

    // Check for ^ and $ anchors
    if (parser.pattern[parser.pos] == '^') {
        starts_with = true;
        parser.pos++;
    }

    MatchResult result = regex_match_pattern(&parser, text);

    if (parser.pattern[parser.pos] == '$') {
        ends_with = true;
        parser.pos++;
    }

    // Check if we consumed the entire pattern
    if (parser.pattern[parser.pos] != '\0') {
        return false;
    }

    // Check anchors
    if (starts_with && ends_with) {
        return result.matched && (result.length == str_length(text));
    } else if (starts_with) {
        return result.matched && (result.length > 0);
    } else if (ends_with) {
        return result.matched && (text[result.length] == '\0');
    } else {
        return result.matched;
    }
}

/*
// Test function
void test_regex(const char *pattern, const char *text, bool expected) {
    bool result = regex_simplified_validate(pattern, text);
    printf("Pattern: '%-10s'\tText: '%-6s'\tExpected: %-5s\tActual: %-5s\t%s\n",
           pattern, text, expected ? "true" : "false", result ? "true" : "false",
           (result == expected) ? "✓" : "✗");
}

int main() {
    // Test cases
    printf("Enhanced Regex Validator Tests\n");
    printf("=============================\n");

    // Basic tests
    test_regex("abc", "abc", true);
    test_regex("^abc$", "abc", true);
    test_regex("^abc$", "abcd", false);

    // Character classes
    test_regex("a-z", "a", true);
    test_regex("a-z", "z", true);
    test_regex("a-z", "A", false);
    test_regex("A-Z", "Z", true);
    test_regex("A-Z", "a", false);
    test_regex("0-9", "5", true);
    test_regex("0-9", "a", false);
    test_regex("\\d", "5", true);
    test_regex("\\d", "a", false);

    // Quantifiers
    test_regex("a*", "", true);
    test_regex("a*", "a", true);
    test_regex("a*", "aaa", true);
    test_regex("a+", "", false);
    test_regex("a+", "a", true);
    test_regex("a+", "aaa", true);
    test_regex("a?b", "b", true);
    test_regex("a?b", "ab", true);
    test_regex("a?b", "aab", false);

    // Groups and alternation
    test_regex("(a|b)c", "ac", true);
    test_regex("(a|b)c", "bc", true);
    test_regex("(a|b)c", "cc", false);
    test_regex("(a-z)+", "abc", true);
    test_regex("(A-Z)+", "ABC", true);
    test_regex("(0-9)+", "123", true);

    // Escape sequences
    test_regex("\\.", ".", true);
    test_regex("\\.", "a", false);
    test_regex("a\\db", "a0b", true);
    test_regex("a\\db", "a9b", true);
    test_regex("a\\db", "aab", false);

    // Any character
    test_regex("a.b", "a b", true);
    test_regex("a.b", "a0b", true);
    test_regex("a.b", "a\nb", true);
    test_regex("a.b", "ab", false);

    // Repetition tests
    test_regex("a{2}", "aa", true);
    test_regex("a{2}", "a", false);
    test_regex("a{2}", "aaa", true);  // More than min is allowed
    test_regex("a{2,4}", "aa", true);
    test_regex("a{2,4}", "aaa", true);
    test_regex("a{2,4}", "aaaa", true);
    test_regex("a{2,4}", "a", false);
    test_regex("a{2,4}", "aaaaa", false);
    test_regex("a{2,}", "aa", true);
    test_regex("a{2,}", "aaaaa", true);
    test_regex("a{2,}", "a", false);
    test_regex("(a-z){3}", "abc", true);
    test_regex("(a-z){3}", "ab", false);
    test_regex("(a-z){2,4}", "ab", true);
    test_regex("(a-z){2,4}", "abcd", true);
    test_regex("(a-z){2,4}", "abcde", false);
    test_regex("\\d{3}-\\d{2}", "123-45", true);
    test_regex("\\d{3}-\\d{2}", "12-345", false);

    // Combined tests
    test_regex("^a{2}b{1,3}c$", "aabbc", true);
    test_regex("^a{2}b{1,3}c$", "aabbbc", true);
    test_regex("^a{2}b{1,3}c$", "aabc", true);
    test_regex("^a{2}b{1,3}c$", "aabbbbc", false);
    test_regex("^a{2}b{1,3}c$", "abbc", false);

    return 0;
}
*/

#endif