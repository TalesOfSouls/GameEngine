/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_TEST_FRAMEWORK_H
#define COMS_TEST_FRAMEWORK_H

#include <stdio.h>
#include <math.h>
#include <inttypes.h>
#include "../architecture/Intrinsics.h"

static char **_test_log;
static int32_t _test_assert_count;
static int32_t _test_assert_error_count;
static int32_t _test_count = -1;

static int32_t _test_suit_count = 0;
static int32_t _test_global_assert_count = 0;
static int32_t _test_global_assert_error_count = 0;
static int32_t _test_global_count = 0;

static int64_t _test_start;
static int64_t _test_total_start;

#define TEST_PROFILING_LOOPS 1000

#define TEST_HEADER()                                             \
    _test_total_start = test_start_time();                \
    printf("\nStat Tests   Assert(OK/NG)   Time(ms)  Details\n"); \
    printf("========================================================================================================================\n")

#define TEST_FOOTER()                                                                                                                     \
    printf("========================================================================================================================\n"); \
    printf(                                                                                                                               \
        "%s %5d   (%5d/%5d)   %8.2f\n\n",                                                                                                 \
        _test_global_assert_count ? "[NG]" : "[OK]",                                                                                      \
        _test_global_count,                                                                                                               \
        _test_global_assert_count - _test_global_assert_error_count,                                                                      \
        _test_global_assert_count,                                                                                                        \
        test_duration_time(_test_total_start) / 1000000);                                                                                 \
    printf("%d test suits\n\n", _test_suit_count)

#ifdef UBER_TEST
#define TEST_INIT_HEADER() (void)0
#define TEST_FINALIZE_FOOTER() (void)0
#else
#define TEST_INIT_HEADER() TEST_HEADER()
#define TEST_FINALIZE_FOOTER() TEST_FOOTER()
#endif

#if _WIN32
#include "../platform/win32/ExceptionHandler.h"
#include <windows.h>
LONG WINAPI test_exception_handler(EXCEPTION_POINTERS *exception_info)
{
    printf("Exception code: 0x%x\n", exception_info->ExceptionRecord->ExceptionCode);
    print_stack_trace(exception_info->ContextRecord);

    return EXCEPTION_EXECUTE_HANDLER;
}

int64_t test_start_time()
{
    LARGE_INTEGER start;
    QueryPerformanceCounter(&start);

    return start.QuadPart;
}

double test_duration_time(int64_t start)
{
    return (double)(test_start_time() - start) / frequency.QuadPart;
}

double test_measure_func_time_ns(void (*func)(volatile void *), volatile void *para)
{
    LARGE_INTEGER frequency, start, end;
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&start);
    for (int32_t i = 0; i < TEST_PROFILING_LOOPS; ++i)
    {
        func(para);
    }
    QueryPerformanceCounter(&end);
    return (double)(end.QuadPart - start.QuadPart) * 1e9 / frequency.QuadPart;
}

#define TEST_INIT(test_count)                                                                  \
    do                                                                                         \
    {                                                                                          \
        TEST_INIT_HEADER();                                                                    \
        setvbuf(stdout, NULL, _IONBF, 0);                                                      \
        SetUnhandledExceptionFilter(test_exception_handler);                                   \
        _test_start = test_start_time();                                                       \
        _test_assert_error_count = 0;                                                          \
        _test_count = 0;                                                                       \
        _test_assert_count = 0;                                                                \
        _test_log = (char **)calloc((test_count), sizeof(char *) + (test_count) * 1024);       \
        if (_test_log)                                                                         \
        {                                                                                      \
            for (int i = 0; i < (test_count); i++)                                             \
            {                                                                                  \
                _test_log[i] = (char *)(_test_log + (test_count) * sizeof(char *) + i * 1024); \
            }                                                                                  \
        }                                                                                      \
    } while (0)
#else
#include "../platform/linux/ExceptionHandler.h"
#include <time.h>
void test_exception_handler(int signum)
{
    printf("Received signal: %d\n", signum);
    print_stack_trace();
    exit(1);
}

int64_t test_start_time()
{
    struct timespec start;
    clock_gettime(CLOCK_MONOTONIC, &start);

    return start.tv_sec * 1e9 + start.tv_nsec;
}

double test_duration_time(int64_t start)
{
    return (double) (test_start_time() - start);
}

