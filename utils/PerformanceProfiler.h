#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdint.h>

#define ITERATIONS 1000

// Function prototypes for the functions to be profiled
void function1();
void function2();

// Profiling function
void profile(void (*func)(), const char* func_name, int iterations, double* total_time, uint64_t* total_cycles, double* mean_time, uint64_t* mean_cycles) {
    struct timespec start_time, end_time;
    uint64_t start_cycle, end_cycle;
    *total_time = 0;
    *total_cycles = 0;

    for (int i = 0; i < iterations; i++) {
        clock_gettime(CLOCK_MONOTONIC, &start_time);
        start_cycle = __builtin_ia32_rdtsc();
        func();
        end_cycle = __builtin_ia32_rdtsc();
        clock_gettime(CLOCK_MONOTONIC, &end_time);

        double elapsed_time = (end_time.tv_sec - start_time.tv_sec) * 1e9 + (end_time.tv_nsec - start_time.tv_nsec);
        uint64_t elapsed_cycles = end_cycle - start_cycle;

        *total_time += elapsed_time;
        *total_cycles += elapsed_cycles;
    }

    *mean_time = *total_time / iterations;
    *mean_cycles = *total_cycles / iterations;
}

// Example functions to profile
void function1() {
    // Simulate some work
    for (volatile int i = 0; i < 1000000; i++);
}

void function2() {
    // Simulate some work
    for (volatile int i = 0; i < 2000000; i++);
}

int main() {
    double total_time1, total_time2, mean_time1, mean_time2;
    uint64_t total_cycles1, total_cycles2, mean_cycles1, mean_cycles2;

    profile(function1, "Function 1", ITERATIONS, &total_time1, &total_cycles1, &mean_time1, &mean_cycles1);
    profile(function2, "Function 2", ITERATIONS, &total_time2, &total_cycles2, &mean_time2, &mean_cycles2);

    // Print table header
    printf("+-----------------+-------------------+-------------------+-------------------+-------------------+\n");
    printf("| Function        | Total Time (ns)   | Total Cycles      | Mean Time (ns)    | Mean Cycles       |\n");
    printf("+-----------------+-------------------+-------------------+-------------------+-------------------+\n");

    // Print results for Function 1
    printf("| %-15s | %17.2f | %17lu | %17.2f | %17lu |\n",
           "Function 1", total_time1, total_cycles1, mean_time1, mean_cycles1);

    // Print results for Function 2
    printf("| %-15s | %17.2f | %17lu | %17.2f | %17lu |\n",
           "Function 2", total_time2, total_cycles2, mean_time2, mean_cycles2);

    // Print table footer
    printf("+-----------------+-------------------+-------------------+-------------------+-------------------+\n");

    return 0;
}