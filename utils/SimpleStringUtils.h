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
    int32 length = str_length(str);
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

#endif