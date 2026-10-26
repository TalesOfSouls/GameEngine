/**
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_SYSTEM_LIBRARY_H
#define COMS_SYSTEM_LIBRARY_H

#include "../stdlib/Stdlib.h"

// Only used as a pointer parameter below - a forward declaration keeps the
// log/debug headers (and their include order) out of this file
struct DebugContainer;

#if _WIN32
    #include "../platform/win32/Library.h"
#elif __linux__
    #include "../platform/linux/Library.h"
#endif

struct Library {
    LibraryHandle handle;

    bool is_valid;

    SimpleStringBuffer<wchar_t, PATH_MAX_LENGTH> dir;
    SimpleStringBuffer<wchar_t, 64> dst;

    #if (defined(DEBUG) && DEBUG) || (defined(INTERNAL) && INTERNAL)
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
    void (*load)(DebugContainer*);
};

#define LIBRARY_MODULE_DESCRIPTOR_SYMBOL "_LIBRARY_MODULE_DESCRIPTOR"

#endif