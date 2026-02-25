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

#include <windows.h>

#include "../../stdlib/Stdlib.h"
#include "../../utils/StringUtils.h"
#include "../../system/Library.h"
#include "../../system/FileUtils.cpp"

const wchar_t _platform_suffix[] = L"_win32";
const wchar_t _library_extension[] = L".dll";

inline
bool library_dyn_load(LibraryHandle* const __restrict lib, const wchar_t* const __restrict name) NO_EXCEPT
{
    *lib = LoadLibraryExW((LPCWSTR) name, NULL, LOAD_LIBRARY_SEARCH_SYSTEM32);
    if (!*lib) {
        LOG_1("[WARNING] Couldn't load library");
        return false;
    }

    return true;
}

inline
void library_dyn_unload(LibraryHandle* const lib) NO_EXCEPT
{
    FreeLibrary(*lib);
    *lib = NULL;
}

inline
void* library_dyn_proc(LibraryHandle const __restrict lib, const char* const __restrict name) {
    return (void *) GetProcAddress(lib, name);
}

inline static
void* library_get_symbol(Library* lib, const char* symbol) NO_EXCEPT
{
    if (!lib || !lib->handle || !symbol) {
        return NULL;
    }

    return (void *) GetProcAddress((HMODULE) lib->handle, (LPCSTR) symbol);
}

inline
bool library_load(Library* const lib) NO_EXCEPT
{
    wchar_t dst[PATH_MAX_LENGTH];
    str_concat_new(dst, lib->dir, lib->dst);

    // In debug mode, we create a copy at runtime, so we can recompile & reload it
    #if DEBUG || INTERNAL
        wchar_t src[PATH_MAX_LENGTH];
        const size_t dst_len = wcslen(dst);

        memcpy(src, dst, (dst_len + 1) * sizeof(wchar_t));
        str_insert(dst, dst_len - (sizeof(".dll") - 1), L"_temp");

        lib->last_load = file_last_modified(src);
        file_copy(src, dst);
    #endif

    // Make sure the dll is actually unloaded (Windows caches this)
    if (GetModuleHandleW((LPCWSTR) dst)) {
        while (GetModuleHandleW((LPCWSTR) dst) && lib->handle) {
            FreeLibrary(lib->handle);
            Sleep(100);
        }

        int32 i = 0;
        while (GetModuleHandleW((LPCWSTR) dst) && i++ < 10) {
            Sleep(100);
        }
    }

    lib->handle = LoadLibraryW((LPCWSTR) dst);
    if (!lib->handle) {
        lib->is_valid = false;
        ASSERT_TRUE(false);

        LOG_1("[ERROR] Couldn't load library %s", {DATA_TYPE_CHAR_STR, (void *) dst});

        return lib->is_valid;
    }

    lib->is_valid = true;

    return true;
}

inline static
bool library_bind_functions(Library* const lib) NO_EXCEPT
{
    lib->is_valid = true;
    for (int c = 0; c < lib->function_count; ++c) {
        void* function = (void *) GetProcAddress(lib->handle, (LPCSTR) lib->function_names[c]);
        if (function) {
            lib->functions[c] = function;
        } else {
            ASSERT_TRUE(false);
            lib->is_valid = false;

            LOG_1("[ERROR] Couldn't load library function %s", {DATA_TYPE_CHAR_STR, (void *) lib->function_names[c]});
        }
    }

    return lib->is_valid;
}

inline
const LibraryModuleDescriptor* library_descriptor_load(Library* const lib) NO_EXCEPT
{
    const LibraryModuleDescriptor* desc = (LibraryModuleDescriptor *) library_get_symbol(lib, LIBRARY_MODULE_DESCRIPTOR_SYMBOL);
    lib->function_names = desc->function_names;
    lib->function_count = desc->function_count;
    lib->functions = (void **) desc->functions;

    library_bind_functions(lib);

    return desc;
}

inline
void library_unload(Library* const lib) NO_EXCEPT
{
    FreeLibrary(lib->handle);
    for (int c = 0; c < lib->function_count; ++c) {
        lib->functions[c] = NULL;
    }
}

#endif