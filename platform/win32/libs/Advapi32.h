/**
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_PLATFORM_WIN32_LIB_ADVAPI32_H
#define COMS_PLATFORM_WIN32_LIB_ADVAPI32_H

#include <winreg.h>
#include <minwindef.h>
#include "../../../stdlib/Stdlib.h"

#if defined(COMS_IMPLICIT_LINKING) && COMS_IMPLICIT_LINKING
    #pragma comment(lib, "Advapi32.lib")

    #define ADVAPI32_RegOpenKeyExW RegOpenKeyExW
    #define ADVAPI32_RegQueryValueExW RegQueryValueExW
    #define ADVAPI32_RegCloseKey RegCloseKey
    #define ADVAPI32_OpenProcessToken OpenProcessToken
    #define ADVAPI32_LookupPrivilegeValueW LookupPrivilegeValueW
    #define ADVAPI32_AdjustTokenPrivileges AdjustTokenPrivileges
#else
    #include "../../../system/Library.h"
    #include "../../../system/Library.cpp"

    typedef LSTATUS (WINAPI *RegOpenKeyExW_t)(HKEY, LPCWSTR, DWORD, REGSAM, PHKEY);
    typedef LSTATUS (WINAPI *RegQueryValueExW_t)(HKEY, LPCWSTR, LPDWORD, LPDWORD, LPBYTE, LPDWORD);
    typedef LSTATUS (WINAPI *RegCloseKey_t)(HKEY);
    typedef BOOL (WINAPI *OpenProcessToken_t)(HANDLE, DWORD, PHANDLE);
    typedef BOOL (WINAPI *LookupPrivilegeValueW_t)(LPCWSTR, LPCWSTR, PLUID);
    typedef BOOL (WINAPI *AdjustTokenPrivileges_t)(HANDLE, BOOL, PTOKEN_PRIVILEGES, DWORD, PTOKEN_PRIVILEGES, PDWORD);

    static RegOpenKeyExW_t pRegOpenKeyExW = NULL;
    static RegQueryValueExW_t pRegQueryValueExW = NULL;
    static RegCloseKey_t pRegCloseKey = NULL;
    static OpenProcessToken_t pOpenProcessToken = NULL;
    static LookupPrivilegeValueW_t pLookupPrivilegeValueW = NULL;
    static AdjustTokenPrivileges_t pAdjustTokenPrivileges = NULL;

    #define ADVAPI32_RegOpenKeyExW pRegOpenKeyExW
    #define ADVAPI32_RegQueryValueExW pRegQueryValueExW
    #define ADVAPI32_RegCloseKey pRegCloseKey
    #define ADVAPI32_OpenProcessToken pOpenProcessToken
    #define ADVAPI32_LookupPrivilegeValueW pLookupPrivilegeValueW
    #define ADVAPI32_AdjustTokenPrivileges pAdjustTokenPrivileges

    static LibraryHandle _advapi32_lib;

    static int _advapi32_lib_ref_count = 0;

    inline
    bool advapi32_init() NO_EXCEPT
    {
        bool success = library_dyn_load(&_advapi32_lib, L"Advapi32.dll");
        if (!success) {
            return false;
        }

        pRegOpenKeyExW = (RegOpenKeyExW_t) library_dyn_proc(_advapi32_lib, "RegOpenKeyExW");
        pRegQueryValueExW = (RegQueryValueExW_t) library_dyn_proc(_advapi32_lib, "RegQueryValueExW");
        pRegCloseKey = (RegCloseKey_t) library_dyn_proc(_advapi32_lib, "RegCloseKey");
        pOpenProcessToken = (OpenProcessToken_t) library_dyn_proc(_advapi32_lib, "OpenProcessToken");
        pLookupPrivilegeValueW = (LookupPrivilegeValueW_t) library_dyn_proc(_advapi32_lib, "LookupPrivilegeValueW");
        pAdjustTokenPrivileges = (AdjustTokenPrivileges_t) library_dyn_proc(_advapi32_lib, "AdjustTokenPrivileges");

        return pRegOpenKeyExW && pRegQueryValueExW && pRegCloseKey
            && pOpenProcessToken && pLookupPrivilegeValueW && pAdjustTokenPrivileges;
    }

    inline
    void advapi32_free() NO_EXCEPT
    {
        library_dyn_unload(&_advapi32_lib);
    }
#endif

#endif
