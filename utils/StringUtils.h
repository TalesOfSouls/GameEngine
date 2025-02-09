/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef TOS_UTILS_STRING_UTILS_H
#define TOS_UTILS_STRING_UTILS_H

#include <string.h>
#include "../stdlib/Types.h"
#include "../utils/TestUtils.h"

#define HAS_ZERO(x) (((x) - ((size_t)-1 / 0xFF)) & ~(x) & (((size_t)-1 / 0xFF) * (0xFF / 2 + 1)))
#define HAS_CHAR(x, c) (HAS_ZERO((x) ^ (((size_t)-1 / 0xFF) * (c))))

inline constexpr
size_t str_length(const char* str)
{
    const char* ptr = str;

    // Align the pointer to the size of size_t
    for (; (uintptr_t) ptr % sizeof(size_t) != 0; ++ptr) {
        if (*ptr == '\0') {
            return ptr - str;
        }
    }

    // Check one longword (size_t) at a time
    const size_t* longword_ptr = (const size_t *) ptr;
    while (true) {
        size_t longword = *longword_ptr++;
        if (HAS_ZERO(longword)) {
            const char* cp = (const char *) (longword_ptr - 1);
            if (cp[0] == '\0') return cp - str;
            if (cp[1] == '\0') return cp + 1 - str;
            if (cp[2] == '\0') return cp + 2 - str;
            if (cp[3] == '\0') return cp + 3 - str;

            // Are we using 8bytes for size_t?
            #if SIZE_MAX > 0xFFFFFFFF
                if (cp[4] == '\0') return cp + 4 - str;
                if (cp[5] == '\0') return cp + 5 - str;
                if (cp[6] == '\0') return cp + 6 - str;
                if (cp[7] == '\0') return cp + 7 - str;
            #endif
        }
    }
}

const char* str_find(const char* str, const char* needle) {
    size_t needle_len = str_length(needle);
    size_t str_len = str_length(str);
    size_t limit = str_len - needle_len + 1;

    for (size_t i = 0; i < limit; ++i) {
        if (str[i] == needle[0] && memcmp(&str[i + 1], &needle[1], needle_len - 1) == 0) {
            return &str[i];
        }
    }

    return NULL;
}

const char* str_find(const char* str, char needle) {
    byte target = (byte) needle;

    // Process byte-by-byte until alignment is achieved
    for (; (uintptr_t) str % sizeof(size_t) != 0; ++str) {
        if (*str == target) {
            return str;
        }

        if (*str == '\0') {
            return NULL;
        }
    }

    // Broadcast the target character to all bytes of a word
    size_t target_word = target;
    for (size_t i = 1; i < sizeof(size_t); ++i) {
        target_word |= target_word << 8;
    }

    const size_t* word_ptr = (const size_t *) str;
    while (true) {
        size_t word = *word_ptr++;
        if (HAS_CHAR(word, target)) {
            const char* byte_ptr = (const char *) (word_ptr - 1);
            for (size_t i = 0; i < sizeof(size_t); ++i) {
                if (byte_ptr[i] == target) {
                    return byte_ptr + i;
                }
            }
        }

        if (HAS_ZERO(word)) {
            const char* byte_ptr = (const char *) (word_ptr - 1);
            for (size_t i = 0; i < sizeof(size_t); ++i) {
                if (byte_ptr[i] == '\0') {
                    return NULL;
                }
            }
        }
    }
}

inline
int32 utf8_encode(uint32 codepoint, char* out)
{
    if (codepoint <= 0x7F) {
        // 1-byte sequence: 0xxxxxxx
        out[0] = (byte) codepoint;

        return 1;
    } else if (codepoint <= 0x7FF) {
        // 2-byte sequence: 110xxxxx 10xxxxxx
        out[0] = 0xC0 | ((codepoint >> 6) & 0x1F);
        out[1] = 0x80 | (codepoint & 0x3F);

        return 2;
    } else if (codepoint <= 0xFFFF) {
        // 3-byte sequence: 1110xxxx 10xxxxxx 10xxxxxx
        out[0] = 0xE0 | ((codepoint >> 12) & 0x0F);
        out[1] = 0x80 | ((codepoint >> 6) & 0x3F);
        out[2] = 0x80 | (codepoint & 0x3F);

        return 3;
    } else if (codepoint <= 0x10FFFF) {
        // 4-byte sequence: 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
        out[0] = 0xF0 | ((codepoint >> 18) & 0x07);
        out[1] = 0x80 | ((codepoint >> 12) & 0x3F);
        out[2] = 0x80 | ((codepoint >> 6) & 0x3F);
        out[3] = 0x80 | (codepoint & 0x3F);

        return 4;
    }

    return -1;
}

inline
int32 utf8_decode(const char* __restrict in, uint32* __restrict codepoint) {
    byte ch = (byte) *in;

    if (ch <= 0x7F) {
        // 1-byte sequence (ASCII)
        *codepoint = ch;

        return 1;
    } else if ((ch & 0xE0) == 0xC0) {
        // 2-byte sequence
        *codepoint = ((ch & 0x1F) << 6) | (in[1] & 0x3F);

        return 2;
    } else if ((ch & 0xF0) == 0xE0) {
        // 3-byte sequence
        *codepoint = ((ch & 0x0F) << 12) | ((in[1] & 0x3F) << 6) | (in[2] & 0x3F);

        return 3;
    } else if ((ch & 0xF8) == 0xF0) {
        // 4-byte sequence
        *codepoint = ((ch & 0x07) << 18) | ((in[1] & 0x3F) << 12) | ((in[2] & 0x3F) << 6) | (in[3] & 0x3F);

        return 4;
    }

    return -1;
}

