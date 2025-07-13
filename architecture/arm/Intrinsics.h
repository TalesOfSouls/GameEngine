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
#include "../../compiler/CompilerUtils.h"

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

#if _WIN32
    #define intrin_timestamp_counter() ({ uint64_t cntvct; asm volatile("mrs %0, cntvct_el0" : "=r"(cntvct)); cntvct;  })
#else
    #define intrin_timestamp_counter() __builtin_readcyclecounter()
#endif

// a * b + c
#define intrin_fmadd(a, b, c) vgetq_lane_f32(vmlaq_f32(vdupq_n_f32(c), vdupq_n_f32(a), vdupq_n_f32(b)), 0)

#endif