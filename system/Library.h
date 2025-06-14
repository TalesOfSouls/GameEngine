/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_SYSTEM_LIBRARY_H
#define COMS_SYSTEM_LIBRARY_H

#include "../stdlib/Types.h"

#if _WIN32
    #include "../platform/win32/Library.h"
#elif __linux__
    #include "../platform/linux/Library.h"
#endif

struct Library {
    LibraryHandle handle;

    bool is_valid;

    char dir[MAX_PATH];
    char dst[64];

    #if DEBUG || INTERNAL
        uint64 last_load;
    #endif

    int32 function_count;
    const char** function_names;
    void** functions;
};

#endif