inline
int32 utf8_decode(const uint32 codepoint, char* __restrict out) {
    if (codepoint <= 0x7F) {
        // 1-byte sequence (ASCII)
        out[0] = (char) codepoint;

        return 1;
    } else if (codepoint <= 0x7FF) {
        // 2-byte sequence
        out[0] = (char) (0xC0 | ((codepoint >> 6) & 0x1F));
        out[1] = (char) (0x80 | (codepoint & 0x3F));

        return 2;
    } else if (codepoint <= 0xFFFF) {
        // 3-byte sequence
        out[0] = (char) (0xE0 | ((codepoint >> 12) & 0x0F));
        out[1] = (char) (0x80 | ((codepoint >> 6) & 0x3F));
        out[2] = (char) (0x80 | (codepoint & 0x3F));

        return 3;
    } else if (codepoint <= 0x10FFFF) {
        // 4-byte sequence
        out[0] = (char) (0xF0 | ((codepoint >> 18) & 0x07));
        out[1] = (char) (0x80 | ((codepoint >> 12) & 0x3F));
        out[2] = (char) (0x80 | ((codepoint >> 6) & 0x3F));
        out[3] = (char) (0x80 | (codepoint & 0x3F));

        return 4;
    }

    return -1;
}

inline
int32 utf8_str_length(const char* in) {
    int32 length = 0;
    int32 bytes;
    uint32 codepoint;

    while (*in) {
        bytes = utf8_decode(in, &codepoint);
        if (bytes < 0) {
            return -1;
        }

        in += bytes;
        ++length;
    }

    return length;
}

inline
void string_to_utf8(const uint32* in, char* out) {
    char buffer[5] = {0};
    while (*in) {
        int32 len = utf8_encode(*in, buffer);
        if (len > 0) {
            strncat(out, buffer, len);
        }

        ++in;
    }
}

inline
int32 utf8_get_char_at(const char* in, int32 index) {
    int32 i = 0;
    int32 bytes_consumed;
    uint32 codepoint;

    while (*in) {
        bytes_consumed = utf8_decode(in, &codepoint);
        if (bytes_consumed < 0) {
            return -1;
        }

        if (i == index) {
            return codepoint;
        }

        ++i;
        in += bytes_consumed;
    }

    return -1;
}

inline
void wchar_to_char(wchar_t* str)
{
    char* src = (char*) str;
    char* dest = src;

    while (*src != '\0' && src[1] != '\0') {
        if (*src != '\0') {
            *dest++ = *src;
        }

        ++src;
    }

    *dest = '\0';
}

inline
void wchar_to_char(const char* __restrict str, char* __restrict dest)
{
    while (*str != '\0' && str[1] != '\0') {
        if (*str != '\0') {
            *dest++ = (char) *str;
        }

        ++str;
    }

    *dest = '\0';
}

static const bool STR_IS_ALPHA_LOOKUP_TABLE[] = {
    false, false, false, false, false, false, false, false, // 0-7
    false, false, false, false, false, false, false, false, // 8-15
    false, false, false, false, false, false, false, false, // 16-23
    false, false, false, false, false, false, false, false, // 24-31
    false, false, false, false, false, false, false, false, // 32-39
    false, false, false, false, false, false, false, false, // 40-47
    false, false, false, false, false, false, false, false, // 48-55 ('0'-'7')
    false, false, false, false, false, false, false, false, // 56-63 ('8'-'9', others)
    false, true,  true,  true,  true,  true,  true,  true,  // 64-71 ('A'-'G')
    true,  true,  true,  true,  true,  true,  true,  true,  // 72-79 ('H'-'O')
    true,  true,  true,  true,  true,  true,  true,  true,  // 80-87 ('P'-'W')
    true,  true,  true, false, false, false, false, false, // 88-95 ('X'-'Z', others)
    false, true,  true,  true,  true,  true,  true,  true,  // 96-103 ('a'-'g')
    true,  true,  true,  true,  true,  true,  true,  true,  // 104-111 ('h'-'o')
    true,  true,  true,  true,  true,  true,  true,  true,  // 112-119 ('p'-'w')
    true,  true,  true, false, false, false, false, false, // 120-127 ('x'-'z', others)
    false, false, false, false, false, false, false, false, // 128-135
    false, false, false, false, false, false, false, false, // 136-143
    false, false, false, false, false, false, false, false, // 144-151
    false, false, false, false, false, false, false, false, // 152-159
    false, false, false, false, false, false, false, false, // 160-167
    false, false, false, false, false, false, false, false, // 168-175
    false, false, false, false, false, false, false, false, // 176-183
    false, false, false, false, false, false, false, false, // 184-191
    false, false, false, false, false, false, false, false, // 192-199
    false, false, false, false, false, false, false, false, // 200-207
    false, false, false, false, false, false, false, false, // 208-215
    false, false, false, false, false, false, false, false, // 216-223
    false, false, false, false, false, false, false, false, // 224-231
    false, false, false, false, false, false, false, false, // 232-239
    false, false, false, false, false, false, false, false, // 240-247
    false, false, false, false, false, false, false, false  // 248-255
};

inline constexpr
bool str_is_alpha(char str) {
    return STR_IS_ALPHA_LOOKUP_TABLE[(byte) str];
}

inline constexpr
bool str_is_alpha(const char* str) {
    while (*str != '\0') {
        if (!str_is_alpha(*str++)) {
            return false;
        }
    }

    return true;
}

