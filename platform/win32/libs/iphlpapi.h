/**
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_PLATFORM_WIN32_LIB_IPHLPAPI_H
#define COMS_PLATFORM_WIN32_LIB_IPHLPAPI_H

#include <iphlpapi.h>
#include <netioapi.h>
#include "../../../stdlib/Stdlib.h"

#if COMS_STATIC_LINKING
    #pragma comment(lib, "iphlpapi.lib")

    #define IPHLPAPI_GetAdaptersAddresses GetAdaptersAddresses
    #define IPHLPAPI_GetIpForwardTable2 GetIpForwardTable2
    #define IPHLPAPI_FreeMibTable FreeMibTable
    #define IPHLPAPI_Icmp6CreateFile Icmp6CreateFile
    #define IPHLPAPI_Icmp6SendEcho2 Icmp6SendEcho2
    #define IPHLPAPI_IcmpCloseHandle IcmpCloseHandle
#else
    #include "../../../system/Library.h"
    #include "../../../system/Library.cpp"
    #include <winternl.h>

    typedef ULONG (WINAPI *GetAdaptersAddresses_t)(ULONG, ULONG, PVOID, PIP_ADAPTER_ADDRESSES, PULONG);
    typedef NETIO_STATUS (WINAPI *GetIpForwardTable2_t)(ADDRESS_FAMILY, PMIB_IPFORWARD_TABLE2*);
    typedef VOID (WINAPI *FreeMibTable_t)(PVOID);
    typedef HANDLE (WINAPI *Icmp6CreateFile_t)(VOID);
    typedef DWORD (WINAPI *Icmp6SendEcho2_t)(
        HANDLE, HANDLE, PIO_APC_ROUTINE, PVOID,
        struct sockaddr_in6*, struct sockaddr_in6*,
        LPVOID, WORD, PIP_OPTION_INFORMATION,
        LPVOID, DWORD, DWORD
    );
    typedef BOOL (WINAPI *IcmpCloseHandle_t)(HANDLE);

    static GetAdaptersAddresses_t pGetAdaptersAddresses = NULL;
    static GetIpForwardTable2_t pGetIpForwardTable2 = NULL;
    static FreeMibTable_t pFreeMibTable = NULL;
    static Icmp6CreateFile_t pIcmp6CreateFile = NULL;
    static Icmp6SendEcho2_t pIcmp6SendEcho2 = NULL;
    static IcmpCloseHandle_t pIcmpCloseHandle = NULL;

    #define IPHLPAPI_GetAdaptersAddresses pGetAdaptersAddresses
    #define IPHLPAPI_GetIpForwardTable2 pGetIpForwardTable2
    #define IPHLPAPI_FreeMibTable pFreeMibTable
    #define IPHLPAPI_Icmp6CreateFile pIcmp6CreateFile
    #define IPHLPAPI_Icmp6SendEcho2 pIcmp6SendEcho2
    #define IPHLPAPI_IcmpCloseHandle pIcmpCloseHandle

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
        pGetIpForwardTable2 = (GetIpForwardTable2_t) library_dyn_proc(_iphlpapi_lib, "GetIpForwardTable2");
        pFreeMibTable = (FreeMibTable_t) library_dyn_proc(_iphlpapi_lib, "FreeMibTable");

        pIcmp6CreateFile = (Icmp6CreateFile_t) library_dyn_proc(_iphlpapi_lib, "Icmp6CreateFile");
        pIcmp6SendEcho2  = (Icmp6SendEcho2_t) library_dyn_proc(_iphlpapi_lib, "Icmp6SendEcho2");
        pIcmpCloseHandle = (IcmpCloseHandle_t) library_dyn_proc(_iphlpapi_lib, "IcmpCloseHandle");

        ++_iphlpapi_lib_ref_count;

        return pGetAdaptersAddresses && pGetIpForwardTable2 && pFreeMibTable
            && pIcmp6CreateFile && pIcmp6SendEcho2 && pIcmpCloseHandle;
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

#endif
