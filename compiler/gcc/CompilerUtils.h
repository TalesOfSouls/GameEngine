/**
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_COMPILER_GCC_COMPILER_UTILS_H
#define COMS_COMPILER_GCC_COMPILER_UTILS_H

#include "../../utils/Assert.h"
#include "../../stdlib/Types.h"

#if defined(CPP_VERSION) && CPP_VERSION < 17
    #define IF_CONSTEXPR(cond) if (cond)
#endif

#define PACKED_STRUCT  __attribute__((__packed__))
#define UNPACKED_STRUCT ((void) 0)

#define EXPORT_LIB extern "C" __attribute__((visibility("default")))

#if defined(DEBUG) && DEBUG
    #define UNREACHABLE() ASSERT_THROW(); __builtin_unreachable()
#else
    #define UNREACHABLE() __builtin_unreachable()
#endif

#define FORCE_INLINE __attribute__((always_inline)) inline
#define FORCE_FLATTEN __attribute__((flatten))

// Excludes a function from AddressSanitizer checks
// Only for intended reads beyond an object that can't fault, e.g. word-wise string scans
// (aligned word reads never cross into the next page, the bytes behind the terminator are ignored)
#define NO_SANITIZE_ADDRESS __attribute__((no_sanitize_address))

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

#define DECLARE_SECTION(name)
#define SECTION_ALLOC(name) __attribute__((section(name), used))
#define SECTION_START(name) __start_##name
#define SECTION_END(name)   __stop_##name

// Bit positions are independent of the byte order (same as compiler/msvc/CompilerUtils.h):
//      r2l = index of the lowest set bit (_BitScanForward)
//      l2r = index of the highest set bit (_BitScanReverse)
// The mask must not be 0 (the builtins are undefined for 0)

FORCE_INLINE
int32 compiler_find_first_bit_r2l(uint64 mask) NO_EXCEPT
{
    ASSERT_STRICT(mask);

    return __builtin_ctzll(mask);
}

FORCE_INLINE
int32 compiler_find_first_bit_r2l(uint32 mask) NO_EXCEPT
{
    ASSERT_STRICT(mask);

    return __builtin_ctz(mask);
}

FORCE_INLINE
int32 compiler_find_first_bit_l2r(uint64 mask) NO_EXCEPT
{
    ASSERT_STRICT(mask);

    return 63 - __builtin_clzll(mask);
}

FORCE_INLINE
int32 compiler_find_first_bit_l2r(uint32 mask) NO_EXCEPT
{
    ASSERT_STRICT(mask);

    return 31 - __builtin_clz(mask);
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

#ifdef __aarch64__
    // No cpuid on ARM (@see architecture/arm/CpuInfo.cpp)
    inline
    void compiler_cpuid(uint32 cpu_info[4], int32 function_id, int32 level = 0) NO_EXCEPT
    {
        (void) function_id;
        (void) level;
        cpu_info[0] = cpu_info[1] = cpu_info[2] = cpu_info[3] = 0;
    }
#else
    inline
    void compiler_cpuid(uint32 cpu_info[4], int32 function_id) NO_EXCEPT
    {
        asm volatile(
            "cpuid"
            : "=a" (cpu_info[0]), "=b" (cpu_info[1]), "=c" (cpu_info[2]), "=d" (cpu_info[3])
            : "a" (function_id)
        );
    }

    inline
    void compiler_cpuid(uint32 cpu_info[4], int32 function_id, int32 level) NO_EXCEPT
    {
        asm volatile(
            "cpuid"
            : "=a" (cpu_info[0]), "=b" (cpu_info[1]), "=c" (cpu_info[2]), "=d" (cpu_info[3])
            : "a" (function_id), "c" (level)
        );
    }
#endif

// 8 byte alignment required
FORCE_INLINE
void compiler_memcpy_aligned_8(
    void* const __restrict dst,
    const void* const __restrict src,
    size_t size
)
{
    __builtin_memcpy(dst, src, size);
}

// Stores a word at any address (a direct store through a misaligned size_t* is undefined behavior)
// Compiles to a single store
FORCE_INLINE
void compiler_store_unaligned(void* const dst, size_t value) NO_EXCEPT
{
    __builtin_memcpy(dst, &value, sizeof(value));
}

// Loads a value from any address (a load through a misaligned pointer is undefined behavior)
// Compiles to a single load
template <typename T>
FORCE_INLINE
T compiler_load_unaligned(const void* const src) NO_EXCEPT
{
    T value;
    __builtin_memcpy(&value, src, sizeof(value));

    return value;
}

// 8 byte alignment required and size needs to be multiple of 8
FORCE_INLINE
void compiler_memset_aligned_8(void* dst, int value, size_t size) {
    __builtin_memset(dst, value, size);
}

#define SWAP_ENDIAN_16(val) __builtin_bswap16((val))
#define SWAP_ENDIAN_32(val) __builtin_bswap32((val))
#define SWAP_ENDIAN_64(val) __builtin_bswap64((val))

// Same as compiler/msvc/CompilerUtils.h
// __BYTE_ORDER__ is predefined by gcc/clang -> independent of the include order (OMS_LITTLE_ENDIAN is defined in Defines.h)
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    #define SWAP_ENDIAN_LITTLE_16(val) (val)
    #define SWAP_ENDIAN_LITTLE_32(val) (val)
    #define SWAP_ENDIAN_LITTLE_64(val) (val)
    #define SWAP_ENDIAN_BIG_16(val) SWAP_ENDIAN_16(val)
    #define SWAP_ENDIAN_BIG_32(val) SWAP_ENDIAN_32(val)
    #define SWAP_ENDIAN_BIG_64(val) SWAP_ENDIAN_64(val)
#else
    #define SWAP_ENDIAN_LITTLE_16(val) SWAP_ENDIAN_16(val)
    #define SWAP_ENDIAN_LITTLE_32(val) SWAP_ENDIAN_32(val)
    #define SWAP_ENDIAN_LITTLE_64(val) SWAP_ENDIAN_64(val)
    #define SWAP_ENDIAN_BIG_16(val) (val)
    #define SWAP_ENDIAN_BIG_32(val) (val)
    #define SWAP_ENDIAN_BIG_64(val) (val)
#endif

#define SINCOSF(x, s, c) sincosf(x, &s, &c)
#define SINCOS(x, s, c) sincos(x, &s, &c)

#endif