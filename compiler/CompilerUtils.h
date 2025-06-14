/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_COMPILER_COMPILER_UTILS_H
#define COMS_COMPILER_COMPILER_UTILS_H

#include "../utils/TestUtils.h"

#if _MSC_VER
    #include "msvc/CompilerUtils.h"
#elif __GNUC__
    #include "gcc/CompilerUtils.h"
#endif

#endif