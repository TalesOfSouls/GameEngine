/**
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_UTILS_SIMPLE_STRING_UTILS_H
#define COMS_UTILS_SIMPLE_STRING_UTILS_H

#include <stdarg.h>
#include "../stdlib/Stdlib.h"
#include "SimpleString.h"
#include "StringUtils.h"

// Creates a simple string from a string literal
template <typename C, size_t N>
CONSTEVAL
SimpleString<C> simple_string_literal(const C (&str)[N]) NO_EXCEPT
{
    return SimpleString<C>{
        (int32) (N - 1),
        CharTypeOf<C>::value,
        (C*) str
    };
}

FORCE_INLINE
SimpleString<char> simple_string_dynamic(char* str, int32 length) NO_EXCEPT
{
    return SimpleString<char>{
        length,
        CharTypeOf<char>::value,
        str
    };
}

template <typename C>
FORCE_INLINE CONSTEXPR
SimpleString<C> simple_string_init(C* str, int32 length) NO_EXCEPT
{
    return SimpleString<C>{ length, CharTypeOf<C>::value, str };
}

template <typename C>
FORCE_INLINE CONSTEXPR
SimpleString<C> simple_string_empty_init() NO_EXCEPT
{
    return SimpleString<C>{ 0, CharTypeOf<C>::value, NULL };
}

template <typename C>
CONSTEXPR FORCE_INLINE
SimpleString<C> simple_string_literal(SimpleString<C> str) NO_EXCEPT
{
    return str;
}

template <typename C>
FORCE_INLINE CONSTEXPR
size_t str_length(const SimpleString<C>& str) NO_EXCEPT
{
    return (size_t) str.length;
}

static inline
int32 utf8_decode_bounded(const char* __restrict in, int32 remaining, uint32* __restrict codepoint) NO_EXCEPT
{
    if (remaining <= 0) {
        return 0;
    }

    const byte ch = (const byte) *in;
    int32 needed;

    if (ch <= 0x7F) {
        needed = 1;
    } else if ((ch & 0xE0) == 0xC0) {
        needed = 2;
    } else if ((ch & 0xF0) == 0xE0) {
        needed = 3;
    } else if ((ch & 0xF8) == 0xF0) {
        needed = 4;
    } else {
        return 0;
    }

    if (needed > remaining) {
        return -1;
    }

    return utf8_decode(in, codepoint);
}

inline
int32 utf8_strlen(const SimpleString<char>& in) NO_EXCEPT
{
    int32 length = 0;
    int32 pos = 0;
    uint32 codepoint;

    while (pos < in.length) {
        const int32 bytes = utf8_decode_bounded(in.str + pos, in.length - pos, &codepoint);
        if (bytes <= 0) {
            return -1;
        }

        pos += bytes;
        ++length;
    }

    return length;
}

inline
SimpleString<char> string_to_utf8(const SimpleString<uint32>& in, char* __restrict out) NO_EXCEPT
{
    char* const start = out;
    char buffer[5] = {0};

    for (int32 i = 0; i < in.length; ++i) {
        const int32 len = utf8_encode(in.str[i], buffer);
        if (len < 0) {
            continue;
        }

        for (int32 j = 0; j < len; ++j) {
            *out++ = buffer[j];
        }
    }

    return simple_string_init(start, (int32) (out - start));
}

inline
int32 utf8_get_char_at(const SimpleString<char>& in, int32 index) NO_EXCEPT
{
    int32 i = 0;
    int32 pos = 0;
    uint32 codepoint;

    while (pos < in.length) {
        const int32 bytes = utf8_decode_bounded(in.str + pos, in.length - pos, &codepoint);
        if (bytes <= 0) {
            return -1;
        } else if (i == index) {
            return (int32) codepoint;
        }

        ++i;
        pos += bytes;
    }

    return -1;
}

template <typename C>
inline
C* str_right(const SimpleString<C>& str, C c) NO_EXCEPT
{
    for (int32 i = str.length - 1; i >= 0; --i) {
        if (str.str[i] == c) {
            return str.str + i;
        }
    }

    return NULL;
}

template <typename C>
FORCE_INLINE
void str_copy(C* __restrict destination, const SimpleString<C>* const __restrict str) NO_EXCEPT
{
    memcpy(destination, str->str, sizeof(C) * str->length);
}

template <typename C>
FORCE_INLINE
void str_copy(C* __restrict destination, const SimpleString<C>& str) NO_EXCEPT
{
    memcpy(destination, str.str, sizeof(C) * str.length);
}

template <typename C>
FORCE_INLINE
void str_copy(C* __restrict destination, const SimpleString<const C>* const __restrict str) NO_EXCEPT
{
    memcpy(destination, str->str, sizeof(C) * str->length);
}

template <typename C>
FORCE_INLINE
void str_copy(C* __restrict destination, const SimpleString<const C>& str) NO_EXCEPT
{
    memcpy(destination, str.str, sizeof(C) * str.length);
}

inline CONSTEXPR
bool str_is_alpha(const SimpleString<char>& str) NO_EXCEPT
{
    for (int32 i = 0; i < str.length; ++i) {
        if (!isalpha((unsigned char) str.str[i])) {
            return false;
        }
    }

    return true;
}

inline CONSTEXPR
bool str_is_num(const SimpleString<char>& str) NO_EXCEPT
{
    for (int32 i = 0; i < str.length; ++i) {
        if (!isdigit((unsigned char) str.str[i])) {
            return false;
        }
    }

    return true;
}

inline
bool str_is_alphanum(const SimpleString<char>& str) NO_EXCEPT
{
    for (int32 i = 0; i < str.length; ++i) {
        if (!isalnum((unsigned char) str.str[i])) {
            return false;
        }
    }

    return true;
}

inline CONSTEXPR
bool str_starts_with_num(const SimpleString<char>& str) NO_EXCEPT
{
    int32 i = 0;

    if (i < str.length && (str.str[i] == '-' || str.str[i] == '+')) {
        ++i;
    }

    bool decimal = false;
    while (i < str.length && str.str[i] != ' ') {
        if (!isdigit((unsigned char) str.str[i]) || (decimal && str.str[i] == '.')) {
            return false;
        }

        if (str.str[i] == '.') {
            decimal = true;
        }

        ++i;
    }

    return true;
}

inline
bool str_is_float(const SimpleString<char>& str) NO_EXCEPT
{
    bool has_dot = false;
    int32 i = 0;

    if (i < str.length && (str.str[i] == '-' || str.str[i] == '+')) {
        ++i;
    }

    for (; i < str.length; ++i) {
        if (str.str[i] == '.') {
            if (has_dot) {
                return false;
            }

            has_dot = true;
        } else if (!isdigit((unsigned char) str.str[i])) {
            return false;
        }
    }

    return has_dot;
}

inline
bool str_is_integer(const SimpleString<char>& str) NO_EXCEPT
{
    int32 i = 0;

    if (i < str.length && (str.str[i] == '-' || str.str[i] == '+')) {
        ++i;
    }

    bool is_int = false;
    for (; i < str.length; ++i) {
        if (!isdigit((unsigned char) str.str[i])) {
            return false;
        }

        is_int = true;
    }

    return is_int;
}

inline
bool str_is_hex_color(const SimpleString<char>& str) NO_EXCEPT
{
    if (str.length < 1 || str.str[0] != '#') {
        return false;
    }

    for (int32 i = 1; i < str.length; ++i) {
        if (!HEX_LOOKUP_TABLE[(byte) str.str[i]]) {
            return false;
        }
    }

    return true;
}

inline
void str_toupper(SimpleString<char>* str) NO_EXCEPT
{
    for (int32 i = 0; i < str->length; ++i) {
        str->str[i] = (char) toupper((unsigned char) str->str[i]);
    }
}

inline
void str_tolower(SimpleString<char>* str) NO_EXCEPT
{
    for (int32 i = 0; i < str->length; ++i) {
        str->str[i] = (char) tolower((unsigned char) str->str[i]);
    }
}

inline CONSTEXPR
SimpleString<wchar_t> char_to_wchar(
    wchar_t* __restrict dest,
    const SimpleString<char>& src,
    int32 max_wchars
) NO_EXCEPT
{
    int32 written = 0;
    int32 i = 0;

    while (written < max_wchars - 1 && i < src.length) {
        dest[written++] = (wchar_t) (unsigned char) src.str[i++];
    }

    dest[written] = 0;

    return simple_string_init(dest, written);
}

inline
SimpleString<char> wchar_to_char(SimpleString<wchar_t>* str) NO_EXCEPT
{
    ASSERT_TRUE_CONST(sizeof(wchar_t) == 2 || sizeof(wchar_t) == 4);

    char* dest = (char *) str->str;
    char* const start = dest;

    for (int32 i = 0; i < str->length; ++i) {
        const wchar_t wc = str->str[i];

        IF_CONSTEXPR(sizeof(wchar_t) == 2) {
            const byte low  = (byte) (wc & 0xFF);
            const byte high = (byte) ((wc >> 8) & 0xFF);

            if (low) {
                *dest++ = (char) low;
            }

            if (high) {
                *dest++ = (char) high;
            }
        } else {
            for (int32 j = 0; j < 4; ++j) {
                const byte b = (byte) ((wc >> (8 * j)) & 0xFF);
                if (b) {
                    *dest++ = (char) b;
                }
            }
        }
    }

    return simple_string_init(start, (int32) (dest - start));
}

inline
SimpleString<char> wchar_to_char(char* __restrict dest, const SimpleString<wchar_t>& str) NO_EXCEPT
{
    ASSERT_TRUE_CONST(sizeof(wchar_t) == 2 || sizeof(wchar_t) == 4);

    char* const start = dest;

    for (int32 i = 0; i < str.length; ++i) {
        const wchar_t wc = str.str[i];

        IF_CONSTEXPR(sizeof(wchar_t) == 2) {
            const byte low  = (byte) (wc & 0xFF);
            const byte high = (byte) ((wc >> 8) & 0xFF);

            if (low) {
                *dest++ = (char) low;
            }

            if (high) {
                *dest++ = (char) high;
            }
        } else {
            for (int32 j = 0; j < 4; ++j) {
                const byte b = (byte) ((wc >> (8 * j)) & 0xFF);
                if (b) {
                    *dest++ = (char) b;
                }
            }
        }
    }

    return simple_string_init(start, (int32) (dest - start));
}

inline
SimpleString<char> wchar_to_char(
    char* __restrict dest,
    const SimpleString<wchar_t>& str,
    int32 max_length
) NO_EXCEPT
{
    ASSERT_TRUE_CONST(sizeof(wchar_t) == 2 || sizeof(wchar_t) == 4);

    char* const start = dest;
    int32 i = 0;
    int32 written = 0;

    while (i < str.length && written < max_length) {
        const wchar_t wc = str.str[i++];

        if constexpr (sizeof(wchar_t) == 2) {
            const byte low  = (byte) (wc & 0xFF);
            const byte high = (byte) ((wc >> 8) & 0xFF);

            if (low && written < max_length) {
                *dest++ = (char) low;
                ++written;
            }

            if (high && written < max_length) {
                *dest++ = (char) high;
                ++written;
            }
        } else {
            for (int32 j = 0; j < 4 && written < max_length; ++j) {
                const byte b = (byte) ((wc >> (8 * j)) & 0xFF);
                if (b) {
                    *dest++ = (char) b;
                    ++written;
                }
            }
        }
    }

    return simple_string_init(start, written);
}

inline
SimpleString<wchar_t> utf8_to_wchar(const SimpleString<char>& in, wchar_t* out, int32 max_length) NO_EXCEPT
{
    int32 pos = 0;
    int32 out_pos = 0;

    while (pos < in.length && out_pos < max_length - 1) {
        const unsigned char c = (unsigned char) in.str[pos];

        if (c < 0x80) {
            out[out_pos++] = (wchar_t) c;
            ++pos;
            continue;
        }

        uint32 codepoint;
        int32 bytes;

        if ((c >> 5) == 0x6 && pos + 1 < in.length) {
            bytes = 2;
        } else if ((c >> 4) == 0xE && pos + 2 < in.length) {
            bytes = 3;
        } else if ((c >> 3) == 0x1E && pos + 3 < in.length) {
            bytes = 4;
        } else {
            return simple_string_empty_init<wchar_t>();
        }

        const int32 consumed = utf8_decode(in.str + pos, &codepoint);
        if (consumed != bytes) {
            return simple_string_empty_init<wchar_t>();
        }

        pos += bytes;

        if (bytes == 4) {
            if (out_pos + 2 >= max_length) {
                return simple_string_empty_init<wchar_t>();
            }

            const uint32 cp = codepoint - 0x10000;
            out[out_pos++] = (wchar_t) (0xD800 + (cp >> 10));
            out[out_pos++] = (wchar_t) (0xDC00 + (cp & 0x3FF));
        } else {
            out[out_pos++] = (wchar_t) codepoint;
        }
    }

    out[out_pos] = 0;

    return simple_string_init(out, out_pos);
}

inline
int64 str_to_int(const SimpleString<char>& str, int32* consumed = NULL) NO_EXCEPT
{
    int32 i = 0;
    int64 sign = 1;

    if (i < str.length && str.str[i] == '-') {
        sign = -1;
        ++i;
    }

    int64 result = 0;
    while (i < str.length && isdigit((unsigned char) str.str[i])) {
        result = result * 10 + (str.str[i] - '0');
        ++i;
    }

    if (consumed) {
        *consumed = i;
    }

    return result * sign;
}

inline
f32 str_to_float(const SimpleString<char>& str, int32* consumed = NULL) NO_EXCEPT
{
    int32 i = 0;
    f32 result = 0.0f;
    int32 sign = 1;

    while (i < str.length && is_whitespace(str.str[i])) {
        ++i;
    }

    if (i < str.length && (str.str[i] == '+' || str.str[i] == '-')) {
        sign = (str.str[i] == '-') ? -1 : 1;
        ++i;
    }

    while (i < str.length && isdigit((unsigned char) str.str[i])) {
        result = result * 10.0f + (str.str[i] - '0');
        ++i;
    }

    if (i < str.length && str.str[i] == '.') {
        int32 decimals = 0;
        ++i;

        while (i < str.length && isdigit((unsigned char) str.str[i])) {
            result = result * 10.0f + (str.str[i] - '0');
            ++decimals;
            ++i;
        }

        static const float powers_of_ten[] = {
            1.0f, 10.0f, 100.0f, 1000.0f, 10000.0f, 100000.0f, 1000000.0f
        };

        result /= powers_of_ten[decimals];
    }

    if (consumed) {
        *consumed = i;
    }

    return sign * result;
}

inline CONSTEXPR
int64 hex_to_int(const SimpleString<char>& hex) NO_EXCEPT
{
    int64 result = 0;

    for (int32 i = 0; i < hex.length && HEX_LOOKUP_TABLE[(byte) hex.str[i]]; ++i) {
        byte value = (byte) hex.str[i];

        if (isdigit(value)) {
            value = value - '0';
        } else if (value >= 'A' && value <= 'F') {
            value = value - 'A' + 10;
        } else {
            value = value - 'a' + 10;
        }

        result = (result << 4) | (value & 0xF);
    }

    return result;
}

template <typename T>
inline
SimpleString<T> int_to_simple_str(int64 number, T str[15], T thousands) NO_EXCEPT
{
    return simple_string_init(str, int_to_str(number, str, thousands));
}

template <typename T>
inline CONSTEXPR
SimpleString<T> int_to_simple_str(int64 number, T str[12]) NO_EXCEPT
{
    return simple_string_init(str, int_to_str(number, str));
}

template <typename T>
inline CONSTEXPR
SimpleString<T> int_to_simple_str(uint64 number, T str[12]) NO_EXCEPT
{
    return simple_string_init(str, int_to_str(number, str));
}

template <typename T>
inline CONSTEXPR
SimpleString<T> int_to_simple_str(int32 number, T str[12]) NO_EXCEPT
{
    return simple_string_init(str, int_to_str(number, str));
}

template <typename T>
inline CONSTEXPR
SimpleString<T> int_to_simple_str(uint32 number, T str[12]) NO_EXCEPT
{
    return simple_string_init(str, int_to_str(number, str));
}

/**
 * @param char* str Output string needs to be 9 or 17 chars depending on integer bit size
 */
