/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_SYSTEM_FILE_UTILS_H
#define COMS_SYSTEM_FILE_UTILS_H

#include "../stdlib/Stdlib.h"

#if _WIN32
    #include "../platform/win32/FileUtils.h"
#elif __linux__
    #include "../platform/linux/FileUtils.h"
#endif

struct FileBody {
    // doesn't include null termination (same as strlen)
    size_t size;

    // If size is defined you also must allocate memory for content when reading from a file
    // Otherwise the API expects a ring memory it can use for reserving memory
    // Of course this means that the content is only temporarily available and will be overwritten any time
    // If you allocate content, make sure to allocate +1 since we always add \0 at the end even in binary
    byte* content;
};

typedef void (*FileToLoadCallback)(FileBody* file, void* data);

struct FileToLoad {
    FileToLoadCallback callback;
    char file_path[64 - sizeof(void*)];
};

#endif