static const bool STR_IS_NUM_LOOKUP_TABLE[] = {
    false, false, false, false, false, false, false, false, // 0-7
    false, false, false, false, false, false, false, false, // 8-15
    false, false, false, false, false, false, false, false, // 16-23
    false, false, false, false, false, false, false, false, // 24-31
    false, false, false, false, false, false, false, false, // 32-39
    false, false, false, false, false, false, false, false, // 40-47
    true,  true,  true,  true,  true,  true,  true,  true,  // 48-55 ('0'-'7')
    true,  true,  false, false, false, false, false, false, // 56-63 ('8'-'9', others)
    false, true,  false, false, false, false, false, false,  // 64-71 ('A'-'G')
    false, false, false, false, false, false, false, false,  // 72-79 ('H'-'O')
    false, false, false, false, false, false, false, false,  // 80-87 ('P'-'W')
    false, false, false, false, false, false, false, false, // 88-95 ('X'-'Z', others)
    false, false, false, false, false, false, false, false,  // 96-103 ('a'-'g')
    false, false, false, false, false, false, false, false,  // 104-111 ('h'-'o')
    false, false, false, false, false, false, false, false,  // 112-119 ('p'-'w')
    false, false, false, false, false, false, false, false, // 120-127 ('x'-'z', others)
    false, false, false, false, false, false, false, false, // 128-135
    false, false, false, false, false, false, false, false, // 136-143
    false, false, false, false, false, false, false, false, // 144-151
    false, false, false, false, false, false, false, false, // 152-159
    false, false, false, false, false, false, false, false, // 160-167
    false, false, false, false, false, false, false, false, // 168-175
    false, false, false, false, false, false, false, false, // 176-183
    false, false, false, false, false, false, false, false, // 184-191
    false, false, false, false, false, false, false, false, // 192-199
    false, false, false, false, false, false, false, false, // 200-207
    false, false, false, false, false, false, false, false, // 208-215
    false, false, false, false, false, false, false, false, // 216-223
    false, false, false, false, false, false, false, false, // 224-231
    false, false, false, false, false, false, false, false, // 232-239
    false, false, false, false, false, false, false, false, // 240-247
    false, false, false, false, false, false, false, false  // 248-255
};

inline constexpr
bool str_is_num(char str) {
    return STR_IS_NUM_LOOKUP_TABLE[(byte) str];
}

static const bool STR_IS_ALPHANUM_LOOKUP_TABLE[] = {
    false, false, false, false, false, false, false, false, // 0-7
    false, false, false, false, false, false, false, false, // 8-15
    false, false, false, false, false, false, false, false, // 16-23
    false, false, false, false, false, false, false, false, // 24-31
    false, false, false, false, false, false, false, false, // 32-39
    false, false, false, false, false, false, false, false, // 40-47
    true,  true,  true,  true,  true,  true,  true,  true,  // 48-55 ('0'-'7')
    true,  true, false, false, false, false, false, false, // 56-63 ('8'-'9', others)
    false, true,  true,  true,  true,  true,  true,  true,  // 64-71 ('A'-'G')
    true,  true,  true,  true,  true,  true,  true,  true,  // 72-79 ('H'-'O')
    true,  true,  true,  true,  true,  true,  true,  true,  // 80-87 ('P'-'W')
    true,  true,  true, false, false, false, false, false, // 88-95 ('X'-'Z', others)
    false, true,  true,  true,  true,  true,  true,  true,  // 96-103 ('a'-'g')
    true,  true,  true,  true,  true,  true,  true,  true,  // 104-111 ('h'-'o')
    true,  true,  true,  true,  true,  true,  true,  true,  // 112-119 ('p'-'w')
    true,  true,  true, false, false, false, false, false, // 120-127 ('x'-'z', others)
    false, false, false, false, false, false, false, false, // 128-135
    false, false, false, false, false, false, false, false, // 136-143
    false, false, false, false, false, false, false, false, // 144-151
    false, false, false, false, false, false, false, false, // 152-159
    false, false, false, false, false, false, false, false, // 160-167
    false, false, false, false, false, false, false, false, // 168-175
    false, false, false, false, false, false, false, false, // 176-183
    false, false, false, false, false, false, false, false, // 184-191
    false, false, false, false, false, false, false, false, // 192-199
    false, false, false, false, false, false, false, false, // 200-207
    false, false, false, false, false, false, false, false, // 208-215
    false, false, false, false, false, false, false, false, // 216-223
    false, false, false, false, false, false, false, false, // 224-231
    false, false, false, false, false, false, false, false, // 232-239
    false, false, false, false, false, false, false, false, // 240-247
    false, false, false, false, false, false, false, false  // 248-255
};

inline constexpr
bool str_is_alphanum(char str) {
    return STR_IS_ALPHANUM_LOOKUP_TABLE[(byte) str];
}

inline
bool str_is_alphanum(const char* str) {
    while (*str != '\0') {
        if (!str_is_alphanum(*str++)) {
            return false;
        }
    }

    return true;
}

inline
bool str_is_float(const char* str) {
    bool has_dot = false;

    if (*str == '-' || *str == '+') {
        str++;
    }

    while (*str) {
        if (*str == '.') {
            if (has_dot) {
                return false;
            }

            has_dot = true;
        } else if (!str_is_num(*str)) {
            return false;
        }

        ++str;
    }

    return has_dot;
}

inline
bool str_is_integer(const char* str) {
    if (*str == '-' || *str == '+') { [[unlikely]]
        str++;
    }

    bool is_int = false;
    while (*str) {
        if (!str_is_num(*str)) {
            return false;
        }

        ++str;
        is_int = true;
    }

    return is_int;
}

inline constexpr
int64 str_to_int(const char* str, const char** pos = NULL)
{
    int64 sign = 1;
    if (*str == '-') {
        sign = -1;
        ++str;
    }

    int64 result = 0;
    while (str_is_num(*str)) {
        result *= 10;
        result += (*str - '0');

        ++str;
    }

    if (pos) {
        *pos = str;
    }

    return result * sign;
}