double test_measure_func_time_ns(void (*func)(volatile void *), volatile void *para)
{
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    for (int32_t i = 0; i < TEST_PROFILING_LOOPS; ++i)
    {
        func(para);
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    return (double)(end.tv_sec * 1e9 + end.tv_nsec) - (double)(start.tv_sec * 1e9 + start.tv_nsec);
}

#define TEST_INIT(test_count)                                                                  \
    do                                                                                         \
    {                                                                                          \
        ++_test_suit_count;                                                                    \
        TEST_INIT_HEADER();                                                                    \
        setvbuf(stdout, NULL, _IONBF, 0);                                                      \
        signal(SIGSEGV, test_exception_handler);                                               \
        signal(SIGABRT, test_exception_handler);                                               \
        _test_start = test_start_time();                                                       \
        _test_assert_error_count = 0;                                                          \
        _test_count = 0;                                                                       \
        _test_assert_count = 0;                                                                \
        _test_log = (char **)calloc((test_count), sizeof(char *) + (test_count) * 1024);       \
        if (_test_log)                                                                         \
        {                                                                                      \
            for (int i = 0; i < (test_count); i++)                                             \
            {                                                                                  \
                _test_log[i] = (char *)(_test_log + (test_count) * sizeof(char *) + i * 1024); \
            }                                                                                  \
        }                                                                                      \
    } while (0)
#endif

#define TEST_FINALIZE()                                                                                                                               \
    do                                                                                                                                                \
    {                                                                                                                                                 \
        if (_test_assert_error_count)                                                                                                                 \
        {                                                                                                                                             \
            printf(                                                                                                                                   \
                "[NG] %5d   (%5d/%5d)   %8.2f   %s\n",                                                                                                \
                _test_count, _test_assert_count - _test_assert_error_count, _test_assert_count, test_duration_time(_test_start) / 1000000, __FILE__); \
            for (int i = 0; i < _test_assert_error_count; ++i)                                                                                        \
            {                                                                                                                                         \
                printf("                                            %s\n", _test_log[i]);                                                             \
                fflush(stdout);                                                                                                                       \
            }                                                                                                                                         \
        }                                                                                                                                             \
        else                                                                                                                                          \
        {                                                                                                                                             \
            printf(                                                                                                                                   \
                "[OK] %5d   (%5d/%5d)   %8.2f   %s\n",                                                                                                \
                _test_count, _test_assert_count - _test_assert_error_count, _test_assert_count, test_duration_time(_test_start) / 1000000, __FILE__); \
        }                                                                                                                                             \
        fflush(stdout);                                                                                                                               \
        free(_test_log);                                                                                                                              \
        _test_log = NULL;                                                                                                                             \
        _test_assert_error_count = 0;                                                                                                                 \
        _test_count = 0;                                                                                                                              \
        _test_assert_count = 0;                                                                                                                       \
        TEST_FINALIZE_FOOTER();                                                                                                                       \
    } while (0)

#define TEST_RUN(func)    \
    ++_test_count;        \
    ++_test_global_count; \
    func()

#define ASSERT_EQUALS(a, b)                                                        \
    do                                                                             \
    {                                                                              \
        ++_test_assert_count;                                                      \
        ++_test_global_assert_count;                                               \
        if ((a) != (b))                                                            \
        {                                                                          \
            ++_test_global_assert_error_count;                                     \
            snprintf(                                                              \
                _test_log[_test_assert_error_count++], 1024,                       \
                "%4i: %" PRId64 " != %" PRId64, __LINE__, (int64)(a), (int64)(b)); \
        }                                                                          \
    } while (0)

#define ASSERT_GREATER_THAN(a, b)                                       \
    do                                                                  \
    {                                                                   \
        ++_test_assert_count;                                           \
        ++_test_global_assert_count;                                    \
        if ((a) <= (b))                                                 \
        {                                                               \
            ++_test_global_assert_error_count;                          \
            snprintf(                                                   \
                _test_log[_test_assert_error_count++], 1024,            \
                "%4i: %.3f <= %.3f", __LINE__, (float)(a), (float)(b)); \
        }                                                               \
    } while (0)

#define ASSERT_LESSER_THAN(a, b)                                        \
    do                                                                  \
    {                                                                   \
        ++_test_assert_count;                                           \
        ++_test_global_assert_count;                                    \
        if ((a) >= (b))                                                 \
        {                                                               \
            ++_test_global_assert_error_count;                          \
            snprintf(                                                   \
                _test_log[_test_assert_error_count++], 1024,            \
                "%4i: %.3f >= %.3f", __LINE__, (float)(a), (float)(b)); \
        }                                                               \
    } while (0)

#define ASSERT_NOT_EQUALS(a, b)                                                    \
    do                                                                             \
    {                                                                              \
        ++_test_assert_count;                                                      \
        ++_test_global_assert_count;                                               \
        if ((a) == (b))                                                            \
        {                                                                          \
            ++_test_global_assert_error_count;                                     \
            snprintf(                                                              \
                _test_log[_test_assert_error_count++], 1024,                       \
                "%4i: %" PRId64 " == %" PRId64, __LINE__, (int64)(a), (int64)(b)); \
        }                                                                          \
    } while (0)

#define ASSERT_EQUALS_WITH_DELTA(a, b, delta)                   \
    do                                                          \
    {                                                           \
        ++_test_assert_count;                                   \
        ++_test_global_assert_count;                            \
        if (OMS_ABS((a) - (b)) > (delta))                       \
        {                                                       \
            ++_test_global_assert_error_count;                  \
            snprintf(                                           \
                _test_log[_test_assert_error_count++], 1024,    \
                "%4i: %f != %f", __LINE__, (f64)(a), (f64)(b)); \
        }                                                       \
    } while (0)

#define ASSERT_CONTAINS(a, b)                                \
    do                                                       \
    {                                                        \
        ++_test_assert_count;                                \
        ++_test_global_assert_count;                         \
        if (str_find((a), (b)) == NULL)                      \
        {                                                    \
            ++_test_global_assert_error_count;               \
            snprintf(                                        \
                _test_log[_test_assert_error_count++], 1024, \
                "%4i: %s !contains %s", __LINE__, (a), (b)); \
        }                                                    \
    } while (0)

#define ASSERT_TRUE(a)                                       \
    do                                                       \
    {                                                        \
        ++_test_assert_count;                                \
        ++_test_global_assert_count;                         \
        if ((a) != true)                                     \
        {                                                    \
            ++_test_global_assert_error_count;               \
            snprintf(                                        \
                _test_log[_test_assert_error_count++], 1024, \
                "%4i: is false", __LINE__);                  \
        }                                                    \
    } while (0)

#define ASSERT_FALSE(a)                                      \
    do                                                       \
    {                                                        \
        ++_test_assert_count;                                \
        ++_test_global_assert_count;                         \
        if ((a) != false)                                    \
        {                                                    \
            ++_test_global_assert_error_count;               \
            snprintf(                                        \
                _test_log[_test_assert_error_count++], 1024, \
                "%4i: is true", __LINE__);                   \
        }                                                    \
    } while (0)

#define ASSERT_TRUE_CONST(a)                                 \
    do                                                       \
    {                                                        \
        ++_test_assert_count;                                \
        ++_test_global_assert_count;                         \
        if constexpr ((a) != true)                           \
        {                                                    \
            ++_test_global_assert_error_count;               \
            snprintf(                                        \
                _test_log[_test_assert_error_count++], 1024, \
                "%4i: is false", __LINE__);                  \
        }                                                    \
    } while (0)

#define ASSERT_FALSE_CONST(a)                                \
    do                                                       \
    {                                                        \
        ++_test_assert_count;                                \
        ++_test_global_assert_count;                         \
        if constexpr ((a) != false)                          \
        {                                                    \
            ++_test_global_assert_error_count;               \
            snprintf(                                        \
                _test_log[_test_assert_error_count++], 1024, \
                "%4i: is true", __LINE__);                   \
        }                                                    \
    } while (0)

#define ASSERT_MEMORY_EQUALS(ptr1, ptr2, len)                              \
    do                                                                     \
    {                                                                      \
        ++_test_assert_count;                                              \
        ++_test_global_assert_count;                                       \
        const uint8_t *p1 = (const uint8_t *)(ptr1);                       \
        const uint8_t *p2 = (const uint8_t *)(ptr2);                       \
        for (uint64_t i = 0; i < (uint64_t)(len); ++i)                     \
        {                                                                  \
            if (p1[i] != p2[i])                                            \
            {                                                              \
                ++_test_global_assert_error_count;                         \
                snprintf(_test_log[_test_global_assert_error_count], 1024, \
                         "%4i: mismatch at offset %" PRId64, __LINE__, i); \
                break;                                                     \
            }                                                              \
        }                                                                  \
    } while (0)

#define COMPARE_FUNCTION_TEST_CYCLE(func1, func2, x_percent)                                                      \
    do                                                                                                            \
    {                                                                                                             \
        ++_test_assert_count;                                                                                     \
        ++_test_global_assert_count;                                                                              \
        uint64_t cycles_func1, cycles_func2;                                                                      \
        uint64_t start, end;                                                                                      \
        int64_t a = 0, b = 0;                                                                                     \
                                                                                                                  \
        /* Measure func1 */                                                                                       \
        start = intrin_timestamp_counter();                                                                       \
        for (int32_t i = 0; i < TEST_PROFILING_LOOPS; ++i)                                                        \
        {                                                                                                         \
            func1((volatile void *)&a);                                                                           \
        }                                                                                                         \
        end = intrin_timestamp_counter();                                                                         \
        cycles_func1 = end - start;                                                                               \
                                                                                                                  \
        /* Measure func2 */                                                                                       \
        start = intrin_timestamp_counter();                                                                       \
        for (int32_t i = 0; i < TEST_PROFILING_LOOPS; ++i)                                                        \
        {                                                                                                         \
            func2((volatile void *)&b);                                                                           \
        }                                                                                                         \
        end = intrin_timestamp_counter();                                                                         \
        cycles_func2 = end - start;                                                                               \
                                                                                                                  \
        /* Calculate percentage difference */                                                                     \
        double percent_diff = 100.0 * ((double)cycles_func1 - (double)cycles_func2) / (double)cycles_func2;       \
                                                                                                                  \
        /* Check if within x% */                                                                                  \
        if (isnormal(percent_diff) && percent_diff >= x_percent)                                                  \
        {                                                                                                         \
            ++_test_global_assert_error_count;                                                                    \
            snprintf(                                                                                             \
                _test_log[_test_assert_error_count++], 1024,                                                      \
                "%4i: %.2f%% (%s: %" PRIu64 " cycles, %s: %" PRIu64 " cycles)",                                   \
                __LINE__, percent_diff + 100.0f, #func1, (uint64_t)cycles_func1, #func2, (uint64_t)cycles_func2); \
        }                                                                                                         \
        ASSERT_TRUE((a && b) || a == b);                                                                          \
    } while (0)

#define ASSERT_FUNCTION_TEST_CYCLE(func, cycles)                                \
    do                                                                          \
    {                                                                           \
        ++_test_assert_count;                                                   \
        ++_test_global_assert_count;                                            \
        uint64_t cycles_func;                                                   \
        uint64_t start, end;                                                    \
        int64_t para = 0;                                                       \
                                                                                \
        /* Measure func */                                                      \
        start = intrin_timestamp_counter();                                     \
        for (int32_t i = 0; i < TEST_PROFILING_LOOPS; ++i)                      \
        {                                                                       \
            func((volatile void *)&para);                                       \
        }                                                                       \
        end = intrin_timestamp_counter();                                       \
        cycles_func = end - start;                                              \
                                                                                \
        if (cycles_func >= cycles)                                              \
        {                                                                       \
            ++_test_global_assert_error_count;                                  \
            snprintf(                                                           \
                _test_log[_test_assert_error_count++], 1024,                    \
                "%4i: %.2f%% (%s: %" PRIu64 " cycles)",                         \
                __LINE__, percent_diff + 100.0f, #func, (uint64_t)cycles_func); \
        }                                                                       \
    } while (0)

#define COMPARE_FUNCTION_TEST_TIME(func1, func2, x_percent)                               \
    do                                                                                    \
    {                                                                                     \
        ++_test_assert_count;                                                             \
        ++_test_global_assert_count;                                                      \
        double time_func1, time_func2;                                                    \
        int64_t a = 0, b = 0;                                                             \
                                                                                          \
        /* Measure func1 */                                                               \
        time_func1 = test_measure_func_time_ns(func1, (volatile void *)&a);               \
                                                                                          \
        /* Measure func2 */                                                               \
        time_func2 = test_measure_func_time_ns(func2, (volatile void *)&b);               \
                                                                                          \
        /* Calculate percentage difference */                                             \
        double percent_diff = 100.0 * (time_func1 - time_func2) / time_func2;             \
                                                                                          \
        /* Print results */                                                               \
        if (isnormal(percent_diff) && percent_diff >= x_percent)                          \
        {                                                                                 \
            ++_test_global_assert_error_count;                                            \
            snprintf(                                                                     \
                _test_log[_test_assert_error_count++], 1024,                              \
                "%4i: %.2f%% (%s: %.2f us, %s: %.2f us)",                                 \
                __LINE__, percent_diff + 100.0f, #func1, time_func1, #func2, time_func2); \
        }                                                                                 \
        ASSERT_TRUE((a && b) || a == b);                                                  \
    } while (0)

#endif