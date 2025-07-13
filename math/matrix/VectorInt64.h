/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_MATH_MATRIX_VECTOR_INT64_H
#define COMS_MATH_MATRIX_VECTOR_INT64_H

#include <immintrin.h>
#include <xmmintrin.h>

#include "../../stdlib/Simd.h"

struct v3_int64_2 {
    union {
        struct {
            union {
                int64_2 x;
                int64_2 r;
            };
            union {
                int64_2 y;
                int64_2 g;
            };
            union {
                int64_2 z;
                int64_2 b;
            };
        };

        int64_2 v[3];
    };
};

struct v3_int64_4 {
    union {
        struct {
            union {
                int64_4 x;
                int64_4 r;
            };
            union {
                int64_4 y;
                int64_4 g;
            };
            union {
                int64_4 z;
                int64_4 b;
            };
        };

        int64_4 v[3];
    };
};

struct v3_int64_8 {
    union {
        struct {
            union {
                int64_8 x;
                int64_8 r;
            };
            union {
                int64_8 y;
                int64_8 g;
            };
            union {
                int64_8 z;
                int64_8 b;
            };
        };

        int64_8 v[3];
    };
};

struct v4_int64_2 {
    union {
        struct {
            union {
                int64_2 x;
                int64_2 r;
            };
            union {
                int64_2 y;
                int64_2 g;
            };
            union {
                int64_2 z;
                int64_2 b;
            };
            union {
                int64_2 w;
                int64_2 a;
            };
        };

        int64_2 v[4];
    };
};

struct v4_int64_4 {
    union {
        struct {
            union {
                int64_4 x;
                int64_4 r;
            };
            union {
                int64_4 y;
                int64_4 g;
            };
            union {
                int64_4 z;
                int64_4 b;
            };
            union {
                int64_4 w;
                int64_4 a;
            };
        };

        int64_4 v[4];
    };
};

struct v4_int64_8 {
    union {
        struct {
            union {
                int64_8 x;
                int64_8 r;
            };
            union {
                int64_8 y;
                int64_8 g;
            };
            union {
                int64_8 z;
                int64_8 b;
            };
            union {
                int64_8 w;
                int64_8 a;
            };
        };

        int64_8 v[4];
    };
};

#endif
