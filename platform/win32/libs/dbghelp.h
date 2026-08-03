#pragma once
#ifndef COMS_PLATFORM_WIN32_LIB_DBGHELP_H
#define COMS_PLATFORM_WIN32_LIB_DBGHELP_H

#include "../../../stdlib/Stdlib.h"
#include "../../../system/Library.h"
#include "../../../system/Library.cpp"
#include <windows.h>
#include <dbghelp.h>

#if COMS_STATIC_LINKING
    #pragma comment(lib, "dbghelp.lib")

    #define DBGHELP_MiniDumpWriteDump MiniDumpWriteDump
    #define DBGHELP_SymInitialize SymInitialize
    #define DBGHELP_SymSetOptions SymSetOptions
    #define DBGHELP_SymFromAddr SymFromAddr
    #define DBGHELP_SymGetLineFromAddr64 SymGetLineFromAddr64
    #define DBGHELP_SymGetModuleInfo64 SymGetModuleInfo64
    #define DBGHELP_SymCleanup SymCleanup
    #define DBGHELP_StackWalk64 StackWalk64
    #define DBGHELP_SymFunctionTableAccess64 SymFunctionTableAccess64
    #define DBGHELP_SymGetModuleBase64 SymGetModuleBase64
#else
    typedef BOOL (WINAPI *MiniDumpWriteDump_t)(
        HANDLE, DWORD, HANDLE, MINIDUMP_TYPE,
        PMINIDUMP_EXCEPTION_INFORMATION,
        PMINIDUMP_USER_STREAM_INFORMATION,
        PMINIDUMP_CALLBACK_INFORMATION
    );

    typedef BOOL (WINAPI *SymInitialize_t)(HANDLE, PCSTR, BOOL);
    typedef DWORD (WINAPI *SymSetOptions_t)(DWORD);
    typedef BOOL (WINAPI *SymFromAddr_t)(HANDLE, DWORD64, PDWORD64, PSYMBOL_INFO);
    typedef BOOL (WINAPI *SymGetLineFromAddr64_t)(HANDLE, DWORD64, PDWORD, PIMAGEHLP_LINE64);
    typedef BOOL (WINAPI *SymGetModuleInfo64_t)(HANDLE, DWORD64, PIMAGEHLP_MODULE64);
    typedef BOOL (WINAPI *SymCleanup_t)(HANDLE);
    typedef BOOL (WINAPI *StackWalk64_t)(
        DWORD, HANDLE, HANDLE, LPSTACKFRAME64, PVOID,
        PREAD_PROCESS_MEMORY_ROUTINE64, PFUNCTION_TABLE_ACCESS_ROUTINE64,
        PGET_MODULE_BASE_ROUTINE64, PTRANSLATE_ADDRESS_ROUTINE64
    );
    typedef PVOID (WINAPI *SymFunctionTableAccess64_t)(HANDLE, DWORD64);
    typedef DWORD64 (WINAPI *SymGetModuleBase64_t)(HANDLE, DWORD64);

    static MiniDumpWriteDump_t pMiniDumpWriteDump = NULL;
    static SymInitialize_t pSymInitialize = NULL;
    static SymSetOptions_t pSymSetOptions = NULL;
    static SymFromAddr_t pSymFromAddr = NULL;
    static SymGetLineFromAddr64_t pSymGetLineFromAddr64 = NULL;
    static SymGetModuleInfo64_t pSymGetModuleInfo64 = NULL;
    static SymCleanup_t pSymCleanup = NULL;
    static StackWalk64_t pStackWalk64 = NULL;
    static SymFunctionTableAccess64_t pSymFunctionTableAccess64 = NULL;
    static SymGetModuleBase64_t pSymGetModuleBase64 = NULL;

    #define DBGHELP_MiniDumpWriteDump pMiniDumpWriteDump
    #define DBGHELP_SymInitialize pSymInitialize
    #define DBGHELP_SymSetOptions pSymSetOptions
    #define DBGHELP_SymFromAddr pSymFromAddr
    #define DBGHELP_SymGetLineFromAddr64 pSymGetLineFromAddr64
    #define DBGHELP_SymGetModuleInfo64 pSymGetModuleInfo64
    #define DBGHELP_SymCleanup pSymCleanup
    #define DBGHELP_StackWalk64 pStackWalk64
    #define DBGHELP_SymFunctionTableAccess64 pSymFunctionTableAccess64
    #define DBGHELP_SymGetModuleBase64 pSymGetModuleBase64

    static LibraryHandle _dbghelp_lib;

    static int _dbghelp_lib_ref_count = 0;

    inline
    bool dbghelp_init() NO_EXCEPT
    {
        if (_dbghelp_lib_ref_count) {
            ++_dbghelp_lib_ref_count;
            return true;
        }

        bool success = library_dyn_load(&_dbghelp_lib, L"dbghelp.dll");
        if (!success) {
            return false;
        }

        pMiniDumpWriteDump = (MiniDumpWriteDump_t) library_dyn_proc(_dbghelp_lib, "MiniDumpWriteDump");
        pSymInitialize = (SymInitialize_t) library_dyn_proc(_dbghelp_lib, "SymInitialize");
        pSymSetOptions = (SymSetOptions_t) library_dyn_proc(_dbghelp_lib, "SymSetOptions");
        pSymFromAddr = (SymFromAddr_t) library_dyn_proc(_dbghelp_lib, "SymFromAddr");
        pSymGetLineFromAddr64 = (SymGetLineFromAddr64_t) library_dyn_proc(_dbghelp_lib, "SymGetLineFromAddr64");
        pSymGetModuleInfo64 = (SymGetModuleInfo64_t) library_dyn_proc(_dbghelp_lib, "SymGetModuleInfo64");
        pSymCleanup = (SymCleanup_t) library_dyn_proc(_dbghelp_lib, "SymCleanup");
        pStackWalk64 = (StackWalk64_t) library_dyn_proc(_dbghelp_lib, "StackWalk64");
        pSymFunctionTableAccess64 = (SymFunctionTableAccess64_t) library_dyn_proc(_dbghelp_lib, "SymFunctionTableAccess64");
        pSymGetModuleBase64 = (SymGetModuleBase64_t) library_dyn_proc(_dbghelp_lib, "SymGetModuleBase64");

        ++_dbghelp_lib_ref_count;

        return pMiniDumpWriteDump
            && pSymInitialize
            && pSymSetOptions
            && pSymFromAddr
            && pSymGetLineFromAddr64
            && pSymGetModuleInfo64
            && pSymCleanup
            && pStackWalk64
            && pSymFunctionTableAccess64
            && pSymGetModuleBase64;
    }

    inline
    void dbghelp_free() NO_EXCEPT
    {
        if (_dbghelp_lib_ref_count > 1) {
            --_dbghelp_lib_ref_count;
            return;
        }

        library_dyn_unload(&_dbghelp_lib);

        _dbghelp_lib_ref_count = 0;
    }
#endif

#endif
