/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_PLATFORM_WIN32_THREADING_SPINLOCK_C
#define COMS_PLATFORM_WIN32_THREADING_SPINLOCK_C

#include <windows.h>
#include "../../../stdlib/Types.h"
#include "../TimeUtils.h"
#include "Spinlock.h"
#include "../../../compiler/CompilerUtils.h"

FORCE_INLINE
void spinlock_init(spinlock32* lock) {
    lock = 0;
}

FORCE_INLINE
void spinlock_start(spinlock32* lock, int32 delay = 10) {
    while (InterlockedExchange(lock, 1) != 0) {
        usleep(delay);
    }
}

FORCE_INLINE
void spinlock_end(spinlock32* lock) {
    InterlockedExchange(lock, 0);
}

#endif