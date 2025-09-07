/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_UTILS_STRING_UTILS_H
#define COMS_UTILS_STRING_UTILS_H

#include <string.h>
#include <stdarg.h>
#include "../stdlib/Types.h"
#include "../utils/Assert.h"

// WARNING: We need this function because the other function relies on none-constexpr performance features
constexpr
size_t str_length_constexpr(const char* str) NO_EXCEPT {
    size_t len = 0;
    while (str[len] != '\0') {
        ++len;
    }
    return len;
}

inline
size_t str_length(const char* str) NO_EXCEPT {
        const char* ptr = str;

    while ((uintptr_t)ptr % sizeof(size_t) != 0) {
        if (*ptr == '\0')
            return ptr - str;
        ++ptr;
    }

    const size_t* longword_ptr = (const size_t*)ptr;

    while (true) {
        size_t v = *longword_ptr;

        if (OMS_HAS_ZERO(v)) {
            const char* cp = (const char*)longword_ptr;
            for (size_t i = 0; i < sizeof(size_t); ++i) {
                if (cp[i] == '\0')
                    return cp + i - str;
            }
        }

        ++longword_ptr;
    }
}

// WARNING: We need this function because the other function relies on none-constexpr performance features
inline constexpr
const char* str_find_constexpr(const char* str, const char* needle) NO_EXCEPT {
    size_t needle_len = str_length_constexpr(needle);
    size_t str_len = str_length_constexpr(str);
    size_t limit = str_len - needle_len + 1;

    for (size_t i = 0; i < limit; ++i) {
        if (str[i] == needle[0] && memcmp(&str[i + 1], &needle[1], needle_len - 1) == 0) {
            return &str[i];
        }
    }

    return NULL;
}

inline
const char* str_find(const char* haystack, const char* needle) NO_EXCEPT {
    const char first = needle[0];
    size_t needle_len = str_length(needle);
    const char* ptr = haystack;

    while (*ptr != '\0') {
        // Align pointer for size_t access
        while ((uintptr_t) ptr % sizeof(size_t) != 0 && *ptr != '\0') {
            if (*ptr == first) {
                break;
            }

            ++ptr;
        }

        if (*ptr == '\0') {
            break;
        }

        // Word-wise scanning
        const size_t* wptr = (const size_t*) ptr;
        const size_t first_mask = ((size_t) -1 / 0xFF) * first;

        while (true) {
            size_t v = *wptr;
            if (((v - ((size_t)-1 / 0xFF)) & ~v & (((size_t)-1 / 0xFF) * 0x80))
                || (((v ^ first_mask) - ((size_t)-1 / 0xFF)) & ~(v ^ first_mask) & (((size_t)-1 / 0xFF) * 0x80))
            ) {
                break;
            }

            ++wptr;
        }

        ptr = (const char*)wptr;

        const char* p1 = ptr;
        size_t i = 0;
        while (i < needle_len && p1[i] != '\0' && p1[i] == needle[i]) {
            ++i;
        }

        if (i == needle_len) {
            return ptr;
        }

        ++ptr;
    }

    return NULL;
}

static const unsigned char TO_LOWER_TABLE[256] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F,
    0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F,
    0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F,
    0x40, 'a',  'b',  'c',  'd',  'e',  'f',  'g',  'h',  'i',  'j',  'k',  'l',  'm',  'n',  'o',
    'p',  'q',  'r',  's',  't',  'u',  'v',  'w',  'x',  'y',  'z',  0x5B, 0x5C, 0x5D, 0x5E, 0x5F,
    0x60, 'a',  'b',  'c',  'd',  'e',  'f',  'g',  'h',  'i',  'j',  'k',  'l',  'm',  'n',  'o',
    'p',  'q',  'r',  's',  't',  'u',  'v',  'w',  'x',  'y',  'z',  0x7B, 0x7C, 0x7D, 0x7E, 0x7F,
    0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8A, 0x8B, 0x8C, 0x8D, 0x8E, 0x8F,
    0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9A, 0x9B, 0x9C, 0x9D, 0x9E, 0x9F,
    0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xAB, 0xAC, 0xAD, 0xAE, 0xAF,
    0xB0, 0xB1, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB7, 0xB8, 0xB9, 0xBA, 0xBB, 0xBC, 0xBD, 0xBE, 0xBF,
    0xC0, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xCB, 0xCC, 0xCD, 0xCE, 0xCF,
    0xD0, 0xD1, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xDA, 0xDB, 0xDC, 0xDD, 0xDE, 0xDF,
    0xE0, 0xE1, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9, 0xEA, 0xEB, 0xEC, 0xED, 0xEE, 0xEF,
    0xF0, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF
};

