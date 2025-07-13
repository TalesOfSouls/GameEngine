/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_COMPILER_GCC_COMPILER_UTILS_H
#define COMS_COMPILER_GCC_COMPILER_UTILS_H

#include "../../stdlib/Types.h"
#include "../../utils/TestUtils.h"

#define PACKED_STRUCT  __attribute__((__packed__))
#define UNPACKED_STRUCT ((void) 0)

#define EXPORT_LIB extern "C" __attribute__((visibility("default")))

#if DEBUG
    #define UNREACHABLE() ASSERT_SIMPLE(false); __builtin_unreachable()
#else
    #define UNREACHABLE() __builtin_unreachable()
#endif

#define FORCE_INLINE __attribute__((always_inline)) inline

#include <unistd.h>
#define compiler_debug_print(message) ({ const char* message_temp = message; while (*message_temp) { write(STDOUT_FILENO, (message_temp++), 1); } })

#define compiler_popcount_32(data) __builtin_popcount((data))
#define compiler_popcount_64(data) __builtin_popcountl((data))
#define __restrict __restrict__

#define compiler_prefetch(mem) __builtin_prefetch((mem), 0, 3)
#define compiler_prefetch_l1(mem) __builtin_prefetch((mem), 0, 3)
#define compiler_prefetch_l2(mem) __builtin_prefetch((mem), 0, 2)
#define compiler_prefetch_l3(mem) __builtin_prefetch((mem), 0, 1)

int32 compiler_find_first_bit_r2l(uint64 mask) noexcept {
    if (!mask) {
        return -1;
    }

    #if __LITTLE_ENDIAN__
        return 63 - __builtin_clzll(mask);
    #else
        return __builtin_ctzll(mask);
    #endif
}

int32 compiler_find_first_bit_r2l(uint32 mask) noexcept {
    if (!mask) {
        return -1;
    }

    #if __LITTLE_ENDIAN__
        return __builtin_ctz(mask);
    #else
        return 31 - __builtin_clz(mask);
    #endif
}

int32 compiler_find_first_bit_l2r(uint64 mask) noexcept {
    if (!mask) {
        return -1;
    }

    #if __LITTLE_ENDIAN__
        return 63 - __builtin_clzll(mask);
    #else
        return __builtin_ctzll(mask);
    #endif
}

int32 compiler_find_first_bit_l2r(uint32 mask) noexcept {
    if (!mask) {
        return -1;
    }

    #if __LITTLE_ENDIAN__
        return __builtin_ctz(mask);
    #else
        return 31 - __builtin_clz(mask);
    #endif
}

/*
#include <cpuid.h>
inline
void compiler_cpuid(uint32 cpu_info[4], int32 function_id) {
    __cpuid(function_id, cpu_info[0], cpu_info[1], cpu_info[2], cpu_info[3]);
}
*/

inline
void compiler_cpuid(uint32 cpu_info[4], int32 function_id) noexcept {
    asm volatile(
        "cpuid"
        : "=a" (cpu_info[0]), "=b" (cpu_info[1]), "=c" (cpu_info[2]), "=d" (cpu_info[3])
        : "a" (function_id)
    );
}

inline
void compiler_cpuid(uint32 cpu_info[4], int32 function_id, int32 level) noexcept {
    asm volatile(
        "cpuid"
        : "=a" (cpu_info[0]), "=b" (cpu_info[1]), "=c" (cpu_info[2]), "=d" (cpu_info[3])
        : "a" (function_id), "c" (level)
    );
}

#endif