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

    wchar_t dir[MAX_PATH];
    wchar_t dst[64];

    #if DEBUG || INTERNAL
        uint64 last_load;
    #endif

    int32 function_count;
    const char* const* function_names;
    void** functions;
};

struct LibraryModuleDescriptor {
    const char* name;

    const char* const* function_names;
    int32 function_count;
    void* functions;

    // A callback to run after binding
    void (*load)(DebugContainer);
};

#define LIBRARY_MODULE_DESCRIPTOR_SYMBOL "LIBRARY_MODULE_DESCRIPTOR"

#endif