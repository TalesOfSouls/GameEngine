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

static char **_test_log;
static int _test_assert_count;
static int _test_assert_error_count;
static int _test_count = -1;

static int _test_suit_count = 0;
static int _test_global_assert_count = 0;
static int _test_global_assert_error_count = 0;
static int _test_global_count = 0;

static int64_t _test_start;
static int64_t _test_total_start;

// If you need more loop iterations when performing a profiling test, temporarily modify this value
static int _test_profiling_loops = 1000;

#if _WIN32
    #include <windows.h>
    #include <dbghelp.h>

    #ifdef _MSC_VER
        #pragma comment(lib, "dbghelp.lib")
    #endif

    #ifdef __aarch64__
        #define test_timestamp_counter() ({ uint64_t cntvct; asm volatile("mrs %0, cntvct_el0" : "=r"(cntvct)); cntvct; })
    #else
        #include <immintrin.h>
        #include <intrin.h>
        uint64_t test_timestamp_counter() {
            #if DEBUG || INTERNAL
                _mm_lfence();
            #endif

            return __rdtsc();
        }
    #endif
#else
    #define test_timestamp_counter() __builtin_readcyclecounter()
#endif

#define TEST_HEADER()                                              \
    _test_total_start = test_start_time();                         \
    printf("\nStat Tests   Assert(OK/NG)    Time(s)   Details\n"); \
    printf("========================================================================================================================\n")

#define TEST_FOOTER()                                                                                                                     \
    printf("========================================================================================================================\n"); \
    printf(                                                                                                                               \
        "%s %5d   (%5d/%5d)   %8.2f\n\n",                                                                                                 \
        _test_global_assert_error_count ? "[NG]" : "[OK]",                                                                                \
        _test_global_count,                                                                                                               \
        _test_global_assert_count - _test_global_assert_error_count,                                                                      \
        _test_global_assert_count,                                                                                                        \
        test_duration_time(_test_total_start));                                                                                           \
    printf("%d test suits\n\n", _test_suit_count)

#ifdef UBER_TEST
    #define TEST_INIT_HEADER() (void)0
    #define TEST_FINALIZE_FOOTER() (void)0
#else
    #define TEST_INIT_HEADER() TEST_HEADER()
    #define TEST_FINALIZE_FOOTER() TEST_FOOTER()
#endif

#if _WIN32
static inline
void test_print_stack_trace(CONTEXT* context)
{
    HANDLE process = GetCurrentProcess();
    HANDLE thread = GetCurrentThread();

    // Initialize symbols
    SymInitialize(process, NULL, TRUE);

    STACKFRAME64 stack_frame = {};
    DWORD machine_type = IMAGE_FILE_MACHINE_AMD64;

    // Initialize stack frame for x64
    stack_frame.AddrPC.Offset = context->Rip;
    stack_frame.AddrPC.Mode = AddrModeFlat;
    stack_frame.AddrFrame.Offset = context->Rbp;
    stack_frame.AddrFrame.Mode = AddrModeFlat;
    stack_frame.AddrStack.Offset = context->Rsp;
    stack_frame.AddrStack.Mode = AddrModeFlat;

    printf("Stack trace:\n");

    while (StackWalk64(machine_type, process, thread, &stack_frame, context, NULL,
        SymFunctionTableAccess64, SymGetModuleBase64, NULL)
    ) {
        DWORD64 address = stack_frame.AddrPC.Offset;

        // Resolve symbol information
        char symbol_buffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(TCHAR)];
        PSYMBOL_INFO symbol = (PSYMBOL_INFO)symbol_buffer;
        symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
        symbol->MaxNameLen = MAX_SYM_NAME;

        if (SymFromAddr(process, address, NULL, symbol)) {
            printf("Function: %s - Address: 0x%llx\n", symbol->Name, symbol->Address);
        } else {
            printf("Function: (unknown) - Address: 0x%llx\n", address);
        }

        // Resolve file and line number
        IMAGEHLP_LINE64 line;
        DWORD displacement = 0;
        line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);

        if (SymGetLineFromAddr64(process, address, &displacement, &line)) {
            printf("    File: %s, Line: %lu\n", line.FileName, line.LineNumber);
        } else {
            printf("    File: (unknown), Line: (unknown)\n");
        }
    }

    SymCleanup(process);
}

LONG WINAPI test_exception_handler(EXCEPTION_POINTERS *exception_info)
{
    printf("Exception code: 0x%x\n", exception_info->ExceptionRecord->ExceptionCode);
    test_print_stack_trace(exception_info->ContextRecord);

    return EXCEPTION_EXECUTE_HANDLER;
}

inline int64_t test_start_time()
{
    LARGE_INTEGER start;
    QueryPerformanceCounter(&start);

    return start.QuadPart;
}

