/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_COMPILER_MSVC_COMPILER_UTILS_H
#define COMS_COMPILER_MSVC_COMPILER_UTILS_H

#include <basetsd.h>
#include <intrin.h>
#include "../../utils/Assert.h"
#include "../../stdlib/Types.h"
#include "../../stdlib/stdlib_has.h"

#if CPP_VERSION < 17
    #define IF_CONSTEXPR(cond) __pragma(warning(suppress:4127)) if (cond)
#endif

#define PACKED_STRUCT  __pragma(pack(push, 1))
#define UNPACKED_STRUCT __pragma(pack(pop))

#define EXPORT_LIB extern "C" __declspec(dllexport)

#ifdef DEBUG
    #define UNREACHABLE() ASSERT_TRUE(false); __assume(0)
#else
    #define UNREACHABLE() __assume(0)
#endif

#define FORCE_INLINE __forceinline
#define FORCE_FLATTEN [[msvc::flatten]]

#define compiler_debug_print(message) OutputDebugStringA((message))

#define compiler_popcount_32(data) __popcnt((data))
#define compiler_popcount_64(data) __popcnt64((data))

#define compiler_prefetch(mem) __prefetch((mem))
#define compiler_prefetch_l1(mem) __prefetch((mem))
#define compiler_prefetch_l2(mem) __prefetch((mem))
#define compiler_prefetch_l3(mem) __prefetch((mem))

#define HOT_CODE __declspec(code_seg(".text$hot"))
#define COLD_CODE __declspec(code_seg(".text$cold"))

#define DECLARE_SECTION(name) __pragma(section(name, read))
#define SECTION_ALLOC(name) __declspec(allocate(name)) __declspec(selectany)
#define SECTION_START(name) __##name##_start
#define SECTION_END(name)   __##name##_end

FORCE_INLINE
int32 compiler_find_first_bit_r2l(uint64 mask) NO_EXCEPT
{
    ASSERT_STRICT(mask);

    unsigned long index;
    return _BitScanForward64(&index, mask) ? index : -1;
}

FORCE_INLINE
int32 compiler_find_first_bit_r2l(uint32 mask) NO_EXCEPT
{
    ASSERT_STRICT(mask);

    unsigned long index;
    return _BitScanForward(&index, mask) ? index : -1;
}

FORCE_INLINE
int32 compiler_find_first_bit_l2r(uint64 mask) NO_EXCEPT
{
    ASSERT_STRICT(mask);

    unsigned long index;
    return _BitScanReverse64(&index, mask) ? index : -1;
}

FORCE_INLINE
int32 compiler_find_first_bit_l2r(uint32 mask) NO_EXCEPT
{
    ASSERT_STRICT(mask);

    unsigned long index;
    return _BitScanReverse(&index, mask) ? index : -1;
}

FORCE_INLINE
void compiler_cpuid(uint32 cpu_info[4], int32 function_id, int32 level = 0) NO_EXCEPT
{
    __cpuidex((int32 *) cpu_info, function_id, level);
}

#define compiler_is_bit_set_r2l(num, pos) _bittest(num, pos)
#define compiler_is_bit_set_64_r2l(num, pos) _bittest64(num, pos)

// Optimized way to calculate the division where b is a power of 2
// Only required for compile time if we can guarantee b = power of 2
FORCE_INLINE
uint32 compiler_div_pow2(uint32 a, uint32 b) NO_EXCEPT
{
    unsigned long index;
    _BitScanForward(&index, b);

    return (uint32) (a >> index);
}

// Optimized way to calculate the division where b is a power of 2
// Only required for compile time if we can guarantee b = power of 2
FORCE_INLINE
size_t compiler_div_pow2(size_t a, size_t b) NO_EXCEPT
{
    unsigned long index;
    _BitScanForward64(&index, b);

    return (size_t) (a >> index);
}

// Optimized way to calculate the division where b is a power of 2
// Only required for compile time if we can guarantee b = power of 2
FORCE_INLINE
uint16 compiler_div_pow2(uint16 a, uint16 b) NO_EXCEPT
{
    unsigned long index = 0;
    _BitScanForward(&index, (uint32) b);

    return (uint16) (a >> index);
}

// Optimized way to calculate the division where b is a power of 2
// Only required for compile time if we can guarantee b = power of 2
FORCE_INLINE
size_t compiler_div_pow2(size_t a, uint32 b) NO_EXCEPT
{
    unsigned long index;
    _BitScanForward(&index, b);

    return (size_t) (a >> index);
}

// Optimized way to calculate the division where b is a power of 2
// Only required for compile time if we can guarantee b = power of 2
FORCE_INLINE
uint16 compiler_div_pow2(uint16 a, uint32 b) NO_EXCEPT
{
    unsigned long index = 0;
    _BitScanForward(&index, b);

    return (uint16) (a >> index);
}