static const unsigned char TO_UPPER_TABLE[256] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F,
    0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F,
    0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F,
    0x40, 'A',  'B',  'C',  'D',  'E',  'F',  'G',  'H',  'I',  'J',  'K',  'L',  'M',  'N',  'O',
    'P',  'Q',  'R',  'S',  'T',  'U',  'V',  'W',  'X',  'Y',  'Z',  0x5B, 0x5C, 0x5D, 0x5E, 0x5F,
    0x60, 'A',  'B',  'C',  'D',  'E',  'F',  'G',  'H',  'I',  'J',  'K',  'L',  'M',  'N',  'O',
    'P',  'Q',  'R',  'S',  'T',  'U',  'V',  'W',  'X',  'Y',  'Z',  0x7B, 0x7C, 0x7D, 0x7E, 0x7F,
    0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8A, 0x8B, 0x8C, 0x8D, 0x8E, 0x8F,
    0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9A, 0x9B, 0x9C, 0x9D, 0x9E, 0x9F,
    0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xAB, 0xAC, 0xAD, 0xAE, 0xAF,
    0xB0, 0xB1, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB7, 0xB8, 0xB9, 0xBA, 0xBB, 0xBC, 0xBD, 0xBE, 0xBF,
    0xC0, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xCB, 0xCC, 0xCD, 0xCE, 0xCF,
    0xD0, 0xD1, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xDA, 0xDB, 0xDC, 0xDD, 0xDE, 0xDF,
    0xE0, 0xE1, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9, 0xEA, 0xEB, 0xEC, 0xED, 0xEE, 0xEF,
    0xF0, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF
};


FORCE_INLINE constexpr
char toupper_ascii(char c) NO_EXCEPT
{
    return c - 32 * (c >= 'a' && c <= 'z');
}

FORCE_INLINE
void toupper_ascii(char* str) NO_EXCEPT
{
    while (*str != '\0') {
        *str -= 32 * (*str >= 'a' && *str <= 'z');
        ++str;
    }
}

FORCE_INLINE constexpr
char tolower_ascii(char c) NO_EXCEPT
{
    return c + 32 * (c >= 'A' && c <= 'Z');
}

inline
void tolower_ascii(char* str) NO_EXCEPT
{
    while (*str != '\0') {
        *str += 32 * (*str >= 'A' && *str <= 'Z');
        ++str;
    }
}

