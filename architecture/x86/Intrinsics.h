/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_ARCHITECTURE_X86_INTRINSICS_H
#define COMS_ARCHITECTURE_X86_INTRINSICS_H

#include <immintrin.h>
#include <xmmintrin.h>

#if _WIN32
    #include <intrin.h>
#elif __linux__
    #include <x86intrin.h>
    #include <x86gprintrin.h>
#endif

#include "../../stdlib/Types.h"

#ifdef _MSC_VER
    #define intrin_sqrt_f32(a) _mm_cvtss_f32(_mm_sqrt_ss(_mm_set_ss((a))))
#else
    #define intrin_sqrt_f32(a) ({ float res; asm volatile("sqrtss %0, %1" : "=x"(res) : "x"(a)); res; })
#endif

#ifdef _MSC_VER
    #define intrin_sqrt_f64(a) _mm_cvtsd_f64(_mm_sqrt_sd(_mm_set_sd((a)), _mm_set_sd((a))))
#else
    #define intrin_sqrt_f64(a) ({ double res; asm volatile("sqrtsd %0, %1" : "=x" (res) : "x" (a)); res; })
#endif

#define intrin_rsqrt_f32(a) _mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss((a))))
#define intrin_rsqrt_f64(a) _mm_cvtsd_f64(_mm_div_sd(_mm_set_sd(1.0), _mm_sqrt_sd(_mm_set_sd((a)), _mm_set_sd((a)))))

#define intrin_round_f32(a) _mm_cvtss_f32(_mm_round_ss(_mm_setzero_ps(), _mm_set_ss((a)), (_MM_FROUND_TO_NEAREST_INT | _MM_FROUND_NO_EXC)))
#define intrin_round_to_int32(a) _mm_cvtss_si32(_mm_set_ss((a)))
#define intrin_floor_f32(a) _mm_cvtss_f32(_mm_floor_ss(_mm_setzero_ps(), _mm_set_ss((a))))
#define intrin_ceil_f32(a) _mm_cvtss_f32(_mm_ceil_ss(_mm_setzero_ps(), _mm_set_ss((a))))

#define intrin_fence_memory() _mm_mfence()
#define intrin_fence_write() _mm_sfence()
#define intrin_fence_load() _mm_lfence()
#define intrin_invalidate_cache() _mm_clflush()

#define intrin_crc32_u8(crc, data) _mm_crc32_u8((crc), (data))
#define intrin_crc32_u16(crc, data) _mm_crc32_u16((crc), (data))
#define intrin_crc32_u32(crc, data) _mm_crc32_u32((crc), (data))
#define intrin_crc32_u64(crc, data) _mm_crc32_u64((crc), (data))

#define intrin_bits_count_32(data) _mm_popcnt_u32((data))
#define intrin_bits_count_64(data) _mm_popcnt_u64((data))

#define intrin_prefetch_l1(mem) _mm_prefetch((const char *) (mem), _MM_HINT_T0)
#define intrin_prefetch_l2(mem) _mm_prefetch((const char *) (mem), _MM_HINT_T1)
#define intrin_prefetch_l3(mem) _mm_prefetch((const char *) (mem), _MM_HINT_T2)

// a * b + c
#define intrin_fmadd(a, b, c) _mm_cvtss_f32(_mm_fmadd_ss(_mm_set_ss(a), _mm_set_ss(b), _mm_set_ss(c)))

inline
uint64 intrin_timestamp_counter() noexcept {
    _mm_mfence();
    uint64 res = __rdtsc();
    _mm_mfence();

    return res;
}

#endif