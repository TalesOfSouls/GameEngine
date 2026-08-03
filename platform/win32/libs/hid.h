#pragma once
#ifndef COMS_PLATFORM_WIN32_LIB_HID_H
#define COMS_PLATFORM_WIN32_LIB_HID_H

#include <windows.h>
#include <hidsdi.h>
#include "../../../stdlib/Stdlib.h"

#if COMS_STATIC_LINKING
    #pragma comment(lib, "hidsdi.lib")

    #define HID_HidD_GetHidGuid HidD_GetHidGuid
    #define HID_HidD_GetAttributes HidD_GetAttributes
    #define HID_HidD_GetPreparsedData HidD_GetPreparsedData
    #define HID_HidD_FreePreparsedData HidD_FreePreparsedData
    #define HID_HidP_GetCaps HidP_GetCaps
#else
    #include "../../../system/Library.h"
    #include "../../../system/Library.cpp"

    typedef VOID (WINAPI *HidD_GetHidGuid_t)(GUID*);
    typedef BOOLEAN (WINAPI *HidD_GetAttributes_t)(HANDLE, PHIDD_ATTRIBUTES);
    typedef BOOLEAN (WINAPI *HidD_GetPreparsedData_t)(HANDLE, PHIDP_PREPARSED_DATA*);
    typedef BOOLEAN (WINAPI *HidD_FreePreparsedData_t)(PHIDP_PREPARSED_DATA);
    typedef NTSTATUS (WINAPI *HidP_GetCaps_t)(PHIDP_PREPARSED_DATA, PHIDP_CAPS);

    static HidD_GetHidGuid_t pHidD_GetHidGuid = NULL;
    static HidD_GetAttributes_t pHidD_GetAttributes = NULL;
    static HidD_GetPreparsedData_t pHidD_GetPreparsedData = NULL;
    static HidD_FreePreparsedData_t pHidD_FreePreparsedData = NULL;
    static HidP_GetCaps_t pHidP_GetCaps = NULL;

    #define HID_HidD_GetHidGuid pHidD_GetHidGuid
    #define HID_HidD_GetAttributes pHidD_GetAttributes
    #define HID_HidD_GetPreparsedData pHidD_GetPreparsedData
    #define HID_HidD_FreePreparsedData pHidD_FreePreparsedData
    #define HID_HidP_GetCaps pHidP_GetCaps

    static LibraryHandle _hid_lib;

    static int _hid_lib_ref_count = 0;

    inline
    bool hid_init() NO_EXCEPT
    {
        if (_hid_lib_ref_count) {
            ++_hid_lib_ref_count;
            return true;
        }

        bool success = library_dyn_load(&_hid_lib, L"hid.dll");
        if (!success) {
            return false;
        }

        pHidD_GetHidGuid = (HidD_GetHidGuid_t) library_dyn_proc(_hid_lib, "HidD_GetHidGuid");
        pHidD_GetAttributes = (HidD_GetAttributes_t) library_dyn_proc(_hid_lib, "HidD_GetAttributes");
        pHidD_GetPreparsedData = (HidD_GetPreparsedData_t) library_dyn_proc(_hid_lib, "HidD_GetPreparsedData");
        pHidD_FreePreparsedData = (HidD_FreePreparsedData_t) library_dyn_proc(_hid_lib, "HidD_FreePreparsedData");
        pHidP_GetCaps = (HidP_GetCaps_t) library_dyn_proc(_hid_lib, "HidP_GetCaps");

        ++_hid_lib_ref_count;

        return pHidD_GetHidGuid
            && pHidD_GetAttributes
            && pHidD_GetPreparsedData
            && pHidD_FreePreparsedData
            && pHidP_GetCaps;
    }

    inline
    void hid_free() NO_EXCEPT
    {
        if (_hid_lib_ref_count > 1) {
            --_hid_lib_ref_count;
            return;
        }

        library_dyn_unload(&_hid_lib);

        _hid_lib_ref_count = 0;
    }
#endif

#endif