inline double test_duration_time(int64_t start)
{
    LARGE_INTEGER frequency;
    QueryPerformanceFrequency(&frequency);
    return (double)(test_start_time() - start) / frequency.QuadPart;
}

inline double test_measure_func_time_ns(void (*func)(volatile void *), volatile void* para, int profiling_loops)
{
    LARGE_INTEGER frequency, start, end;
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&start);
    for (int i = 0; i < profiling_loops; ++i) {
        func(para);
    }
    QueryPerformanceCounter(&end);
    return (double)(end.QuadPart - start.QuadPart) * 1e9 / frequency.QuadPart;
}

inline void TEST_INIT(int test_count)
{
    ++_test_suit_count;
    TEST_INIT_HEADER();
    setvbuf(stdout, NULL, _IONBF, 0);
    SetUnhandledExceptionFilter(test_exception_handler);
    _test_start = test_start_time();
    _test_assert_error_count = 0;
    _test_count = 0;
    _test_assert_count = 0;
    _test_log = (char **)calloc((test_count), sizeof(char *) + (test_count) * 1024);
    if (_test_log) {
        for (int i = 0; i < (test_count); ++i) {
            _test_log[i] = (char *)(_test_log + (test_count) * sizeof(char *) + i * 1024);
        }
    }
}
#else
#include <time.h>
void test_print_stack_trace() {
    void* buffer[100]; // Array to store the return addresses
    int num_ptrs = backtrace(buffer, 100); // Capture the stack trace
    char** symbols = backtrace_symbols(buffer, num_ptrs); // Resolve symbols

    if (symbols == NULL) {
        perror("backtrace_symbols");
        return;
    }

    printf("Stack trace:\n");
    for (int i = 0; i < num_ptrs; ++i) {
        printf("%s\n", symbols[i]); // Print each symbol
    }

    free(symbols);
}

inline void test_exception_handler(int signum)
{
    printf("Received signal: %d\n", signum);
    test_print_stack_trace();
    exit(1);
}

inline int64_t test_start_time()
{
    struct timespec start;
    clock_gettime(CLOCK_MONOTONIC, &start);

    return start.tv_sec * 1e9 + start.tv_nsec;
}

inline double test_duration_time(int64_t start)
{
    return (double)(test_start_time() - start);
}

