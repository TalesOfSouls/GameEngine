#include "../../stdlib/Types.h"
#include "../../stdlib/Simd.h"
#include "../../math/matrix/Matrix.h"

typedef struct {
    v3_f32 center;           // OBB center
    v3_f32 axis[3];          // Local orthonormal axes u0, u1, u2
    float half[3];           // Half-sizes
    int32 id;
} OBB;

typedef struct {
    float minx, maxx;
    int32 idx;
} AABB1D;

// Project all 3D OBBs onto a single axis (x-axis)
void compute_aabb_x_primitives(const OBB* obbs, int32 n, AABB1D* arr) {
    for (int32 i = 0; i < n; ++i) {
        const OBB* o = &obbs[i];

        float ex = oms_abs(o->axis[0].x) * o->half[0]
            + oms_abs(o->axis[1].x) * o->half[1]
            + oms_abs(o->axis[2].x) * o->half[2];

        arr[i].minx = o->center.x - ex;
        arr[i].maxx = o->center.x + ex;
        arr[i].idx = i;
    }
}

typedef struct { int32 a, b; } AABBPair;

// Generate potential intersecting pairs
AABBPair* generate_aabb_pairs(const AABB1D* arr, int32 n, int* out_count) {
    AABBPair* list = malloc(n * 16 * sizeof(AABBPair)); // rough guess
    int32 cnt = 0;
    for (int32 i = 0; i < n; ++i) {
        for (int32 j = i + 1; j < n && arr[j].minx <= arr[i].maxx; ++j) {
            list[cnt++] = (AABBPair) {arr[i].idx, arr[j].idx};
        }
    }

    *out_count = cnt;

    return list;
}


bool obb_overlap(const OBB *A, const OBB *B)
{
    v3_f32 T;
    vec3_sub(&T, &B->center, &A->center);

    float R[3][3], AbsR[3][3];

    for(int32 i = 0; i < 3; i++) {
        for(int32 j = 0; j < 3; j++) {
            R[i][j] = vec3_dot(&A->axis[i], &B->axis[j]);
            AbsR[i][j] = oms_abs(R[i][j]) + 1e-6f;
        }
    }

    float t[3] = {
        vec3_dot(&T, &A->axis[0]),
        vec3_dot(&T, &A->axis[1]),
        vec3_dot(&T, &A->axis[2])
    };

    for(int32 i = 0; i < 3; i++) {
        float ra = A->half[i];
        float rb = B->half[0] * AbsR[i][0] + B->half[1] * AbsR[i][1] + B->half[2] * AbsR[i][2];

        if (oms_abs(t[i]) > ra + rb) {
            return false;
        }
    }

    for(int32 j = 0; j < 3; j++) {
        float ra = A->half[0] * AbsR[0][j] + A->half[1] * AbsR[1][j] + A->half[2] * AbsR[2][j];
        float tb = vec3_dot(&T, &B->axis[j]);
        float rb = B->half[j];

        if (oms_abs(tb) > ra + rb) {
            return false;
        }
    }

    for(int32 i = 0; i < 3; i++) for(int32 j = 0; j < 3; j++) {
        float ra = A->half[(i + 1) % 3] * AbsR[(i + 2) % 3][j] + A->half[(i + 2) % 3] * AbsR[(i + 1) % 3][j];
        float rb = B->half[(j + 1) % 3] * AbsR[i][(j + 2) % 3] + B->half[(j + 2) % 3] * AbsR[i][(j + 1) % 3];
        float tij = t[(i + 2) % 3] * R[(i + 1) % 3][j] - t[(i + 1) % 3] * R[(i + 2) % 3][j];

        if (oms_abs(tij) > ra + rb) {
            return false;
        }
    }
    return true;
}

typedef struct {
    const OBB* obbs;
    const AABBPair* pairs;
    int32 start, end;
    void(*cb)(int,int);
} TaskB;

void task_worker_broad_scalar(void* _t) {
    TaskB* t = (TaskB*)_t;

    int32 i = t->start;

    for (; i < t->end; ++i) {
        const AABBPair* p = &t->pairs[i];
        const OBB* A = &t->obbs[p->a];
        const OBB* B = &t->obbs[p->b];

        if (obb_overlap(A, B)) {
            // Callback if intersecting
            t->cb(A->id, B->id);
        }
    }
}

