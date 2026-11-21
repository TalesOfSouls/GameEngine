/**
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_PLATFORM_LINUX_FILE_UTILS_H
#define COMS_PLATFORM_LINUX_FILE_UTILS_H

#include "../../stdlib/Stdlib.h"

/**
 * Linux file handles are plain file descriptors.
 *
 * The typedefs are repeated in platform/linux/FileUtils.cpp because the
 * implementation is header only; duplicate typedefs of the same type are
 * allowed in C++.
 */
typedef int32 FileHandle;

// Memory mapped file handle, the file descriptor itself on Linux
typedef int MMFHandle;

// Same layout as platform/win32/FileUtils.h without the OVERLAPPED member
// The "async" reads are synchronous on Linux (pread), file_async_wait() only reports the result
struct FileBodyAsync {
    // doesn't include null termination (same as str_length)
    uint64 size;
    byte* content;
};

#endif
