/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
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
    C* str;
    int32 length;
};

#endif