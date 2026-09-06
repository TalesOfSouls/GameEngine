/**
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_COMPILER_COMPILER_UTILS_H
#define COMS_COMPILER_COMPILER_UTILS_H

#if defined(_MSC_VER)
    #include "msvc/CompilerUtils.h"
#elif defined(__GNUC__)
    #include "gcc/CompilerUtils.h"
#endif

#endif