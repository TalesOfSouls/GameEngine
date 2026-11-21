/**
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_PLATFORM_WIN32_NETWORK_SOCKET_C
#define COMS_PLATFORM_WIN32_NETWORK_SOCKET_C

#include "../../../stdlib/Stdlib.h"
#include "Socket.h"
#include "../libs/Advapi32.h"
#include "../libs/iphlpapi.h"

// Part of mstcpip.h
#ifndef SIO_UDP_CONNRESET
    #define SIO_UDP_CONNRESET _WSAIOW(IOC_VENDOR, 12)
#endif

/**
 * Initializes the socket library (reference counted, every call needs a matching socket_cleanup())
 *
 * @return bool True on success
 */
FORCE_INLINE
bool socket_init() NO_EXCEPT
{
    WSADATA wsa_data;

    return WSAStartup(MAKEWORD(2, 2), &wsa_data) == 0;
}

FORCE_INLINE
void socket_cleanup() NO_EXCEPT
{
    WSACleanup();
}

inline
void socket_close(socketid sd) NO_EXCEPT
{
    shutdown(sd, SD_BOTH);
    closesocket(sd);
}

/**
 * Max. time a blocking receive waits for data
 *
 * @param socketid  sd          Socket
 * @param uint32    timeout_ms  Timeout in milliseconds
 *
 * @return bool True on success
 */
inline
bool socket_receive_timeout_set(socketid sd, uint32 timeout_ms) NO_EXCEPT
{
    const DWORD timeout = timeout_ms;

    return setsockopt(sd, SOL_SOCKET, SO_RCVTIMEO, (const char *) &timeout, sizeof(timeout)) == 0;
}

/**
 * Windows reports an ICMP "port unreachable" (e.g. the server isn't running) as WSAECONNRESET of the next receive
 * of a UDP socket, which then returns immediately instead of waiting for the timeout.
 * This disables that behavior (Linux doesn't need this)
 *
 * @param socketid sd UDP socket
 *
 * @return void
 */
inline
void socket_udp_connreset_ignore(socketid sd) NO_EXCEPT
{
    BOOL report_reset = FALSE;
    DWORD bytes = 0;
    WSAIoctl(sd, SIO_UDP_CONNRESET, &report_reset, sizeof(report_reset), NULL, 0, &bytes, NULL, NULL);
}

static inline
bool network_is_ipv6_enabled_in_os() NO_EXCEPT
{
    if (!ADVAPI32_RegOpenKeyExW || !ADVAPI32_RegQueryValueExW || !ADVAPI32_RegCloseKey) {
        return true;
    }

    DWORD value = 0;
    DWORD size = sizeof(value);

    HKEY key;
    if (ADVAPI32_RegOpenKeyExW(HKEY_LOCAL_MACHINE,
        L"SYSTEM\\CurrentControlSet\\Services\\Tcpip6\\Parameters",
        0, KEY_READ, &key) != ERROR_SUCCESS
    ) {
        // If key missing, IPv6 is enabled by default
        return true;
    }

    if (ADVAPI32_RegQueryValueExW(key, L"DisabledComponents", NULL, NULL,
        (LPBYTE) &value, &size) == ERROR_SUCCESS
    ) {
        ADVAPI32_RegCloseKey(key);

        if (value == 0xffffffff) {
            return false;
        }
    }

    ADVAPI32_RegCloseKey(key);

    return true;
}

static inline
bool network_has_ipv6_address() NO_EXCEPT
{
    if (!IPHLPAPI_GetAdaptersAddresses) {
        return false;
    }

    IP_ADAPTER_ADDRESSES addresses[16384 / sizeof(IP_ADAPTER_ADDRESSES)] = {};
    unsigned long size = sizeof(addresses);

    if (IPHLPAPI_GetAdaptersAddresses(AF_UNSPEC, 0, NULL, addresses, &size) != ERROR_SUCCESS) {
        return false;
    }

    const IP_ADAPTER_ADDRESSES* adapter = addresses;
    while (adapter) {
        const IP_ADAPTER_UNICAST_ADDRESS *ua = adapter->FirstUnicastAddress;

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
bool network_has_ipv6_default_route() NO_EXCEPT
{
    if (!IPHLPAPI_GetIpForwardTable2 || !IPHLPAPI_FreeMibTable) {
        return false;
    }

    PMIB_IPFORWARD_TABLE2 table;
    if (IPHLPAPI_GetIpForwardTable2(AF_INET6, &table) != NO_ERROR) {
        return false;
    }

    bool found = false;

    for (ULONG i = 0; i < table->NumEntries; i++) {
        const MIB_IPFORWARD_ROW2* row = &table->Table[i];

        // DestinationPrefix = ::
        if (row->DestinationPrefix.PrefixLength == 0) {
            found = true;
            break;
        }
    }

    IPHLPAPI_FreeMibTable(table);

    return found;
}

#endif
