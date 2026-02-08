/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_PLATFORM_WIN32_LIB_OLE32_H
#define COMS_PLATFORM_WIN32_LIB_OLE32_H

#include <windows.h>
#include <objbase.h>
#include "../../../stdlib/Stdlib.h"
#include "../../../system/Library.h"
#include "../../../system/Library.cpp"

typedef HRESULT (WINAPI *CoInitializeEx_t)(LPVOID, DWORD);
typedef HRESULT (WINAPI *CoInitializeSecurity_t)(
    PSECURITY_DESCRIPTOR, LONG, SOLE_AUTHENTICATION_SERVICE*,
    void*, DWORD, DWORD, void*, DWORD, void*
);
typedef HRESULT (WINAPI *CoCreateInstance_t)(
    REFCLSID, LPUNKNOWN, DWORD, REFIID, LPVOID*
);
typedef void (WINAPI *CoUninitialize_t)(void);

static CoInitializeEx_t pCoInitializeEx = NULL;
static CoInitializeSecurity_t pCoInitializeSecurity = NULL;
static CoCreateInstance_t pCoCreateInstance = NULL;
static CoUninitialize_t pCoUninitialize = NULL;

static LibraryHandle _ole32_lib;

static int _ole32_lib_ref_count = 0;

inline
bool ole32_init() NO_EXCEPT
{
    if (_ole32_lib_ref_count) {
        ++_ole32_lib_ref_count;
        return true;
    }

    bool success = library_dyn_load(&_ole32_lib, L"ole32.dll");
    if (!success) {
        return false;
    }

    pCoInitializeEx = (CoInitializeEx_t) library_dyn_proc(_ole32_lib, "CoInitializeEx");
    pCoInitializeSecurity = (CoInitializeSecurity_t) library_dyn_proc(_ole32_lib, "CoInitializeSecurity");
    pCoCreateInstance = (CoCreateInstance_t) library_dyn_proc(_ole32_lib, "CoCreateInstance");
    pCoUninitialize = (CoUninitialize_t) library_dyn_proc(_ole32_lib, "CoUninitialize");

    pCoInitializeEx(NULL, COINIT_MULTITHREADED);
    pCoInitializeSecurity(
        NULL,
        -1,
        NULL,
        NULL,
        RPC_C_AUTHN_LEVEL_DEFAULT,
        RPC_C_IMP_LEVEL_IMPERSONATE,
        NULL,
        EOAC_NONE,
        NULL
    );

    return pCoInitializeEx
        && pCoInitializeSecurity
        && pCoCreateInstance
        && pCoUninitialize;
}

inline
void ole32_free() NO_EXCEPT
{
    if (_ole32_lib_ref_count > 1) {
        --_ole32_lib_ref_count;
        return;
    }

    pCoUninitialize();
    library_dyn_unload(&_ole32_lib);

    _ole32_lib_ref_count = 0;
}

#endif