template <typename T>
inline CONSTEXPR
SimpleString<char> int_to_simple_hex(T number, char* str) NO_EXCEPT
{
    return simple_string_init(str, int_to_hex(number, str));
}

template <typename T>
inline
SimpleString<T> bytes_to_simple_hex(const byte* data, size_t len, T* out) NO_EXCEPT
{
    bytes_to_hex(data, len, out);
    return simple_string_init(out, (int32) (len * 2));
}

template <typename T>
inline
SimpleString<T> float_to_simple_str(f64 value, T* buffer, int32 precision = 5) NO_EXCEPT
{
    return simple_string_init(buffer, float_to_str(value, buffer, precision));
}

template <typename T>
inline
SimpleString<T> format_time_hh_mm_ss_ms_simple_str(T time_str[13], int32 hours, int32 minutes, int32 secs, int32 ms) NO_EXCEPT
{
    format_time_hh_mm_ss_ms(time_str, hours, minutes, secs, ms);
    return simple_string_init(time_str, 12);
}

template <typename T>
inline
SimpleString<T> format_time_hh_mm_ss_ms_simple_str(T time_str[13], uint64 ms) NO_EXCEPT
{
    format_time_hh_mm_ss_ms(time_str, ms);
    return simple_string_init(time_str, 12);
}

