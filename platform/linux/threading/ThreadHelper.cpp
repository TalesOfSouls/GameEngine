/**
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_PLATFORM_LINUX_THREADING_THREAD_HELPER_C
#define COMS_PLATFORM_LINUX_THREADING_THREAD_HELPER_C

/**
 * Linux mutex helper
 *
 * The Linux mutex implementation is macro based and lives in
 * platform/linux/threading/ThreadDefines.h (futex based). This file only
 * exists so the generic thread/ThreadHelper.cpp include chain works on Linux,
 * mirroring the win32 header of the same name.
 */

#include "ThreadDefines.h"

#endif
