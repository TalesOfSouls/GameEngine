/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_TYPES_H
#define COMS_TYPES_H

#include <stddef.h>

#ifdef __aarch64__
    #include <arm_sve.h>
    #include <arm_acle.h>
    #include <arm_neon.h>
#else
    #if _WIN32
        #include <intrin.h>
    #elif __linux__
        #include <x86intrin.h>
        #include <x86gprintrin.h>
    #endif

    #include <immintrin.h>
    #include <xmmintrin.h>
#endif

#if defined(NO_STDLIB)
    typedef signed char int8;
    typedef signed short int16;
    typedef signed int int32;
    typedef signed long long int64;

    typedef unsigned char uint8;
    typedef unsigned short uint16;
    typedef unsigned int uint32;
    typedef unsigned long long uint64;
#else
    #include <stdint.h>

    typedef int8_t int8;
    typedef int16_t int16;
    typedef int32_t int32;
    typedef int64_t int64;

    typedef uint8_t uint8;
    typedef uint16_t uint16;
    typedef uint32_t uint32;
    typedef uint64_t uint64;
#endif

typedef unsigned int uint;

#if _WIN32
    typedef long long ssize_t;
#endif

/**
 * Some implementations should support 32 bit and 64 bit.
 * For this reason we need types that are universal.
 * And who knows, maybe we will get to 128 bits as default integer sizes.
 */
#if !defined(_WIN64) && !__x86_64__ && !__ppc64__
    typedef int32 sint_max;
    typedef uint32 uint_max;
#else
    typedef int64 sint_max;
    typedef uint64 uint_max;
#endif

typedef float f32;
typedef double f64;

typedef union {
    f32 f;
    uint32 u;
} f32_bits;

typedef union {
    f64 f;
    uint64 u;
} f64_bits;

typedef unsigned char byte;
typedef char sbyte;

typedef uintptr_t umm;
typedef intptr_t smm;

// Do I have to use volatile as atomic_8 etc. (e.g. #define atomic_8 volatile)?
// The problem with that is that I may want to use the variable in single threaded use cases as well
// However, this would block the compiler from optimizing some of the code in single threaded use cases
// I believe volatile in conjunction with atomics is a old C++ memory I have that isn't required any longer
// especially not in our use case.
#define atomic_8
#define atomic_16
#define atomic_32 alignas(4)
#define atomic_64 alignas(8)
#define atomic_ptr alignas(sizeof(uintptr_t))
#define atomic_min alignas(sizeof(int))
#define atomic_max alignas(sizeof(size_t))

// Careful, the sizeof(utf16) or sizeof(wchar_t) is different based on the platform
typedef wchar_t utf16;
typedef uint32 utf8;

struct v2_int8 {
    union {
        struct { int8 x, y; };
        struct { int8 width, height; };
        struct { int8 min, max; };

        int8 vec[2];
        int16 val;
    };
};

struct v3_int8 {
    union {
        struct { int8 x, y, z; };
        struct { int8 r, g, b; };

        int8 vec[3];
    };
};

struct v4_int8 {
    union {
        struct { int8 x, y, z, w; };
        struct { int8 r, g, b, a; };

        int8 vec[4];
        int32 val;
    };
};

struct v3_byte {
    union {
        struct { byte x, y, z; };
        struct { byte r, g, b; };

        byte vec[3];
    };
};

struct v4_byte {
    union {
        struct { byte x, y, z, w; };
        struct { byte r, g, b, a; };

        byte vec[4];
        uint32 val;
    };
};

typedef v3_byte v3_uint8;
typedef v4_byte v4_uint8;

struct v2_int16 {
    union {
        struct { int16 x, y; };
        struct { int16 width, height; };
        struct { int16 min, max; };

        int16 vec[2];
        int32 val;;
    };
};

struct v2_uint16 {
    union {
        struct { uint16 x, y; };
        struct { uint16 width, height; };
        struct { uint16 min, max; };

        uint16 vec[2];
        uint32 val;;
    };
};

struct v4_int16 {
    union {
        struct {
            int16 x, y;

            union { int16 z, width; };
            union { int16 w, height; };
        };

        int16 vec[4];
    };
};

struct v2_int32 {
    union {
        struct { int32 x, y; };
        struct { int32 width, height; };
        struct { int32 min, max; };

        int32 vec[2];
    };
};

struct v3_int32 {
    union {
        struct { int32 x, y, z; };
        struct { int32 r, g, b; };

        int32 vec[3];
    };
};

struct alignas(16) v4_int32 {
    union {
        struct {
            int32 x, y;

            union { int32 z, width; };
            union { int32 w, height; };
        };

        int32 vec[4];

        // Reference to a vec[4]
        const int32* ref;

        #if defined(__SSE4_2__)
            __m128i s_16;
        #endif
    };
};

struct alignas(16) v4_uint32 {
    union {
        struct {
            uint32 x, y;

            union { uint32 z, width; };
            union { uint32 w, height; };
        };

        uint32 vec[4];

        // Reference to a vec[4]
        const uint32* ref;

        #if defined(__SSE4_2__)
            __m128i s_16;
        #endif
    };
};

struct v2_int64 {
    union {
        struct { int64 x, y; };

        int64 vec[2];
    };
};

struct v3_int64 {
    union {
        struct { int64 x, y, z; };
        struct { int64 r, g, b; };

        int64 vec[3];
    };
};

