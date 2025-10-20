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
#include "../../compiler/CompilerUtils.h"

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

FORCE_INLINE
uint64 intrin_timestamp_counter() NO_EXCEPT {
    #if DEBUG || INTERNAL
        _mm_lfence();
    #endif

    return __rdtsc();
}

#if __AVX2__
    static __m128i _sink128;
    static __m256i _sink256;

    // Pre-loads 128 bit chunks of memory into cache as fast as possible
    // size MUST be multiple of 128
    inline
    void intrin_prefetch_128(void* memory, size_t size)
    {
        void* const end = ((char *) memory) + size;
        __m128i* p0 = (__m128i *) memory;
        __m128i* p1 = (__m128i *) end;
        for (const __m128i* p = p0; p < p1; ++p) {
            _sink128 = *p;
        }
    }

    // Pre-loads 256 bit chunks of memory into cache as fast as possible
    // size MUST be multiple of 256
    inline
    void intrin_prefetch_256(void* memory, size_t size)
    {
        void* const end = ((char *) memory) + size;
        __m256i* p0 = (__m256i *) memory;
        __m256i* p1 = (__m256i *) end;
        for (const __m256i* p = p0; p < p1; ++p) {
            _sink256 = *p;
        }
    }
#else
    static volatile uint64_t _sink128;
    static volatile uint64_t _sink256;

    // Pre-loads 128 bit chunks of memory into cache as fast as possible
    // size MUST be multiple of 128
    inline
    void intrin_prefetch_128(void* memory, size_t size)
    {
        void* const end = ((char *) memory) + size;
        uint64_t* p0 = (uint64_t *) memory;
        uint64_t* p1 = (uint64_t *) end;
        for (const volatile uint64_t* p = p0; p < p1; ++p) {
            _sink128 = *p;
        }
    }

    // Pre-loads 256 bit chunks of memory into cache as fast as possible
    // size MUST be multiple of 256
    inline
    void intrin_prefetch_256(void* memory, size_t size)
    {
        void* const end = ((char *) memory) + size;
        uint64_t* p0 = (uint64_t *) memory;
        uint64_t* p1 = (uint64_t *) end;
        for (const volatile uint64_t* p = p0; p < p1; ++p) {
            _sink256 = *p;
        }
    }
#endif

#endif