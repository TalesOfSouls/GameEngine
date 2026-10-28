/**
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_PLATFORM_WIN32_NETWORK_PING_H
#define COMS_PLATFORM_WIN32_NETWORK_PING_H

#include <stdio.h>
#include <stdlib.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <winternl.h>
#include "../libs/iphlpapi.h"
#include <icmpapi.h>

#pragma comment(lib, "ws2_32.lib")

/**
 * Sends one ICMP echo request to an IPv4 address
 *
 * The reply buffer must be large enough for the reply header, the echoed
 * payload, an ICMP error message (8 bytes) and the IO_STATUS_BLOCK.
 *
 * @param IPAddr    address         Destination IPv4 address
 * @param int32     payload_size    Echo payload size in bytes
 * @param int32     timeout         Timeout in milliseconds
 *
 * @return -1 on error, 0 on timeout, round trip time in ms on success
 */
static inline
int32 ping_ipv4(IPAddr address, int32 payload_size, int32 timeout) NO_EXCEPT
{
    #if !defined(COMS_IMPLICIT_LINKING) || !COMS_IMPLICIT_LINKING
        if (!IPHLPAPI_IcmpCreateFile || !IPHLPAPI_IcmpSendEcho || !IPHLPAPI_IcmpCloseHandle) {
            return -1;
        }
    #endif

    HANDLE hIcmp = IPHLPAPI_IcmpCreateFile();
    if (hIcmp == INVALID_HANDLE_VALUE) {
        return -1;
    }

    char* send_data = (char *) malloc(payload_size);
    if (!send_data) {
        IPHLPAPI_IcmpCloseHandle(hIcmp);

        return -1;
    }

    memset(send_data, 'A', payload_size);

    const DWORD reply_size = (DWORD) (
        sizeof(ICMP_ECHO_REPLY) + payload_size + 8 + sizeof(IO_STATUS_BLOCK)
    );

    BYTE* reply_buffer = (BYTE *) malloc(reply_size);
    if (!reply_buffer) {
        free(send_data);
        IPHLPAPI_IcmpCloseHandle(hIcmp);

        return -1;
    }

    const DWORD replies = IPHLPAPI_IcmpSendEcho(
        hIcmp, address,
        send_data, (WORD) payload_size,
        NULL,
        reply_buffer, reply_size,
        (DWORD) timeout
    );

    int32 result = 0;
    if (replies) {
        const PICMP_ECHO_REPLY reply = (PICMP_ECHO_REPLY) reply_buffer;
        if (reply->Status == IP_SUCCESS) {
            // A loopback reply can legitimately report 0 ms; 0 is reserved for timeout
            result = OMS_MAX((int32) reply->RoundTripTime, 1);
        }
    }

    free(reply_buffer);
    free(send_data);
    IPHLPAPI_IcmpCloseHandle(hIcmp);

    return result;
}

/**
 * Sends one ICMPv6 echo request to an IPv6 address
 *
 * @param const SOCKADDR_IN6*   address         Destination IPv6 address
 * @param int32                 payload_size    Echo payload size in bytes
 * @param int32                 timeout         Timeout in milliseconds
 *
 * @return -1 on error, 0 on timeout, round trip time in ms on success
 */
static inline
int32 ping_ipv6(const SOCKADDR_IN6* const address, int32 payload_size, int32 timeout) NO_EXCEPT
{
    #if !defined(COMS_IMPLICIT_LINKING) || !COMS_IMPLICIT_LINKING
        if (!IPHLPAPI_Icmp6CreateFile || !IPHLPAPI_Icmp6SendEcho2 || !IPHLPAPI_IcmpCloseHandle) {
            return -1;
        }
    #endif

    HANDLE hIcmp = IPHLPAPI_Icmp6CreateFile();
    if (hIcmp == INVALID_HANDLE_VALUE) {
        return -1;
    }

    char* send_data = (char *) malloc(payload_size);
    if (!send_data) {
        IPHLPAPI_IcmpCloseHandle(hIcmp);

        return -1;
    }

    memset(send_data, 'A', payload_size);

    const DWORD reply_size = (DWORD) (
        sizeof(ICMPV6_ECHO_REPLY) + payload_size + 8 + sizeof(IO_STATUS_BLOCK)
    );

    BYTE* reply_buffer = (BYTE *) malloc(reply_size);
    if (!reply_buffer) {
        free(send_data);
        IPHLPAPI_IcmpCloseHandle(hIcmp);

        return -1;
    }

    // in6addr_any lets the stack select the local source address.
    SOCKADDR_IN6 source = {};
    source.sin6_family = AF_INET6;

    const DWORD replies = IPHLPAPI_Icmp6SendEcho2(
        hIcmp,
        NULL, NULL, NULL,
        &source, (SOCKADDR_IN6 *) address,
        send_data, (WORD) payload_size,
        NULL,
        reply_buffer, reply_size,
        (DWORD) timeout
    );

    int32 result = 0;
    if (replies) {
        const PICMPV6_ECHO_REPLY reply = (PICMPV6_ECHO_REPLY) reply_buffer;
        if (reply->Status == IP_SUCCESS) {
            // A loopback reply can legitimately report 0 ms; 0 is reserved for timeout
            result = OMS_MAX((int32) reply->RoundTripTime, 1);
        }
    }

    free(reply_buffer);
    free(send_data);
    IPHLPAPI_IcmpCloseHandle(hIcmp);

    return result;
}

/**
 * Pings a hostname, IPv4 address or IPv6 address
 *
 * Resolves the host with getaddrinfo() so server_list.cfg entries can be
 * DNS names, IPv4 literals or IPv6 literals. IPv4 is tried first to avoid
 * waiting for an IPv6 timeout on dual-stack hosts with no working IPv6.
 *
 * @param const char*   host            Hostname or IP address
 * @param int32         payload_size    Echo payload size in bytes
 * @param int32         timeout         Timeout in milliseconds
 *
 * @return -1 on error, 0 on timeout, round trip time in ms on success
 */
static inline
int32 ping_host(const char* const host, int32 payload_size, int32 timeout = 2000) NO_EXCEPT
{
    if (!host || !*host || payload_size <= 0 || payload_size > 65500) {
        return -1;
    }

    WSADATA wsa_data;
    if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0) {
        return -1;
    }

    ADDRINFOA hints = {};
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;

    ADDRINFOA* addresses = NULL;
    if (getaddrinfo(host, NULL, &hints, &addresses) != 0) {
        WSACleanup();

        return -1;
    }

    int32 result = -1;

    // Prefer IPv4 to avoid a timeout on dual-stack hosts with no working IPv6
    for (int32 pass = 0; pass < 2; ++pass) {
        const int32 family = pass == 0 ? AF_INET : AF_INET6;

        for (ADDRINFOA* address = addresses; address; address = address->ai_next) {
            if (address->ai_family != family || !address->ai_addr) {
                continue;
            }

            if (family == AF_INET) {
                const sockaddr_in* const v4 = (const sockaddr_in *) address->ai_addr;
                result = ping_ipv4(v4->sin_addr.S_un.S_addr, payload_size, timeout);
            } else {
                result = ping_ipv6((const SOCKADDR_IN6 *) address->ai_addr, payload_size, timeout);
            }

            if (result > 0) {
                freeaddrinfo(addresses);
                WSACleanup();

                return result;
            }
        }
    }

    freeaddrinfo(addresses);
    WSACleanup();

    return result;
}

#endif
