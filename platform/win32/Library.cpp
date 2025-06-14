/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_PLATFORM_WIN32_LIBRARY_C
#define COMS_PLATFORM_WIN32_LIBRARY_C

#include <stdio.h>
#include <windows.h>
#include <string.h>

#include "../../stdlib/Types.h"
#include "../../utils/StringUtils.h"
#include "../../system/Library.h"
#include "../../system/FileUtils.cpp"

inline
bool library_load(Library* lib)
{
    char dst[MAX_PATH];
    str_concat_new(dst, lib->dir, lib->dst);

    // In debug mode, we create a copy at runtime, so we can recompile & reload it
    #if DEBUG || INTERNAL
        char src[MAX_PATH];
        size_t dst_len = str_length(dst);

        memcpy(src, dst, dst_len + 1);
        str_insert(dst, dst_len - (sizeof(".dll") - 1), "_temp");

        lib->last_load = file_last_modified(src);
        file_copy(src, dst);
    #endif

    // Make sure the dll is actually unloaded (Windows caches this)
    if (GetModuleHandleA((LPCSTR) dst)) {
        while (GetModuleHandleA((LPCSTR) dst) && lib->handle) {
            FreeLibrary(lib->handle);
            Sleep(100);
        }

        int32 i = 0;
        while (GetModuleHandleA((LPCSTR) dst) && i++ < 10) {
            Sleep(100);
        }
    }

    lib->handle = LoadLibraryA((LPCSTR) dst);
    if (!lib->handle) {
        lib->is_valid= false;
        ASSERT_SIMPLE(false);

        return lib->is_valid;
    }

    lib->is_valid = true;
    for (int32 c = 0; c < lib->function_count; ++c) {
        void* function = (void *) GetProcAddress(lib->handle, (LPCSTR) lib->function_names[c]);
        if (function) {
            lib->functions[c] = function;
        } else {
            ASSERT_SIMPLE(false);
            lib->is_valid = false;
        }
    }

    return lib->is_valid;
}

inline
void library_unload(Library* lib)
{
    FreeLibrary(lib->handle);
    for (int32 c = 0; c < lib->function_count; ++c) {
        lib->functions[c] = NULL;
    }
}

#endif