#ifdef __SSE4_2__
#include <xmmintrin.h>

void task_worker_broad_sse(void* _t) {
    TaskB* t = (TaskB*)_t;

    int32 i = t->start;
    for (; i + 4 - 1 < t->end; i += 4) {
        alignas(16) float cxA_arr[4], cyA_arr[4], czA_arr[4];
        alignas(16) float cxB_arr[4], cyB_arr[4], czB_arr[4];

        alignas(16) float axA_arr[3][4], ayA_arr[3][4], azA_arr[3][4];
        alignas(16) float axB_arr[3][4], ayB_arr[3][4], azB_arr[3][4];
        alignas(16) float hxA_arr[3][4], hxB_arr[3][4];

        int32 ida[4], idb[4];

        for (int32 j = 0; j < 4; ++j) {
            const OBB* A = &t->obbs[t->pairs[i + j].a];
            const OBB* B = &t->obbs[t->pairs[i + j].b];

            cxA_arr[j] = A->center.x;
            cyA_arr[j] = A->center.y;
            czA_arr[j] = A->center.z;

            cxB_arr[j] = B->center.x;
            cyB_arr[j] = B->center.y;
            czB_arr[j] = B->center.z;

            for (int32 k = 0; k < 3; ++k) {
                axA_arr[k][j] = A->axis[k].x;
                ayA_arr[k][j] = A->axis[k].y;
                azA_arr[k][j] = A->axis[k].z;

                axB_arr[k][j] = B->axis[k].x;
                ayB_arr[k][j] = B->axis[k].y;
                azB_arr[k][j] = B->axis[k].z;

                hxA_arr[k][j] = A->half[k];
                hxB_arr[k][j] = B->half[k];
            }

            ida[j] = A->id;
            idb[j] = B->id;
        }

        __m128 cxA = _mm_load_ps(cxA_arr);
        __m128 cyA = _mm_load_ps(cyA_arr);
        __m128 czA = _mm_load_ps(czA_arr);
        __m128 cxB = _mm_load_ps(cxB_arr);
        __m128 cyB = _mm_load_ps(cyB_arr);
        __m128 czB = _mm_load_ps(czB_arr);

        __m128 dx = _mm_sub_ps(cxB, cxA);
        __m128 dy = _mm_sub_ps(cyB, cyA);
        __m128 dz = _mm_sub_ps(czB, czA);

        // Replace dot3_avx with SSE version or scalar
        // Similar loop logic as AVX2, just using __m128

        // ... Implement the rest as in AVX2 but with SSE ops ...
    }

    for (; i < t->end; ++i) {
        const AABBPair* p = &t->pairs[i];
        const OBB* A = &t->obbs[p->a];
        const OBB* B = &t->obbs[p->b];

        if (obb_overlap(A, B)) {
            t->cb(A->id, B->id);
        }
    }
}
#endif

#ifdef __AVX2__
#include <immintrin.h>

