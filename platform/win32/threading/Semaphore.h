/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_PLATFORM_WIN32_THREADING_SEMAPHORE_H
#define COMS_PLATFORM_WIN32_THREADING_SEMAPHORE_H

#include <windows.h>
#include "../../../stdlib/Types.h"
#include "../../../compiler/CompilerUtils.h"

typedef HANDLE sem;

FORCE_INLINE
void coms_sem_init(sem* semaphore, int32 value)
{
    *semaphore = CreateSemaphore(NULL, value, MAX_UINT32, NULL);
}

FORCE_INLINE
void coms_sem_destroy(sem* semaphore)
{
    CloseHandle(*semaphore);
}

// decrement if != 0, if = 0 wait
FORCE_INLINE
void coms_sem_wait(sem* semaphore) {
    WaitForSingleObject(*semaphore, INFINITE);
}

FORCE_INLINE
int32 semimedwait(sem* semaphore, uint64 ms) {
    return (int32) WaitForSingleObject(*semaphore, (DWORD) ms);
}

FORCE_INLINE
int32 semrywait(sem* semaphore) {
    return (int32) WaitForSingleObject(*semaphore, 0);
}

// increment
FORCE_INLINE
void coms_sem_post(sem* semaphore) {
    ReleaseSemaphore(*semaphore, 1, NULL);
}

#endif