const char* str_find(const char* str, char needle) NO_EXCEPT {
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
        if (OMS_HAS_CHAR(word, target)) {
            const char* byte_ptr = (const char *) (word_ptr - 1);
            for (size_t i = 0; i < sizeof(size_t); ++i) {
                if (byte_ptr[i] == target) {
                    return byte_ptr + i;
                }
            }
        }

        if (OMS_HAS_ZERO(word)) {
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
int32 utf8_encode(uint32 codepoint, char* out) NO_EXCEPT
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
int32 utf8_decode(const char* __restrict in, uint32* __restrict codepoint) NO_EXCEPT {
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

    return 0;
}

inline
int32 utf8_decode(const uint32 codepoint, char* out) NO_EXCEPT {
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

    // @bug For some reason this can happen for alt tab etc. That's why we need to allow this special case.
    return 0;
}

inline
int32 utf8_str_length(const char* in) NO_EXCEPT {
    int32 length = 0;
    uint32 codepoint;

    while (*in) {
        int32 bytes = utf8_decode(in, &codepoint);
        if (bytes < 0) {
            return -1;
        }

        in += bytes;
        ++length;
    }

    return length;
}

inline
void string_to_utf8(const uint32* __restrict in, char* __restrict out) NO_EXCEPT {
    char buffer[5] = {0};
    while (*in) {
        int32 len = utf8_encode(*in++, buffer);
        for (int32 i = 0; i < len; ++i) {
            *out++ = buffer[i];
        }
    }
}

inline
int32 utf8_get_char_at(const char* in, int32 index) NO_EXCEPT {
    int32 i = 0;
    uint32 codepoint;

    while (*in) {
        int32 bytes_consumed = utf8_decode(in, &codepoint);
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
void wchar_to_char(wchar_t* str) NO_EXCEPT
{
    char* src = (char*) str;
    char* dest = src;

    while (*src != '\0' || src[1] != '\0') {
        if (*src != '\0') {
            *dest++ = *src;
        }

        ++src;
    }

    *dest = '\0';
}

inline
void wchar_to_char(const char* __restrict str, char* __restrict dest) NO_EXCEPT
{
    while (*str != '\0' || str[1] != '\0') {
        if (*str != '\0') {
            *dest++ = (char) *str;
        }

        ++str;
    }

    *dest = '\0';
}

static constexpr const bool STR_IS_ALPHA_LOOKUP_TABLE[] = {
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
    true,  true,  true, false, false, false, false, false,  // 88-95 ('X'-'Z', others)
    false, true,  true,  true,  true,  true,  true,  true,  // 96-103 ('a'-'g')
    true,  true,  true,  true,  true,  true,  true,  true,  // 104-111 ('h'-'o')
    true,  true,  true,  true,  true,  true,  true,  true,  // 112-119 ('p'-'w')
    true,  true,  true, false, false, false, false, false,  // 120-127 ('x'-'z', others)
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

FORCE_INLINE constexpr
bool str_is_alpha(char str) NO_EXCEPT {
    return STR_IS_ALPHA_LOOKUP_TABLE[(byte) str];
}

inline constexpr
bool str_is_alpha(const char* str) NO_EXCEPT {
    while (*str != '\0') {
        if (!str_is_alpha(*str++)) {
            return false;
        }
    }

    return true;
}

static constexpr const bool STR_IS_NUM_LOOKUP_TABLE[] = {
    false, false, false, false, false, false, false, false, // 0-7
    false, false, false, false, false, false, false, false, // 8-15
    false, false, false, false, false, false, false, false, // 16-23
    false, false, false, false, false, false, false, false, // 24-31
    false, false, false, false, false, false, false, false, // 32-39
    false, false, false, false, false, false, false, false, // 40-47
    true,  true,  true,  true,  true,  true,  true,  true,  // 48-55 ('0'-'7')
    true,  true,  false, false, false, false, false, false, // 56-63 ('8'-'9', others)
    false, true,  false, false, false, false, false, false, // 64-71 ('A'-'G')
    false, false, false, false, false, false, false, false, // 72-79 ('H'-'O')
    false, false, false, false, false, false, false, false, // 80-87 ('P'-'W')
    false, false, false, false, false, false, false, false, // 88-95 ('X'-'Z', others)
    false, false, false, false, false, false, false, false, // 96-103 ('a'-'g')
    false, false, false, false, false, false, false, false, // 104-111 ('h'-'o')
    false, false, false, false, false, false, false, false, // 112-119 ('p'-'w')
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

FORCE_INLINE constexpr
bool str_is_num(char str) NO_EXCEPT {
    return STR_IS_NUM_LOOKUP_TABLE[(byte) str];
}

static constexpr const bool STR_IS_ALPHANUM_LOOKUP_TABLE[] = {
    false, false, false, false, false, false, false, false, // 0-7
    false, false, false, false, false, false, false, false, // 8-15
    false, false, false, false, false, false, false, false, // 16-23
    false, false, false, false, false, false, false, false, // 24-31
    false, false, false, false, false, false, false, false, // 32-39
    false, false, false, false, false, false, false, false, // 40-47
    true,  true,  true,  true,  true,  true,  true,  true,  // 48-55 ('0'-'7')
    true,  true, false, false, false, false, false, false,  // 56-63 ('8'-'9', others)
    false, true,  true,  true,  true,  true,  true,  true,  // 64-71 ('A'-'G')
    true,  true,  true,  true,  true,  true,  true,  true,  // 72-79 ('H'-'O')
    true,  true,  true,  true,  true,  true,  true,  true,  // 80-87 ('P'-'W')
    true,  true,  true, false, false, false, false, false,  // 88-95 ('X'-'Z', others)
    false, true,  true,  true,  true,  true,  true,  true,  // 96-103 ('a'-'g')
    true,  true,  true,  true,  true,  true,  true,  true,  // 104-111 ('h'-'o')
    true,  true,  true,  true,  true,  true,  true,  true,  // 112-119 ('p'-'w')
    true,  true,  true, false, false, false, false, false,  // 120-127 ('x'-'z', others)
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

FORCE_INLINE constexpr
bool str_is_alphanum(char str) NO_EXCEPT {
    return STR_IS_ALPHANUM_LOOKUP_TABLE[(byte) str];
}

inline
bool str_is_alphanum(const char* str) NO_EXCEPT {
    while (*str != '\0') {
        if (!str_is_alphanum(*str++)) {
            return false;
        }
    }

    return true;
}

inline
bool str_is_float(const char* str) NO_EXCEPT {
    bool has_dot = false;

    if (*str == '-' || *str == '+') {
        ++str;
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
bool str_is_integer(const char* str) NO_EXCEPT {
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
int64 str_to_int(const char* str, const char** pos = NULL) NO_EXCEPT
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
int32 int_length(int64 number) NO_EXCEPT {
    if (number == 0) {
        return 1;
    }

    int len = 0;
    if (number < 0) {
        ++len;
        number = -number;
    }

    while (number > 0) {
        number /= 10;
        ++len;
    }

    return len;
}

inline
int32 int_to_str(int64 number, char str[15], char thousands) NO_EXCEPT
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
int32 int_to_str(int64 number, char str[12]) NO_EXCEPT {
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
int32 int_to_str(uint64 number, char str[12]) NO_EXCEPT {
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

inline constexpr
int32 int_to_str(int32 number, char str[12]) NO_EXCEPT {
    int32 i = -1;
    int32 sign = number;

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
int32 int_to_str(uint32 number, char str[12]) NO_EXCEPT {
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

static constexpr const char HEX_TABLE[] = {
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
    'A', 'B', 'C', 'D', 'E', 'F'
};

static constexpr const bool HEX_LOOKUP_TABLE[256] = {
    false, false, false, false, false, false, false, false, // 0-7
    false, false, false, false, false, false, false, false, // 8-15
    false, false, false, false, false, false, false, false, // 16-23
    false, false, false, false, false, false, false, false, // 24-31
    false, false, false, false, false, false, false, false, // 32-39
    false, false, false, false, false, false, false, false, // 40-47
    true,  true,  true,  true,  true,  true,  true,  true,  // 48-55 ('0'-'7')
    true,  true,  false, false, false, false, false, false, // 56-63 ('8'-'9', others)
    false, true,  true,  true,  true,  true,  true,  false, // 64-71 ('A'-'G')
    false, false, false, false, false, false, false, false, // 72-79 ('H'-'O')
    false, false, false, false, false, false, false, false, // 80-87 ('P'-'W')
    false, false, false, false, false, false, false, false, // 88-95 ('X'-'Z', others)
    false, false, false, false, false, false, false, false, // 96-103 ('a'-'g')
    false, false, false, false, false, false, false, false, // 104-111 ('h'-'o')
    false, false, false, false, false, false, false, false, // 112-119 ('p'-'w')
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
bool str_is_hex_color(const char* str) NO_EXCEPT
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
int32 int_to_hex(int64 number, char str[9]) NO_EXCEPT {
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
int64 hex_to_int(const char* hex) NO_EXCEPT
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
size_t str_count(const char* __restrict str, const char* __restrict substr) NO_EXCEPT
{
    size_t count = 0;
    size_t substr_len = str_length(substr);

    const char* s = str;
    char first = substr[0];

    // Align pointer for word-wise scanning
    while ((uintptr_t)s % sizeof(size_t) != 0 && *s != '\0') {
        if (*s == first) {
            // Compare remaining substring
            size_t i = 1;
            while (i < substr_len && s[i] != '\0' && s[i] == substr[i]) {
                ++i;
            }

            if (i == substr_len) {
                ++count;
                s += substr_len;
                continue;
            }
        }

        ++s;
    }

    const size_t* wptr = (const size_t*)s;

    while (true) {
        size_t v = *wptr;
        if (OMS_HAS_ZERO(v) || OMS_HAS_CHAR(v, first)) {
            break; // null byte or first char found
        }

        ++wptr;
    }

    s = (const char*) wptr;

    // Fallback to byte-by-byte scanning for matches
    while (*s != '\0') {
        if (*s == first) {
            size_t i = 1;
            while (i < substr_len && s[i] != '\0' && s[i] == substr[i]) {
                ++i;
            }

            if (i == substr_len) {
                ++count;
                s += substr_len;
                continue;
            }
        }

        ++s;
    }

    return count;
}

inline constexpr
int32 is_eol(const char* str) NO_EXCEPT
{
    if (*str == '\n') { [[unlikely]]
        return 1;
    } else if (*str == '\r' && str[1] == '\n') { [[unlikely]]
        return 2;
    }

    return 0;
}

inline
int32 str_copy_until(char* __restrict dest, const char* __restrict src, char delim) NO_EXCEPT
{
    const char* s = src;
    char* d = dest;

    if (*s == '\0' || *s == delim) {
        *d = '\0';

        return 0;
    }

    // Align pointer for size_t access
    while ((uintptr_t)s % sizeof(size_t) != 0 && *s != '\0' && *s != delim) {
        *d++ = *s++;
    }

    const size_t delim_mask = ((size_t)-1 / 0xFF) * delim;
    const size_t* wptr = (const size_t*) s;

    while (true) {
        size_t v = *wptr;

        // Detect either null or delimiter
        if (OMS_HAS_ZERO(v) || OMS_HAS_ZERO(v ^ delim_mask)) {
            const char* cp = (const char*) wptr;
            for (size_t i = 0; i < sizeof(size_t); ++i) {
                if (cp[i] == '\0' || cp[i] == delim) {
                    size_t len = cp + i - src;
                    for (size_t j = 0; j < len; ++j) {
                        dest[j] = src[j];
                    }

                    dest[len] = '\0';

                    return (int32) len;
                }
            }
        }

        ++wptr;
    }
}

inline
void str_copy_until(char* __restrict dest, const char* __restrict src, const char* __restrict delim) NO_EXCEPT
{
    size_t len = str_length(delim);

    while (*src != '\0') {
        for (size_t i = 0; i < len; ++i) {
            if (*src == delim[i]) {
                *dest = '\0';
                return;
            }
        }

        *dest++ = *src++;
    }

    *dest = '\0';
}

inline constexpr
void str_copy(char* __restrict dest, const char* __restrict src, int32 length) NO_EXCEPT
{
    int32 copied = 0;

    // Align src pointer to size_t boundary
    while ((uintptr_t)src % sizeof(size_t) != 0 && copied < length - 1) {
        char c = *src++;
        *dest++ = c;
        ++copied;

        if (c == '\0') {
            return;
        }
    }

    const size_t* wptr = (const size_t*) src;
    size_t* dptr = (size_t*) dest;

    while (copied + (int32) sizeof(size_t) < length) {
        size_t v = *wptr;
        if (OMS_HAS_ZERO(v)) {
            break;
        }

        *dptr++ = v;
        ++wptr;
        copied += sizeof(size_t);
    }

    // Copy remaining bytes one by one
    src = (const char*)wptr;
    dest = (char*)dptr;
    while (copied < length - 1) {
        char c = *src++;
        *dest++ = c;
        ++copied;

        if (c == '\0') {
            break;
        }
    }

    *dest = '\0';
}

inline constexpr
void str_copy(char* __restrict dest, const char* __restrict src) NO_EXCEPT
{
    // Align src pointer
    while ((uintptr_t)src % sizeof(size_t) != 0) {
        char c = *src++;
        *dest++ = c;
        if (c == '\0') return;
    }

    const size_t* wptr = (const size_t*)src;
    size_t* dptr = (size_t*)dest;

    while (true) {
        size_t v = *wptr;
        if (OMS_HAS_ZERO(v)) break; // null byte found
        *dptr++ = v;
        wptr++;
    }

    // Copy remaining bytes
    src = (const char*)wptr;
    dest = (char*)dptr;
    while (true) {
        char c = *src++;
        *dest++ = c;
        if (c == '\0') break;
    }
}

inline
void str_copy_move_until(char* __restrict dest, const char* __restrict* __restrict src, char delim) NO_EXCEPT
{
    const char* s = *src;
    char* d = dest;

    // Copy unaligned prefix
    while ((uintptr_t)s % sizeof(size_t) != 0) {
        char c = *s++;
        if (c == '\0' || c == delim) {
            *d = '\0';
            *src = s - 1; // leave src at delimiter or null

            return;
        }

        *d++ = c;
    }

    const size_t delim_mask = ((size_t)-1 / 0xFF) * delim;
    const size_t* wptr = (const size_t*) s;
    size_t* dptr = (size_t*) d;

    while (true) {
        size_t v = *wptr;

        // Check for delimiter or null byte
        if (OMS_HAS_ZERO(v) || OMS_HAS_ZERO(v ^ delim_mask)) {
            break;
        }

        *dptr++ = v;
        ++wptr;
    }

    // Copy remaining bytes byte-by-byte
    s = (const char*)wptr;
    d = (char*)dptr;
    while (true) {
        char c = *s++;
        if (c == '\0' || c == delim) {
            break;
        }

        *d++ = c;
    }

    *d = '\0';
    *src = s - 1;
}

inline
void str_copy_move_until(char* __restrict dest, const char* __restrict* __restrict src, const char* __restrict delim) NO_EXCEPT
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
int32 str_copy_to_eol(const char* src, char* dst) NO_EXCEPT
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
char* strsep(const char** sp, const char* sep) NO_EXCEPT
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
) NO_EXCEPT {
    while (*src1) { *dst++ = *src1++; }
    while (*src2) { *dst++ = *src2++; }

    *dst = '\0';
}

inline void
str_concat_append(char* dst, const char* src) NO_EXCEPT
{
    dst += str_length(dst);
    str_copy(dst, src);
}

inline void
str_concat_new(char* dst, const char* src1, const char* src2, const char* src3) NO_EXCEPT
{
    const char* sources[3] = { src1, src2, src3 };

    for (int s = 0; s < 3; ++s) {
        const char* src = sources[s];

        // Align src pointer
        while ((uintptr_t)src % sizeof(size_t) != 0 && *src != '\0') {
            *dst++ = *src++;
        }

        const size_t* wptr = (const size_t*)src;
        size_t* dptr = (size_t*)dst;

        while (true) {
            size_t v = *wptr;
            if (OMS_HAS_ZERO(v)) {
                break;
            }

            *dptr++ = v;
            ++wptr;
        }

        // Copy remaining bytes byte-by-byte
        src = (const char*)wptr;
        dst = (char*)dptr;
        while (*src != '\0') {
            *dst++ = *src++;
        }
    }

    *dst = '\0';
}

inline int64
str_concat_append(char* dst, size_t dst_length, const char* src, size_t src_length) NO_EXCEPT
{
    memcpy(&dst[dst_length], src, src_length);
    dst[dst_length + src_length] = '\0';

    return dst_length + src_length;
}

inline void
str_concat_append(char* dst, size_t dst_length, const char* src) NO_EXCEPT
{
    str_copy(&dst[dst_length], src);
}

inline int64
str_concat_new(
    char* __restrict dst,
    const char* src1, size_t src1_length,
    const char* src2, size_t src2_length
) NO_EXCEPT {
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
) NO_EXCEPT {
    memcpy(dst, src, src_length);
    int32 len = int_to_str(data, dst + src_length);

    dst[src_length + len] = '\0';
}

inline
void str_concat_append(
    char* dst,
    int64 data
) NO_EXCEPT {
    size_t dst_len = str_length(dst);
    int_to_str(data, dst + dst_len);
}

inline void
str_concat_new(char* __restrict dst, const char* __restrict src, int64 data) NO_EXCEPT
{
    size_t src_len = str_length(src);
    memcpy(dst, src, src_len);

    int_to_str(data, dst + src_len);
}

inline
void str_insert(char* __restrict dst, size_t insert_pos, const char* __restrict src) NO_EXCEPT {
    size_t src_length = str_length(src);
    size_t dst_length = str_length(dst);
    memcpy(dst + insert_pos + src_length, dst + insert_pos, dst_length - insert_pos + 1);
    memcpy(dst + insert_pos, src, src_length);
}

inline
void str_remove(char* __restrict dst, size_t remove_pos, size_t remove_length) NO_EXCEPT {
    size_t src_length = str_length(dst);
    memmove(dst + remove_pos, dst + remove_pos + remove_length, src_length - (remove_pos + remove_length) + 1);
}

inline
char* strtok(char* str, const char* __restrict delim, char** key) NO_EXCEPT {
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
bool str_contains(const char* __restrict haystack, const char* __restrict needle) NO_EXCEPT
{
    const char first = needle[0];
    const char* ptr = haystack;

    while (*ptr != '\0') {
        // Align pointer for size_t access
        while ((uintptr_t)ptr % sizeof(size_t) != 0 && *ptr != '\0') {
            if (*ptr == first) {
                break;
            }

            ++ptr;
        }

        if (*ptr == '\0') {
            break;
        }

        // Word-wise scanning
        const size_t* wptr = (const size_t*) ptr;
        while (true) {
            size_t v = *wptr;
            if (OMS_HAS_CHAR(v, first)) {
                break;
            }

            ++wptr;
        }

        ptr = (const char*) wptr;

        // Fallback byte-by-byte compare for needle
        const char* p1 = ptr;
        const char* p2 = needle;

        while (*p1 != '\0' && *p2 != '\0' && *p1 == *p2) {
            ++p1;
            ++p2;
        }

        if (*p2 == '\0') {
            return true;
        }

        ++ptr;
    }

    return false;
}

inline constexpr
bool str_contains(const char* __restrict haystack, const char* __restrict needle, size_t length) NO_EXCEPT
{
    while (*haystack != '\0' && length > 0) {
        const char* p1 = haystack;
        const char* p2 = needle;
        size_t remaining = length;

        while (*p2 != '\0' && remaining > 0 && *p1 == *p2) {
            ++p1;
            ++p2;
            --remaining;
        }

        if (*p2 == '\0') {
            return true;
        }

        ++haystack;
        --length;
    }

    return false;
}

inline constexpr
bool byte_contains(
    const byte* __restrict haystack, size_t haystack_len,
    const byte* __restrict needle, size_t needle_len) NO_EXCEPT
{
    size_t skip[256];
    for (size_t i = 0; i < 256; ++i) {
        skip[i] = needle_len;
    }

    for (size_t i = 0; i < needle_len - 1; ++i) {
        skip[needle[i]] = needle_len - i - 1;
    }

    size_t i = 0;
    while (i <= haystack_len - needle_len) {
        size_t j = needle_len;
        while (j > 0 && needle[j - 1] == haystack[i + j - 1]) {
            --j;
        }

        if (j == 0) {
            return true;
        }

        i += skip[haystack[i + needle_len - 1]];
    }

    return false;
}

inline constexpr
int32 str_compare(const char* str1, const char* str2) NO_EXCEPT
{
    byte c1, c2;

    do {
        c1 = (byte) *str1++;
        c2 = (byte) *str2++;
    } while (c1 == c2 && c1 != '\0');

    return c1 - c2;
}

constexpr
int32 str_compare(const char* str1, const char* str2, size_t n) NO_EXCEPT
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

inline
int32 str_compare_caseless(const char* str1, const char* str2) NO_EXCEPT
{
    byte c1, c2;

    do {
        c1 = TO_LOWER_TABLE[(byte) *str1++];
        c2 = TO_LOWER_TABLE[(byte) *str2++];
    } while (c1 == c2 && c1 != '\0');

    return c1 - c2;
}

int32 str_compare_caseless(const char* str1, const char* str2, size_t n) NO_EXCEPT
{
    byte c1 = '\0';
    byte c2 = '\0';

    if (n >= 4) {
        size_t n4 = n >> 2;

        do {
            c1 = TO_LOWER_TABLE[(byte) *str1++];
            c2 = TO_LOWER_TABLE[(byte) *str2++];
            if (c1 == '\0' || c1 != c2) {
                return c1 - c2;
            }

            c1 = TO_LOWER_TABLE[(byte) *str1++];
            c2 = TO_LOWER_TABLE[(byte) *str2++];
            if (c1 == '\0' || c1 != c2) {
                return c1 - c2;
            }

            c1 = TO_LOWER_TABLE[(byte) *str1++];
            c2 = TO_LOWER_TABLE[(byte) *str2++];
            if (c1 == '\0' || c1 != c2) {
                return c1 - c2;
            }

            c1 = TO_LOWER_TABLE[(byte) *str1++];
            c2 = TO_LOWER_TABLE[(byte) *str2++];
            if (c1 == '\0' || c1 != c2) {
                return c1 - c2;
            }
        } while (--n4 > 0);

        n &= 3;
    }

    while (n > 0) {
        c1 = TO_LOWER_TABLE[(byte) *str1++];
        c2 = TO_LOWER_TABLE[(byte) *str2++];

        if (c1 == '\0' || c1 != c2) {
            return c1 - c2;
        }

        --n;
    }

    return c1 - c2;
}

inline constexpr
bool str_ends_with(const char* __restrict str, const char* __restrict suffix) NO_EXCEPT {
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
void str_replace(const char* str, const char* __restrict search, const char* __restrict replace, char* result) NO_EXCEPT {
    if (str == NULL || search == NULL || replace == NULL || result == NULL) {
        return;
    }

    size_t search_len = str_length(search);
    size_t replace_len = str_length(replace);

    if (search_len == 0) {
        str_copy(result, str);
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

    str_copy(result_ptr, str);
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

FORCE_INLINE constexpr
bool is_whitespace(char str) NO_EXCEPT
{
    return str == ' ' || str == '\t';
}

inline
int32 str_to_eol(const char* str) NO_EXCEPT
{
    const char* ptr = str;

    if (*ptr == '\0') return 0;

    // Process unaligned bytes first
    while ((uintptr_t) ptr % sizeof(size_t) != 0 && *ptr != '\0') {
        if (*ptr == '\n' || *ptr == '\r') {
            return (int32) (ptr - str);
        }

        ++ptr;
    }

    // Word-wise scanning
    const size_t* wptr = (const size_t *) ptr;

    const size_t nl_mask = ((size_t) -1 / 0xFF) * '\n';
    const size_t cr_mask = ((size_t) -1 / 0xFF) * '\r';

    while (true) {
        size_t v = *wptr;

        if (OMS_HAS_ZERO(v) || OMS_HAS_ZERO(v ^ nl_mask) || OMS_HAS_ZERO(v ^ cr_mask)) {
            // Fallback to byte-by-byte inside this word
            const char* cp = (const char*) wptr;
            for (size_t i = 0; i < sizeof(size_t); ++i) {
                if (cp[i] == '\n' || cp[i] == '\r' || cp[i] == '\0') {
                    return (int32) (cp + i - str);
                }
            }
        }

        ++wptr;
    }
}

inline
int32 str_to(const char* str, char delim) NO_EXCEPT
{
    int32_t offset = 0;
    const size_t delim_mask = ((size_t)-1 / 0xFF) * (uint8_t)delim;

    // Align to size_t
    while ((uintptr_t)str % sizeof(size_t) != 0 && *str != '\0' && *str != delim) {
        ++str;
        ++offset;
    }

    const size_t* wptr = (const size_t*)str;

    while (true) {
        size_t v = *wptr;
        // Check for zero or delimiter byte
        if (OMS_HAS_ZERO(v ^ delim_mask)) {
            break;
        }

        ++wptr;
        offset += sizeof(size_t);
    }

    // Scan remaining bytes byte-by-byte
    str = (const char*)wptr;
    while (*str != '\0' && *str != delim) {
        ++str;
        ++offset;
    }

    return offset;
}

inline
void str_move_to(const char** str, char delim) NO_EXCEPT
{
    const char* s = *str;

    // Handle unaligned prefix
    while ((uintptr_t)s % sizeof(size_t) != 0 && *s != '\0' && *s != delim) {
        ++s;
    }

    const size_t delim_mask = ((size_t)-1 / 0xFF) * (uint8_t)delim;
    const size_t* wptr = (const size_t*)s;

    while (true) {
        size_t v = *wptr;
        if (OMS_HAS_ZERO(v) || OMS_HAS_ZERO(v ^ delim_mask)) {
            break;
        }

        ++wptr;
    }

    // Fallback to byte scan
    s = (const char*) wptr;
    while (*s != '\0' && *s != delim) {
        ++s;
    }

    *str = s;
}

// Negative pos counts backwards
FORCE_INLINE
void str_move_to_pos(const char** str, int32 pos) NO_EXCEPT
{
    *str += pos >= 0
        ? pos
        : OMS_MAX(((int32) str_length(*str) + pos), 0);
}

inline
void str_move_past(const char* __restrict* __restrict str, char delim) NO_EXCEPT
{
    const char* s = *str;

    // Handle unaligned prefix
    while ((uintptr_t)s % sizeof(size_t) != 0 && *s != '\0' && *s != delim) {
        ++s;
    }

    const size_t delim_mask = ((size_t)-1 / 0xFF) * (uint8_t)delim;
    const size_t* wptr = (const size_t*)s;

    // Word-wise scan
    while (true) {
        size_t v = *wptr;
        if (OMS_HAS_ZERO(v) || OMS_HAS_ZERO(v ^ delim_mask)) {
            break;
        }

        ++wptr;
    }

    // Fallback to byte scan
    s = (const char*)wptr;
    while (*s != '\0' && *s != delim) {
        ++s;
    }

    // Move past delimiter if present
    if (*s == delim) {
        ++s;
    }

    *str = s;
}

inline
void str_move_past_alpha_num(const char** str) NO_EXCEPT
{
    while (str_is_alphanum(**str)
        || **str == 45 || **str == 95
    )  {
        ++(*str);
    }
}

inline
bool str_is_comment(const char* str) NO_EXCEPT
{
    return (*str == '/' && str[1] == '/') || (*str == '/' && str[1] == '*');
}

inline
void str_skip(const char** str, char delim) NO_EXCEPT
{
    while (**str && **str == delim)  {
        ++(*str);
    }
}

inline
void str_skip_whitespace(const char** str) NO_EXCEPT
{
    while (**str && (**str == ' ' || **str == '\t'))  {
        ++(*str);
    }
}

FORCE_INLINE
bool str_is_empty(const char str) NO_EXCEPT
{
    return str == ' ' || str == '\t' || str == '\n' || str == '\r';
}

FORCE_INLINE
bool str_is_eol(const char str) NO_EXCEPT
{
    return str == '\n' || str == '\r';
}

inline
void str_skip_empty(const char** str) NO_EXCEPT
{
    while (**str == ' ' || **str == '\t' || **str == '\n' || **str == '\r')  {
        ++(*str);
    }
}

inline
void str_skip_eol(const char** str) NO_EXCEPT
{
    while (**str == '\n' || **str == '\r')  {
        ++(*str);
    }
}

inline
void str_skip_non_empty(const char** str) NO_EXCEPT
{
    while (**str != ' ' && **str != '\t' && **str != '\n' && **str != '\0')  {
        ++(*str);
    }
}

inline
void str_skip_list(const char** __restrict str, const char* __restrict delim, int32 len) NO_EXCEPT
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
void str_skip_until_list(const char** __restrict str, const char* __restrict delim) NO_EXCEPT
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
void hexstr_to_rgba(v4_f32* __restrict rgba, const char* __restrict hex) NO_EXCEPT
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
void str_pad_right(const char* input, char* output, char pad, size_t len) NO_EXCEPT {
    size_t i = 0;
    for (; i < len && input[i] != '\0'; ++i) {
        output[i] = input[i];
    }

    for (; i < len; ++i) {
        output[i] = pad;
    }
}

inline
void str_pad_left(const char* input, char* output, char pad, size_t len) NO_EXCEPT {
    size_t input_len = str_length(input);

    size_t i = 0;
    for (; i < len - input_len; ++i) {
        *output++ = pad;
    }

    for (; i < len && input[i] != '\0'; ++i) {
        output[i] = input[i];
    }
}

inline
f32 str_to_float(const char* str, const char** pos = NULL) NO_EXCEPT
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
int32 float_to_str(f64 value, char* buffer, int32 precision = 5) NO_EXCEPT
{
    ASSERT_TRUE(precision < 6);

    char* start = buffer;

    if (value < 0) {
        *buffer++ = '-';
        value = -value;
    }

    static const float powers_of_ten[] = {
        1.0f, 10.0f, 100.0f, 1000.0f, 10000.0f, 100000.0f
    };

    f32 scale = powers_of_ten[precision];
    value = OMS_ROUND_POSITIVE_32(value * scale) / scale;

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
void format_time_hh_mm_ss_ms(char time_str[13], int32 hours, int32 minutes, int32 secs, int32 ms) NO_EXCEPT {
    time_str[0] = (char) ('0' + (hours / 10));
    time_str[1] = (char) ('0' + (hours % 10));
    time_str[2] = ':';
    time_str[3] = (char) ('0' + (minutes / 10));
    time_str[4] = (char) ('0' + (minutes % 10));
    time_str[5] = ':';
    time_str[6] = (char) ('0' + (secs / 10));
    time_str[7] = (char) ('0' + (secs % 10));
    time_str[8] = '.';
    time_str[9] = (char) ('0' + (ms / 100));
    time_str[10] = (char) ('0' + ((ms / 10) % 10));
    time_str[11] = (char) ('0' + (ms % 10));
    time_str[12] = '\0';
}

inline
void format_time_hh_mm_ss_ms(char time_str[13], uint64 ms) NO_EXCEPT {
    uint64 seconds = ms / 1000;
    int32 hours = (seconds / 3600) % 24;
    int32 minutes = (seconds / 60) % 60;
    int32 secs = seconds % 60;

    format_time_hh_mm_ss_ms(time_str, hours, minutes, secs, ms % 1000);
}

inline
void format_time_hh_mm_ss(char time_str[9], int32 hours, int32 minutes, int32 secs) NO_EXCEPT {
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
void format_time_hh_mm_ss(char time_str[9], uint64 seconds) NO_EXCEPT {
    int32 hours = (seconds / 3600) % 24;
    int32 minutes = (seconds / 60) % 60;
    int32 secs = seconds % 60;

    format_time_hh_mm_ss(time_str, hours, minutes, secs);
}

inline
void format_time_hh_mm(char time_str[6], int32 hours, int32 minutes) NO_EXCEPT {
    time_str[0] = (char) ('0' + (hours / 10));
    time_str[1] = (char) ('0' + (hours % 10));
    time_str[2] = ':';
    time_str[3] = (char) ('0' + (minutes / 10));
    time_str[4] = (char) ('0' + (minutes % 10));
    time_str[5] = '\0';
}

inline
void format_time_hh_mm(char time_str[6], uint64 seconds) NO_EXCEPT {
    int32 hours = (seconds / 3600) % 24;
    int32 minutes = (seconds / 60) % 60;

    format_time_hh_mm(time_str, hours, minutes);
}

void sprintf_fast(char* __restrict buffer, const char* __restrict format, ...) NO_EXCEPT {
    va_list args;
    va_start(args, format);

    while (*format) {
        if (*format == '\\' && format[1] == '%') {
            ++format;
            *buffer++ = *format;
        } else if (*format != '%') {
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

void sprintf_fast_s(char* __restrict buffer, size_t length, const char* __restrict format, ...) NO_EXCEPT {
    va_list args;
    va_start(args, format);

    char* start = buffer;

    while (*format) {
        size_t remainder = length - (buffer - start) - 1;
        if (remainder <= 0) {
            break;
        } else if (*format == '\\' && format[1] == '%') {
            ++format;
            *buffer++ = *format;
        } else if (*format != '%') {
            *buffer++ = *format;
        } else {
            ++format;

            switch (*format) {
                case 's': {
                    const char* str = va_arg(args, const char*);
                    while (*str && remainder) {
                        --remainder;
                        *buffer++ = *str++;
                    }
                } break;
                case 'c': {
                    *buffer++ = (char) va_arg(args, int32);
                } break;
                case 'n': {
                    int64 val = va_arg(args, int64);
                    int32 len = int_length(val);
                    if (remainder < len + len / 3 + 1) {
                        break;
                    }

                    buffer += int_to_str(val, buffer, ',');
                } break;
                case 'd': {
                    int32 val = va_arg(args, int32);
                    if (remainder < int_length(val)) {
                        break;
                    }

                    buffer += int_to_str(val, buffer);
                } break;
                case 'l': {
                    int64 val = va_arg(args, int64);
                    if (remainder < int_length(val)) {
                        break;
                    }

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

                    // @bug + 5 is a lazy way to handle the digits before the decimal
                    if (remainder < precision + 5) {
                        break;
                    }

                    buffer += float_to_str(val, buffer, precision);
                } break;
                case 'T': {
                     if (remainder < 8) {
                        break;
                    }

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

int32 sprintf_fast(char* __restrict buffer, int32 buffer_length, const char* __restrict format, ...) NO_EXCEPT {
    va_list args;
    va_start(args, format);

    // We start at 1 since we need 1 char for '\0'
    int32 length = 1;

    while (*format && length < buffer_length) {
        int32 offset = 1;
        if (*format == '\\' && format[1] == '%') {
            ++format;
            *buffer++ = *format;
        } else if (*format != '%') {
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

    return length - 1;
}

// There are situations where you only want to replace a certain amount of %
void sprintf_fast_iter(char* __restrict buffer, const char* __restrict format, ...) NO_EXCEPT {
    va_list args;
    va_start(args, format);

    int32 count_index = 0;

    while (*format) {
        if (*format == '\\' && format[1] == '%') {
            ++format;
            *buffer++ = *format;
        } else if (*format != '%' || count_index >= 1) {
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