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

#include "../utils/Assert.h"

#if defined(_MSC_VER)
    #include "msvc/CompilerUtils.h"
#elif defined(__GNUC__)
    #include "gcc/CompilerUtils.h"
#endif

#if !defined(CPP_VERSION) || CPP_VERSION >= 20
    #define LIKELY [[likely]]
    #define UNLIKELY [[unlikely]]
    #define MAYBE_UNUSED [[maybe_unused]]

    #define INITIALIZER

    #define PSEUDO_USE(a) ((void) (0))
#else
    #define LIKELY
    #define UNLIKELY
    #define MAYBE_UNUSED

    // This is stupid but CONSTEXPR variables in c++17 must be initialized
    #define INITIALIZER ={0}

    // In c++17 we have to simulate a pseudo variable use since maybe_unused doesn't exist
    // Otherwise the compiler will complain about a unused variable
    #define PSEUDO_USE(a) ((void) (a))
#endif

#if !defined(CPP_VERSION) || CPP_VERSION >= 17
    #define FALLTHROUGH [[fallthrough]]
    #define IF_CONSTEXPR(cond) if constexpr (cond)
    #define CONSTEXPR constexpr
#else
    #define FALLTHROUGH
    #define CONSTEXPR constexpr
#endif

#endif