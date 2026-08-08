/**
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_PLATFORM_LINUX_THREADING_SPINLOCK_H
#define COMS_PLATFORM_LINUX_THREADING_SPINLOCK_H

#include "../../../stdlib/Stdlib.h"

#if defined(NO_STDLIB) && NO_STDLIB
    typedef volatile int32 spinlock32;
#else
    #include <atomic>
    typedef atomic<int> spinlock32;
#endif

#endif