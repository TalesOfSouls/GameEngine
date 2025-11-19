/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_UTILS_TEST_UTILS_H
#define COMS_UTILS_TEST_UTILS_H

#include <stdint.h>

#if DEBUG
    #define ASSERT_TRUE(a) if (!(a)) {      \
        /* cppcheck-suppress nullPointer */ \
        *(volatile int *)0 = 0;             \
    }

    #define ASSERT_TRUE_CONST(a) IF_CONSTEXPR(!(a)) {    \
        /* cppcheck-suppress nullPointer */              \
        *(volatile int *)0 = 0;                          \
    }

    #define ASSERT_MEM_ZERO(ptr, size) do {    \
        const char *p_ = (const char *)(ptr);  \
        size_t chunk_size = (size);            \
                                               \
        while (chunk_size >= sizeof(size_t)) { \
            if (*(const size_t *)p_ != 0) {    \
                *(volatile int *)0 = 0;        \
                break;                         \
            }                                  \
            p_ += sizeof(size_t);              \
            chunk_size -= sizeof(size_t);      \
        }                                      \
                                               \
        while (chunk_size > 0) {               \
            if (*p_ != 0) {                    \
                *(volatile int *)0 = 0;        \
                break;                         \
            }                                  \
            ++p_;                              \
            --chunk_size;                      \
        }                                      \
    } while (0)

#else
    #define ASSERT_TRUE(a) ((void)0)
    #define ASSERT_TRUE_CONST(a) ((void)0)
    #define ASSERT_MEM_ZERO(ptr, size) ((void)0)
#endif

#if DEBUG_STRICT
    // This macro is only used during strict debugging
    // Strict debugging is a mode that performs a lot of assertions
    // This slows down the application by a lot and is therefore not applicable for normal debugging
    #define ASSERT_STRICT ASSERT_TRUE
#else
    #define ASSERT_STRICT(a) ((void)0)
#endif

#endif
