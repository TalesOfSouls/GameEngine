/**
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_PLATFORM_LINUX_THREADING_SPINLOCK_C
#define COMS_PLATFORM_LINUX_THREADING_SPINLOCK_C

#include "../../../stdlib/Stdlib.h"
#include "../TimeUtils.h"
#include "Spinlock.h"

// Same as platform/win32/threading/Spinlock.cpp

FORCE_INLINE
void spinlock_init(spinlock32* const lock) NO_EXCEPT
{
    lock->store(0);
}

FORCE_INLINE
void spinlock_start(spinlock32* const lock) NO_EXCEPT
{
    PROFILE_START_DEBUG(PROFILE_MUTEX_ACQUIRE);
    // We only try the exchange if we can get a good read
    // Otherwise the exchange may invalidate the cache
    // for all other threads that also try to access this
    while (lock->load() || lock->exchange(1)) {
        cpu_yield();
    }
    PROFILE_END_DEBUG(PROFILE_MUTEX_ACQUIRE);
    PROFILE_START_DEBUG(PROFILE_MUTEX_LOCK);
}

FORCE_INLINE
void spinlock_start(spinlock32* const lock, int32 delay) NO_EXCEPT
{
    PROFILE_START_DEBUG(PROFILE_MUTEX_ACQUIRE);
    // We only try the exchange if we can get a good read
    // Otherwise the exchange may invalidate the cache
    // for all other threads that also try to access this
    while (lock->load() || lock->exchange(1)) {
        // usleep(int32) would be ambiguous with the POSIX usleep
        thread_usleep((uint64) delay);
    }
    PROFILE_END_DEBUG(PROFILE_MUTEX_ACQUIRE);
}

FORCE_INLINE
void spinlock_end(spinlock32* const lock) NO_EXCEPT
{
    lock->store(0);
    PROFILE_END_DEBUG(PROFILE_MUTEX_LOCK);
}

#endif
