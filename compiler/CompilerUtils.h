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
    #define CONSTEXPR constexpr
#endif

/**
 * Performs an optimized memcpy IFF the alignment and size are valid.
 *
 * This is useful for cases where you don't want to manually re-calculate the size.
 * Sometimes you modify a struct but don't want to re-check the size of it, or
 * even if you would check it, you then would have to maybe modify the source code everywhere.
 * This way you can simply type `memcpy_aligned(dst, src, sizeof(MY_STRUCT));`
 *
 * WARNING: The alignment and size need to be known at compile time.
 *          NEVER use this macro if alignment or size are only known at runtime
 *
 * @example memcpy_aligned(dst, src, sizeof(MY_STRUCT));
 *
 * @param char*         dst     8/4 byte aligned memory
 * @param const char*   src     8/4 byte aligned memory
 * @param size_t        size    Multiple of 8 if 8 byte aligned or multiple of 4 otherwise
 *
 * @return void
 */
#define memcpy_aligned(dst, src, size) {                 \
    IF_CONSTEXPR((size) % 8 == 0) {                      \
        compiler_memcpy_aligned_8((dst), (src), (size)); \
    } else IF_CONSTEXPR((size) % 4 == 0) {               \
        compiler_memcpy_aligned_4((dst), (src), (size)); \
    } else {                                             \
        memcpy((dst), (src), (size));                    \
    }                                                    \
}

/**
 * Performs an optimized memcpy IFF the alignment and size are valid.
 *
 * In some cases the size is a multiple of a runtime value and a compile time value:
 *      e.g. my_value * sizeof(uint64)
 * This macro allows you to still use this optimized version since one of the factors is known at compile time
 *
 * WARNING: The alignment and factor need to be known at compile time.
 *          NEVER use this macro if alignment or factor are only known at runtime
 *
 * @example memcpy_aligned(dst, src, my_value * sizeof(MY_STRUCT), sizeof(MY_STRUCT));
 *
 * @param char*         dst     8/4 byte aligned memory
 * @param const char*   src     8/4 byte aligned memory
 * @param size_t        size    Total size to copy
 * @param size_t        factor  Factor of the size element (e.g. my_value * sizeof(uint64) where sizeof(uint64) is the compile time known factor)
 *
 * @return void
 */
#define memcpy_aligned_factored(dst, value, size, factor) {  \
    IF_CONSTEXPR((factor) % 8 == 0) {                        \
        compiler_memcpy_aligned_8((dst), (value), (size));   \
    } else IF_CONSTEXPR((factor) % 4 == 0) {                 \
        compiler_memcpy_aligned_4((dst), (value), (size));   \
    } else {                                                 \
        memcpy((dst), (value), (size));                      \
    }                                                        \
}

/**
 * Performs an optimized memset IFF the alignment and size are valid
 *
 * This is useful for cases where you don't want to manually re-calculate the size.
 * Sometimes you modify a struct but don't want to re-check the size of it, or
 * even if you would check it, you then would have to maybe modify the source code everywhere.
 * This way you can simply type `memset_aligned(dst, 0, sizeof(MY_STRUCT));`
 *
 * WARNING: The alignment and size need to be known at compile time.
 *          NEVER use this macro if alignment or size are only known at runtime
 *
 * @example memset_aligned(dst, 0, sizeof(MY_STRUCT));
 *
 * @param char*     dst     8/4 byte aligned memory
 * @param uint8     value   Value to set
 * @param size_t    size    Multiple of 8 if 8 byte aligned or multiple of 4 otherwise
 *
 * @return void
 */
#define memset_aligned(dst, value, size) {                   \
    IF_CONSTEXPR((size) % 8 == 0) {                          \
        compiler_memset_aligned_8((dst), (value), (size));   \
    } else IF_CONSTEXPR((size) % 4 == 0) {                   \
        compiler_memset_aligned_4((dst), (value), (size));   \
    } else {                                                 \
        memset((dst), (value), (size));                      \
    }                                                        \
}

/**
 * Performs an optimized memset IFF the alignment and size are valid.
 *
 * In some cases the size is a multiple of a runtime value and a compile time value:
 *      e.g. my_value * sizeof(uint64)
 * This macro allows you to still use this optimized version since one of the factors is known at compile time.
 *
 * WARNING: The alignment and factor need to be known at compile time.
 *          NEVER use this macro if alignment or factor are only known at runtime
 *
 * @example memset_aligned_factored(dst, 0, my_value * sizeof(MY_STRUCT), sizeof(MY_STRUCT));
 *
 * @param char*     dst     8/4 byte aligned memory
 * @param uint8     value   Value to set
 * @param size_t    size    Total size to copy
 * @param size_t    factor  Factor of the size element (e.g. my_value * sizeof(uint64) where sizeof(uint64) is the compile time known factor)
 *
 * @return void
 */
#define memset_aligned_factored(dst, value, size, factor) {  \
    IF_CONSTEXPR((factor) % 8 == 0) {                        \
        compiler_memset_aligned_8((dst), (value), (size));   \
    } else IF_CONSTEXPR((factor) % 4 == 0) {                 \
        compiler_memset_aligned_4((dst), (value), (size));   \
    } else {                                                 \
        memset((dst), (value), (size));                      \
    }                                                        \
}

#endif