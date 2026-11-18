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
        (int32) (N - 1),
        (C*) str
    };
}

template <typename C, size_t N>
CONSTEVAL
SimpleString<C> simple_string_literal(const C (&str)[N], int32 length) NO_EXCEPT
{
    return SimpleString<C>{
        (int32) (length),
        (int32) (N - 1),
        (C*) str
    };
}

template <typename C>
FORCE_INLINE CONSTEXPR
SimpleString<C> simple_string_init(C* str, int32 length) NO_EXCEPT
{
    return SimpleString<C>{ length, length, str };
}

template <typename C>
FORCE_INLINE CONSTEXPR
SimpleString<C> simple_string_init(C* str, int32 length, int32 max_length) NO_EXCEPT
{
    return SimpleString<C>{ length, max_length, str };
}

template <typename C>
FORCE_INLINE CONSTEXPR
SimpleString<C> simple_string_init(C* str) NO_EXCEPT
{
    const int32 length = (int32) str_length(str);
    return SimpleString<C>{ length, length, str };
}

template <typename C>
FORCE_INLINE CONSTEXPR
SimpleString<C> simple_string_empty_init() NO_EXCEPT
{
    return SimpleString<C>{ 0, 0, NULL };
}

template <typename C>
CONSTEXPR FORCE_INLINE
SimpleString<C> simple_string_literal(SimpleString<C> str) NO_EXCEPT
{
    return str;
}

/**
 * Memory in bytes required by a SimpleStringInline with max_length characters (header + characters)
 */
template <typename C>
FORCE_INLINE CONSTEXPR
size_t simple_string_inline_size(int32 max_length) NO_EXCEPT
{
    return sizeof(SimpleStringInline<C>) + (size_t) max_length * sizeof(C);
}

// The characters are stored directly behind the struct
template <typename C>
FORCE_INLINE
C* simple_string_inline_str(SimpleStringInline<C>* const str) NO_EXCEPT
{
    return (C *) (str + 1);
}

template <typename C>
FORCE_INLINE
const C* simple_string_inline_str(const SimpleStringInline<C>* const str) NO_EXCEPT
{
    return (const C *) (str + 1);
}

/**
 * The SimpleStringInline that belongs to characters which are stored behind a SimpleStringInline
 * Useful if only the position of the characters is stored (e.g. because they are also used as C string)
 */
template <typename C>
FORCE_INLINE
SimpleStringInline<C>* simple_string_inline_from_str(C* const characters) NO_EXCEPT
{
    return ((SimpleStringInline<C> *) characters) - 1;
}

template <typename C>
FORCE_INLINE
const SimpleStringInline<C>* simple_string_inline_from_str(const C* const characters) NO_EXCEPT
{
    return ((const SimpleStringInline<C> *) characters) - 1;
}

/**
 * Creates an empty SimpleStringInline in memory
 *
 * @param void* memory      At least simple_string_inline_size<C>(max_length) bytes, 4 byte aligned
 * @param int32 max_length  Max amount of characters
 */
template <typename C>
FORCE_INLINE
SimpleStringInline<C>* simple_string_inline_init(void* const memory, int32 max_length) NO_EXCEPT
{
    ASSERT_TRUE(((uintptr_t) memory % alignof(SimpleStringInline<C>)) == 0);

    SimpleStringInline<C>* const str = (SimpleStringInline<C> *) memory;
    str->length = 0;
    str->max_length = max_length;

    return str;
}

/**
 * Copies characters into the string (cut at max_length)
 *
 * @return int32 Amount of copied characters
 */
template <typename C>
inline
int32 simple_string_inline_set(SimpleStringInline<C>* const __restrict dst, const C* const __restrict src, int32 length) NO_EXCEPT
{
    length = OMS_MIN(length, dst->max_length);
    if (length > 0) {
        memcpy(simple_string_inline_str(dst), src, (size_t) length * sizeof(C));
    }

    dst->length = length;

    return length;
}

// View of a pointer-less string for functions that take a SimpleString
template <typename C>
FORCE_INLINE
SimpleString<C> simple_string_view(SimpleStringInline<C>* const str) NO_EXCEPT
{
    return SimpleString<C>{ str->length, str->max_length, simple_string_inline_str(str) };
}

template <typename C>
FORCE_INLINE
SimpleString<const C> simple_string_view(const SimpleStringInline<C>* const str) NO_EXCEPT
{
    return SimpleString<const C>{ str->length, str->max_length, simple_string_inline_str(str) };
}

template <size_t N>
CONSTEVAL
Utf8String utf8_string_literal(const char (&str)[N]) NO_EXCEPT
{
    int32 length = 0;
    size_t i = 0;

    while (i < N - 1) {
        uint32 codepoint;
        const int32 byte_len = utf8_decode(str + i, &codepoint);

        i += (size_t) (byte_len > 0 ? byte_len : 1);
        length += 1;
    }

    return Utf8String{ length, N - 1, (char*) str };
}

FORCE_INLINE CONSTEXPR
Utf8String utf8_string_init(char* str, int32 length) NO_EXCEPT
{
    return Utf8String{ length, length, str };
}

FORCE_INLINE CONSTEXPR
Utf8String utf8_string_init(char* str, int32 length, int32 max_length) NO_EXCEPT
{
    return Utf8String{ length, max_length, str };
}

FORCE_INLINE CONSTEXPR
Utf8String utf8_string_init(char* str) NO_EXCEPT
{
    int32 length = 0;
    size_t i = 0;

    while (str[i] != '\0') {
        uint32 codepoint;
        const int32 byte_len = utf8_decode(str + i, &codepoint);

        i += (size_t) (byte_len > 0 ? byte_len : 1);
        length += 1;
    }

    return Utf8String{ length, length, str };
}

FORCE_INLINE CONSTEXPR
Utf8String utf8_string_empty_init() NO_EXCEPT
{
    return Utf8String{ 0, 0, NULL };
}

CONSTEXPR FORCE_INLINE
Utf8String utf8_string_literal(Utf8String str) NO_EXCEPT
{
    return str;
}

/**
 * Memory in bytes required by a Utf8StringInline with max_length bytes of characters (header + characters)
 */
FORCE_INLINE CONSTEXPR
size_t utf8_string_inline_size(int32 max_length) NO_EXCEPT
{
    return sizeof(Utf8StringInline) + (size_t) max_length;
}

// The characters are stored directly behind the struct
FORCE_INLINE
char* utf8_string_inline_str(Utf8StringInline* const str) NO_EXCEPT
{
    return (char *) (str + 1);
}

FORCE_INLINE
const char* utf8_string_inline_str(const Utf8StringInline* const str) NO_EXCEPT
{
    return (const char *) (str + 1);
}

/**
 * Creates an empty Utf8StringInline in memory
 *
 * @param void* memory      At least utf8_string_inline_size(max_length) bytes, 4 byte aligned
 * @param int32 max_length  Max amount of bytes
 */
FORCE_INLINE
Utf8StringInline* utf8_string_inline_init(void* const memory, int32 max_length) NO_EXCEPT
{
    ASSERT_TRUE(((uintptr_t) memory % alignof(Utf8StringInline)) == 0);

    Utf8StringInline* const str = (Utf8StringInline *) memory;
    str->length = 0;
    str->max_length = max_length;

    return str;
}

// View of a pointer-less string for functions that take a Utf8String
FORCE_INLINE
Utf8String utf8_string_view(Utf8StringInline* const str) NO_EXCEPT
{
    return Utf8String{ str->length, str->max_length, utf8_string_inline_str(str) };
}

#endif