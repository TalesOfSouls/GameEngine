/**
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_PLATFORM_WIN32_THREADING_SEMAPHORE_H
#define COMS_PLATFORM_WIN32_THREADING_SEMAPHORE_H

#include <windows.h>
#include "../../../stdlib/Stdlib.h"

typedef HANDLE sem;

FORCE_INLINE
void coms_sem_init(sem* semaphore, int32 value) NO_EXCEPT
{
    *semaphore = CreateSemaphore(NULL, value, MAX_INT32, NULL);
}

FORCE_INLINE
void coms_sem_destroy(sem* semaphore) NO_EXCEPT
{
    CloseHandle(*semaphore);
}

// decrement if != 0, if = 0 wait
FORCE_INLINE
int32 coms_sem_wait(sem* semaphore) NO_EXCEPT
{
    return (int32) WaitForSingleObject(*semaphore, INFINITE);
}

FORCE_INLINE
int32 coms_sem_wait(sem* semaphore, uint64 ms) NO_EXCEPT
{
    return (int32) WaitForSingleObject(*semaphore, (DWORD) ms);
}

bool coms_sem_trywait(sem* semaphore) NO_EXCEPT
{
    return WaitForSingleObject(*semaphore, 0) == WAIT_OBJECT_0;
}

// increment
FORCE_INLINE
void coms_sem_post(sem* semaphore, long value = 1) NO_EXCEPT
{
    ReleaseSemaphore(*semaphore, value, NULL);
}

#endif