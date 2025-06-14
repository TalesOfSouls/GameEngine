/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_PLATFORM_LINUX_EXCEPTION_HANDLER_H
#define COMS_PLATFORM_LINUX_EXCEPTION_HANDLER_H

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <execinfo.h>
#include <unistd.h>

#define MAX_STACK_FRAMES 64

// @todo fix nasty fprintf usage
// @todo should also log backtrace similar to windows version
void signal_handler(int sig) {
    void *stack_frames[MAX_STACK_FRAMES];
    char **stack_symbols;
    int num_frames;

    num_frames = backtrace(stack_frames, MAX_STACK_FRAMES);
    stack_symbols = backtrace_symbols(stack_frames, num_frames);

    fprintf(stderr, "Error: signal %d:\n", sig);
    for (int i = 0; i < num_frames; i++) {
        fprintf(stderr, "%s\n", stack_symbols[i]);
    }

    FILE *file = fopen("crash_dump.log", "w");
    if (file) {
        fprintf(file, "Error: signal %d:\n", sig);
        for (int i = 0; i < num_frames; i++) {
            fprintf(file, "%s\n", stack_symbols[i]);
        }
        fclose(file);
    }

    free(stack_symbols);

    exit(EXIT_FAILURE);
}

void setup_signal_handler() {
    struct sigaction sa;

    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;

    sigaction(SIGSEGV, &sa, NULL);
    sigaction(SIGABRT, &sa, NULL);
}

void print_stack_trace() {
    void *buffer[100]; // Array to store the return addresses
    int num_ptrs = backtrace(buffer, 100); // Capture the stack trace
    char **symbols = backtrace_symbols(buffer, num_ptrs); // Resolve symbols

    if (symbols == NULL) {
        perror("backtrace_symbols");
        return;
    }

    printf("Stack trace:\n");
    for (int i = 0; i < num_ptrs; i++) {
        printf("%s\n", symbols[i]); // Print each symbol
    }

    free(symbols); // Free the memory allocated by backtrace_symbols
}

#endif