template <typename T>
inline
SimpleString<T> format_time_hh_mm_ss_simple_str(T time_str[9], int32 hours, int32 minutes, int32 secs) NO_EXCEPT
{
    format_time_hh_mm_ss(time_str, hours, minutes, secs);
    return simple_string_init(time_str, 8);
}

template <typename T>
inline
SimpleString<T> format_time_hh_mm_ss_simple_str(T time_str[9], uint64 seconds) NO_EXCEPT
{
    format_time_hh_mm_ss(time_str, seconds);
    return simple_string_init(time_str, 8);
}

template <typename T>
inline
SimpleString<T> format_time_hh_mm_simple_str(T time_str[6], int32 hours, int32 minutes) NO_EXCEPT
{
    format_time_hh_mm(time_str, hours, minutes);
    return simple_string_init(time_str, 5);
}

template <typename T>
inline
SimpleString<T> format_time_hh_mm_simple_str(T time_str[6], uint64 seconds) NO_EXCEPT
{
    format_time_hh_mm(time_str, seconds);
    return simple_string_init(time_str, 5);
}

template <typename C>
inline
SimpleString<C> str_copy_until_view(const SimpleString<C>& src, C delim) NO_EXCEPT
{
    int32 i = 0;
    while (i < src.length && src.str[i] != delim) {
        ++i;
    }

    return simple_string_init(src.str, i);
}