// Optimized way to calculate the division where b is a power of 2
// Only required for compile time if we can guarantee b = power of 2
FORCE_INLINE
uint32 compiler_div_pow2(uint32 a, int32 b) NO_EXCEPT
{
    unsigned long index;
    _BitScanForward(&index, b);

    return (uint32) (a >> index);
}

// Optimized way to calculate the division where b is a power of 2
// Only required for compile time if we can guarantee b = power of 2
FORCE_INLINE
int32 compiler_div_pow2(int32 a, int32 b) NO_EXCEPT
{
    unsigned long index;
    _BitScanForward(&index, b);

    return (int32) (a >> index);
}

// Optimized way to calculate the division where b is a power of 2
// Only required for compile time if we can guarantee b = power of 2
FORCE_INLINE
uint16 compiler_div_pow2(uint16 a, int16 b) NO_EXCEPT
{
    unsigned long index = 0;
    _BitScanForward(&index, (uint32) b);

    return (uint16) (a >> index);
}

// Optimized way to calculate the division where b is a power of 2
// Only required for compile time if we can guarantee b = power of 2
FORCE_INLINE
size_t compiler_div_pow2(size_t a, int32 b) NO_EXCEPT
{
    unsigned long index;
    _BitScanForward(&index, b);

    return (size_t) (a >> index);
}

// Optimized way to calculate the division where b is a power of 2
// Only required for compile time if we can guarantee b = power of 2
FORCE_INLINE
uint16 compiler_div_pow2(uint16 a, int32 b) NO_EXCEPT
{
    unsigned long index = 0;
    _BitScanForward(&index, b);

    return (uint16) (a >> index);
}

// Optimized way to calculate the division where b is a power of 2
// Only required for compile time if we can guarantee b = power of 2
FORCE_INLINE
byte compiler_div_pow2(byte a, int32 b) NO_EXCEPT
{
    unsigned long index = 0;
    _BitScanForward(&index, b);

    return (byte) (a >> index);
}

// Optimized way to calculate the division where b is a power of 2
// Only required for compile time if we can guarantee b = power of 2
FORCE_INLINE
byte compiler_div_pow2(byte a, uint32 b) NO_EXCEPT
{
    unsigned long index = 0;
    _BitScanForward(&index, b);

    return (byte) (a >> index);
}

// 8 byte alignment required and size needs to be multiple of 8
FORCE_INLINE HOT_CODE
void compiler_memcpy_aligned_8(
    void* const __restrict dst,
    const void* const __restrict src,
    size_t size
) NO_EXCEPT
{
    ASSERT_STRICT((size & 7) == 0);
    ASSERT_STRICT(((uintptr_t) dst) & 7 == 0);
    ASSERT_STRICT(((uintptr_t) src) & 7 == 0);

    #ifdef __aarch64__
        memcpy(dst, src, size);
    #else
        __movsq((unsigned long long*) dst, (const unsigned long long*) src, size >> 3);
    #endif
}

FORCE_INLINE HOT_CODE
void compiler_memcpy_aligned_4(
    void* const __restrict dst,
    const void* const __restrict src,
    size_t size
) NO_EXCEPT
{
    ASSERT_STRICT((size & 3) == 0);
    ASSERT_STRICT(((uintptr_t) dst) & 3 == 0);
    ASSERT_STRICT(((uintptr_t) src) & 3 == 0);

    #ifdef __aarch64__
        memcpy(dst, src, size);
    #else
        __movsd((unsigned long*) dst, (const unsigned long*) src, size >> 3);
    #endif
}

// 8 byte alignment required and size needs to be multiple of 8
FORCE_INLINE HOT_CODE
void compiler_memset_aligned_8(void* const dst, int value, size_t size) NO_EXCEPT
{
    ASSERT_STRICT((size & 7) == 0);
    ASSERT_STRICT(((uintptr_t) dst) & 7 == 0);

    #ifdef __aarch64__
        memset(dst, 0, size);
    #else
        __stosq((unsigned __int64*) dst, (unsigned __int64) value, size >> 3);
    #endif
}

FORCE_INLINE HOT_CODE
void compiler_memset_aligned_4(void* const dst, int value, size_t size) NO_EXCEPT
{
    ASSERT_STRICT((size & 3) == 0);
    ASSERT_STRICT(((uintptr_t) dst) & 3 == 0);

    #ifdef __aarch64__
        memset(dst, 0, size);
    #else
        __stosd((unsigned long*) dst, (unsigned long) value, size >> 2);
    #endif
}

#define SWAP_ENDIAN_16(val) _byteswap_ushort((val))
#define SWAP_ENDIAN_32(val) _byteswap_ulong((val))
#define SWAP_ENDIAN_64(val) _byteswap_uint64((val))

#if _WIN32 || __LITTLE_ENDIAN__
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

#define SINCOSF(x, s, c) s = sinf(x); c = cosf(x)
#define SINCOS(x, s, c) s = sin(x); c = cos(x)

#endif