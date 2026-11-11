/**
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_UTILS_SIMPLE_STRING_H
#define COMS_UTILS_SIMPLE_STRING_H

#include "../stdlib/Stdlib.h"

/**
 * In some situations we don't have the ability to use \0 terminated strings.
 * In such situations the SimpleString version can help
 */
template <typename C>
struct SimpleString {
    int32 length;
    int32 max_length;
    C* str;
};

template <typename C, size_t N>
struct SimpleStringBuffer {
    int32 length;
    C str[N];

    operator SimpleString<C>() {
        return SimpleString<C>{ length, length, str };
    }
};

struct Utf8String {
    int32 length;
    int32 max_length;
    char* str;
};

/**
 * Pointer-less version of SimpleString
 *
 * The characters are not referenced by a pointer, they are stored directly behind the struct (this + 1).
 * This saves the 8 byte pointer and makes the string position independent
 * (it can be copied/dumped/loaded together with its memory, no pointer needs to be fixed).
 *
 * Memory layout:
 *      int32 length
 *      int32 max_length
 *      C characters[max_length]        <- simple_string_inline_size<C>(max_length) bytes in total
 *
 * WARNING: It is only valid as pointer/reference into the memory that also contains the characters
 *              - never copy or pass it by value (the copy has no characters behind it)
 *              - never use it for string literals, as array element or as non-last struct member
 *          Functions that only read a string should take a SimpleString (@see simple_string_view)
 */
template <typename C>
struct SimpleStringInline {
    int32 length;
    int32 max_length;
};

/**
 * Pointer-less version of Utf8String (@see SimpleStringInline for the rules)
 *
 * Memory layout:
 *      int32 length                    <- characters (code points)
 *      int32 max_length                <- bytes
 *      char characters[max_length]
 */
struct Utf8StringInline {
    int32 length;
    int32 max_length;
};

template <size_t N>
struct Utf8StringBuffer {
    int32 length;
    char str[N];

    operator Utf8String() {
        return Utf8String{ length, length, str };
    }
};

#endif