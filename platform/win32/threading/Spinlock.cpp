/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_PLATFORM_WIN32_THREADING_SPINLOCK_C
#define COMS_PLATFORM_WIN32_THREADING_SPINLOCK_C

#include <windows.h>
#include "../../../stdlib/Stdlib.h"
#include "../TimeUtils.h"
#include "Spinlock.h"

FORCE_INLINE
void spinlock_init(spinlock32* const lock) NO_EXCEPT
{
    *lock = 0;
}

FORCE_INLINE
void spinlock_start(spinlock32* const lock) NO_EXCEPT
{
    PROFILE_START_DEBUG(PROFILE_MUTEX_ACQUIRE);
    while (InterlockedExchange(lock, 1) != 0) {
        YieldProcessor();
    }
    PPROFILE_END_DEBUG(PROFILE_MUTEX_ACQUIRE);
    PROFILE_START_DEBUG(PROFILE_MUTEX_LOCK);
}

FORCE_INLINE
void spinlock_start(spinlock32* const lock, int32 delay) NO_EXCEPT
{
    PROFILE_START_DEBUG(PROFILE_MUTEX_ACQUIRE);
    while (InterlockedExchange(lock, 1) != 0) {
        usleep(delay);
    }
    PPROFILE_END_DEBUG(PROFILE_MUTEX_ACQUIRE);
}

FORCE_INLINE
void spinlock_end(spinlock32* const lock) NO_EXCEPT
{
    InterlockedExchange(lock, 0);
    PPROFILE_END_DEBUG(PROFILE_MUTEX_LOCK);
}

#endif