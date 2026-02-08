/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_PLATFORM_WIN32_NETWORK_SOCKET_H
#define COMS_PLATFORM_WIN32_NETWORK_SOCKET_H

#include "../../../stdlib/Stdlib.h"
#include <winsock2.h>
#include <ws2ipdef.h>
#include <ws2tcpip.h>

#include "../libs/Advapi32.h"
#include "../libs/iphlpapi.h"

#pragma comment(lib, "ws2_32.lib")

void socket_close(SOCKET sd) {
    shutdown(sd, SD_BOTH);
    closesocket(sd);
}

#define socket_prepare() { \
    WSADATA wsa_data; \
    if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0) { \
        return false; \
    } \
}
#define socket_cleanup() WSACleanup()

static inline
bool network_is_ipv6_enabled_in_os() NO_EXCEPT {
    if (!pRegOpenKeyExW || !pRegQueryValueExW || !pRegCloseKey) {
        return true;
    }

    DWORD value = 0;
    DWORD size = sizeof(value);

    HKEY key;
    if (pRegOpenKeyExW(HKEY_LOCAL_MACHINE,
        L"SYSTEM\\CurrentControlSet\\Services\\Tcpip6\\Parameters",
        0, KEY_READ, &key) != ERROR_SUCCESS
    ) {
        // If key missing, IPv6 is enabled by default
        return true;
    }

    if (pRegQueryValueExW(key, L"DisabledComponents", NULL, NULL,
        (LPBYTE) &value, &size) == ERROR_SUCCESS
    ) {
        pRegCloseKey(key);

        if (value == 0xffffffff) {
            return false;
        }
    }

    pRegCloseKey(key);

    return true;
}

static inline
bool network_has_ipv6_address() NO_EXCEPT {
    if (!pGetAdaptersAddresses) {
        return false;
    }

    IP_ADAPTER_ADDRESSES addresses[16384 / sizeof(IP_ADAPTER_ADDRESSES)] = {0};
    unsigned long size = sizeof(addresses);

    if (pGetAdaptersAddresses(AF_UNSPEC, 0, NULL, addresses, &size) != ERROR_SUCCESS) {
        return false;
    }

    IP_ADAPTER_ADDRESSES* adapter = addresses;
    while (adapter) {
        IP_ADAPTER_UNICAST_ADDRESS *ua = adapter->FirstUnicastAddress;

        while (ua) {
            if (ua->Address.lpSockaddr->sa_family == AF_INET6) {
                // At least one IPv6 address exists
                return true;
            }

            ua = ua->Next;
        }

        adapter = adapter->Next;
    }

    return false;
}

static inline
bool network_has_ipv6_default_route() NO_EXCEPT {
    if (!pGetIpForwardTable2 || !pFreeMibTable) {
        return false;
    }

    PMIB_IPFORWARD_TABLE2 table;
    if (pGetIpForwardTable2(AF_INET6, &table) != NO_ERROR) {
        return false;
    }

    bool found = false;

    for (ULONG i = 0; i < table->NumEntries; i++) {
        MIB_IPFORWARD_ROW2 *row = &table->Table[i];

        // DestinationPrefix = ::
        if (row->DestinationPrefix.PrefixLength == 0) {
            found = true;
            break;
        }
    }

    pFreeMibTable(table);

    return found;
}

#endif