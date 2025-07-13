/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_PLATFORM_LINUX_THREADING_SEMAPHORE_H
#define COMS_PLATFORM_LINUX_THREADING_SEMAPHORE_H

#include <semaphore.h>
#include <time.h>
#include "../../../stdlib/Types.h"
#include "../../../compiler/CompilerUtils.h"

typedef sem_t sem;

#define coms_sem_init(semaphore, value) sem_init((semaphore), 0, (value))
#define coms_sem_destroy(semaphore) sem_destroy((semaphore))
#define coms_sem_wait(semaphore) sem_wait((semaphore))
#define coms_sem_post(semaphore) sem_post((semaphore))

int32 semimedwait(sem* semaphore, int32 wait) {
    timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += wait;

    return sem_timedwait(semaphore, &ts);
}

#endif