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

#include <stddef.h>
#include <stdint.h>

#if _WIN32
    // @question Do I really need <windows.h> here or could I go lower?
    #include <windows.h>
    typedef SSIZE_T ssize_t;
#elif __linux__
    #include <linux/limits.h>
    #define MAX_PATH PATH_MAX
#endif

// Counts the elements in an array IFF its size is defined at compile time
#define ARRAY_COUNT(a) ((a) == NULL ? 0 : (sizeof(a) / sizeof((a)[0])))

// Gets the size of a struct member
#define MEMBER_SIZEOF(type, member) (sizeof( ((type *)0)->member ))

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

// PI
#define OMS_PI 3.14159265358979323846f
#define OMS_PI_OVER_TWO (OMS_PI / 2.0f)
#define OMS_PI_OVER_FOUR (OMS_PI / 4.0f)
#define OMS_TWO_PI (2.0f * OMS_PI)

// Limits
#define OMS_MAX_BRANCHED(a, b) ((a) > (b) ? (a) : (b))
#define OMS_MIN_BRANCHED(a, b) ((a) > (b) ? (b) : (a))
#define OMS_MAX_BRANCHLESS(a, b) ((a) ^ (((a) ^ (b)) & -((a) < (b))))
#define OMS_MIN_BRANCHLESS(a, b) ((b) ^ (((a) ^ (b)) & -((a) < (b))))
#define OMS_CLAMP_BRANCHED(val, low, high) ((val) < (low) ? (low) : ((val) > (high) ? (high) : (val)))
#define OMS_CLAMP_BRANCHLESS(val, low, high) \
    ((val) ^ (((val) ^ (low)) & -(int32)((val) < (low))) ^ \
    ((((val) ^ ((val) ^ (low)) & -(int32)((val) < (low)))) ^ (high)) & -(int32)((((val) ^ ((val) ^ (low)) & -(int32)((val) < (low)))) < (high)))

// Abs
#define OMS_ABS(a) ((a) > 0 ? (a) : -(a))
#define OMS_ABS_INT8(a) (((a) ^ ((a) >> 7)) - ((a) >> 7))
#define OMS_ABS_INT16(a) (((a) ^ ((a) >> 15)) - ((a) >> 15))
#define OMS_ABS_INT32(a) (((a) ^ ((a) >> 31)) - ((a) >> 31))
#define OMS_ABS_INT64(a) (((a) ^ ((a) >> 63)) - ((a) >> 63))

inline
f32 OMS_ABS_F32(f32 a) {
    union { f32 f; uint32 i; } u;
    u.f = a;
    u.i &= 0x7FFFFFFF;
    return u.f;
}

inline
f64 OMS_ABS_F64(f64 a) {
    union { f64 f; uint64 i; } u;
    u.f = a;
    u.i &= 0x7FFFFFFFFFFFFFFF;
    return u.f;
}

// Trig
#define OMS_DEG2RAD(angle) ((angle) * OMS_PI / 180.0f)
#define OMS_RAD2DEG(angle) ((angle) * 180.0f / OMS_PI)

// Rounding
#define OMS_ROUND_POSITIVE_32(x) ((int32)((x) + 0.5f))
#define OMS_ROUND_POSITIVE_64(x) ((int64)((x) + 0.5f))
#define OMS_ROUND(x) (((x) >= 0) ? (f32)((int32)((x) + 0.5f)) : (f32)((int32)((x) - 0.5f)))

#define CEIL_DIV(a, b) (((a) + (b) - 1) / (b))
#define OMS_CEIL(x) ((x) == (int32)(x) ? (int32)(x) : ((x) > 0 ? (int32)(x) + 1 : (int32)(x)))

#define FLOORF(x) ((float)((int)(x) - ((x) < 0.0f && (x) != (int)(x))))

// Zero and comparison
#define OMS_EPSILON_F32 1.19209290e-07f
#define OMS_EPSILON_F64 2.2204460492503131e-16

#define OMS_IS_ZERO_F32(x) (OMS_ABS(x) < OMS_EPSILON_F32)
#define OMS_IS_ZERO_F64(x) (OMS_ABS(x) < OMS_EPSILON_F64)
#define OMS_FEQUAL_F32(a, b) (OMS_ABS((a) - (b)) < OMS_EPSILON_F32)
#define OMS_FEQUAL_F64(a, b) (OMS_ABS((a) - (b)) < OMS_EPSILON_F64)

#define OMS_HAS_ZERO(x) (((x) - ((size_t)-1 / 0xFF)) & ~(x) & (((size_t)-1 / 0xFF) * (0xFF / 2 + 1)))
#define OMS_HAS_CHAR(x, c) (OMS_HAS_ZERO((x) ^ (((size_t)-1 / 0xFF) * (c))))

