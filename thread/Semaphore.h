/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_THREADS_SEMAPHORE_H
#define COMS_THREADS_SEMAPHORE_H

#if _WIN32
    #include "../platform/win32/threading/Semaphore.h"
#elif __linux__
    #include "../platform/linux/threading/Semaphore.h"
#endif

#endif