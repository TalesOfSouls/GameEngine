/**
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_ARCHITECTURE_ARM_INTRINSICS_ARM_H
#define COMS_ARCHITECTURE_ARM_INTRINSICS_ARM_H

// arm_acle.h/arm_neon.h (gcc, clang) or intrin.h/arm_neon.h (msvc) are included by Types.h
#include <math.h>
#include <string.h>
#include "../../stdlib/Types.h"

// Scalar math: the math.h functions compile to the single AArch64 instructions (fsqrt, frintn, frintm, frintp)
// No SVE: baseline ARMv8 (and every Windows on ARM cpu) has no SVE
#define intrin_sqrt_f32(a) sqrtf((a))
#define intrin_sqrt_f64(a) sqrt((a))

#define intrin_rsqrt_f32(a) (1.0f / sqrtf((a)))
#define intrin_rsqrt_f64(a) (1.0 / sqrt((a)))

// Round to nearest, ties to even (default rounding mode) = _MM_FROUND_TO_NEAREST_INT of the x86 version
#define intrin_round_f32(a) nearbyintf((a))
#define intrin_round_to_int32(a) ((int32) lrintf((a)))
#define intrin_floor_f32(a) floorf((a))
#define intrin_ceil_f32(a) ceilf((a))

// Data memory barriers (inner shareable domain): full, stores, loads
// gcc: the instructions directly (the ACLE barrier/hint intrinsics only exist in newer gcc versions)
#ifdef _MSC_VER
    #define intrin_fence_memory() __dmb(_ARM64_BARRIER_ISH)
    #define intrin_fence_write() __dmb(_ARM64_BARRIER_ISHST)
    #define intrin_fence_load() __dmb(_ARM64_BARRIER_ISHLD)
#else
    #define intrin_fence_memory() asm volatile("dmb ish" : : : "memory")
    #define intrin_fence_write() asm volatile("dmb ishst" : : : "memory")
    #define intrin_fence_load() asm volatile("dmb ishld" : : : "memory")
#endif

#ifdef _MSC_VER
    #define intrin_invalidate_cache() ((void) 0)
#else
    #define intrin_invalidate_cache() asm volatile("dc ivac, %0" : : "r"(address) : "memory")
#endif

// CRC-32C (Castagnoli) like the x86 version (_mm_crc32_*), NOT the CRC-32 of __crc32b/h/w/d (@see hash/Crc.h)
// The crc instructions are optional in ARMv8.0 (gcc: -march=armv8-a+crc), Windows on ARM always has them
#if defined(_MSC_VER) || defined(__ARM_FEATURE_CRC32)
    #define intrin_crc32_u8(crc, data) __crc32cb((uint32) (crc), (uint8) (data))
    #define intrin_crc32_u16(crc, data) __crc32ch((uint32) (crc), (uint16) (data))
    #define intrin_crc32_u32(crc, data) __crc32cw((uint32) (crc), (uint32) (data))
    #define intrin_crc32_u64(crc, data) __crc32cd((uint32) (crc), (uint64) (data))
#else
    // Software CRC-32C (reflected polynomial 0x82F63B78), the bytes of data in memory order (little endian)
    FORCE_INLINE
    uint32 intrin_crc32c_software(uint32 crc, uint64 data, int32 bytes) NO_EXCEPT
    {
        for (int32 i = 0; i < bytes; ++i) {
            crc ^= (uint32) (data & 0xFF);
            data >>= 8;

            for (int32 bit = 0; bit < 8; ++bit) {
                crc = (crc >> 1) ^ (0x82F63B78 & (0U - (crc & 1)));
            }
        }

        return crc;
    }

    #define intrin_crc32_u8(crc, data) intrin_crc32c_software((uint32) (crc), (uint8) (data), 1)
    #define intrin_crc32_u16(crc, data) intrin_crc32c_software((uint32) (crc), (uint16) (data), 2)
    #define intrin_crc32_u32(crc, data) intrin_crc32c_software((uint32) (crc), (uint32) (data), 4)
    #define intrin_crc32_u64(crc, data) intrin_crc32c_software((uint32) (crc), (uint64) (data), 8)
#endif

#define intrin_bits_count_32(data) compiler_popcount_32((data))
#define intrin_bits_count_64(data) compiler_popcount_64((data))

#define intrin_prefetch(mem) compiler_prefetch((mem))
#define intrin_prefetch_l1(mem) compiler_prefetch_l1((mem))
#define intrin_prefetch_l2(mem) compiler_prefetch_l2((mem))
#define intrin_prefetch_l3(mem) compiler_prefetch_l3((mem))

#ifdef _MSC_VER
    #define cpu_yield() __yield()
#else
    #define cpu_yield() asm volatile("yield")
#endif

// CNTVCT_EL0 for _ReadStatusReg = ARM64_CNTVCT = ARM64_SYSREG(3, 3, 14, 0, 2) of winnt.h
// Own name: this header doesn't need windows.h + a later winnt.h would redefine ARM64_CNTVCT (warning C4005)
#define COMS_ARM64_CNTVCT 0x5F02

FORCE_INLINE
uint64 intrin_timestamp_counter() NO_EXCEPT
{
    // Virtual counter (readable from user mode, constant frequency)
    #ifdef _MSC_VER
        return (uint64) _ReadStatusReg(COMS_ARM64_CNTVCT);
    #else
        uint64 cntvct;
        asm volatile("mrs %0, cntvct_el0" : "=r"(cntvct));

        return cntvct;
    #endif
}

static volatile size_t _sink128;
static volatile size_t _sink256;
static volatile size_t _sink512;

// Pre-loads 128 bit chunks of memory into cache as fast as possible
// size MUST be multiple of 128
inline
void intrin_prefetch_128(void* const memory, size_t size) NO_EXCEPT
{
    const size_t* const end = (const size_t *) (((char *) memory) + size);
    for (const volatile size_t* p = (const size_t *) memory; p < end; ++p) {
        _sink128 = *p;
    }
}

// Pre-loads 256 bit chunks of memory into cache as fast as possible
// size MUST be multiple of 256
inline
void intrin_prefetch_256(void* const memory, size_t size) NO_EXCEPT
{
    const size_t* const end = (const size_t *) (((char *) memory) + size);
    for (const volatile size_t* p = (const size_t *) memory; p < end; ++p) {
        _sink256 = *p;
    }
}

// Pre-loads 512 bit chunks of memory into cache as fast as possible
// size MUST be multiple of 512
inline
void intrin_prefetch_512(void* const memory, size_t size) NO_EXCEPT
{
    const size_t* const end = (const size_t *) (((char *) memory) + size);
    for (const volatile size_t* p = (const size_t *) memory; p < end; ++p) {
        _sink512 = *p;
    }
}

// Same interface as the x86 version, the compiler vectorizes the word loop
inline
void intrin_swap_memory(void* const __restrict a, void* const __restrict b, size_t size, MAYBE_UNUSED int32_t steps = 16) NO_EXCEPT
{
    byte* p = (byte *) a;
    byte* q = (byte *) b;

    while (size >= sizeof(size_t)) {
        size_t tmp;
        memcpy(&tmp, p, sizeof(size_t));
        memcpy(p, q, sizeof(size_t));
        memcpy(q, &tmp, sizeof(size_t));

        p += sizeof(size_t);
        q += sizeof(size_t);
        size -= sizeof(size_t);
    }

    while (size > 0) {
        byte tmp = *p;
        *p = *q;
        *q = tmp;

        ++p;
        ++q;
        --size;
    }
}

#endif
