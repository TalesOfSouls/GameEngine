/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_THREADS_ATOMIC_H
#define COMS_THREADS_ATOMIC_H

#if _WIN32
    #include "../platform/win32/threading/Atomic.h"
#elif __linux__
    #include "../platform/linux/threading/Atomic.h"
#endif

#endif