inline
int32 int_to_str(int64 number, char str[15], const char thousands)
{
    if (number == 0) {
        *str++ = '0';
        *str = '\0';

        return 1;
    }

    int32 i = 0;
    int32 digit_count = 0;
    int64 sign = number;

    if (number < 0) {
        number = -number;
    }

    while (number > 0) {
        if (digit_count && digit_count % 3 == 0) {
            str[i++] = thousands;
        }

        str[i++] = number % 10 + '0';
        number /= 10;
        ++digit_count;
    }

    if (sign < 0) {
        str[i++] = '-';
    }

    for (int32 j = 0, k = i - 1; j < k; ++j, --k) {
        char temp = str[j];
        str[j] = str[k];
        str[k] = temp;
    }

    str[i] = '\0';

    return i;
}

inline constexpr
int32 int_to_str(int64 number, char str[12]) {
    int32 i = -1;
    int64 sign = number;

    if (number < 0) {
        number = -number;
    }

    do {
        str[++i] = number % 10 + '0';
        number /= 10;
    } while (number > 0);

    if (sign < 0) {
        str[++i] = '-';
    }

    for (int32 j = 0, k = i; j < k; ++j, --k) {
        char temp = str[j];
        str[j] = str[k];
        str[k] = temp;
    }

    str[++i] = '\0';

    return i;
}

inline constexpr
int32 uint_to_str(uint64 number, char str[12]) {
    int32 i = -1;

    do {
        str[++i] = number % 10 + '0';
        number /= 10;
    } while (number > 0);

    for (int32 j = 0, k = i; j < k; ++j, --k) {
        char temp = str[j];
        str[j] = str[k];
        str[k] = temp;
    }

    str[++i] = '\0';

    return i;
}

static const char HEX_TABLE[] = {
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
    'A', 'B', 'C', 'D', 'E', 'F'
};

static const bool HEX_LOOKUP_TABLE[256] = {
    false, false, false, false, false, false, false, false, // 0-7
    false, false, false, false, false, false, false, false, // 8-15
    false, false, false, false, false, false, false, false, // 16-23
    false, false, false, false, false, false, false, false, // 24-31
    false, false, false, false, false, false, false, false, // 32-39
    false, false, false, false, false, false, false, false, // 40-47
    true,  true,  true,  true,  true,  true,  true,  true,  // 48-55 ('0'-'7')
    true,  true,  false, false, false, false, false, false, // 56-63 ('8'-'9', others)
    false, true,  true,  true,  true,  true,  true,  false,  // 64-71 ('A'-'G')
    false, false, false, false, false, false, false, false,  // 72-79 ('H'-'O')
    false, false, false, false, false, false, false, false,  // 80-87 ('P'-'W')
    false, false, false, false, false, false, false, false, // 88-95 ('X'-'Z', others)
    false, false, false, false, false, false, false, false,  // 96-103 ('a'-'g')
    false, false, false, false, false, false, false, false,  // 104-111 ('h'-'o')
    false, false, false, false, false, false, false, false,  // 112-119 ('p'-'w')
    false, false, false, false, false, false, false, false, // 120-127 ('x'-'z', others)
    false, false, false, false, false, false, false, false, // 128-135
    false, false, false, false, false, false, false, false, // 136-143
    false, false, false, false, false, false, false, false, // 144-151
    false, false, false, false, false, false, false, false, // 152-159
    false, false, false, false, false, false, false, false, // 160-167
    false, false, false, false, false, false, false, false, // 168-175
    false, false, false, false, false, false, false, false, // 176-183
    false, false, false, false, false, false, false, false, // 184-191
    false, false, false, false, false, false, false, false, // 192-199
    false, false, false, false, false, false, false, false, // 200-207
    false, false, false, false, false, false, false, false, // 208-215
    false, false, false, false, false, false, false, false, // 216-223
    false, false, false, false, false, false, false, false, // 224-231
    false, false, false, false, false, false, false, false, // 232-239
    false, false, false, false, false, false, false, false, // 240-247
    false, false, false, false, false, false, false, false  // 248-255
};

inline
bool str_is_hex_color(const char* str)
{
    if (str[0] != '#') {
        return false;
    }

    ++str;

    while (*str) {
        if (!HEX_LOOKUP_TABLE[(byte) *str]) {
            return false;
        }

        ++str;
    }

    return true;
}

inline constexpr
int32 int_to_hex(int64 number, char str[9]) {
    int32 i = -1;
    uint64 n = (uint64) number;

    do {
        byte digit = n % 16;
        str[++i] = HEX_TABLE[digit];
        n /= 16;
    } while (n > 0);

    str[++i] = '\0';

    for (int32 j = 0, k = i - 1; j < k; ++j, --k) {
        char temp = str[j];
        str[j] = str[k];
        str[k] = temp;
    }

    return i;
}

inline constexpr
int64 hex_to_int(const char* hex)
{
    int64 result = 0;
    while (HEX_LOOKUP_TABLE[(byte) *hex]) {
        byte value = *hex++;
        if (str_is_num(value)) {
            value = value - '0';
        } else if (value >= 'A' && value <='F') {
            value = value - 'A' + 10;
        } else {
            value = value - 'a' + 10;
        }

        result = (result << 4) | (value & 0xF);
    }

    return result;
}

inline
size_t str_count(const char* __restrict str, const char* __restrict substr)
{
    size_t l1 = str_length(str);
    size_t l2 = str_length(substr);

    if (l2 == 0 || l1 < l2) {
        return 0;
    }

    size_t count = 0;
    for (str = str_find(str, substr); str; str = str_find(str + l2, substr)) {
        ++count;
    }

    return count;
}

inline constexpr
int32 is_eol(const char* str)
{
    if (*str == '\n') { [[unlikely]]
        return 1;
    } else if (*str == '\r' && str[1] == '\n') { [[unlikely]]
        return 2;
    }

    return 0;
}

inline
int32 str_copy_until(char* __restrict dest, const char* __restrict src, char delim)
{
    int32 len = 0;
    while (*src != delim && *src != '\0') {
        *dest++ = *src++;
        ++len;
    }

    *dest = '\0';

    return len;
}