template <typename C>
inline
SimpleString<C> str_copy_until_view(const SimpleString<C>& src, const SimpleString<C>& delim) NO_EXCEPT
{
    int32 i = 0;
    while (i < src.length) {
        bool is_delim = false;
        for (int32 j = 0; j < delim.length; ++j) {
            if (src.str[i] == delim.str[j]) {
                is_delim = true;
                break;
            }
        }

        if (is_delim) {
            break;
        }

        ++i;
    }

    return simple_string_init(src.str, i);
}

template <typename C>
inline
SimpleString<C> str_copy_until(C* __restrict dest, const SimpleString<C>& src, C delim) NO_EXCEPT
{
    const SimpleString<C> view = str_copy_until_view(src, delim);
    memcpy(dest, view.str, sizeof(C) * view.length);

    return simple_string_init(dest, view.length);
}

template <typename C>
inline
SimpleString<C> str_copy_until(C* __restrict dest, const SimpleString<C>& src, const SimpleString<C>& delim) NO_EXCEPT
{
    const SimpleString<C> view = str_copy_until_view(src, delim);
    memcpy(dest, view.str, sizeof(C) * view.length);

    return simple_string_init(dest, view.length);
}

template <typename C>
inline
SimpleString<C> str_copy_move_until(SimpleString<C>* src, C delim) NO_EXCEPT
{
    const SimpleString<C> head = str_copy_until_view(*src, delim);

    src->str += head.length;
    src->length -= head.length;

    return head;
}

