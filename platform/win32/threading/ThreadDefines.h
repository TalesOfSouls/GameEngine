/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_PLATFORM_WIN32_THREADING_THREAD_DEFINES_H
#define COMS_PLATFORM_WIN32_THREADING_THREAD_DEFINES_H

#include "../../../stdlib/Stdlib.h"
#include "../../../thread/Atomic.h"
#include "../../../log/PerformanceProfiler.h"
#include <windows.h>

#define THREAD_RETURN DWORD WINAPI
#define THREAD_RETURN_BODY DWORD
typedef DWORD (WINAPI *ThreadJobFunc)(void*);
typedef CRITICAL_SECTION mutex;
typedef void mutexattr_t;
typedef void coms_pthread_condattr_t;
typedef void coms_pthread_rwlockattr_t;

struct coms_pthread_t {
    int32 id;
    HANDLE h;
    void* stack;
};

typedef CONDITION_VARIABLE mutex_cond;

// Thread local variable Already exists in c++11
#if defined(CPP_VERSION) && CPP_VERSION < 11
    #define thread_local __declspec(thread)
#endif

struct coms_pthread_rwlock_t {
    SRWLOCK lock;
    bool exclusive;
};

#endif