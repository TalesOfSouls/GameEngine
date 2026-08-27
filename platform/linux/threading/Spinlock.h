/**
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_PLATFORM_LINUX_THREADING_SPINLOCK_H
#define COMS_PLATFORM_LINUX_THREADING_SPINLOCK_H

#if defined(NO_STDLIB) && NO_STDLIB
    typedef volatile int spinlock32;
#else
    #include "../../../thread/Atomic.h"
    typedef atomic<int> spinlock32;
#endif

#endif