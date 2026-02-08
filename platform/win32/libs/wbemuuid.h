#ifndef COMS_PLATFORM_WIN32_WMI_DYN_H
#define COMS_PLATFORM_WIN32_WMI_DYN_H

#include <windows.h>
#include "../../../stdlib/Stdlib.h"
#include "../../../system/Library.h"
#include "../../../system/Library.cpp"

typedef HRESULT (WINAPI *CLSIDFromString_t)(LPCOLESTR, LPCLSID);
typedef HRESULT (WINAPI *IIDFromString_t)(LPCOLESTR, LPIID);

static CLSIDFromString_t pCLSIDFromString = NULL;
static IIDFromString_t pIIDFromString  = NULL;

static LibraryHandle _wbemuuid_lib;
static int _wbemuuid_lib_ref_count = 0;

static CLSID pCLSID_WbemLocator;
static IID pIID_IWbemLocator;

inline
bool wmi_guid_init() NO_EXCEPT
{
    if (_wbemuuid_lib_ref_count) {
        ++_wbemuuid_lib_ref_count;
        return true;
    }

    // @bug This is from ole32 not wbemuuid
    pCLSIDFromString = (CLSIDFromString_t) library_dyn_proc(_wbemuuid_lib, "CLSIDFromString");
    pIIDFromString = (IIDFromString_t) library_dyn_proc(_wbemuuid_lib, "IIDFromString");

    if (!pCLSIDFromString || !pIIDFromString) {
        return false;
    }

    if (FAILED(pCLSIDFromString(
        L"{4590F811-1D3A-11D0-891F-00AA004B2E24}",
        &pCLSID_WbemLocator))
    ) {
        return false;
    }

    if (FAILED(pIIDFromString(
        L"{DC12A687-737F-11CF-884D-00AA004B2E24}",
        &pIID_IWbemLocator))
    ) {
        return false;
    }

    return true;
}

inline
void wmi_guid_free() NO_EXCEPT
{
    if (_wbemuuid_lib_ref_count > 1) {
        --_wbemuuid_lib_ref_count;
        return;
    }

    library_dyn_unload(&_wbemuuid_lib);

    _wbemuuid_lib_ref_count = 0;
}

#endif