template <typename C>
inline
SimpleString<C> str_copy_move_until(SimpleString<C>* src, const SimpleString<C>& delim) NO_EXCEPT
{
    const SimpleString<C> head = str_copy_until_view(*src, delim);

    src->str += head.length;
    src->length -= head.length;

    return head;
}

inline
int32 is_eol(const SimpleString<char>& str, int32 pos) NO_EXCEPT
{
    if (pos >= str.length) {
        return 0;
    }

    if (str.str[pos] == '\n') {
        return 1;
    } else if (str.str[pos] == '\r' && pos + 1 < str.length && str.str[pos + 1] == '\n') {
        return 2;
    }

    return 0;
}

inline
SimpleString<char> str_copy_to_eol_view(const SimpleString<char>& src) NO_EXCEPT
{
    int32 i = 0;
    while (i < src.length && !is_eol(src, i)) {
        ++i;
    }

    return simple_string_init(src.str, i);
}

inline
SimpleString<char> str_copy_to_eol(const SimpleString<char>& src, char* __restrict dst) NO_EXCEPT
{
    const SimpleString<char> view = str_copy_to_eol_view(src);
    memcpy(dst, view.str, view.length);

    return simple_string_init(dst, view.length);
}

inline
SimpleString<char> str_copy_to_empty_view(const SimpleString<char>& src) NO_EXCEPT
{
    int32 i = 0;
    while (i < src.length && !is_eol(src, i) && src.str[i] != ' ') {
        ++i;
    }

    return simple_string_init(src.str, i);
}

inline
SimpleString<char> str_copy_to_empty(const SimpleString<char>& src, char* __restrict dst) NO_EXCEPT
{
    const SimpleString<char> view = str_copy_to_empty_view(src);
    memcpy(dst, view.str, view.length);

    return simple_string_init(dst, view.length);
}

inline
SimpleString<char> strsep(SimpleString<char>* sp, const SimpleString<char>& sep) NO_EXCEPT
{
    if (sp == NULL || sp->length == 0) {
        return simple_string_empty_init<char>();
    }

    int32 i = 0;
    while (i < sp->length) {
        bool is_sep = false;
        for (int32 j = 0; j < sep.length; ++j) {
            if (sp->str[i] == sep.str[j]) {
                is_sep = true;
                break;
            }
        }

        if (is_sep) {
            break;
        }

        ++i;
    }

    const SimpleString<char> token = simple_string_init(sp->str, i);
    const int32 advance = (i < sp->length) ? i + 1 : i;

    sp->str += advance;
    sp->length -= advance;

    return token;
}

inline
SimpleString<char> strtok(SimpleString<char>* key, const SimpleString<char>& delim) NO_EXCEPT
{
    int32 i = 0;

    while (i < key->length) {
        bool is_delim = false;
        for (int32 j = 0; j < delim.length; ++j) {
            if (key->str[i] == delim.str[j]) {
                is_delim = true;
                break;
            }
        }

        if (!is_delim) {
            break;
        }

        ++i;
    }

    key->str += i;
    key->length -= i;

    if (key->length == 0) {
        return simple_string_empty_init<char>();
    }

    int32 token_length = 0;
    while (token_length < key->length) {
        bool is_delim = false;
        for (int32 j = 0; j < delim.length; ++j) {
            if (key->str[token_length] == delim.str[j]) {
                is_delim = true;
                break;
            }
        }

        if (is_delim) {
            break;
        }

        ++token_length;
    }

    const SimpleString<char> token = simple_string_init(key->str, token_length);

    key->str += token_length;
    key->length -= token_length;

    return token;
}

template <typename C>
inline
SimpleString<C> str_concat_new(
    C* __restrict dst,
    const SimpleString<C>& src1,
    const SimpleString<C>& src2
) NO_EXCEPT
{
    C* const start = dst;

    memcpy(dst, src1.str, sizeof(C) * src1.length);
    dst += src1.length;

    memcpy(dst, src2.str, sizeof(C) * src2.length);
    dst += src2.length;

    return simple_string_init(start, (int32) (dst - start));
}

