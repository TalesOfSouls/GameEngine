/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef TOS_MATH_MATRIX_VECTOR_FLOAT32_H
#define TOS_MATH_MATRIX_VECTOR_FLOAT32_H

#include "../../stdlib/Intrinsics.h"
#include "../../utils/MathUtils.h"
#include "../../stdlib/simd/SIMD_F32.h"

struct v3_f32_4 {
    union {
        struct {
            union {
                f32_4 x;
                f32_4 r;
            };
            union {
                f32_4 y;
                f32_4 g;
            };
            union {
                f32_4 z;
                f32_4 b;
            };
        };

        f32_4 v[3];
    };
};

struct v3_f32_8 {
    union {
        struct {
            union {
                f32_8 x;
                f32_8 r;
            };
            union {
                f32_8 y;
                f32_8 g;
            };
            union {
                f32_8 z;
                f32_8 b;
            };
        };

        f32_8 v[3];
    };
};

struct v3_f32_16 {
    union {
        struct {
            union {
                f32_16 x;
                f32_16 r;
            };
            union {
                f32_16 y;
                f32_16 g;
            };
            union {
                f32_16 z;
                f32_16 b;
            };
        };

        f32_16 v[3];
    };
};

struct v4_f32_4 {
    union {
        struct {
            union {
                f32_4 x;
                f32_4 r;
            };
            union {
                f32_4 y;
                f32_4 g;
            };
            union {
                f32_4 z;
                f32_4 b;
            };
            union {
                f32_4 w;
                f32_4 a;
            };
        };

        f32_4 v[4];
    };
};

struct v4_f32_8 {
    union {
        struct {
            union {
                f32_8 x;
                f32_8 r;
            };
            union {
                f32_8 y;
                f32_8 g;
            };
            union {
                f32_8 z;
                f32_8 b;
            };
            union {
                f32_8 w;
                f32_8 a;
            };
        };

        f32_8 v[4];
    };
};

struct v4_f32_16 {
    union {
        struct {
            union {
                f32_16 x;
                f32_16 r;
            };
            union {
                f32_16 y;
                f32_16 g;
            };
            union {
                f32_16 z;
                f32_16 b;
            };
            union {
                f32_16 w;
                f32_16 a;
            };
        };

        f32_16 v[4];
    };
};

void vec3_normalize_f32(float* x, float* y, float* z)
{
    float d = sqrt((*x) * (*x) + (*y) * (*y) + (*z) * (*z));

    *x /= d;
    *y /= d;
    *z /= d;
}

void vec4_normalize_f32(float* x, float* y, float* z, float* w)
{
    float d = sqrt((*x) * (*x) + (*y) * (*y) + (*z) * (*z) + (*w) * (*w));

    *x /= d;
    *y /= d;
    *z /= d;
    *w /= d;
}

#endif