/**
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_PLATFORM_WIN32_LIB_OLE32_H
#define COMS_PLATFORM_WIN32_LIB_OLE32_H

#include "../../../stdlib/Stdlib.h"
#include <windows.h>
#include <objbase.h>

#if COMS_STATIC_LINKING
    #include <combaseapi.h>

    #pragma comment(lib, "ole32.lib")

    #define OLE32_CoInitializeEx CoInitializeEx
    #define OLE32_CoInitializeSecurity CoInitializeSecurity
    #define OLE32_CoCreateInstance CoCreateInstance
    #define OLE32_CoUninitialize CoUninitialize
    #define OLE32_CoSetProxyBlanket CoSetProxyBlanket
    #define OLE32_CLSIDFromString CLSIDFromString
    #define OLE32_IIDFromString IIDFromString

    bool ole32_init() NO_EXCEPT
    {
        HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
        if (FAILED(hr)) {
            return false;
        }

        hr = CoInitializeSecurity(
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

        if (FAILED(hr)) {
            return false;
        }

        return true;
    }

    inline
    void ole32_free() NO_EXCEPT
    {
        CoUninitialize();
    }
#else
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
    typedef HRESULT (WINAPI *CoSetProxyBlanket_t)(
        IUnknown* pProxy,
        DWORD dwAuthnSvc,
        DWORD dwAuthzSvc,
        OLECHAR* pServerPrincName,
        DWORD dwAuthnLevel,
        DWORD dwImpLevel,
        RPC_AUTH_IDENTITY_HANDLE pAuthInfo,
        DWORD dwCapabilities
    );
    typedef HRESULT (WINAPI *CLSIDFromString_t)(LPCOLESTR, LPCLSID);
    typedef HRESULT (WINAPI *IIDFromString_t)(LPCOLESTR, LPIID);

    static CoInitializeEx_t pCoInitializeEx = NULL;
    static CoInitializeSecurity_t pCoInitializeSecurity = NULL;
    static CoCreateInstance_t pCoCreateInstance = NULL;
    static CoUninitialize_t pCoUninitialize = NULL;
    static CoSetProxyBlanket_t pCoSetProxyBlanket = NULL;
    static CLSIDFromString_t pCLSIDFromString = NULL;
    static IIDFromString_t pIIDFromString = NULL;

    #define OLE32_CoInitializeEx pCoInitializeEx
    #define OLE32_CoInitializeSecurity pCoInitializeSecurity
    #define OLE32_CoCreateInstance pCoCreateInstance
    #define OLE32_CoUninitialize pCoUninitialize
    #define OLE32_CoSetProxyBlanket pCoSetProxyBlanket
    #define OLE32_CLSIDFromString pCLSIDFromString
    #define OLE32_IIDFromString pIIDFromString

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
        pCoSetProxyBlanket = (CoSetProxyBlanket_t) library_dyn_proc(_ole32_lib, "CoSetProxyBlanket");
        pCLSIDFromString = (CLSIDFromString_t) library_dyn_proc(_ole32_lib, "CLSIDFromString");
        pIIDFromString = (IIDFromString_t) library_dyn_proc(_ole32_lib, "IIDFromString");

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

        ++_ole32_lib_ref_count;

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

#endif