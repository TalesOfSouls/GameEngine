#pragma once
#ifndef COMS_PLATFORM_WIN32_WMI_DYN_H
#define COMS_PLATFORM_WIN32_WMI_DYN_H

#include <windows.h>
#include "../../../stdlib/Stdlib.h"
#include "../../../system/Library.cpp"
#include "ole32.h"

static CLSID WBEMUUID_CLSID_WbemLocator;
static IID WBEMUUID_IID_IWbemLocator;

#if COMS_STATIC_LINKING
    #pragma comment(lib, "wbemuuid.lib")

    #define OLE32_CoInitializeEx CoInitializeEx

    inline
    bool wmi_guid_init() NO_EXCEPT
    {
        if (FAILED(OLE32_CLSIDFromString(
            L"{4590F811-1D3A-11D0-891F-00AA004B2E24}",
            &WBEMUUID_CLSID_WbemLocator))
        ) {
            return false;
        }

        if (FAILED(OLE32_IIDFromString(
            L"{DC12A687-737F-11CF-884D-00AA004B2E24}",
            &WBEMUUID_IID_IWbemLocator))
        ) {
            return false;
        }

        return true;
    }
#else
    static LibraryHandle _wbemuuid_lib;
    static int _wbemuuid_lib_ref_count = 0;

    inline
    bool wmi_guid_init() NO_EXCEPT
    {
        if (!_ole32_lib_ref_count) {
            // This depends on ole32
            ASSERT_TRUE(_ole32_lib_ref_count);
            return false;
        }

        if (_wbemuuid_lib_ref_count) {
            ++_wbemuuid_lib_ref_count;
            return true;
        }

        /*
        We don't have to load any lib/dll
        bool success = library_dyn_load(&_wbemuuid_lib, L"wbemuuid.dll");
        if (!success) {
            return false;
        }
        */

        if (FAILED(OLE32_CLSIDFromString(
            L"{4590F811-1D3A-11D0-891F-00AA004B2E24}",
            &WBEMUUID_CLSID_WbemLocator))
        ) {
            return false;
        }

        if (FAILED(OLE32_IIDFromString(
            L"{DC12A687-737F-11CF-884D-00AA004B2E24}",
            &WBEMUUID_IID_IWbemLocator))
        ) {
            return false;
        }

        ++_wbemuuid_lib_ref_count;

        return true;
    }

    inline
    void wmi_guid_free() NO_EXCEPT
    {
        if (_wbemuuid_lib_ref_count > 1) {
            --_wbemuuid_lib_ref_count;
            return;
        }

        /*
        Since we don't load any lib/dll we don't need this
        library_dyn_unload(&_wbemuuid_lib);
        */

        _wbemuuid_lib_ref_count = 0;
    }
#endif

#endif
