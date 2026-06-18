/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_PLATFORM_WIN32_THREADING_THREAD_HELPER_C
#define COMS_PLATFORM_WIN32_THREADING_THREAD_HELPER_C

// We use Macros to avoid the function call overhead and exception handling overhead
#define mutex_init(mutex, attr) { \
    ASSERT_TRUE((mutex)); \
    InitializeCriticalSection((mutex)); \
}

#define mutex_destroy(mutex) { \
    ASSERT_TRUE((mutex)); \
    DeleteCriticalSection((mutex)); \
}

#define mutex_lock(mutex) { \
    ASSERT_TRUE((mutex)); \
    PROFILE_STATIC_START(PROFILE_STATIC_MUTEX_ACQUIRE); \
    EnterCriticalSection((mutex)); \
    PROFILE_STATIC_END(PROFILE_STATIC_MUTEX_ACQUIRE); \
    PROFILE_STATIC_START(PROFILE_STATIC_MUTEX_LOCK); \
}

#define mutex_unlock(mutex) { \
    ASSERT_TRUE((mutex)); \
    LeaveCriticalSection((mutex)); \
    PROFILE_STATIC_END(PROFILE_STATIC_MUTEX_LOCK); \
}

#endif