struct v4_int64 {
    union {
        struct { int64 x, y, z, w; };

        int64 vec[4];

        // Reference to a vec[4]
        const int64* ref;

        #if defined(__AVX2__)
            __m256i s_16;
        #endif
    };
};

struct v2_f32 {
    union {
        struct { f32 x, y; };
        struct { f32 width, height; };
        struct { f32 min, max; };

        f32 vec[2];
    };
};

struct v3_f32 {
    union {
        struct { f32 x, y, z; };
        struct { f32 r, g, b; };
        struct { f32 pitch, yaw, roll; };
        struct { f32 u, v, w; };

        f32 vec[3];
    };
};

struct alignas(16) v4_f32 {
    union {
        struct {
            f32 x, y;
            union {
                struct { f32 z, w; };
                struct { f32 width, height; };
            };
        };
        struct { f32 x1, y1, x2, y2; };
        struct { f32 r, g, b, a; };

        f32 vec[4];

        // Reference to a vec[4]
        const f32* ref;

        #if defined(__SSE4_2__)
            __m128 s_16;
        #endif
    };
};
typedef v4_f32 quaternion;

struct v2_f64 {
    union {
        struct { f64 x; f64 y; };

        f64 vec[2];
    };
};

struct v3_f64 {
    union {
        struct { f64 x, y, z; };
        struct { f64 r, g, b; };

        f64 vec[3];
    };
};

struct alignas(64) v16_f32 {
    union {
        v4_f32 rows[4];
        f32 vec[16];
        f32 mat[4][4];

        // Reference to a vec[16]
        const f32* ref;

        #if defined(__SSE4_2__)
            __m128 s_16[4];
        #endif

        #if defined(__AVX2__)
            __m256 s_32[2];
        #endif

        #if defined(__AVX512F__)
            __m512 s_64;
        #endif
    };
};

struct v4_f64 {
    union {
        struct {
            f64 x, y;
            union {
                struct { f64 z, w; };
                struct { f64 width, height; };
            };
        };
        struct { f64 x1, y1, x2, y2; };
        struct { f64 r, g, b, a; };

        f64 vec[4];

        #if defined(__AVX2__)
            __m256d s_16;
        #endif
    };
};

// @todo We cannot use FORCE_INLINE because CompilerUtils depends on this file -> circular dependency
inline v2_f32 to_v2_f32(v2_int32 vec) { return {(f32) vec.x, (f32) vec.y}; }
inline v3_f32 to_v3_f32(v3_int32 vec) { return {(f32) vec.x, (f32) vec.y, (f32) vec.z}; }
inline v4_f32 to_v4_f32(v4_int32 vec) { return {(f32) vec.x, (f32) vec.y, (f32) vec.z, (f32) vec.w}; }

inline v2_int32 to_v2_int32(v2_f32 vec) { return {(int32) vec.x, (int32) vec.y}; }
inline v3_int32 to_v3_int32(v3_f32 vec) { return {(int32) vec.x, (int32) vec.y, (int32) vec.z}; }
inline v4_int32 to_v4_int32(v4_f32 vec) { return {(int32) vec.x, (int32) vec.y, (int32) vec.z, (int32) vec.w}; }

// Data type helpers
enum DataType : byte {
    DATA_TYPE_VOID,

    DATA_TYPE_BOOL,

    DATA_TYPE_INT8,
    DATA_TYPE_INT16,
    DATA_TYPE_INT32,
    DATA_TYPE_INT64,

    DATA_TYPE_UINT8,
    DATA_TYPE_UINT16,
    DATA_TYPE_UINT32,
    DATA_TYPE_UINT64,

    DATA_TYPE_F32,
    DATA_TYPE_F64,

    DATA_TYPE_BOOL_ARRAY,

    DATA_TYPE_INT8_ARRAY,
    DATA_TYPE_INT16_ARRAY,
    DATA_TYPE_INT32_ARRAY,
    DATA_TYPE_INT64_ARRAY,

    DATA_TYPE_UINT8_ARRAY,
    DATA_TYPE_UINT16_ARRAY,
    DATA_TYPE_UINT32_ARRAY,
    DATA_TYPE_UINT64_ARRAY,

    DATA_TYPE_F32_ARRAY,
    DATA_TYPE_F64_ARRAY,

    DATA_TYPE_CHAR,
    DATA_TYPE_CHAR_STR,
    DATA_TYPE_WCHAR_STR,

    DATA_TYPE_BYTE_ARRAY,

    DATA_TYPE_V2_INT16,
    DATA_TYPE_V2_UINT16,

    DATA_TYPE_V2_INT32,
    DATA_TYPE_V3_INT32,
    DATA_TYPE_V4_INT32,

    DATA_TYPE_V2_F32,
    DATA_TYPE_V3_F32,
    DATA_TYPE_V4_F32,

    DATA_TYPE_V2_INT32_ARRAY,
    DATA_TYPE_V3_INT32_ARRAY,
    DATA_TYPE_V4_INT32_ARRAY,

    DATA_TYPE_V2_F32_ARRAY,
    DATA_TYPE_V3_F32_ARRAY,
    DATA_TYPE_V4_F32_ARRAY,

    DATA_TYPE_STRUCT,
};

struct timespec
{
    time_t tv_sec;  // Seconds - >= 0
    long   tv_nsec; // Nanoseconds - [0, 999999999]
};
#define _CRT_NO_TIME_T 1

#endif