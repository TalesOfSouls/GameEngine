/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_THREADS_THREAD_DEFINES_H
#define COMS_THREADS_THREAD_DEFINES_H

#if _WIN32
    #include "../platform/win32/threading/ThreadDefines.h"
#elif __linux__
    #include "../platform/linux/threading/ThreadDefines.h"
#endif

#endif