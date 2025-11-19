/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_STDLIB_TYPES_H
#define COMS_STDLIB_TYPES_H

// @question Consider to rename to Stdlib.h

#include <stddef.h>
#include <stdint.h>
#include "Defines.h"

#if _WIN32
    #include <intrin.h>
#elif __linux__
    #include <linux/limits.h>
    #include <x86intrin.h>
#endif

#ifdef DEBUG
    #define NO_EXCEPT
#else
    #define NO_EXCEPT noexcept
#endif

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef uint16_t f16;
typedef float f32;
typedef double f64;

typedef unsigned char byte;
typedef char sbyte;

typedef uintptr_t umm;
typedef intptr_t smm;

#define atomic_8 volatile
#define atomic_16 volatile
#define atomic_32 alignas(4) volatile
#define atomic_64 alignas(8) volatile

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
enum DataType {
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
    DATA_TYPE_V4_F32_ARRAY
};

#include "../compiler/CompilerUtils.h"
#include "Helper.h"
#include "../architecture/Intrinsics.h"
#include "../platform/Platform.h"

#endif
