/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_PLATFORM_LINUX_THREADING_SPINLOCK_C
#define COMS_PLATFORM_LINUX_THREADING_SPINLOCK_C

#include "../../../stdlib/Types.h"
#include "Spinlock.h"
#include <time.h>

inline
void spinlock_start(spinlock32* lock, int32 delay = 10) {
    while (__atomic_exchange_n(lock, 1, __ATOMIC_ACQUIRE) != 0) {
        usleep(delay);
    }
}

FORCE_INLINE
void spinlock_end(spinlock32* lock) {
    __atomic_store_n(lock, 0, __ATOMIC_RELEASE);
}

#endif