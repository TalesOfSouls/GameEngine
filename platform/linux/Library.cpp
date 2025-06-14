/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_PLATFORM_LINUX_LIBRARY_C
#define COMS_PLATFORM_LINUX_LIBRARY_C

#include <stdio.h>
#include <dlfcn.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#include "../../stdlib/Types.h"
#include "../../utils/StringUtils.h"
#include "../../system/Library.h"
#include "../../system/FileUtils.cpp"

inline
bool library_load(Library* lib)
{
    char dst[PATH_MAX];
    str_concat_new(dst, lib->dir, lib->dst);

    // In debug mode, we create a copy at runtime, so we can recompile & reload it
    #if DEBUG || INTERNAL
        char src[PATH_MAX];
        size_t dst_len = str_length(dst);

        memcpy(src, dst, dst_len + 1);
        str_insert(dst, dst_len - (sizeof(".so") - 1), "_temp");

        lib->last_load = file_last_modified(src);
        file_copy(src, dst);
    #endif

    // Unload any previous instance of the library if it’s already loaded
    if (lib->handle) {
        dlclose(lib->handle);
        lib->handle = NULL;
        usleep(100000); // 100 ms
    }

    // @question we might want RTLD_NOW?
    lib->handle = dlopen(dst, RTLD_LAZY);
    if (!lib->handle) {
        LOG_1(dlerror());

        lib->is_valid = false;
        return false;
    }

    lib->is_valid = true;
    for (int32 c = 0; c < lib->function_count; ++c) {
        void* function = dlsym(lib->handle, lib->function_names[c]);
        if (function) {
            lib->functions[c] = function;
        } else {
            lib->is_valid = false;
            return false;
        }
    }

    return lib->is_valid;
}

inline
void library_unload(Library* lib)
{
    if (lib->handle) {
        dlclose(lib->handle);
        lib->handle = NULL;
    }

    for (int32 c = 0; c < lib->function_count; ++c) {
        lib->functions[c] = NULL;
    }
}

#endif