// Bitwise utilities
#define OMS_IS_POW2(x) (((x) > 0) && (((x) & ((x) - 1)) == 0))
#define OMS_ALIGN_UP(x, align) (((x) + ((align) - 1)) & ~((align) - 1))
#define OMS_ALIGN_DOWN(x, align) ((x) & ~((align) - 1))
#define OMS_IS_ALIGNED(x, align) (((x) & ((align) - 1)) == 0)
#define OMS_HAS_FLAG(val, flag) (((val) & (flag)))
#define OMS_SET_FLAG(val, flag) ((val) |= (flag))
#define OMS_CLEAR_FLAG(val, flag) ((val) &= ~(flag))
#define OMS_TOGGLE_FLAG(val, flag) ((val) ^= (flag))

// This is the same as using % but for sufficiently large wrapping this is faster
// WARNING: if wrap is a power of 2 don't use this but use the & operator
//          I recommend to use this macro if wrap >= 1,000
#define OMS_WRAPPED_INCREMENT(value, wrap) ++value; if (value >= wrap) [[unlikely]] value = 0
#define OMS_WRAPPED_DECREMENT(value, wrap) --value; if (value < 0) [[unlikely]] value = wrap - 1

// Casting between e.g. f32 and int32 without changing bits
#define BITCAST(x, new_type) bitcast_impl_##new_type(x)
#define DEFINE_BITCAST_FUNCTION(from_type, to_type) \
    static inline to_type bitcast_impl_##to_type(from_type src) { \
        union { from_type src; to_type dst; } u; \
        u.src = src; \
        return u.dst; \
    }

DEFINE_BITCAST_FUNCTION(f32, uint32)
DEFINE_BITCAST_FUNCTION(uint32, f32)
DEFINE_BITCAST_FUNCTION(f64, uint64)
DEFINE_BITCAST_FUNCTION(uint64, f64)
DEFINE_BITCAST_FUNCTION(f32, int32)
DEFINE_BITCAST_FUNCTION(int32, f32)
DEFINE_BITCAST_FUNCTION(f64, int64)
DEFINE_BITCAST_FUNCTION(int64, f64)

#define FLOAT_CAST_EPS 0.001953125f

// Modulo function when b is a power of 2
#define MODULO_2(a, b) ((a) & (b - 1))

#define SQRT_2 1.4142135623730950488016887242097f

#define KILOBYTE 1024
#define MEGABYTE 1048576
#define GIGABYTE 1073741824

#define MAX_BYTE 0xFF
#define MAX_UINT16 0xFFFF
#define MAX_UINT32 0xFFFFFFFF
#define MAX_UINT64 0xFFFFFFFFFFFFFFFF

#define MAX_CHAR 0x7F
#define MAX_INT16 0x7FFF
#define MAX_INT32 0x7FFFFFFF
#define MAX_INT64 0x7FFFFFFFFFFFFFFF

#define MIN_CHAR 0x80
#define MIN_INT16 0x8000
#define MIN_INT32 0x80000000
#define MIN_INT64 0x8000000000000000

#define MIN_MILLI 60000
#define SEC_MILLI 1000
#define MIN_MICRO 60000000
#define SEC_MICRO 1000000
#define MILLI_MICRO 1000

#define MHZ 1000000
#define GHZ 1000000000

struct v2_int8 {
    union {
        struct { int8 x, y; };
        struct { int8 width, height; };
        struct { int8 min, max; };

        union { int8 v[2]; int16 val; };
    };
};

struct v3_int8 {
    union {
        struct { int8 x, y, z; };
        struct { int8 r, g, b; };

        int8 v[3];
    };
};

struct v4_int8 {
    union {
        struct { int8 x, y, z, w; };
        struct { int8 r, g, b, a; };

        union { int8 v[4]; int32 val; };
    };
};

struct v3_byte {
    union {
        struct { byte x, y, z; };
        struct { byte r, g, b; };

        byte v[3];
    };
};

struct v4_byte {
    union {
        struct { byte x, y, z, w; };
        struct { byte r, g, b, a; };

        union { byte v[4]; uint32 val; };
    };
};

struct v2_int16 {
    union {
        struct { int16 x, y; };
        struct { int16 width, height; };
        struct { int16 min, max; };

        union { int16 v[2]; int32 val; };
    };
};

struct v4_int16 {
    union {
        struct {
            int16 x, y;

            union { int16 z, width; };
            union { int16 w, height; };
        };

        int16 v[4];
    };
};

struct v2_int32 {
    union {
        struct { int32 x, y; };
        struct { int32 width, height; };
        struct { int32 min, max; };

        int32 v[2];
    };
};

struct v3_int32 {
    union {
        struct { int32 x, y, z; };
        struct { int32 r, g, b; };

        int32 v[3];
    };
};

struct v4_int32 {
    union {
        struct {
            int32 x, y;

            union { int32 z, width; };
            union { int32 w, height; };
        };

        int32 v[4];
    };
};

struct v2_int64 {
    union {
        struct { int64 x, y; };

        int64 v[2];
    };
};

struct v3_int64 {
    union {
        struct { int64 x, y, z; };
        struct { int64 r, g, b; };

        int64 v[3];
    };
};

struct v4_int64 {
    union {
        struct { int64 x, y, z, w; };

        int64 v[4];
    };
};