inline double test_measure_func_time_ns(void (*func)(volatile void *), volatile void* para, int profiling_loops)
{
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    for (int i = 0; i < profiling_loops; ++i) {
        func(para);
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    return (double)(end.tv_sec * 1e9 + end.tv_nsec) - (double)(start.tv_sec * 1e9 + start.tv_nsec);
}

inline void TEST_INIT(int test_count)
{
    ++_test_suit_count;
    TEST_INIT_HEADER();
    setvbuf(stdout, NULL, _IONBF, 0);
    signal(SIGSEGV, test_exception_handler);
    signal(SIGABRT, test_exception_handler);
    _test_start = test_start_time();
    _test_assert_error_count = 0;
    _test_count = 0;
    _test_assert_count = 0;
    _test_log = (char **)calloc((test_count), sizeof(char *) + (test_count) * 1024);
    if (_test_log) {
        for (int i = 0; i < (test_count); ++i) {
            _test_log[i] = (char *)(_test_log + (test_count) * sizeof(char *) + i * 1024);
        }
    }
}
#endif

inline void test_finalize_impl(const char* file)
{
    if (_test_assert_error_count) {
        printf(
            "[NG] %5d   (%5d/%5d)   %8.2f   %s\n",
            _test_count, _test_assert_count - _test_assert_error_count, _test_assert_count, test_duration_time(_test_start), file
        );
        for (int i = 0; i < _test_assert_error_count; ++i) {
            printf("                                            %s\n", _test_log[i]);
            fflush(stdout);
        }
    } else {
        printf(
            "[OK] %5d   (%5d/%5d)   %8.2f   %s\n",
            _test_count, _test_assert_count - _test_assert_error_count, _test_assert_count, test_duration_time(_test_start), file
        );
    }
    fflush(stdout);
    free(_test_log);
    _test_log = NULL;
    _test_assert_error_count = 0;
    _test_count = 0;
    _test_assert_count = 0;
    TEST_FINALIZE_FOOTER();
}

#define TEST_FINALIZE() test_finalize_impl(__FILE__)

#define TEST_RUN(func)    \
    ++_test_count;        \
    ++_test_global_count; \
    func()

#define TEST_EQUALS(a, b)                                                          \
    do                                                                             \
    {                                                                              \
        ++_test_assert_count;                                                      \
        ++_test_global_assert_count;                                               \
        if ((a) != (b))                                                            \
        {                                                                          \
            ++_test_global_assert_error_count;                                     \
            snprintf(                                                              \
                _test_log[_test_assert_error_count++], 1024,                       \
                "%4i: %" PRId64 " != %" PRId64 " (%s)", __LINE__, (int64)(a), (int64)(b), __func__); \
        }                                                                          \
    } while (0)

#define TEST_NOT_EQUALS(a, b)                                                      \
    do                                                                             \
    {                                                                              \
        ++_test_assert_count;                                                      \
        ++_test_global_assert_count;                                               \
        if ((a) == (b))                                                            \
        {                                                                          \
            ++_test_global_assert_error_count;                                     \
            snprintf(                                                              \
                _test_log[_test_assert_error_count++], 1024,                       \
                "%4i: %" PRId64 " == %" PRId64 " (%s)", __LINE__, (int64)(a), (int64)(b), __func__); \
        }                                                                          \
    } while (0)

template<typename A, typename B>
inline void test_lesser_greater_impl(A a, B b, bool is_lesser, const char* func_name, int line)
{
    ++_test_assert_count;
    ++_test_global_assert_count;
    if ((is_lesser && (a) >= (b)) || (!is_lesser && (a) <= (b))) {
        ++_test_global_assert_error_count;
        snprintf(
            _test_log[_test_assert_error_count++], 1024,
            "%4i: %.3f %s= %.3f (%s)", line, (float)(a), is_lesser ? ">" : "<", (float)(b), func_name
        );
    }
}

#define TEST_GREATER_THAN(a, b) test_lesser_greater_impl((a), (b), false, __func__, __LINE__)
#define TEST_LESSER_THAN(a, b) test_lesser_greater_impl((a), (b), true, __func__, __LINE__)

inline void test_equals_with_delta_impl(double a, double b, double delta, const char* func_name, int line)
{
    ++_test_assert_count;
    ++_test_global_assert_count;
    if (fabs(a - b) > delta) {
        ++_test_global_assert_error_count;
        snprintf(
            _test_log[_test_assert_error_count++], 1024,
            "%4i: %f != %f (%s)", line, (f64)a, (f64)b, func_name
        );
    }
}

#define TEST_EQUALS_WITH_DELTA(a, b, delta) test_equals_with_delta_impl((a), (b), (delta), __func__, __LINE__)

inline void test_contains_impl(const char* a, const char* b, const char* func_name, int line)
{
    ++_test_assert_count;
    ++_test_global_assert_count;
    if (strstr(a, b) != NULL) {
        ++_test_global_assert_error_count;
        snprintf(
            _test_log[_test_assert_error_count++], 1024,
            "%4i: %s !contains %s (%s)", line, a, b, func_name
        );
    }
}

#define TEST_CONTAINS(a, b) test_contains_impl((a), (b), __func__, __LINE__)

inline void test_bool_impl(bool a, bool against_true, const char* func_name, int line)
{
    ++_test_assert_count;
    ++_test_global_assert_count;
    if (!(a)) {
        ++_test_global_assert_error_count;
        snprintf(
            _test_log[_test_assert_error_count++], 1024,
            "%4i: is %s (%s)", line, against_true ? "false" : "true", func_name
        );
    }
}

#define TEST_TRUE(a) test_bool_impl((bool) (a), true, __func__, __LINE__)
#define TEST_FALSE(a) test_bool_impl((bool) !(a), false, __func__, __LINE__)

#define TEST_TRUE_CONST(a)                                   \
    do                                                       \
    {                                                        \
        ++_test_assert_count;                                \
        ++_test_global_assert_count;                         \
        if constexpr (!(a))                                  \
        {                                                    \
            ++_test_global_assert_error_count;               \
            snprintf(                                        \
                _test_log[_test_assert_error_count++], 1024, \
                "%4i: is false", __LINE__);                  \
        }                                                    \
    } while (0)

#define TEST_FALSE_CONST(a)                                  \
    do                                                       \
    {                                                        \
        ++_test_assert_count;                                \
        ++_test_global_assert_count;                         \
        if constexpr ((a))                                   \
        {                                                    \
            ++_test_global_assert_error_count;               \
            snprintf(                                        \
                _test_log[_test_assert_error_count++], 1024, \
                "%4i: is true", __LINE__);                   \
        }                                                    \
    } while (0)

inline void test_memory_equals_impl(const uint8_t* ptr1, const uint8_t* ptr2, uint64_t len, int line)
{
    ++_test_assert_count;
    ++_test_global_assert_count;
    const uint8_t* p1 = ptr1;
    const uint8_t* p2 = ptr2;
    for (uint64_t i = 0; i < len; ++i) {
        if (p1[i] != p2[i]) {
            ++_test_global_assert_error_count;
            snprintf(_test_log[_test_assert_error_count++], 1024,
                "%4i: Memory mismatch at offset %" PRId64, line, i
            );
            break;
        }
    }
}

#define TEST_MEMORY_EQUALS(ptr1, ptr2, len) test_memory_equals_impl((const uint8_t *) ptr1, (const uint8_t *) ptr2, len, __LINE__)

inline void compare_function_test_cycle_impl(
    void (*func1)(volatile void *),
    void (*func2)(volatile void *),
    double x_percent,
    const char* func_name1,
    const char* func_name2,
    int line,
    int profiling_loops
) {
    ++_test_assert_count;
    ++_test_global_assert_count;
    int64_t a = 0, b = 0;

    /* Measure func1 */
    uint64_t start = test_timestamp_counter();
    for (int i = 0; i < profiling_loops; ++i) {
        func1((volatile void *) &a);
    }
    uint64_t end = test_timestamp_counter();
    uint64_t cycles_func1 = end - start;

    /* Measure func2 */
    start = test_timestamp_counter();
    for (int i = 0; i < profiling_loops; ++i) {
        func2((volatile void *) &b);
    }
    end = test_timestamp_counter();
    uint64_t cycles_func2 = end - start;

    /* Calculate percentage difference */
    double percent_diff = 100.0 * ((double) cycles_func1 - (double) cycles_func2) / (double) cycles_func2;

    /* Check if within x% */
    if (isnormal(percent_diff) && percent_diff >= x_percent) {
        ++_test_global_assert_error_count;
        snprintf(
            _test_log[_test_assert_error_count++], 1024,
            "%4i: %.2f%% (%s: %" PRIu64 " cycles, %s: %" PRIu64 " cycles)",
            line, percent_diff + 100.0f, func_name1, (uint64_t) cycles_func1, func_name2, (uint64_t) cycles_func2
        );
    }
    test_bool_impl((a && b) || a == b, true, __func__, line);
}

#define COMPARE_FUNCTION_TEST_CYCLE(func1, func2, x_percent) compare_function_test_cycle_impl(func1, func2, x_percent, #func1, #func2, __LINE__, _test_profiling_loops)

inline void test_function_test_cycle_impl(
    void (*func)(volatile void *), uint64_t cycles,
    const char* func_name, int line,
    int profiling_loops
) {
    ++_test_assert_count;
    ++_test_global_assert_count;
    int64_t para = 0;

    /* Measure func */
    uint64_t start = test_timestamp_counter();
    for (int i = 0; i < profiling_loops; ++i) {
        func((volatile void *) &para);
    }
    uint64_t end = test_timestamp_counter();
    uint64_t cycles_func = end - start;

    /* Calculate percentage difference */
    double percent_diff = 100.0 * (cycles_func - cycles) / cycles;

    if (cycles_func >= cycles) {
        ++_test_global_assert_error_count;
        snprintf(
            _test_log[_test_assert_error_count++], 1024,
            "%4i: %.2f%% (%s: %" PRIu64 " cycles)",
            line, percent_diff + 100.0f, func_name, cycles_func
        );
    }
}

#define TEST_FUNCTION_TEST_CYCLE(func, cycles) test_function_test_cycle_impl(func, cycles, #func, __LINE__, _test_profiling_loops)

inline void compare_function_test_time_impl(
    void (*func1)(volatile void *),
    void (*func2)(volatile void *),
    double x_percent,
    const char* func_name1,
    const char* func_name2,
    int line,
    int profiling_loops
) {
    ++_test_assert_count;
    ++_test_global_assert_count;
    int64_t a = 0, b = 0;

    /* Measure func1 */
    double time_func1 = test_measure_func_time_ns(func1, (volatile void *) &a, profiling_loops);

    /* Measure func2 */
    double time_func2 = test_measure_func_time_ns(func2, (volatile void *) &b, profiling_loops);

    /* Calculate percentage difference */
    double percent_diff = 100.0 * (time_func1 - time_func2) / time_func2;

    /* Print results */
    if (isnormal(percent_diff) && percent_diff >= x_percent) {
        ++_test_global_assert_error_count;
        snprintf(
            _test_log[_test_assert_error_count++], 1024,
            "%4i: %.2f%% (%s: %.2f us, %s: %.2f us)",
            line, percent_diff + 100.0f, func_name1, time_func1, func_name2, time_func2
        );
    }
    test_bool_impl((a && b) || a == b, true, __func__, line);
}

#define COMPARE_FUNCTION_TEST_TIME(func1, func2, x_percent) compare_function_test_time_impl(func1, func2, x_percent, #func1, #func2, __LINE__, _test_profiling_loops)

#endif