// @todo Inconsistent parameter order of dest and src with other functions
inline
void str_copy_until(const char* __restrict src, char* __restrict dest, const char* __restrict delim)
{
    size_t len = str_length(delim);

    while (*src != '\0') {
        for (int32 i = 0; i < len; ++i) {
            if (*src == delim[i]) {
                *dest = '\0';
                return;
            }
        }

        *dest++ = *src++;
    }

    *dest = '\0';
}

inline
void str_copy_short(char* __restrict dest, const char* __restrict src, int32 length)
{
    int32 i = -1;
    while (*src != '\0' && ++i < length) {
        *dest++ = *src++;
    }

    *dest = '\0';
}

inline
void str_copy_short(char* __restrict dest, const char* __restrict src)
{
    while (*src != '\0') {
        *dest++ = *src++;
    }

    *dest = '\0';
}

inline
void str_copy_long(char* __restrict dest, const char* __restrict src)
{
    char* d = dest;
    const char *s = src;

    // Align destination to its natural alignment
    while (((uintptr_t) d & (sizeof(uintptr_t) - 1)) != 0 && *s != '\0') {
        *d++ = *s++;
    }

    // Copy using larger chunks (size of uintptr_t)
    uintptr_t* aligned_dest = (uintptr_t *) d;
    const uintptr_t* aligned_src = (const uintptr_t *) s;

    while (*aligned_src != 0) {
        *aligned_dest++ = *aligned_src++;
    }

    d = (char *) aligned_dest;
    s = (const char *) aligned_src;

    // Copy remaining bytes
    while (*s != '\0') {
        *d++ = *s++;
    }

    *d = '\0';
}

inline
void str_copy_move_until(const char** __restrict src, char* __restrict dest, char delim)
{
    while (**src != delim && **src != '\0') {
        *dest++ = **src;
        ++(*src);
    }

    *dest = '\0';
}

inline
void str_copy_move_until(const char** __restrict src, char* __restrict dest, const char* __restrict delim)
{
    size_t len = str_length(delim);

    while (**src != '\0') {
        for (size_t i = 0; i < len; ++i) {
            if (**src == delim[i]) {
                *dest = '\0';
                return;
            }
        }

        *dest++ = **src;
        ++(*src);
    }

    *dest = '\0';
}

inline
int32 strcpy_to_eol(const char* src, char* dst)
{
    int32 offset = 0;
    while (!is_eol(src) && *src != '\0')  {
        *dst++ = *src++;
        ++offset;
    }

    *dst = '\0';

    return offset;
}

inline
char* strsep(const char** sp, const char* sep)
{
    char* p, *s;

    if (sp == NULL || *sp == NULL || **sp == '\0') {
        return (NULL);
    }

    s = (char *) *sp;
    p = s + strcspn(s, sep);

    if (*p != '\0') {
        *p++ = '\0';
    }

    *sp = p;

    return s;
}

inline void
str_concat_new(
    char* dst,
    const char* src1,
    const char* src2
) {
    while (*src1) { *dst++ = *src1++; }
    while (*src2) { *dst++ = *src2++; }

    *dst = '\0';
}

inline void
str_concat_append(char* dst, const char* src)
{
    while (*dst) {
        ++dst;
    }

    str_copy_short(dst, src);
}

inline void
str_concat_new(char* dst, const char* src1, const char* src2, const char* src3)
{
    while (*src1) { *dst++ = *src1++; }
    while (*src2) { *dst++ = *src2++; }
    while (*src3) { *dst++ = *src3++; }

    *dst = '\0';
}

inline int64
str_concat_append(char* dst, size_t dst_length, const char* src, size_t src_length)
{
    memcpy(&dst[dst_length], src, src_length);
    dst[dst_length + src_length] = '\0';

    return dst_length + src_length;
}

inline void
str_concat_append(char* dst, size_t dst_length, const char* src)
{
    str_copy_short(&dst[dst_length], src);
}

inline int64
str_concat_new(
    char* dst,
    const char* src1, size_t src1_length,
    const char* src2, size_t src2_length
) {
    memcpy(dst, src1, src1_length);
    dst += src1_length;

    memcpy(dst, src2, src2_length);
    dst += src2_length;

    *dst = '\0';

    return src1_length + src2_length;
}

inline
void str_concat_new(
    char* dst,
    const char* src, size_t src_length,
    int64 data
) {
    memcpy(dst, src, src_length);
    int32 len = int_to_str(data, dst + src_length);

    dst[src_length + len] = '\0';
}

inline
void str_concat_append(
    char* dst,
    int64 data
) {
    size_t dst_len = str_length(dst);
    int_to_str(data, dst + dst_len);
}

inline void
str_concat_new(char* dst, const char* src, int64 data)
{
    size_t src_len = str_length(src);
    memcpy(dst, src, src_len);

    int_to_str(data, dst + src_len);
}

inline
void str_insert(char* __restrict dst, size_t insert_pos, const char* __restrict src) {
    size_t src_length = str_length(src);
    size_t dst_length = str_length(dst);
    memcpy(dst + insert_pos + src_length, dst + insert_pos, dst_length - insert_pos + 1);
    memcpy(dst + insert_pos, src, src_length);
}

inline
void str_remove(char* __restrict dst, size_t remove_pos, size_t remove_length) {
    size_t src_length = str_length(dst);
    memmove(dst + remove_pos, dst + remove_pos + remove_length, src_length - (remove_pos + remove_length) + 1);
}

inline
char* strtok(char* str, const char* __restrict delim, char* *key) {
    char* result;
    if (str == NULL) {
        str = *key;
    }

    str += strspn(str, delim);
    if (*str == '\0') {
        return NULL;
    }

    result = str;
    str += strcspn(str, delim);

    if (*str) {
        *str++ = '\0';
    }

    *key = str;

    return result;
}

