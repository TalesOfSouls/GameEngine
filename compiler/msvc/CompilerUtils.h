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

#include "../../utils/Assert.h"
#include "../../stdlib/Types.h"
#include <basetsd.h>
#include <intrin.h>
#include <string.h>

#define PACKED_STRUCT  __pragma(pack(push, 1))
#define UNPACKED_STRUCT __pragma(pack(pop))

#define EXPORT_LIB extern "C" __declspec(dllexport)

#if DEBUG
    #define UNREACHABLE() ASSERT_TRUE(false); __assume(0)
#else
    #define UNREACHABLE() __assume(0)
#endif

#define FORCE_INLINE __forceinline

#define compiler_debug_print(message) OutputDebugStringA((message))

#define compiler_popcount_32(data) __popcnt((data))
#define compiler_popcount_64(data) __popcnt64((data))

#define compiler_prefetch(mem) __prefetch((mem))
#define compiler_prefetch_l1(mem) __prefetch((mem))
#define compiler_prefetch_l2(mem) __prefetch((mem))
#define compiler_prefetch_l3(mem) __prefetch((mem))

#define HOT_CODE __declspec(code_seg(".text$hot"))
#define COLD_CODE __declspec(code_seg(".text$cold"))

FORCE_INLINE
int32 compiler_find_first_bit_r2l(uint64 mask) NO_EXCEPT {
    ASSERT_STRICT(mask);

    unsigned long index;
    return _BitScanForward64(&index, mask) ? index : -1;
}

FORCE_INLINE
int32 compiler_find_first_bit_r2l(uint32 mask) NO_EXCEPT {
    ASSERT_STRICT(mask);

    unsigned long index;
    return _BitScanForward(&index, mask) ? index : -1;
}

FORCE_INLINE
int32 compiler_find_first_bit_l2r(uint64 mask) NO_EXCEPT {
    ASSERT_STRICT(mask);

    unsigned long index;
    return _BitScanReverse64(&index, mask) ? index : -1;
}

FORCE_INLINE
int32 compiler_find_first_bit_l2r(uint32 mask) NO_EXCEPT {
    ASSERT_STRICT(mask);

    unsigned long index;
    return _BitScanReverse(&index, mask) ? index : -1;
}

FORCE_INLINE
void compiler_cpuid(uint32 cpu_info[4], int32 function_id, int32 level = 0) NO_EXCEPT {
    __cpuidex((int32 *) cpu_info, function_id, level);
}

#define compiler_is_bit_set_r2l(num, pos) _bittest(num, pos)
#define compiler_is_bit_set_64_r2l(num, pos) _bittest64(num, pos)

// Optimized way to calculate the division where b is a power of 2
// Only required for compile time if we can guarantee b = power of 2
FORCE_INLINE
uint32 compiler_div_pow2(uint32 a, uint32 b)
{
    unsigned long index;
    _BitScanForward(&index, b);

    return (uint32) (a >> index);
}

// Optimized way to calculate the division where b is a power of 2
// Only required for compile time if we can guarantee b = power of 2
FORCE_INLINE
size_t compiler_div_pow2(size_t a, size_t b)
{
    unsigned long index;
    _BitScanForward64(&index, b);

    return (size_t) (a >> index);
}

// Optimized way to calculate the division where b is a power of 2
// Only required for compile time if we can guarantee b = power of 2
FORCE_INLINE
uint16 compiler_div_pow2(uint16 a, uint16 b) {
    unsigned long index = 0;
    _BitScanForward(&index, (uint32) b);

    return (uint16) (a >> index);
}

// Optimized way to calculate the division where b is a power of 2
// Only required for compile time if we can guarantee b = power of 2
FORCE_INLINE
size_t compiler_div_pow2(size_t a, uint32 b)
{
    unsigned long index;
    _BitScanForward(&index, b);

    return (size_t) (a >> index);
}

// Optimized way to calculate the division where b is a power of 2
// Only required for compile time if we can guarantee b = power of 2
FORCE_INLINE
uint16 compiler_div_pow2(uint16 a, uint32 b) {
    unsigned long index = 0;
    _BitScanForward(&index, b);

    return (uint16) (a >> index);
}