void task_worker_broad_avx2(void* _t) {
    TaskB* t = (TaskB*)_t;

    int32 i = t->start;

    for (; i + 8 - 1 < t->end; i += 8) {
        alignas(32) float cxA_arr[8], cyA_arr[8], czA_arr[8];
        alignas(32) float cxB_arr[8], cyB_arr[8], czB_arr[8];

        alignas(32) float axA_arr[3][8], ayA_arr[3][8], azA_arr[3][8];
        alignas(32) float axB_arr[3][8], ayB_arr[3][8], azB_arr[3][8];
        alignas(32) float hxA_arr[3][8], hxB_arr[3][8];

        int32 ida[8], idb[8];

        // Fill scalar arrays
        for (int32 j = 0; j < 8; ++j) {
            const OBB* A = &t->obbs[t->pairs[i + j].a];
            const OBB* B = &t->obbs[t->pairs[i + j].b];

            cxA_arr[j] = A->center.x;
            cyA_arr[j] = A->center.y;
            czA_arr[j] = A->center.z;

            cxB_arr[j] = B->center.x;
            cyB_arr[j] = B->center.y;
            czB_arr[j] = B->center.z;

            for (int32 k = 0; k < 3; ++k) {
                axA_arr[k][j] = A->axis[k].x;
                ayA_arr[k][j] = A->axis[k].y;
                azA_arr[k][j] = A->axis[k].z;

                axB_arr[k][j] = B->axis[k].x;
                ayB_arr[k][j] = B->axis[k].y;
                azB_arr[k][j] = B->axis[k].z;

                hxA_arr[k][j] = A->half[k];
                hxB_arr[k][j] = B->half[k];
            }

            ida[j] = A->id;
            idb[j] = B->id;
        }

        // Load SIMD vectors from temp arrays
        __m256 cxA = _mm256_load_ps(cxA_arr);
        __m256 cyA = _mm256_load_ps(cyA_arr);
        __m256 czA = _mm256_load_ps(czA_arr);
        __m256 cxB = _mm256_load_ps(cxB_arr);
        __m256 cyB = _mm256_load_ps(cyB_arr);
        __m256 czB = _mm256_load_ps(czB_arr);

        __m256 axA[3], ayA[3], azA[3], axB[3], ayB[3], azB[3], hxA[3], hxB[3];

        for (int32 k = 0; k < 3; ++k) {
            axA[k] = _mm256_load_ps(axA_arr[k]);
            ayA[k] = _mm256_load_ps(ayA_arr[k]);
            azA[k] = _mm256_load_ps(azA_arr[k]);

            axB[k] = _mm256_load_ps(axB_arr[k]);
            ayB[k] = _mm256_load_ps(ayB_arr[k]);
            azB[k] = _mm256_load_ps(azB_arr[k]);

            hxA[k] = _mm256_load_ps(hxA_arr[k]);
            hxB[k] = _mm256_load_ps(hxB_arr[k]);
        }

        // Relative vector T = centerB - centerA
        __m256 dx = _mm256_sub_ps(cxB, cxA);
        __m256 dy = _mm256_sub_ps(cyB, cyA);
        __m256 dz = _mm256_sub_ps(czB, czA);

        // Project T into A's local frame
        __m256 tA[3] = {
            dot3_avx(dx, dy, dz, axA[0], ayA[0], azA[0]),
            dot3_avx(dx, dy, dz, axA[1], ayA[1], azA[1]),
            dot3_avx(dx, dy, dz, axA[2], ayA[2], azA[2]),
        };

        // Partial SAT: test only A's 3 axes
        for (int32 axis = 0; axis < 3; ++axis) {
            __m256 ra = hxA[axis];

            __m256 rb = _mm256_add_ps(
                _mm256_add_ps(
                    _mm256_mul_ps(_mm256_abs_ps(dot3_avx(axA[axis], ayA[axis], azA[axis], axB[0], ayB[0], azB[0])), hxB[0]),
                    _mm256_mul_ps(_mm256_abs_ps(dot3_avx(axA[axis], ayA[axis], azA[axis], axB[1], ayB[1], azB[1])), hxB[1])
                ),
                _mm256_mul_ps(_mm256_abs_ps(dot3_avx(axA[axis], ayA[axis], azA[axis], axB[2], ayB[2], azB[2])), hxB[2])
            );

            __m256 sum = _mm256_add_ps(ra, rb);
            __m256 abs_t = _mm256_abs_ps(tA[axis]);

            __m256 overlap = _mm256_cmp_ps(abs_t, sum, _CMP_LE_OS);
            int32 mask = _mm256_movemask_ps(overlap);

            for (int32 j = 0; j < 8; ++j) {
                if ((mask >> j) & 1) {
                    // Callback if intersecting
                    t->cb(ida[j], idb[j]);
                }
            }
        }
    }

    // Fallback for remaining pairs
    for (; i < t->end; ++i) {
        const AABBPair* p = &t->pairs[i];
        const OBB* A = &t->obbs[p->a];
        const OBB* B = &t->obbs[p->b];

        if (obb_overlap(A, B)) {
            // Callback if intersecting
            t->cb(A->id, B->id);
        }
    }
}
#endif

#ifdef __AVX512F__
#include <immintrin.h>