inline constexpr
char toupper_ascii(char c)
{
    return c - 32 * (c >= 'a' && c <= 'z');
}

inline
void toupper_ascii(char* str)
{
    while (*str != '\0') {
        *str -= 32 * (*str >= 'a' && *str <= 'z');
        ++str;
    }
}

inline constexpr
char tolower_ascii(char c)
{
    return c + 32 * (c >= 'A' && c <= 'Z');
}

inline
void tolower_ascii(char* str)
{
    while (*str != '\0') {
        *str += 32 * (*str >= 'A' && *str <= 'Z');
        ++str;
    }
}

inline constexpr
void create_const_name(const byte* name, char* modified_name)
{
    size_t i = 0;
    while (*name != '\0') {
        modified_name[i] = *name == ' ' ? '_' : toupper_ascii(*name);
        ++name;
        ++i;
    }

    modified_name[i] = '\0';
}

inline
void create_const_name(byte* name)
{
    while (*name != '\0') {
        *name = *name == ' ' ? '_' : toupper_ascii(*name);
    }

    *name = '\0';
}

constexpr inline
int32 str_compare(const char* str1, const char* str2)
{
    byte c1, c2;

    do {
        c1 = (byte) *str1++;
        c2 = (byte) *str2++;
    } while (c1 == c2 && c1 != '\0');

    return c1 - c2;
}

int32 str_compare(const char* str1, const char* str2, size_t n)
{
    byte c1 = '\0';
    byte c2 = '\0';

    if (n >= 4) {
        size_t n4 = n >> 2;

        do {
            c1 = (byte) *str1++;
            c2 = (byte) *str2++;

            if (c1 == '\0' || c1 != c2) {
                return c1 - c2;
            }

            c1 = (byte) *str1++;
            c2 = (byte) *str2++;

            if (c1 == '\0' || c1 != c2) {
                return c1 - c2;
            }

            c1 = (byte) *str1++;
            c2 = (byte) *str2++;

            if (c1 == '\0' || c1 != c2) {
                return c1 - c2;
            }

            c1 = (byte) *str1++;
            c2 = (byte) *str2++;

            if (c1 == '\0' || c1 != c2) {
                return c1 - c2;
            }
        } while (--n4 > 0);

        n &= 3;
    }

    while (n > 0) {
        c1 = (byte) *str1++;
        c2 = (byte) *str2++;

        if (c1 == '\0' || c1 != c2) {
            return c1 - c2;
        }

        --n;
    }

    return c1 - c2;
}

inline constexpr
bool str_ends_with(const char* str, const char* suffix) {
    if (!str || !suffix) {
        return false;
    }

    size_t str_len = str_length(str);
    size_t suffix_len = str_length(suffix);

    if (suffix_len > str_len) {
        return false;
    }

    return str_compare(str + str_len - suffix_len, suffix, suffix_len) == 0;
}

// WARNING: result needs to have the correct length
void str_replace(const char* str, const char* __restrict search, const char* __restrict replace, char* result) {
    if (str == NULL || search == NULL || replace == NULL || result == NULL) {
        return;
    }

    size_t search_len = str_length(search);
    size_t replace_len = str_length(replace);

    if (search_len == 0) {
        str_copy_short(result, str);
        return;
    }

    const char* current = str;
    char* result_ptr = result;

    while (*current && (current = str_find(current, search)) != NULL) {
        size_t bytes_to_copy = current - str;
        memcpy(result_ptr, str, bytes_to_copy);
        result_ptr += bytes_to_copy;

        memcpy(result_ptr, replace, replace_len);
        result_ptr += replace_len;

        current += search_len;
        str = current;
    }

    str_copy_short(result_ptr, str);
}

/*
void print_bytes(const void* ptr, size_t size)
{
    const byte* bytePtr = (const byte *) ptr;

    size_t count = 0;

    for (size_t i = 0; i < size; i++) {
        ++count;
        if (count == 1) {
            printf("%03zd - %03zd: %02x ", i + 1, i + 8, bytePtr[i]);
        } else if (count < 8) {
            printf("%02x ", bytePtr[i]);
        } else {
            printf("%02x\n", bytePtr[i]);
            count = 0;
        }
    }
}
*/

inline constexpr
bool is_whitespace(char str)
{
    return str == ' ' || str == '\t';
}

inline
int32 str_to_eol(const char* str)
{
    int32 offset = 0;
    while (!is_eol(str) && *str++ != '\0')  {
        ++offset;
    }

    return offset;
}

inline
int32 str_to(const char* str, char delim)
{
    int32 offset = 0;
    while (*str != delim && *str++ != '\0')  {
        ++offset;
    }

    return offset;
}

inline
void str_move_to(const char** str, char delim)
{
    while (**str != delim && **str != '\0')  {
        ++(*str);
    }
}

// Negative pos counts backwards
inline
void str_move_to_pos(const char** str, int32 pos)
{
    if (pos >= 0) {
        *str += pos;
    } else {
        (*str) += OMS_MAX(((int32) str_length(*str) + pos), 0);
    }
}

inline
void str_move_past(const char** str, char delim)
{
    while (**str != delim && **str != '\0')  {
        ++(*str);
    }

    if (**str == delim) {
        ++(*str);
    }
}

inline
void str_move_past_alpha_num(const char** str)
{
    while (str_is_alphanum(**str)
        || **str == 45 || **str == 95
    )  {
        ++(*str);
    }
}

inline
bool str_is_comment(const char* str)
{
    return (*str == '/' && str[1] == '/') || (*str == '/' && str[1] == '*');
}

inline
void str_skip(const char** str, char delim)
{
    while (**str && **str == delim)  {
        ++(*str);
    }
}

inline
void str_skip_whitespace(const char** str)
{
    while (**str && (**str == ' ' || **str == '\t'))  {
        ++(*str);
    }
}