template <typename C>
inline
SimpleString<C> str_concat_new(
    C* __restrict dst,
    const SimpleString<C>& src1,
    const SimpleString<C>& src2,
    const SimpleString<C>& src3
) NO_EXCEPT
{
    C* const start = dst;

    memcpy(dst, src1.str, sizeof(C) * src1.length);
    dst += src1.length;

    memcpy(dst, src2.str, sizeof(C) * src2.length);
    dst += src2.length;

    memcpy(dst, src3.str, sizeof(C) * src3.length);
    dst += src3.length;

    return simple_string_init(start, (int32) (dst - start));
}

inline
SimpleString<char> str_concat_new(char* __restrict dst, const SimpleString<char>& src, int64 data) NO_EXCEPT
{
    memcpy(dst, src.str, src.length);
    const int32 len = int_to_str(data, dst + src.length);

    return simple_string_init(dst, src.length + len);
}

inline
void strcat(SimpleString<char>* __restrict dst, const SimpleString<char>& src) NO_EXCEPT
{
    memcpy(dst->str + dst->length, src.str, src.length);
    dst->length += src.length;
}

inline
void strcat(SimpleString<char>* __restrict dst, int64 data) NO_EXCEPT
{
    const int32 len = int_to_str(data, dst->str + dst->length);
    dst->length += len;
}

template <typename C>
inline
void str_insert(SimpleString<C>* __restrict dst, size_t insert_pos, const SimpleString<C>& src) NO_EXCEPT
{
    memmove(
        dst->str + insert_pos + src.length,
        dst->str + insert_pos,
        sizeof(C) * (dst->length - (int32) insert_pos)
    );
    memcpy(dst->str + insert_pos, src.str, sizeof(C) * src.length);

    dst->length += src.length;
}

template <typename C>
inline
void str_remove(SimpleString<C>* __restrict dst, size_t remove_pos, size_t remove_length) NO_EXCEPT
{
    memmove(
        dst->str + remove_pos,
        dst->str + remove_pos + remove_length,
        sizeof(C) * (dst->length - (int32) (remove_pos + remove_length))
    );

    dst->length -= (int32) remove_length;
}

template <typename C>
inline CONSTEXPR
bool str_contains(const SimpleString<C>& haystack, const SimpleString<C>& needle) NO_EXCEPT
{
    if (needle.length == 0) {
        return true;
    }

    if (needle.length > haystack.length) {
        return false;
    }

    const C first = needle.str[0];

    for (int32 i = 0; i <= haystack.length - needle.length; ++i) {
        if (haystack.str[i] != first) {
            continue;
        }

        int32 j = 1;
        while (j < needle.length && haystack.str[i + j] == needle.str[j]) {
            ++j;
        }

        if (j == needle.length) {
            return true;
        }
    }

    return false;
}

FORCE_INLINE
bool str_contains_fast(const SimpleString<char>& haystack, const SimpleString<char>& needle) NO_EXCEPT
{
    if (needle.length == 0) {
        return true;
    }

    if (needle.length > haystack.length) {
        return false;
    }

    return byte_contains(
        (const byte *) haystack.str, (size_t) haystack.length,
        (const byte *) needle.str, (size_t) needle.length
    );
}

inline
size_t str_count(const SimpleString<char>& str, const SimpleString<char>& substr) NO_EXCEPT
{
    if (substr.length == 0 || substr.length > str.length) {
        return 0;
    }

    size_t count = 0;
    const char first = substr.str[0];

    int32 i = 0;
    while (i <= str.length - substr.length) {
        if (str.str[i] == first) {
            int32 j = 1;
            while (j < substr.length && str.str[i + j] == substr.str[j]) {
                ++j;
            }

            if (j == substr.length) {
                ++count;
                i += substr.length;
                continue;
            }
        }

        ++i;
    }

    return count;
}

inline
int32 str_compare_caseless(const SimpleString<char>& str1, const SimpleString<char>& str2) NO_EXCEPT
{
    const int32 n = str1.length < str2.length ? str1.length : str2.length;

    for (int32 i = 0; i < n; ++i) {
        const byte c1 = TO_LOWER_TABLE[(byte) str1.str[i]];
        const byte c2 = TO_LOWER_TABLE[(byte) str2.str[i]];

        if (c1 != c2) {
            return c1 - c2;
        }
    }

    return str1.length - str2.length;
}

template <typename C>
FORCE_INLINE
bool str_equals(const SimpleString<C>& str1, const SimpleString<C>& str2) NO_EXCEPT
{
    return str1.length == str2.length
        && memcmp(str1.str, str2.str, sizeof(C) * str1.length) == 0;
}

inline
bool str_equals_caseless(const SimpleString<char>& str1, const SimpleString<char>& str2) NO_EXCEPT
{
    if (str1.length != str2.length) {
        return false;
    }

    for (int32 i = 0; i < str1.length; ++i) {
        if (TO_LOWER_TABLE[(byte) str1.str[i]] != TO_LOWER_TABLE[(byte) str2.str[i]]) {
            return false;
        }
    }

    return true;
}

