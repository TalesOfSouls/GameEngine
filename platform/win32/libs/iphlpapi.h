/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_PLATFORM_WIN32_LIB_IPHLPAPI_H
#define COMS_PLATFORM_WIN32_LIB_IPHLPAPI_H

#include <iphlpapi.h>
#include <netioapi.h>
#include "../../../stdlib/Stdlib.h"
#include "../../../system/Library.h"
#include "../../../system/Library.cpp"

typedef ULONG (WINAPI *GetAdaptersAddresses_t)(ULONG, ULONG, PVOID, PIP_ADAPTER_ADDRESSES, PULONG);
typedef NETIO_STATUS (WINAPI *GetIpForwardTable2_t)(ADDRESS_FAMILY, PMIB_IPFORWARD_TABLE2*);
typedef VOID (WINAPI *FreeMibTable_t)(PVOID);

static GetAdaptersAddresses_t pGetAdaptersAddresses = NULL;
static GetIpForwardTable2_t pGetIpForwardTable2 = NULL;
static FreeMibTable_t pFreeMibTable = NULL;

static LibraryHandle _iphlpapi_lib;

static int _iphlpapi_lib_ref_count = 0;

inline
bool iphlpapi_init() NO_EXCEPT
{
    if (_iphlpapi_lib_ref_count) {
        ++_iphlpapi_lib_ref_count;
        return true;
    }

    bool success = library_dyn_load(&_iphlpapi_lib, L"iphlpapi.dll");
    if (!success) {
        return false;
    }

    pGetAdaptersAddresses = (GetAdaptersAddresses_t) library_dyn_proc(_iphlpapi_lib, "GetAdaptersAddresses");
    pGetIpForwardTable2   = (GetIpForwardTable2_t) library_dyn_proc(_iphlpapi_lib, "GetIpForwardTable2");
    pFreeMibTable         = (FreeMibTable_t) library_dyn_proc(_iphlpapi_lib, "FreeMibTable");

    return pGetAdaptersAddresses && pGetIpForwardTable2 && pFreeMibTable;
}

inline
void iphlpapi_free() NO_EXCEPT
{
    if (_iphlpapi_lib_ref_count > 1) {
        --_iphlpapi_lib_ref_count;
        return;
    }

    library_dyn_unload(&_iphlpapi_lib);

    _iphlpapi_lib_ref_count = 0;
}

#endif