inline
void str_skip_empty(const char** str)
{
    while (**str == ' ' || **str == '\t' || **str == '\n' || **str == '\r')  {
        ++(*str);
    }
}

inline
void str_skip_non_empty(const char** str)
{
    while (**str != ' ' && **str != '\t' && **str != '\n' && **str != '\0')  {
        ++(*str);
    }
}

inline
void str_skip_list(const char** __restrict str, const char* __restrict delim, int32 len)
{
    bool run = true;
    while (run && **str != '\0') {
        run = false;

        for (int32 i = 0; i < len; ++i) {
            if (**str == delim[i]) {
                run = true;
                ++(*str);

                break;
            }
        }
    }
}

inline
void str_skip_until_list(const char** __restrict str, const char* __restrict delim)
{
    while (**str != '\0') {
        const char* delim_temp = delim;
        while (*delim_temp) {
            if (**str == *delim_temp) {
                return;
            }

            ++delim_temp;
        }

        ++(*str);
    }
}

inline
void hexstr_to_rgba(v4_f32* rgba, const char* hex)
{
    if (*hex == '#') {
        ++hex;
    }

    uint32 value = (uint32) hex_to_int(hex);
    rgba->r = (f32) ((value >> 24) & 0xFF) / 255.0f;
    rgba->g = (f32) ((value >> 16) & 0xFF) / 255.0f;
    rgba->b = (f32) ((value >> 8) & 0xFF) / 255.0f;
    rgba->a = (f32) (value & 0xFF) / 255.0f;
}

inline constexpr
void str_pad(const char* input, char* output, char pad, size_t len) {
    size_t i = 0;
    for (; i < len && input[i] != '\0'; ++i) {
        output[i] = input[i];
    }

    for (; i < len; ++i) {
        output[i] = pad;
    }
}

inline
f32 str_to_float(const char* str, const char** pos = NULL)
{
    const char *p = str;
    f32 result = 0.0f;
    int32 sign = 1;

    // Skip leading whitespace
    while (is_whitespace(*p)) {
        ++p;
    }

    // Handle optional sign
    if (*p == '+' || *p == '-') {
        sign = (*p == '-') ? -1 : 1;
        ++p;
    }

    // Parse integer part
    while (str_is_num(*p)) {
        result = result * 10.0f + (*p - '0');
        ++p;
    }

    // Parse fractional part
    if (*p == '.') {
        int32 decimals = 0;
        ++p;

        while (str_is_num(*p)) {
            result = result * 10.0f + (*p - '0');
            ++decimals;
            ++p;
        }

        // Adjust for decimal places and exponent
        static const float powers_of_ten[] = {
            1.0f, 10.0f, 100.0f, 1000.0f, 10000.0f, 100000.0f, 1000000.0f
        };

        result /= powers_of_ten[decimals];
    }

    // Set end pointer
    if (pos) {
        *pos = (char *) p;
    }

    return sign * result;
}

inline
int32 float_to_str(f64 value, char* buffer, int32 precision = 5)
{
    ASSERT_SIMPLE(precision < 6);

    char* start = buffer;

    if (value < 0) {
        *buffer++ = '-';
        value = -value;
    }

    static const float powers_of_ten[] = {
        1.0f, 10.0f, 100.0f, 1000.0f, 10000.0f, 100000.0f
    };

    f32 scale = powers_of_ten[precision];
    value = OMS_ROUND_POSITIVE(value * scale) / scale;

    // Handle integer part
    int32 int_part = (int32) value;
    f64 frac_part = value - int_part;

    char temp[20];
    int32 index = 0;

    do {
        temp[index++] = (int_part % 10) + '0';
        int_part /= 10;
    } while (int_part > 0);

    while (index > 0) {
        *buffer++ = temp[--index];
    }

    // Handle fractional part
    if (precision > 0) {
        *buffer++ = '.';
        while (precision--) {
            frac_part *= 10;
            int32 digit = (int32) frac_part;
            *buffer++ = (char) (digit + '0');
            frac_part -= digit;
        }
    }

    *buffer = '\0';

    return (int32) (buffer - start);
}

inline
void format_time_hh_mm_ss(char time_str[9], int32 hours, int32 minutes, int32 secs) {
    time_str[0] = (char) ('0' + (hours / 10));
    time_str[1] = (char) ('0' + (hours % 10));
    time_str[2] = ':';
    time_str[3] = (char) ('0' + (minutes / 10));
    time_str[4] = (char) ('0' + (minutes % 10));
    time_str[5] = ':';
    time_str[6] = (char) ('0' + (secs / 10));
    time_str[7] = (char) ('0' + (secs % 10));
    time_str[8] = '\0';
}

inline
void format_time_hh_mm_ss(char time_str[9], uint64 time) {
    int32 hours = (time / 3600) % 24;
    int32 minutes = (time / 60) % 60;
    int32 secs = time % 60;

    format_time_hh_mm_ss(time_str, hours, minutes, secs);
}

inline
void format_time_hh_mm(char time_str[6], int32 hours, int32 minutes) {
    time_str[0] = (char) ('0' + (hours / 10));
    time_str[1] = (char) ('0' + (hours % 10));
    time_str[2] = ':';
    time_str[3] = (char) ('0' + (minutes / 10));
    time_str[4] = (char) ('0' + (minutes % 10));
    time_str[5] = '\0';
}

inline
void format_time_hh_mm(char time_str[6], uint64 time) {
    int32 hours = (time / 3600) % 24;
    int32 minutes = (time / 60) % 60;

    format_time_hh_mm(time_str, hours, minutes);
}

