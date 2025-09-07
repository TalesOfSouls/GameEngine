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
#include "../../utils/Assert.h"

#define PACKED_STRUCT  __attribute__((__packed__))
#define UNPACKED_STRUCT ((void) 0)

#define EXPORT_LIB extern "C" __attribute__((visibility("default")))

#if DEBUG
    #define UNREACHABLE() ASSERT_TRUE(false); __builtin_unreachable()
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

#define HOT_CODE __attribute__((hot))
#define COLD_CODE __attribute__((cold))

FORCE_INLINE
int32 compiler_find_first_bit_r2l(uint64 mask) NO_EXCEPT {
    ASSERT_STRICT(mask);

    #if __LITTLE_ENDIAN__
        return 63 - __builtin_clzll(mask);
    #else
        return __builtin_ctzll(mask);
    #endif
}

FORCE_INLINE
int32 compiler_find_first_bit_r2l(uint32 mask) NO_EXCEPT {
    ASSERT_STRICT(mask);

    #if __LITTLE_ENDIAN__
        return __builtin_ctz(mask);
    #else
        return 31 - __builtin_clz(mask);
    #endif
}

FORCE_INLINE
int32 compiler_find_first_bit_l2r(uint64 mask) NO_EXCEPT {
    ASSERT_STRICT(mask);

    #if __LITTLE_ENDIAN__
        return 63 - __builtin_clzll(mask);
    #else
        return __builtin_ctzll(mask);
    #endif
}

FORCE_INLINE
int32 compiler_find_first_bit_l2r(uint32 mask) NO_EXCEPT {
    ASSERT_STRICT(mask);

    #if __LITTLE_ENDIAN__
        return __builtin_ctz(mask);
    #else
        return 31 - __builtin_clz(mask);
    #endif
}

#define compiler_is_bit_set_r2l(num, pos) ((bool) ((num) & (1 << (pos))))
#define compiler_is_bit_set_64_r2l(num, pos) ((bool) ((num) & (1ULL << (pos))))

#define compiler_div_pow2(a, b) a >> __builtin_ctz(b)

/*
#include <cpuid.h>
inline
void compiler_cpuid(uint32 cpu_info[4], int32 function_id) {
    __cpuid(function_id, cpu_info[0], cpu_info[1], cpu_info[2], cpu_info[3]);
}
*/

inline
void compiler_cpuid(uint32 cpu_info[4], int32 function_id) NO_EXCEPT {
    asm volatile(
        "cpuid"
        : "=a" (cpu_info[0]), "=b" (cpu_info[1]), "=c" (cpu_info[2]), "=d" (cpu_info[3])
        : "a" (function_id)
    );
}

inline
void compiler_cpuid(uint32 cpu_info[4], int32 function_id, int32 level) NO_EXCEPT {
    asm volatile(
        "cpuid"
        : "=a" (cpu_info[0]), "=b" (cpu_info[1]), "=c" (cpu_info[2]), "=d" (cpu_info[3])
        : "a" (function_id), "c" (level)
    );
}

FORCE_INLINE
void compiler_memcpy_unaligned(void* __restrict dst, const void* __restrict src, size_t size)
{
    __builtin_memcpy(dst, src, size);
}

// 8 byte alignment required
FORCE_INLINE
void compiler_memcpy_aligned(void* __restrict dst, const void* __restrict src, size_t size)
{
    __builtin_memcpy(dst, src, size);
}

FORCE_INLINE
void compiler_memset_unaligned(void* dst, int value, size_t size) {
    __builtin_memset(dst, value, size);
}

// 8 byte alignment required and size needs to be multiple of 8
FORCE_INLINE
void compiler_memset_aligned(void* dst, int value, size_t size) {
    __builtin_memset(dst, value, size);
}

#define SWAP_ENDIAN_16(val) __builtin_bswap16((val))
#define SWAP_ENDIAN_32(val) __builtin_bswap32((val))
#define SWAP_ENDIAN_64(val) __builtin_bswap64((val))

#endif