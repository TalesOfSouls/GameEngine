/**
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_PLATFORM_LINUX_THREADING_SPINLOCK_C
#define COMS_PLATFORM_LINUX_THREADING_SPINLOCK_C

#include "../../../stdlib/Stdlib.h"
#include "Spinlock.h"
#include <time.h>

inline
void spinlock_start(spinlock32* lock) {
    // We only try the exchange if we can get a good read
    // Otherwise InterlockedExchange may invalidate the cache
    // for all other threads that also try to access this
    while (__atomic_load_n(lock, __ATOMIC_ACQUIRE)
        || __atomic_exchange_n(lock, 1, __ATOMIC_ACQUIRE)
    ) {
        cpu_yield();
    }
}

inline
void spinlock_start(spinlock32* lock, int32 delay) {
    // We only try the exchange if we can get a good read
    // Otherwise InterlockedExchange may invalidate the cache
    // for all other threads that also try to access this
    while (__atomic_load_n(lock, __ATOMIC_ACQUIRE)
        || __atomic_exchange_n(lock, 1, __ATOMIC_ACQUIRE) != 0
    ) {
        usleep(delay);
    }
}

FORCE_INLINE
void spinlock_end(spinlock32* lock) {
    __atomic_store_n(lock, 0, __ATOMIC_RELEASE);
}

#endif