void sprintf_fast(char* __restrict buffer, const char* __restrict format, ...) {
    va_list args;
    va_start(args, format);

    while (*format) {
        if (*format != '%') {
            *buffer++ = *format;
        } else if (*format == '\\' && *(format + 1) == '%') {
            ++format;
            *buffer++ = *format;
        } else {
            ++format;

            switch (*format) {
                case 's': {
                    const char* str = va_arg(args, const char*);
                    while (*str) {
                        *buffer++ = *str++;
                    }
                } break;
                case 'c': {
                    *buffer++ = (char) va_arg(args, int32);
                } break;
                case 'n': {
                    int64 val = va_arg(args, int64);
                    buffer += int_to_str(val, buffer, ',');
                } break;
                case 'd': {
                    int32 val = va_arg(args, int32);
                    buffer += int_to_str(val, buffer);
                } break;
                case 'l': {
                    int64 val = va_arg(args, int64);
                    buffer += int_to_str(val, buffer);
                } break;
                case 'f': {
                    f64 val = va_arg(args, f64);

                    // Default precision
                    int32 precision = 5;

                    // Check for optional precision specifier
                    const char* prec_ptr = format + 1;
                    if (*prec_ptr >= '0' && *prec_ptr <= '9') {
                        precision = 0;
                        while (*prec_ptr >= '0' && *prec_ptr <= '9') {
                            precision = precision * 10 + (*prec_ptr - '0');
                            prec_ptr++;
                        }

                        format = prec_ptr - 1;
                    }

                    buffer += float_to_str(val, buffer, precision);
                } break;
                case 'T': {
                    int64 time = va_arg(args, int64);
                    format_time_hh_mm_ss(buffer, time);
                    buffer += 8;
                } break;
                default: {
                    // Handle unknown format specifiers
                    *buffer++ = '%';
                } break;
            }
        }

        ++format;
    }

    *buffer = '\0';
    va_end(args);
}

void sprintf_fast(char* __restrict buffer, int32 buffer_length, const char* __restrict format, ...) {
    va_list args;
    va_start(args, format);

    // We start at 1 since we need 1 char for '\0'
    int32 length = 1;
    int32 offset;

    while (*format && length < buffer_length) {
        offset = 1;
        if (*format != '%') {
            *buffer++ = *format;
        } else if (*format == '\\' && *(format + 1) == '%') {
            ++format;
            *buffer++ = *format;
        } else {
            ++format;

            switch (*format) {
                case 's': {
                    const char* str = va_arg(args, const char*);
                    --offset;
                    while (*str) {
                        *buffer++ = *str++;
                        ++offset;
                    }
                } break;
                case 'c': {
                    *buffer++ = (char) va_arg(args, int32);
                } break;
                case 'n': {
                    int64 val = va_arg(args, int64);
                    buffer += offset = int_to_str(val, buffer, ',');
                } break;
                case 'd': {
                    int32 val = va_arg(args, int32);
                    buffer += offset = int_to_str(val, buffer);
                } break;
                case 'l': {
                    int64 val = va_arg(args, int64);
                    buffer += offset = int_to_str(val, buffer);
                } break;
                case 'f': {
                    f64 val = va_arg(args, f64);

                    // Default precision
                    int32 precision = 5;

                    // Check for optional precision specifier
                    const char* prec_ptr = format + 1;
                    if (*prec_ptr >= '0' && *prec_ptr <= '9') {
                        precision = 0;
                        while (*prec_ptr >= '0' && *prec_ptr <= '9') {
                            precision = precision * 10 + (*prec_ptr - '0');
                            prec_ptr++;
                        }

                        format = prec_ptr - 1;
                    }

                    buffer += offset = float_to_str(val, buffer, precision);
                } break;
                case 'T': {
                    int64 time = va_arg(args, int64);
                    format_time_hh_mm_ss(buffer, time);
                    buffer += 8;
                } break;
                default: {
                    // Handle unknown format specifiers
                    *buffer++ = '%';
                } break;
            }
        }

        length += offset;
        ++format;
    }

    *buffer = '\0';
    va_end(args);
}

// There are situations where you only want to replace a certain amount of %
void sprintf_fast_iter(char* buffer, const char* format, ...) {
    va_list args;
    va_start(args, format);

    int32 count_index = 0;

    while (*format) {
        if (*format != '%' || count_index >= 1) {
            *buffer++ = *format;
        } else if (*format == '\\' && *(format + 1) == '%') {
            ++format;
            *buffer++ = *format;
        } else {
            ++count_index;
            ++format;

            switch (*format) {
                case 's': {
                    const char* str = va_arg(args, const char*);
                    while (*str) {
                        *buffer++ = *str++;
                    }
                } break;
                case 'c': {
                    *buffer++ = (char) va_arg(args, int32);
                } break;
                case 'n': {
                    int64 val = va_arg(args, int64);
                    buffer += int_to_str(val, buffer, ',');
                } break;
                case 'd': {
                    int32 val = va_arg(args, int32);
                    buffer += int_to_str(val, buffer);
                } break;
                case 'l': {
                    int64 val = va_arg(args, int64);
                    buffer += int_to_str(val, buffer);
                } break;
                case 'f': {
                    f64 val = va_arg(args, f64);

                    // Default precision
                    int32 precision = 5;

                    // Check for optional precision specifier
                    const char* prec_ptr = format + 1;
                    if (*prec_ptr >= '0' && *prec_ptr <= '9') {
                        precision = 0;
                        while (*prec_ptr >= '0' && *prec_ptr <= '9') {
                            precision = precision * 10 + (*prec_ptr - '0');
                            prec_ptr++;
                        }

                        format = prec_ptr - 1;
                    }

                    buffer += float_to_str(val, buffer, precision);
                } break;
                case 'T': {
                    int64 time = va_arg(args, int64);
                    format_time_hh_mm_ss(buffer, time);
                    buffer += 8;
                } break;
                default: {
                    // Handle unknown format specifiers
                    *buffer++ = '%';
                } break;
            }
        }

        ++format;
    }

    *buffer = '\0';
    va_end(args);
}

#endif