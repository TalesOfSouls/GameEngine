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

#if _MSC_VER
    #include "msvc/CompilerUtils.h"
#elif __GNUC__
    #include "gcc/CompilerUtils.h"
#endif

#if CPP_VERSION >= 20
    #define LIKELY [[likely]]
    #define UNLIKELY [[unlikely]]
    #define MAYBE_UNUSED [[maybe_unused]]

    #define INITIALIZER

    #define PSEUDO_USE(a) ((void) (0))
#else
    #define LIKELY
    #define UNLIKELY
    #define MAYBE_UNUSED

    // This is stupid but CONSTEXPR in c++17 must be initialized
    #define INITIALIZER ={0}

    // In c++17 we have to simulate a pseudo variable use since maybe_unused doesn't exist
    // Otherwise the compiler will complain about a unused variable
    #define PSEUDO_USE(a) ((void) (a))
#endif

#if CPP_VERSION >= 17
    #define FALLTHROUGH [[fallthrough]]
    #define IF_CONSTEXPR(cond) if constexpr (cond)
    #define CONSTEXPR constexpr
#else
    #define FALLTHROUGH
    #define IF_CONSTEXPR(cond) __pragma(warning(suppress:4127)) if (cond)
    #define CONSTEXPR constexpr
#endif

// dst and src need to be aligned
// size needs to be multiple of 8
#define memcpy_aligned(dst, src, size) {                            \
    IF_CONSTEXPR((size) % 8 == 0) {                                \
        compiler_memcpy_aligned((dst), (src), (size));             \
    } else {                                                        \
        memcpy((dst), (src), (size));                               \
    }                                                               \
}

// Sometimes we know a factor at compile time that is part of size and we can check that
// dst and src need to be aligned
// factor needs to be multiple of 8
#define memcpy_aligned_factored(dst, value, size, factor) {           \
    IF_CONSTEXPR((factor) % 8 == 0) {                                \
        compiler_memcpy_aligned((dst), (value), (size));             \
    } else {                                                        \
        memcpy((dst), (value), (size));                               \
    }                                                               \
}

// dst needs to be aligned
// size needs to be multiple of 8
#define memset_aligned(dst, value, size) {                            \
    IF_CONSTEXPR((size) % 8 == 0) {                                \
        compiler_memset_aligned((dst), (value), (size));             \
    } else {                                                        \
        memset((dst), (value), (size));                               \
    }                                                               \
}

// Sometimes we know a factor at compile time that is part of size and we can check that
// dst needs to be aligned
// factor needs to be multiple of 8
#define memset_aligned_factored(dst, value, size, factor) {           \
    IF_CONSTEXPR((factor) % 8 == 0) {                                \
        compiler_memset_aligned((dst), (value), (size));             \
    } else {                                                        \
        memset((dst), (value), (size));                               \
    }                                                               \
}

#endif