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

#define ARRAY_COUNT(a) ((a) == NULL ? 0 : (sizeof(a) / sizeof((a)[0])))

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
#define atomic_32 volatile
#define atomic_64 volatile

#define OMS_PI 3.14159265358979323846f
#define OMS_PI_OVER_TWO (OMS_PI / 2.0f)
#define OMS_PI_OVER_FOUR (OMS_PI / 4.0f)
#define OMS_TWO_PI (2.0f * OMS_PI)

#define OMS_MAX(a, b) ((a) > (b) ? (a) : (b))
#define OMS_MIN(a, b) ((a) > (b) ? (b) : (a))

#define OMS_CLAMP(val, low, high) ((val) < (low) ? (low) : ((val) > (high) ? (high) : (val)))

#define OMS_ABS(a) ((a) > 0 ? (a) : -(a))
#define OMS_ABS_INT8(a) ((uint8) ((a) & 0x7F))
#define OMS_ABS_INT16(a) ((uint16) ((a) & 0x7FFF))
#define OMS_ABS_INT32(a) ((uint32) ((a) & 0x7FFFFFFF))
#define OMS_ABS_INT64(a) ((uint64) ((a) & 0x7FFFFFFFFFFFFFFF))
#define OMS_ABS_F32(a) ((f32) (((int32) (a)) & 0x7FFFFFFF))
#define OMS_ABS_F64(a) ((f64) (((int64) (a)) & 0x7FFFFFFFFFFFFFFF))

#define OMS_DEG2RAD(angle) ((angle) * OMS_PI / 180.0f)
#define OMS_RAD2DEG(angle) ((angle) * 180.0f / OMS_PI)

#define ROUND_TO_NEAREST(a, b) (((a) + ((b) - 1)) & ~((b) - 1))
#define OMS_ROUND_POSITIVE(x) ((int32)((x) + 0.5f))
#define OMS_ROUND(x) (((x) >= 0) ? ((int32)((x) + 0.5f)) : ((int32)((x) - 0.5f)))

#define CEIL_DIV(a, b) (((a) + (b) - 1) / (b))
#define OMS_CEIL(x) ((x) == (int32)(x) ? (int32)(x) : ((x) > 0 ? (int32)(x) + 1 : (int32)(x)))

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

struct v3_byte {
    union {
        struct {
            byte x, y, z;
        };

        struct {
            byte r, g, b;
        };

        byte v[3];
    };
};

struct v4_byte {
    union {
        struct {
            byte x, y, z, w;
        };

        struct {
            byte r, g, b, a;
        };

        union {
            byte v[4];
            uint32 val;
        };
    };
};

struct v4_int16 {
    union {
        struct {
            int16 x, y;

            union {
                int16 z, width;
            };

            union {
                int16 w, height;
            };
        };

        int16 v[4];
    };
};

struct v2_int32 {
    union {
        struct {
            int32 x, y;
        };

        struct {
            int32 width, height;
        };

        int32 v[2];
    };
};

struct v3_int32 {
    union {
        struct {
            int32 x, y, z;
        };

        struct {
            int32 r, g, b;
        };

        int32 v[3];
    };
};

struct v4_int32 {
    union {
        struct {
            int32 x, y;

            union {
                int32 z, width;
            };

            union {
                int32 w, height;
            };
        };

        int32 v[4];
    };
};

struct v2_int64 {
    union {
        struct {
            int64 x, y;
        };

        int64 v[2];
    };
};

struct v3_int64 {
    union {
        struct {
            int64 x, y, z;
        };

        struct {
            int64 r, g, b;
        };

        int64 v[3];
    };
};

struct v4_int64 {
    union {
        struct {
            int64 x, y, z, w;
        };

        int64 v[4];
    };
};

struct v2_f32 {
    union {
        struct {
            f32 x, y;
        };

        struct {
            f32 width, height;
        };

        f32 v[2];
    };
};

struct v3_f32 {
    union {
        struct {
            f32 x, y, z;
        };

        struct {
            f32 r, g, b;
        };

        struct {
            f32 pitch, yaw, roll;
        };

        struct {
            f32 u, v, w;
        };

        f32 vec[3];
    };
};

struct v4_f32 {
    union {
        struct {
            f32 x, y;

            union {
                struct {
                    f32 z, w;
                };

                struct {
                    f32 width, height;
                };
            };
        };

        struct {
            f32 x1, y1, x2, y2;
        };

        struct {
            f32 r, g, b, a;
        };

        f32 vec[4];
    };
};

struct v2_f64 {
    union {
        struct {
            f64 x;
            f64 y;
        };

        f64 v[2];
    };
};

struct v3_f64 {
    union {
        struct {
            f64 x, y, z;
        };

        struct {
            f64 r, g, b;
        };

        f64 v[3];
    };
};

struct v4_f64 {
    union {
        struct {
            f64 x;
            f64 y;
            f64 z;
            f64 w;
        };

        f64 v[4];
    };
};

struct m_int32 {
    int32 *e;
    size_t m, n;
};

struct m_int64 {
    int64 *e;
    size_t m, n;
};

struct m_f32 {
    f32 *e;
    size_t m, n;
};

struct m_f64 {
    f64 *e;
    size_t m, n;
};

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

uint16 float_to_f16(float f) {
    uint32_t f_bits = *((uint32_t*)&f);
    uint16_t f16_bits = 0;

    // Extract sign, exponent, and fraction from float
    uint16_t sign = (f_bits & FLOAT32_SIGN_MASK) >> 16;
    int32_t exponent = (int32_t) ((f_bits & FLOAT32_EXP_MASK) >> FLOAT32_EXP_SHIFT) - FLOAT32_EXP_BIAS + HALF_FLOAT_EXP_BIAS;
    uint32_t fraction = (f_bits & FLOAT32_FRAC_MASK) >> (FLOAT32_EXP_SHIFT - HALF_FLOAT_EXP_SHIFT);

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

    f16_bits = (uint16_t) (sign | (exponent << HALF_FLOAT_EXP_SHIFT) | (fraction & HALF_FLOAT_FRAC_MASK));

    return f16_bits;
}

f32 f16_to_float(f16 f) {
    uint32_t sign = (f & HALF_FLOAT_SIGN_MASK) << 16;
    int32_t exponent = (f & HALF_FLOAT_EXP_MASK) >> HALF_FLOAT_EXP_SHIFT;
    uint32_t fraction = (f & HALF_FLOAT_FRAC_MASK) << (FLOAT32_EXP_SHIFT - HALF_FLOAT_EXP_SHIFT);

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

    uint32_t f_bits = sign | (exponent << FLOAT32_EXP_SHIFT) | fraction;

    return BITCAST(f_bits, f32);
}

#endif