void task_worker_broad_avx512(void* _t) {
    TaskB* t = (TaskB*)_t;

    int32 i = t->start;
    for (; i + 16 - 1 < t->end; i += 16) {
        alignas(64) float cxA_arr[16], cyA_arr[16], czA_arr[16];
        alignas(64) float cxB_arr[16], cyB_arr[16], czB_arr[16];

        alignas(64) float axA_arr[3][16], ayA_arr[3][16], azA_arr[3][16];
        alignas(64) float axB_arr[3][16], ayB_arr[3][16], azB_arr[3][16];
        alignas(64) float hxA_arr[3][16], hxB_arr[3][16];

        int32 ida[16], idb[16];

        for (int32 j = 0; j < 16; ++j) {
            const OBB* A = &t->obbs[t->pairs[i + j].a];
            const OBB* B = &t->obbs[t->pairs[i + j].b];

            cxA_arr[j] = A->center.x;
            cyA_arr[j] = A->center.y;
            czA_arr[j] = A->center.z;

            cxB_arr[j] = B->center.x;
            cyB_arr[j] = B->center.y;
            czB_arr[j] = B->center.z;

            for (int32 k = 0; k < 3; ++k) {
                axA_arr[k][j] = A->axis[k].x;
                ayA_arr[k][j] = A->axis[k].y;
                azA_arr[k][j] = A->axis[k].z;

                axB_arr[k][j] = B->axis[k].x;
                ayB_arr[k][j] = B->axis[k].y;
                azB_arr[k][j] = B->axis[k].z;

                hxA_arr[k][j] = A->half[k];
                hxB_arr[k][j] = B->half[k];
            }

            ida[j] = A->id;
            idb[j] = B->id;
        }

        __m512 cxA = _mm512_load_ps(cxA_arr);
        __m512 cyA = _mm512_load_ps(cyA_arr);
        __m512 czA = _mm512_load_ps(czA_arr);
        __m512 cxB = _mm512_load_ps(cxB_arr);
        __m512 cyB = _mm512_load_ps(cyB_arr);
        __m512 czB = _mm512_load_ps(czB_arr);

        __m512 dx = _mm512_sub_ps(cxB, cxA);
        __m512 dy = _mm512_sub_ps(cyB, cyA);
        __m512 dz = _mm512_sub_ps(czB, czA);

        // Replace dot3_avx with AVX-512 version or _mm512_fmadd_ps for efficiency
        // Similar logic to AVX2

        // ... Implement the rest as in AVX2 but with AVX512 ops ...
    }

    for (; i < t->end; ++i) {
        const AABBPair* p = &t->pairs[i];
        const OBB* A = &t->obbs[p->a];
        const OBB* B = &t->obbs[p->b];

        if (obb_overlap(A, B)) {
            t->cb(A->id, B->id);
        }
    }
}
#endif


int32 cmp_minx(const void* a, const void* b) {
    return ((AABB1D *)a)->minx < ((AABB1D *)b)->minx ? -1 : 1;
}

void detect_all(const OBB* obbs, int32 n, void(*cb)(int, int))
{
    const int32 steps = 8;

    AABB1D* arr = malloc(n * sizeof(AABB1D));
    compute_aabb_x_primitives(obbs, n, arr);
    qsort(arr, n, sizeof(AABB1D), cmp_minx);

    int32 pair_count;
    AABBPair* pairs = generate_aabb_pairs(arr, n, &pair_count);
    free(arr);

    int32 threads = get_thread_count();
    int32 base_chunk = (pair_count + threads - 1) / threads;

    // Round up to nearest multiple of steps
    base_chunk = ((base_chunk + steps - 1) / steps) * steps;

    for (int32 t = 0; t < threads; ++t) {
        int32 s = t * base_chunk;
        int32 e = s + base_chunk;

        if (e > pair_count) {
            e = pair_count;
        }

        if (s >= e) {
            break; // Prevent launching empty tasks
        }

        TaskB* task = malloc(sizeof(TaskB));
        *task = (TaskB) {.obbs = obbs, .pairs = pairs, .start = s, .end = e, .cb = cb};

        #if defined(__AVX512F__)
            if (steps >= 16) {
                thread_pool_submit(task_worker_broad_avx512, task);
                steps = 0;
            }
        #elif defined(__AVX2__)
            if (steps >= 8) {
                thread_pool_submit(task_worker_broad_avx2, task);
                steps = 0;
            }
        #elif defined(__SSE4_2__)
            if (steps >= 4) {
                thread_pool_submit(task_worker_broad_sse, task);
                steps = 0;
            }
        #else
            if (steps > 0) {
                thread_pool_submit(task_worker_broad_scalar, task);
            }
        #endif
    }

    thread_pool_wait_all();
    free(pairs);
}