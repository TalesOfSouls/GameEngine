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
 *
 * WARNING: Obviously this mustn't be used for string literals
 */
template <typename C>
struct SimpleStringInline {
    int32 length;
    int32 max_length;
};

/**
 * Pointer-less version of Utf8String
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