template <typename C>
inline CONSTEXPR
bool str_ends_with(const SimpleString<C>& str, const SimpleString<C>& suffix) NO_EXCEPT
{
    if (suffix.length > str.length) {
        return false;
    }

    return memcmp(str.str + (str.length - suffix.length), suffix.str, sizeof(C) * suffix.length) == 0;
}

template <typename C>
inline CONSTEXPR
bool str_starts_with(const SimpleString<C>& str, const SimpleString<C>& prefix) NO_EXCEPT
{
    if (prefix.length > str.length) {
        return false;
    }

    return memcmp(str.str, prefix.str, sizeof(C) * prefix.length) == 0;
}

inline
SimpleString<char> str_replace(
    const SimpleString<char>& str,
    const SimpleString<char>& search,
    const SimpleString<char>& replace,
    char* __restrict result
) NO_EXCEPT
{
    char* const result_start = result;

    if (search.length == 0) {
        memcpy(result, str.str, str.length);
        return simple_string_init(result_start, str.length);
    }

    int32 i = 0;
    while (i < str.length) {
        const bool matched = i <= str.length - search.length
            && memcmp(str.str + i, search.str, search.length) == 0;

        if (matched) {
            memcpy(result, replace.str, replace.length);
            result += replace.length;
            i += search.length;
        } else {
            *result++ = str.str[i++];
        }
    }

    return simple_string_init(result_start, (int32) (result - result_start));
}

inline
int32 str_to_eol(const SimpleString<char>& str) NO_EXCEPT
{
    int32 i = 0;
    while (i < str.length && str.str[i] != '\n' && str.str[i] != '\r') {
        ++i;
    }

    return i;
}

inline
void str_move_to(SimpleString<char>* str, char delim) NO_EXCEPT
{
    int32 i = 0;
    while (i < str->length && str->str[i] != delim) {
        ++i;
    }

    str->str += i;
    str->length -= i;
}

inline
void str_move_to(SimpleString<char>* str, const SimpleString<char>& delim_set) NO_EXCEPT
{
    int32 i = 0;
    while (i < str->length) {
        bool found = false;
        for (int32 j = 0; j < delim_set.length; ++j) {
            if (str->str[i] == delim_set.str[j]) {
                found = true;
                break;
            }
        }

        if (found) {
            break;
        }

        ++i;
    }

    str->str += i;
    str->length -= i;
}

inline
void str_move_past(SimpleString<char>* str, char delim) NO_EXCEPT
{
    str_move_to(str, delim);
    if (str->length > 0) {
        ++str->str;
        --str->length;
    }
}

// Negative pos counts backwards from the end of str.
FORCE_INLINE
void str_move_to_pos(SimpleString<char>* str, int32 pos) NO_EXCEPT
{
    const int32 offset = pos >= 0
        ? pos
        : OMS_MAX(str->length + pos, 0);

    str->str += offset;
    str->length -= offset;
}

inline
void str_move_past_alpha_num(SimpleString<char>* str) NO_EXCEPT
{
    int32 i = 0;
    while (i < str->length
        && (isalnum((unsigned char) str->str[i]) || str->str[i] == 45 || str->str[i] == 95)
    ) {
        ++i;
    }

    str->str += i;
    str->length -= i;
}

FORCE_INLINE
bool str_is_comment(const SimpleString<char>& str) NO_EXCEPT
{
    return str.length >= 2 && str.str[0] == '/' && (str.str[1] == '/' || str.str[1] == '*');
}

inline
void str_skip(SimpleString<char>* str, char delim) NO_EXCEPT
{
    int32 i = 0;
    while (i < str->length && str->str[i] == delim) {
        ++i;
    }

    str->str += i;
    str->length -= i;
}

inline
void str_skip_whitespace(SimpleString<char>* str) NO_EXCEPT
{
    int32 i = 0;
    while (i < str->length && is_whitespace(str->str[i])) {
        ++i;
    }

    str->str += i;
    str->length -= i;
}

inline
void str_skip_eol(SimpleString<char>* str) NO_EXCEPT
{
    int32 i = 0;
    while (i < str->length && str_is_eol(str->str[i])) {
        ++i;
    }

    str->str += i;
    str->length -= i;
}

inline
void str_skip_non_empty(SimpleString<char>* str) NO_EXCEPT
{
    int32 i = 0;
    while (i < str->length && str->str[i] != ' ' && str->str[i] != '\t' && str->str[i] != '\n') {
        ++i;
    }

    str->str += i;
    str->length -= i;
}

inline
void str_skip_empty(SimpleString<char>* str) NO_EXCEPT
{
    int32 i = 0;
    while (i < str->length && str_is_empty(str->str[i])) {
        ++i;
    }

    str->str += i;
    str->length -= i;
}

