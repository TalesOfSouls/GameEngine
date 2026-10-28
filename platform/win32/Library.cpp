/**
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_PLATFORM_WIN32_LIBRARY_C
#define COMS_PLATFORM_WIN32_LIBRARY_C

#include <windows.h>

#include "../../stdlib/Stdlib.h"
#include "../../utils/StringUtils.h"
#include "../../utils/RandomUtils.h"
#include "../../utils/TimeUtils.h"
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

    STATS_INCREMENT_PERSISTENT_DEBUG(DEBUG_COUNTER_LIB_HANDLE_COUNT);

    return true;
}

inline
void library_dyn_unload(LibraryHandle* const lib) NO_EXCEPT
{
    FreeLibrary(*lib);
    *lib = NULL;
    STATS_DECREMENT_PERSISTENT_DEBUG(DEBUG_COUNTER_LIB_HANDLE_COUNT);
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
    SimpleStringBuffer<wchar_t, PATH_MAX_LENGTH> dst;
    dst.length = str_concat_new(
        dst.str,
        lib->dir.str, lib->dir.length,
        lib->dst.str, lib->dst.length
    );

    // In debug mode, we create a copy at runtime, so we can recompile & reload it
    #if (defined(DEBUG) && DEBUG) || (defined(INTERNAL) && INTERNAL)
        wchar_t rand_name[7];
        str_random(
            rand_name, ARRAY_COUNT(rand_name) - 1,
            L"abcdefghijklmnopqrstuvwxyz0123456789",
            ARRAY_COUNT(L"abcdefghijklmnopqrstuvwxyz0123456789") - 1
        );

        SimpleStringBuffer<wchar_t, PATH_MAX_LENGTH> src;
        str_copy(src.str, dst.str, dst.length);
        str_insert(
            dst.str, dst.length,
            dst.length - (sizeof(".dll") - 1),
            rand_name, ARRAY_COUNT(rand_name) - 1
        );

        lib->last_load = file_last_modified(src.str);
        file_copy(src.str, dst.str);
    #endif

    // Make sure the dll is actually unloaded (Windows caches this)
    if (GetModuleHandleW((LPCWSTR) dst.str)) {
        while (GetModuleHandleW((LPCWSTR) dst.str) && lib->handle) {
            FreeLibrary(lib->handle);
            thread_usleep(100 * MILLI_MICRO);
        }

        int32 i = -1;
        while (GetModuleHandleW((LPCWSTR) dst.str) && ++i < 10) {
            thread_usleep(100 * MILLI_MICRO);
        }
    }

    lib->handle = LoadLibraryW((LPCWSTR) dst.str);
    if (!lib->handle) {
        lib->is_valid = false;
        ASSERT_THROW();

        LOG_1("[ERROR] Couldn't load library %s", {DATA_TYPE_CHAR_STR, (void *) dst.str});

        return lib->is_valid;
    }

    lib->is_valid = true;
    STATS_INCREMENT_PERSISTENT_DEBUG(DEBUG_COUNTER_LIB_HANDLE_COUNT);

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
            ASSERT_THROW();
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
    STATS_DECREMENT_PERSISTENT_DEBUG(DEBUG_COUNTER_LIB_HANDLE_COUNT);
}

#if (defined(DEBUG) && DEBUG) || (defined(INTERNAL) && INTERNAL)
    /**
     * Checks if the library file was rebuilt since it was loaded
     * Only debug/internal builds load a copy of the library, which allows to rebuild the original file
     *
     * @param const Library* lib Library
     *
     * @return bool
     */
    inline
    bool library_has_changed(const Library* const lib) NO_EXCEPT
    {
        if (!lib->handle) {
            return false;
        }

        SimpleStringBuffer<wchar_t, PATH_MAX_LENGTH> src;
        src.length = str_concat_new(
            src.str,
            lib->dir.str, lib->dir.length,
            lib->dst.str, lib->dst.length
        );

        const uint64 modified = file_last_modified(src.str);

        return modified && modified != lib->last_load;
    }

    /**
     * Loads the current version of the library file in addition to the already loaded version
     * The library struct is not changed, the new version only replaces it in library_swap()
     *
     * @param const Library*    lib         Library
     * @param uint64*           last_load   Modification time of the loaded file
     *
     * @return LibraryHandle NULL if the file couldn't be loaded (e.g. the linker is still writing it)
     */
    inline
    LibraryHandle library_load_next(const Library* const __restrict lib, uint64* const __restrict last_load) NO_EXCEPT
    {
        SimpleStringBuffer<wchar_t, PATH_MAX_LENGTH> src;
        src.length = str_concat_new(
            src.str,
            lib->dir.str, lib->dir.length,
            lib->dst.str, lib->dst.length
        );

        // The linker keeps the file open while writing it
        HANDLE const fp = CreateFileW((LPCWSTR) src.str, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (fp == INVALID_HANDLE_VALUE) {
            LOG_1("[WARNING] Library %w is in use or doesn't exist", {DATA_TYPE_WCHAR_STR, (void *) src.str});

            return NULL;
        }
        CloseHandle(fp);

        wchar_t rand_name[7];
        str_random(
            rand_name, ARRAY_COUNT(rand_name) - 1,
            L"abcdefghijklmnopqrstuvwxyz0123456789",
            ARRAY_COUNT(L"abcdefghijklmnopqrstuvwxyz0123456789") - 1
        );

        SimpleStringBuffer<wchar_t, PATH_MAX_LENGTH> dst;
        str_copy(dst.str, src.str, src.length);
        dst.length = src.length;
        str_insert(
            dst.str, dst.length,
            dst.length - (sizeof(".dll") - 1),
            rand_name, ARRAY_COUNT(rand_name) - 1
        );

        const uint64 modified = file_last_modified(src.str);
        if (!file_copy(src.str, dst.str)) {
            LOG_1("[ERROR] Couldn't copy library %w", {DATA_TYPE_WCHAR_STR, (void *) src.str});

            return NULL;
        }

        LibraryHandle const handle = LoadLibraryW((LPCWSTR) dst.str);
        if (!handle) {
            LOG_1("[ERROR] Couldn't load library %w", {DATA_TYPE_WCHAR_STR, (void *) dst.str});

            return NULL;
        }

        STATS_INCREMENT_PERSISTENT_DEBUG(DEBUG_COUNTER_LIB_HANDLE_COUNT);
        *last_load = modified;

        return handle;
    }

    /**
     * Binds the functions of a library version loaded by library_load_next()
     * Unlike library_bind_functions() a missing function is not an assert, it only fails the binding
     *
     * @param LibraryHandle         handle          Library
     * @param const char* const*    function_names  Names of the functions
     * @param int32                 function_count  Number of functions
     * @param void**                functions       Output: function pointers
     *
     * @return bool
     */
    inline
    bool library_bind_next(
        LibraryHandle handle,
        const char* const* const __restrict function_names,
        int32 function_count,
        void** const __restrict functions
    ) NO_EXCEPT
    {
        for (int32 c = 0; c < function_count; ++c) {
            functions[c] = (void *) GetProcAddress(handle, (LPCSTR) function_names[c]);
            if (!functions[c]) {
                LOG_1("[ERROR] Couldn't find library function %s", {DATA_TYPE_CHAR_STR, (void *) function_names[c]});

                return false;
            }
        }

        return true;
    }

    /**
     * Replaces the loaded version of the library with a version loaded by library_load_next()
     *
     * The previous version is NOT unloaded, it is returned instead.
     * Other memory (e.g. log and profiler entries) may still reference strings or code of the previous version.
     *
     * @param Library*          lib         Library
     * @param LibraryHandle     handle      New version
     * @param void* const*      functions   Functions of the new version (lib->function_count)
     * @param uint64            last_load   Modification time of the new version
     *
     * @return LibraryHandle Previous version
     */
    inline
    LibraryHandle library_swap(
        Library* const __restrict lib,
        LibraryHandle handle,
        void* const* const __restrict functions,
        uint64 last_load
    ) NO_EXCEPT
    {
        LibraryHandle const previous = lib->handle;

        memcpy(lib->functions, functions, sizeof(void *) * lib->function_count);
        lib->handle = handle;
        lib->last_load = last_load;
        lib->is_valid = true;

        return previous;
    }
#endif

#endif