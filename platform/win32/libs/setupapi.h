#ifndef COMS_PLATFORM_WIN32_LIB_SETUPAPI_H
#define COMS_PLATFORM_WIN32_LIB_SETUPAPI_H

#include <windows.h>
#include <setupapi.h>
#include "../../../stdlib/Stdlib.h"
#include "../../../system/Library.h"
#include "../../../system/Library.cpp"

typedef HDEVINFO (WINAPI *SetupDiGetClassDevsW_t)(const GUID*, PCWSTR, HWND, DWORD);
typedef BOOL (WINAPI *SetupDiEnumDeviceInterfaces_t)(HDEVINFO, PSP_DEVINFO_DATA, const GUID*, DWORD, PSP_DEVICE_INTERFACE_DATA);
typedef BOOL (WINAPI *SetupDiGetDeviceInterfaceDetailW_t)(HDEVINFO, PSP_DEVICE_INTERFACE_DATA, PSP_DEVICE_INTERFACE_DETAIL_DATA, DWORD, PDWORD, PSP_DEVINFO_DATA);
typedef BOOL (WINAPI *SetupDiDestroyDeviceInfoList_t)(HDEVINFO);

static SetupDiGetClassDevsW_t pSetupDiGetClassDevsW = NULL;
static SetupDiEnumDeviceInterfaces_t pSetupDiEnumDeviceInterfaces = NULL;
static SetupDiGetDeviceInterfaceDetailW_t pSetupDiGetDeviceInterfaceDetailW = NULL;
static SetupDiDestroyDeviceInfoList_t pSetupDiDestroyDeviceInfoList = NULL;

static LibraryHandle _setupapi_lib;

static int _setupapi_lib_ref_count = 0;

inline
bool setupapi_init() NO_EXCEPT
{
    if (_setupapi_lib_ref_count) {
        ++_setupapi_lib_ref_count;
        return true;
    }

    bool success = library_dyn_load(&_setupapi_lib, L"setupapi.dll");
    if (!success) {
        return false;
    }

    pSetupDiGetClassDevsW = (SetupDiGetClassDevsW_t) library_dyn_proc(_setupapi_lib, "SetupDiGetClassDevsW");
    pSetupDiEnumDeviceInterfaces = (SetupDiEnumDeviceInterfaces_t) library_dyn_proc(_setupapi_lib, "SetupDiEnumDeviceInterfaces");
    pSetupDiGetDeviceInterfaceDetailW = (SetupDiGetDeviceInterfaceDetailW_t) library_dyn_proc(_setupapi_lib, "SetupDiGetDeviceInterfaceDetailW");
    pSetupDiDestroyDeviceInfoList = (SetupDiDestroyDeviceInfoList_t) library_dyn_proc(_setupapi_lib, "SetupDiDestroyDeviceInfoList");

    return pSetupDiGetClassDevsW
        && pSetupDiEnumDeviceInterfaces
        && pSetupDiGetDeviceInterfaceDetailW
        && pSetupDiDestroyDeviceInfoList;
}

inline
void setupapi_free() NO_EXCEPT
{
    if (_setupapi_lib_ref_count > 1) {
        --_setupapi_lib_ref_count;
        return;
    }

    library_dyn_unload(&_setupapi_lib);

    _setupapi_lib_ref_count = 0;
}

#endif
