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

#include <signal.h>
#include <execinfo.h>
#include <unistd.h>

#define MAX_STACK_FRAMES 64

void signal_handler(int sig) {
    void* stack_frames[MAX_STACK_FRAMES];
    const int num_frames = backtrace(stack_frames, ARRAY_COUNT(stack_frames));
    const char** stack_symbols = backtrace_symbols(stack_frames, num_frames);

    LOG_1("Error: signal %d:", {DATA_TYPE_INT32, &sig});
    for (int i = 0; i < num_frames; ++i) {
        LOG_1("%s", {DATA_TYPE_CHAR_STR, stack_symbols[i]});
    }

    LOG_TO_FILE();
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
    void* buffer[100];
    const int num_ptrs = backtrace(buffer, ARRAY_COUNT(buffer));
    const char** symbols = backtrace_symbols(buffer, num_ptrs);
    if (symbols == NULL) {
        return;
    }

    LOG_1("Stack trace:");
    for (int i = 0; i < num_ptrs; ++i) {
        LOG_1("%s", {DATA_TYPE_CHAR_STR, symbols[i]});
    }

    LOG_TO_FILE()
    free(symbols);
}

#endif