// Optimized way to calculate the division where b is a power of 2
// Only required for compile time if we can guarantee b = power of 2
FORCE_INLINE
uint32 compiler_div_pow2(uint32 a, int32 b)
{
    unsigned long index;
    _BitScanForward(&index, b);

    return (uint32) (a >> index);
}

// Optimized way to calculate the division where b is a power of 2
// Only required for compile time if we can guarantee b = power of 2
FORCE_INLINE
int32 compiler_div_pow2(int32 a, int32 b)
{
    unsigned long index;
    _BitScanForward(&index, b);

    return (int32) (a >> index);
}

// Optimized way to calculate the division where b is a power of 2
// Only required for compile time if we can guarantee b = power of 2
FORCE_INLINE
uint16 compiler_div_pow2(uint16 a, int16 b) {
    unsigned long index = 0;
    _BitScanForward(&index, (uint32) b);

    return (uint16) (a >> index);
}

// Optimized way to calculate the division where b is a power of 2
// Only required for compile time if we can guarantee b = power of 2
FORCE_INLINE
size_t compiler_div_pow2(size_t a, int32 b)
{
    unsigned long index;
    _BitScanForward(&index, b);

    return (size_t) (a >> index);
}

// Optimized way to calculate the division where b is a power of 2
// Only required for compile time if we can guarantee b = power of 2
FORCE_INLINE
uint16 compiler_div_pow2(uint16 a, int32 b) {
    unsigned long index = 0;
    _BitScanForward(&index, b);

    return (uint16) (a >> index);
}

// Optimized way to calculate the division where b is a power of 2
// Only required for compile time if we can guarantee b = power of 2
FORCE_INLINE
byte compiler_div_pow2(byte a, int32 b) {
    unsigned long index = 0;
    _BitScanForward(&index, b);

    return (byte) (a >> index);
}

// Optimized way to calculate the division where b is a power of 2
// Only required for compile time if we can guarantee b = power of 2
FORCE_INLINE
byte compiler_div_pow2(byte a, uint32 b) {
    unsigned long index = 0;
    _BitScanForward(&index, b);

    return (byte) (a >> index);
}

FORCE_INLINE
void compiler_memcpy_unaligned(void* __restrict dst, const void* __restrict src, size_t size)
{
    #if ARM
        memcpy(dst, src, size);
    #else
        __movsb((unsigned char*) dst, (const unsigned char*) src, size);
    #endif
}

// 8 byte alignment required and size needs to be multiple of 8
FORCE_INLINE
void compiler_memcpy_aligned(void* __restrict dst, const void* __restrict src, size_t size)
{
    ASSERT_STRICT((size & 7) == 0);

    #if ARM
        memcpy(dst, src, size);
    #else
        __movsq((unsigned long long*) dst, (const unsigned long long*) src, compiler_div_pow2(size, 8));
    #endif
}

FORCE_INLINE
void compiler_memset_unaligned(void* dst, int value, size_t size) {

    #if ARM
        memcpy(dst, src, size);
    #else
        __stosb((unsigned char*) dst, (unsigned char) value, size);
    #endif
}

// 8 byte alignment required and size needs to be multiple of 8
FORCE_INLINE
void compiler_memset_aligned(void* dst, int value, size_t size) {
    ASSERT_STRICT((size & 7) == 0);

    #if ARM
        memcpy(dst, src, size);
    #else
        __stosq((unsigned __int64*) dst, (unsigned __int64) value, compiler_div_pow2(size, 8));
    #endif
}

#define SWAP_ENDIAN_16(val) _byteswap_ushort((val))
#define SWAP_ENDIAN_32(val) _byteswap_ulong((val))
#define SWAP_ENDIAN_64(val) _byteswap_uint64((val))

#include <math.h>
FORCE_INLINE
void sincosf(f32 x, f32* sin, f32* cos) {
    *sin = sinf(x);
    *cos = cosf(x);
}

#endif