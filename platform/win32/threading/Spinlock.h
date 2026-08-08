/**
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_PLATFORM_WIN32_THREADING_SPINLOCK_H
#define COMS_PLATFORM_WIN32_THREADING_SPINLOCK_H

#include <windows.h>

#if defined(NO_STDLIB) && NO_STDLIB
    typedef volatile long spinlock32;
#else
    #include <atomic>
    typedef atomic<int> spinlock32;
#endif

#endif