struct v2_f32 {
    union {
        struct { f32 x, y; };
        struct { f32 width, height; };
        struct { f32 min, max; };

        f32 v[2];
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

struct v4_f32 {
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
    };
};

struct v2_f64 {
    union {
        struct { f64 x; f64 y; };

        f64 v[2];
    };
};

struct v3_f64 {
    union {
        struct { f64 x, y, z; };
        struct { f64 r, g, b; };

        f64 v[3];
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
    };
};

struct m3x3_f32 {
    union {
        f32 e[9];
        f32 m[3][3];
    };
};

struct m4x4_f32 {
    union {
        f32 e[16];
        f32 m[4][4];
    };
};

struct m_int32 {
    int32* e;
    size_t m, n;
};

struct m_int64 {
    int64* e;
    size_t m, n;
};

struct m_f32 {
    f32* e;
    size_t m, n;
};

struct m_f64 {
    f64* e;
    size_t m, n;
};

// @todo We cannot use FORCE_INLINE because CompilerUtils depends on this file -> circular dependency
inline v2_f32 to_v2_f32(v2_int32 vec) { return {(f32) vec.x, (f32) vec.y}; }
inline v3_f32 to_v3_f32(v3_int32 vec) { return {(f32) vec.x, (f32) vec.y, (f32) vec.z}; }
inline v4_f32 to_v4_f32(v4_int32 vec) { return {(f32) vec.x, (f32) vec.y, (f32) vec.z, (f32) vec.w}; }

inline v2_int32 to_v2_int32(v2_f32 vec) { return {(int32) vec.x, (int32) vec.y}; }
inline v3_int32 to_v3_int32(v3_f32 vec) { return {(int32) vec.x, (int32) vec.y, (int32) vec.z}; }
inline v4_int32 to_v4_int32(v4_f32 vec) { return {(int32) vec.x, (int32) vec.y, (int32) vec.z, (int32) vec.w}; }

#define HALF_FLOAT_SIGN_MASK 0x8000
#define HALF_FLOAT_EXP_MASK 0x7C00
#define HALF_FLOAT_FRAC_MASK 0x03FF

#define HALF_FLOAT_EXP_SHIFT 10
#define HALF_FLOAT_EXP_BIAS 15

#define FLOAT32_SIGN_MASK 0x80000000
#define FLOAT32_EXP_MASK 0x7F800000
#define FLOAT32_FRAC_MASK 0x007FFFFF

#define FLOAT32_EXP_SHIFT 23
#define FLOAT32_EXP_BIAS 127

f16 float_to_f16(f32 f) NO_EXCEPT {
    uint32 f_bits = *((uint32*)&f);

    // Extract sign, exponent, and fraction from float
    uint16 sign = (f_bits & FLOAT32_SIGN_MASK) >> 16;
    int32 exponent = (int32) ((f_bits & FLOAT32_EXP_MASK) >> FLOAT32_EXP_SHIFT) - FLOAT32_EXP_BIAS + HALF_FLOAT_EXP_BIAS;
    uint32 fraction = (f_bits & FLOAT32_FRAC_MASK) >> (FLOAT32_EXP_SHIFT - HALF_FLOAT_EXP_SHIFT);

    if (exponent <= 0) {
        if (exponent < -10) {
            fraction = 0;
        } else {
            fraction = (fraction | 0x0400) >> (1 - exponent);
        }
        exponent = 0;
    } else if (exponent >= 0x1F) {
        exponent = 0x1F;
        fraction = 0;
    }

    return (f16) (sign | (exponent << HALF_FLOAT_EXP_SHIFT) | (fraction & HALF_FLOAT_FRAC_MASK));
}

f32 f16_to_float(f16 f) NO_EXCEPT {
    uint32 sign = (f & HALF_FLOAT_SIGN_MASK) << 16;
    int32 exponent = (f & HALF_FLOAT_EXP_MASK) >> HALF_FLOAT_EXP_SHIFT;
    uint32 fraction = (f & HALF_FLOAT_FRAC_MASK) << (FLOAT32_EXP_SHIFT - HALF_FLOAT_EXP_SHIFT);

    if (exponent == 0) {
        if (fraction != 0) {
            exponent = 1;
            while ((fraction & (1 << FLOAT32_EXP_SHIFT)) == 0) {
                fraction <<= 1;
                --exponent;
            }
            fraction &= ~FLOAT32_EXP_MASK;
        }
    } else if (exponent == 0x1F) {
        exponent = 0xFF;
    } else {
        exponent += FLOAT32_EXP_BIAS - HALF_FLOAT_EXP_BIAS;
    }

    uint32 f_bits = sign | (exponent << FLOAT32_EXP_SHIFT) | fraction;

    return BITCAST(f_bits, f32);
}

// Simple iterator implementation
#define iterator_start(start, end, obj) {   \
    uint32 _i = start;                      \
    while (_i++ < end) {

#define iterator_end    \
        obj += 1;       \
    }}                  \

#include "../compiler/CompilerUtils.h"
#include "../architecture/Intrinsics.h"
#include "../platform/Platform.h"

#endif
