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
#include <string.h>

#if DEBUG
    #define ASSERT_TRUE(a) if (!(a)) {    \
        /* cppcheck-suppress nullPointer */ \
        *(volatile int *)0 = 0;             \
    }

    #define ASSERT_TRUE_CONST(a) if constexpr (!(a)) {    \
        /* cppcheck-suppress nullPointer */                 \
        *(volatile int *)0 = 0;                             \
    }

    #define ASSERT_MEM_ZERO(ptr, size) do {                            \
        static const uint64_t zero_pattern = 0;                    \
        const char *p_ = (const char *)(ptr);                      \
        size_t chunk_size = (size);                                \
        while (chunk_size >= sizeof(uint64_t)) {                   \
            if (memcmp(p_, &zero_pattern, sizeof(uint64_t)) != 0) {\
                *(volatile int *)0 = 0;                            \
                break;                                             \
            }                                                      \
            p_ += sizeof(uint64_t);                                \
            chunk_size -= sizeof(uint64_t);                        \
        }                                                          \
        if (memcmp(p_, &zero_pattern, chunk_size) != 0) {          \
            *(volatile int *)0 = 0;                                \
        }                                                          \
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