inline
void str_skip_line(SimpleString<char>* str) NO_EXCEPT
{
    int32 i = 0;
    while (i < str->length && str->str[i] != '\n') {
        ++i;
    }

    if (i < str->length) {
        ++i; // consume the \n
    }

    str->str += i;
    str->length -= i;
}

inline
void str_skip_list(SimpleString<char>* str, const SimpleString<char>& delim) NO_EXCEPT
{
    bool run = true;
    while (run && str->length > 0) {
        run = false;

        for (int32 i = 0; i < delim.length; ++i) {
            if (str->str[0] == delim.str[i]) {
                run = true;
                ++str->str;
                --str->length;
                break;
            }
        }
    }
}

inline
void hexstr_to_rgba(v4_f32* __restrict rgba, const SimpleString<char>& hex) NO_EXCEPT
{
    SimpleString<char> h = hex;
    if (h.length > 0 && h.str[0] == '#') {
        ++h.str;
        --h.length;
    }

    const uint32 value = (uint32) hex_to_int(h);
    rgba->r = (f32) ((value >> 24) & 0xFF) / 255.0f;
    rgba->g = (f32) ((value >> 16) & 0xFF) / 255.0f;
    rgba->b = (f32) ((value >> 8) & 0xFF) / 255.0f;
    rgba->a = (f32) (value & 0xFF) / 255.0f;
}

inline CONSTEXPR
SimpleString<char> str_pad_right(const SimpleString<char>& input, char* __restrict output, char pad, size_t len) NO_EXCEPT
{
    size_t i = 0;
    for (; i < len && (int32) i < input.length; ++i) {
        output[i] = input.str[i];
    }

    for (; i < len; ++i) {
        output[i] = pad;
    }

    return simple_string_init(output, (int32) len);
}

inline
SimpleString<char> str_pad_left(const SimpleString<char>& input, char* __restrict output, char pad, size_t len) NO_EXCEPT
{
    const size_t input_len = (size_t) input.length;
    const size_t pad_len = len > input_len ? len - input_len : 0;

    size_t i = 0;
    for (; i < pad_len; ++i) {
        output[i] = pad;
    }

    for (size_t j = 0; i < len && j < input_len; ++i, ++j) {
        output[i] = input.str[j];
    }

    return simple_string_init(output, (int32) len);
}

template <typename T>
SimpleString<T> sprintf_fast(T* __restrict buffer, int32 buffer_capacity, SimpleString<T> format, ...) NO_EXCEPT
{
    va_list args;
    va_start(args, format);

    T* const start = buffer;

    // reserve 1 slot for '\0'
    T* const limit = buffer + buffer_capacity - 1;

    int32 fi = 0;

    while (fi < format.length && buffer < limit) {
        const T ch = format.str[fi];

        if (ch == T('\\') && fi + 1 < format.length && format.str[fi + 1] == T('%')) {
            ++fi;
            *buffer++ = format.str[fi];
        } else if (ch != T('%')) {
            *buffer++ = ch;
        } else {
            ++fi;
            const T spec = (fi < format.length) ? format.str[fi] : T('\0');

            switch (spec) {
                case T('S'): {
                    const SimpleString<T> s = va_arg(args, SimpleString<T>);
                    const int32 avail = (int32) (limit - buffer);
                    const int32 n = s.length < avail ? s.length : avail;

                    memcpy(buffer, s.str, sizeof(T) * n);
                    buffer += n;
                } break;
                case T('s'): {
                    const T* str = va_arg(args, const T *);
                    if (str) {
                        while (*str && buffer < limit) {
                            *buffer++ = *str++;
                        }
                    }
                } break;
                case T('c'): {
                    *buffer++ = (T) va_arg(args, int32);
                } break;
                case T('n'): {
                    const int64 val = va_arg(args, int64);
                    buffer += int_to_str(val, buffer, T(','));
                } break;
                case T('d'): {
                    const int32 val = va_arg(args, int32);
                    buffer += int_to_str(val, buffer);
                } break;
                case T('u'): {
                    const uint32 val = va_arg(args, uint32);
                    buffer += int_to_str(val, buffer);
                } break;
                case T('l'): {
                    const int64 val = va_arg(args, int64);
                    buffer += int_to_str(val, buffer);
                } break;
                case T('f'): {
                    const f64 val = va_arg(args, f64);

                    int32 precision = 5;
                    int32 pi = fi + 1;
                    if (pi < format.length && format.str[pi] >= T('0') && format.str[pi] <= T('9')) {
                        precision = 0;
                        while (pi < format.length && format.str[pi] >= T('0') && format.str[pi] <= T('9')) {
                            precision = precision * 10 + (format.str[pi] - T('0'));
                            ++pi;
                        }

                        fi = pi - 1;
                    }

                    buffer += float_to_str(val, buffer, precision);
                } break;
                default: {
                    *buffer++ = T('%');
                } break;
            }
        }

        ++fi;
    }

    *buffer = T('\0');
    va_end(args);

    return simple_string_init(start, (int32) (buffer - start));
}

#endif