/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_MATH_MATRIX_VECTOR_INT32_H
#define COMS_MATH_MATRIX_VECTOR_INT32_H

#include <immintrin.h>
#include <xmmintrin.h>

#include "../../stdlib/Simd.h"

struct v3_int32_4 {
    union {
        struct {
            union {
                int32_4 x;
                int32_4 r;
            };
            union {
                int32_4 y;
                int32_4 g;
            };
            union {
                int32_4 z;
                int32_4 b;
            };
        };

        int32_4 v[3];
    };
};

struct v3_int32_8 {
    union {
        struct {
            union {
                int32_8 x;
                int32_8 r;
            };
            union {
                int32_8 y;
                int32_8 g;
            };
            union {
                int32_8 z;
                int32_8 b;
            };
        };

        int32_8 v[3];
    };
};

struct v3_int32_16 {
    union {
        struct {
            union {
                int32_16 x;
                int32_16 r;
            };
            union {
                int32_16 y;
                int32_16 g;
            };
            union {
                int32_16 z;
                int32_16 b;
            };
        };

        int32_16 v[3];
    };
};

struct v4_int32_4 {
    union {
        struct {
            union {
                int32_4 x;
                int32_4 r;
            };
            union {
                int32_4 y;
                int32_4 g;
            };
            union {
                int32_4 z;
                int32_4 b;
            };
            union {
                int32_4 w;
                int32_4 a;
            };
        };

        int32_4 v[4];
    };
};

struct v4_int32_8 {
    union {
        struct {
            union {
                int32_8 x;
                int32_8 r;
            };
            union {
                int32_8 y;
                int32_8 g;
            };
            union {
                int32_8 z;
                int32_8 b;
            };
            union {
                int32_8 w;
                int32_8 a;
            };
        };

        int32_8 v[4];
    };
};

struct v4_int32_16 {
    union {
        struct {
            union {
                int32_16 x;
                int32_16 r;
            };
            union {
                int32_16 y;
                int32_16 g;
            };
            union {
                int32_16 z;
                int32_16 b;
            };
            union {
                int32_16 w;
                int32_16 a;
            };
        };

        int32_16 v[4];
    };
};

#endif
