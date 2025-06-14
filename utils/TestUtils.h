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

#if DEBUG
    #define ASSERT_SIMPLE(a) if (!(a)) {    \
        /* cppcheck-suppress nullPointer */ \
        *(volatile int *)0 = 0;             \
    }
    #define ASSERT_SIMPLE_CONST(a) if constexpr (!(a)) {    \
        /* cppcheck-suppress nullPointer */                 \
        *(volatile int *)0 = 0;                             \
    }

    #if DEBUG_STRICT
        // This macro is only used during strict debugging
        // Strict debugging is a mode that performs a lot of assertions
        // This slows down the application by a lot and is therefore not applicable for normal debugging
        #define ASSERT_STRICT ASSERT_SIMPLE
    #else
        #define ASSERT_STRICT(a) ((void)0)
    #endif
#else
    #define ASSERT_SIMPLE(a) ((void)0)
    #define ASSERT_SIMPLE_CONST(a) ((void)0)

    #define ASSERT_STRICT(a) ((void)0)
#endif

#endif
