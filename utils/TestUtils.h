/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef TOS_UTILS_TEST_UTILS_H
#define TOS_UTILS_TEST_UTILS_H

#include <time.h>
#include "../stdlib/Types.h"

#if _WIN32
    #include <intrin.h>
#else
    #include <x86intrin.h>
#endif

#define MAX_LOG_LENGTH 128

global_persist uint64 performance_count_frequency;
struct TimingStat {
    uint64 old_tick_count;
    uint64 new_tick_count;

    uint64 delta_tick;
    double delta_time;
};

struct LogPool {
    char* memory;
    uint32 pos;
    uint32 count;

    // uint32 size = count * MAX_LOG_LENGTH
};

// IMPORTANT: This function should only be called when you actually use this data
//      e.g. log to display or file
inline
void update_timing_stat(TimingStat *stat)
{
    stat->new_tick_count = __rdtsc();

    stat->delta_tick = stat->new_tick_count - stat->old_tick_count;
    stat->delta_time = (double) stat->delta_tick / (double) performance_count_frequency;

    stat->old_tick_count = stat->new_tick_count;
}

// Sometimes we want to only do logging in debug mode.
// In such cases use the following macro.
#if DEBUG
    #define UPDATE_TIMING_STAT(stat) update_timing_stat(stat)
#else
    #define UPDATE_TIMING_STAT(stat) ((void)0)
#endif

void profile_function(const char* func_name, void (*func)(void*), void* data, int iterations)
{
    clock_t start = clock();
    for (int i = 0; i < iterations; ++i) {
        func(data);
    }

    clock_t end = clock();
    double elapsed_time = (double)(end - start) / CLOCKS_PER_SEC;

    printf("Time taken by %s: %f seconds\n", func_name, elapsed_time);
}

#define ASSERT_EQUALS(a, b, t1, t2)                      \
    ({                                                   \
        if ((a) == (b)) {                                \
            printf(".");                                 \
        } else {                                         \
            printf("\033[31m[F]\033[0m");                \
            printf("\n\n%s - %i: ", __FILE__, __LINE__); \
            printf((t1), (a));                           \
            printf(" != ");                              \
            printf((t2), (b));                           \
            printf("\n");                                \
            return 0;                                    \
        }                                                \
    })

#define ASSERT_NOT_EQUALS(a, b, t1, t2)                  \
    ({                                                   \
        if ((a) != (b)) {                                \
            printf(".");                                 \
        } else {                                         \
            printf("\033[31m[F]\033[0m");                \
            printf("\n\n%s - %i: ", __FILE__, __LINE__); \
            printf((t1), (a));                           \
            printf(" == ");                              \
            printf((t2), (b));                           \
            printf("\n");                                \
            return 0;                                    \
        }                                                \
    })

#define ASSERT_EQUALS_WITH_DELTA(a, b, delta, t1, t2)    \
    ({                                                   \
        if (OMS_ABS((a) - (b)) <= (delta)) {             \
            printf(".");                                 \
        } else {                                         \
            printf("\033[31m[F]\033[0m");                \
            printf("\n\n%s - %i: ", __FILE__, __LINE__); \
            printf((t1), (a));                           \
            printf(" != ");                              \
            printf((t2), (b));                           \
            printf("\n");                                \
            return 0;                                    \
        }                                                \
    })

#define ASSERT_CONTAINS(a, b)                            \
    ({                                                   \
        if (strstr((a), (b)) != NULL) {                  \
            printf(".");                                 \
        } else {                                         \
            printf("\033[31m[F]\033[0m");                \
            printf("\n\n%s - %i: ", __FILE__, __LINE__); \
            printf("%s", (a));                           \
            printf(" !contains ");                       \
            printf("%s", (b));                           \
            printf("\n");                                \
            return 0;                                    \
        }                                                \
    })

#if DEBUG
    #define ASSERT_SIMPLE(a)                             \
        if ((a) == false) {                              \
            *(volatile int *)0 = 0;                      \
        }
#else
    #define ASSERT_SIMPLE(a) ((void)0)
#endif

#define ASSERT_TRUE(a)                                   \
    ({                                                   \
        if ((a) == true) {                               \
            printf(".");                                 \
        } else {                                         \
            printf("\033[31m[F]\033[0m");                \
            printf("\n\n%s - %i: ", __FILE__, __LINE__); \
            printf("%d", (a));                           \
            printf(" != ");                              \
            printf("1");                                 \
            printf("\n");                                \
            return 0;                                    \
        }                                                \
    })

#define ASSERT_FALSE(a)                                  \
    ({                                                   \
        if ((a) == false) {                              \
            printf(".");                                 \
        } else {                                         \
            printf("\033[31m[F]\033[0m");                \
            printf("\n\n%s - %i: ", __FILE__, __LINE__); \
            printf("%d", (a));                           \
            printf(" != ");                              \
            printf("1");                                 \
            printf("\n");                                \
            return 0;                                    \
        }                                                \
    })

#endif