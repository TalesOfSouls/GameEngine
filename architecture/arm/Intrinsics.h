/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_ARCHITECTURE_ARM_INTRINSICS_ARM_H
#define COMS_ARCHITECTURE_ARM_INTRINSICS_ARM_H

#include <arm_sve.h>
#include <arm_acle.h>
#include <arm_neon.h>
#include "../../stdlib/Types.h"

#define intrin_sqrt_f32(a) svget1_f32(svsqrt_f32(svdup_f32((a))))
#define intrin_sqrt_f64(a) svget1_f64(svsqrt_f64(svdup_f64((a))))

#define intrin_rsqrt_f32(a) svget1_f32(svrsqrte_f32(svdup_f32((a))))
#define intrin_rsqrt_f64(a) svget1_f64(svrsqrte_f64(svdup_f64((a))))

#define intrin_round_f32(a) svget1_f32(svrndn_f32(svdup_f32((a))))
#define intrin_round_to_int32(a) svget1_s32(svcvtn_f32_s32(svdup_f32((a)), SVE_32B))
#define intrin_floor_f32(a) svget1_f32(svfloor_f32(svdup_f32((a))))
#define intrin_ceil_f32(a) svget1_f32(svceil_f32(svdup_f32((a))))

#define intrin_fence_memory() __dmb(0xF)
#define intrin_fence_write() __dmb(0xB)
#define intrin_fence_load() __dmb(0x7)
#define intrin_invalidate_cache() asm volatile("dc ivac, %0" : : "r"(address) : "memory")

#define intrin_crc32_u8(crc, data) __crc32b((crc), (data))
#define intrin_crc32_u16(crc, data) __crc32h((crc), (data))
#define intrin_crc32_u32(crc, data) __crc32w((crc), (data))
#define intrin_crc32_u64(crc, data) __crc32d((crc), (data))

#define intrin_bits_count_32(data) compiler_popcount_32((data))
#define intrin_bits_count_64(data) compiler_popcount_64((data))

#define intrin_prefetch(mem) compiler_prefetch((mem))
#define intrin_prefetch_l1(mem) compiler_prefetch_l1((mem))
#define intrin_prefetch_l2(mem) compiler_prefetch_l2((mem))
#define intrin_prefetch_l3(mem) compiler_prefetch_l3((mem))

#define cpu_yield() asm volatile("yield")

#if _WIN32
    #define intrin_timestamp_counter() ({ uint64_t cntvct; asm volatile("mrs %0, cntvct_el0" : "=r"(cntvct)); cntvct;  })
#else
    #define intrin_timestamp_counter() __builtin_readcyclecounter()
#endif

#ifdef __ARM_NEON
    static volatile uint8x16_t _sink128;
    static volatile uint8x16_t _sink256;
    static volatile uint8x16_t _sink512;

    // Pre-loads 128 bit chunks of memory into cache as fast as possible
    // size MUST be multiple of 128
    inline
    void intrin_prefetch_128(void* const memory, size_t size)
    {
        void* const end = ((char *) memory) + size;
        uint8x16_t* p0 = (uint8x16_t *) memory;
        uint8x16_t* p1 = (uint8x16_t *) end;
        for (const volatile uint8x16_t* p = p0; p < p1; ++p) {
            _sink128 = *p;
        }
    }

    // Pre-loads 256 bit chunks of memory into cache as fast as possible
    // size MUST be multiple of 256
    inline
    void intrin_prefetch_256(void* const memory, size_t size)
    {
        void* const end = ((char *) memory) + size;
        uint8x16_t* p0 = (uint8x16_t *) memory;
        uint8x16_t* p1 = (uint8x16_t *) end;
        for (const volatile uint8x16_t* p = p0; p < p1; ++p) {
            _sink256 = *p;
        }
    }

    // Pre-loads 512 bit chunks of memory into cache as fast as possible
    // size MUST be multiple of 512
    inline
    void intrin_prefetch_512(void* const memory, size_t size)
    {
        void* const end = ((char *) memory) + size;
        uint8x16_t* p0 = (uint8x16_t *) memory;
        uint8x16_t* p1 = (uint8x16_t *) end;
        for (const volatile uint8x16_t* p = p0; p < p1; ++p) {
            _sink512 = *p;
        }
    }
#else
    static volatile size_t _sink128;
    static volatile size_t _sink256;
    static volatile size_t _sink512;

    // Pre-loads 128 bit chunks of memory into cache as fast as possible
    // size MUST be multiple of 128
    inline
    void intrin_prefetch_128(void* const memory, size_t size)
    {
        void* const end = ((char *) memory) + size;
        size_t* p0 = (size_t *) memory;
        size_t* p1 = (size_t *) end;
        for (const volatile size_t* p = p0; p < p1; ++p) {
            _sink128 = *p;
        }
    }

    // Pre-loads 256 bit chunks of memory into cache as fast as possible
    // size MUST be multiple of 256
    inline
    void intrin_prefetch_256(void* const memory, size_t size)
    {
        void* const end = ((char *) memory) + size;
        size_t* p0 = (size_t *) memory;
        size_t* p1 = (size_t *) end;
        for (const volatile size_t* p = p0; p < p1; ++p) {
            _sink256 = *p;
        }
    }

    // Pre-loads 512 bit chunks of memory into cache as fast as possible
    // size MUST be multiple of 512
    inline
    void intrin_prefetch_512(void* const memory, size_t size)
    {
        void* const end = ((char *) memory) + size;
        size_t* p0 = (size_t *) memory;
        size_t* p1 = (size_t *) end;
        for (const volatile size_t* p = p0; p < p1; ++p) {
            _sink512 = *p;
        }
    }
#endif

#endif