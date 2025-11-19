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
#include "../../stdlib/Simd.h"
#include "../../compiler/CompilerUtils.h"

#ifdef _MSC_VER
    #define intrin_sqrt_f32(a) _mm_cvtss_f32(_mm_sqrt_ss(_mm_set_ss((a))))
#else
    #define intrin_sqrt_f32(a) ({ float res; asm volatile("sqrtss %0, %1" : "=x"(res) : "x"(a)); res; })
#endif

#ifdef _MSC_VER
    #define intrin_sqrt_f64(a) _mm_cvtsd_f64(_mm_sqrt_sd(_mm_set_sd(0.0), _mm_set_sd((a))))
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

FORCE_INLINE
uint64 intrin_timestamp_counter() NO_EXCEPT {
    #if DEBUG || INTERNAL
        _mm_lfence();
    #endif

    return __rdtsc();
}

#ifdef __AVX512F__
    static __m512i _sink512;
#endif
#if defined(__AVX2__) || defined(__AVX512F__)
    static __m256i _sink256;
#endif
#if defined(__SSE4_2__) || defined(__AVX2__) || defined(__AVX512F__)
    static __m128i _sink128;
#endif
static volatile size_t _sink_scalar;

// the size needs to be a multiple of the steps e.g. steps = 16 means 32 bytes multiple required
inline
void intrin_prefetch(void* memory, size_t size, int32_t steps = 16) NO_EXCEPT
{
    byte* start = (byte*)memory;
    byte* end = start + size;
    steps = intrin_validate_steps(start, steps);

    #ifdef __AVX512F__
        if (steps >= 16) {
            __m512i* p0 = (__m512i*)start;
            __m512i* p1 = (__m512i*)end;

            for (const __m512i* p = p0; p < p1; ++p) {
                _sink512 = *p;
            }

            return;
        }
    #endif

    #ifdef __AVX2__
        if (steps >= 8) {
            __m256i* p0 = (__m256i*)start;
            __m256i* p1 = (__m256i*)end;

            for (const __m256i* p = p0; p < p1; ++p) {
                _sink256 = *p;
            }

            return;
        }
    #endif

    #ifdef __SSE4_2__
        if (steps >= 4) {
            __m128i* p0 = (__m128i*)start;
            __m128i* p1 = (__m128i*)end;

            for (const __m128i* p = p0; p < p1; ++p) {
                _sink128 = *p;
            }

            return;
        }
    #endif

    size_t* p0 = (size_t*)start;
    size_t* p1 = (size_t*)end;
    for (const volatile size_t* p = p0; p < p1; ++p) {
        _sink_scalar = *p;
    }
}

inline
void intrin_swap_memory(void* __restrict a, void* __restrict b, size_t size, int32_t steps = 16) NO_EXCEPT
{
    byte* p = (byte*)a;
    byte* q = (byte*)b;
    steps = intrin_validate_steps((const byte *) a, steps);
    steps = intrin_validate_steps((const byte *) b, steps);

    #ifdef __AVX512F__
        if (steps >= 16) {
            while (size >= sizeof(__m512i)) {
                __m512i tmp = *(__m512i*)p;
                *(__m512i*)p = *(__m512i*)q;
                *(__m512i*)q = tmp;

                p += sizeof(__m512i);
                q += sizeof(__m512i);
                size -= sizeof(__m512i);
            }

            if (size == 0) {
                return;
            }
        }
    #endif

    #ifdef __AVX2__
        if (steps >= 8) {
            while (size >= sizeof(__m256i)) {
                __m256i tmp = *(__m256i*)p;
                *(__m256i*)p = *(__m256i*)q;
                *(__m256i*)q = tmp;

                p += sizeof(__m256i);
                q += sizeof(__m256i);
                size -= sizeof(__m256i);
            }

            if (size == 0) {
                return;
            }
        }
    #endif

    #ifdef __SSE4_2__
        if (steps >= 4) {
            while (size >= sizeof(__m128i)) {
                __m128i tmp = *(__m128i*)p;
                *(__m128i*)p = *(__m128i*)q;
                *(__m128i*)q = tmp;

                p += sizeof(__m128i);
                q += sizeof(__m128i);
                size -= sizeof(__m128i);
            }

            if (size == 0) {
                return;
            }
        }
    #endif

    while (size >= sizeof(size_t)) {
        size_t tmp = *(size_t*)p;
        *(size_t*)p = *(size_t*)q;
        *(size_t*)q = tmp;

        p += sizeof(size_t);
        q += sizeof(size_t);
        size -= sizeof(size_t);
    }

    if (size == 0) {
        return;
    }

    // Byte tail
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