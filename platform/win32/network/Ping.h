/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_PLATFORM_WIN32_NETWORK_PING_H
#define COMS_PLATFORM_WIN32_NETWORK_PING_H


#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <icmpapi.h>

#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "ws2_32.lib")

int32 ping_ipv6(const char* ipv6_address, int32 payload_size, int32 timeout = 2000)
{
    // Convert IPv6 address string to socket structure
    SOCKADDR_IN6 destAddr = {0};
    destAddr.sin6_family = AF_INET6;

    if (inet_pton(AF_INET6, ipv6_address, &destAddr.sin6_addr) != 1) {
        return -1;
    }

    // Payload buffer
    char send_data[16384];
    memset(send_data, 'A', payload_size);

    HANDLE hIcmp = Icmp6CreateFile();
    if (hIcmp == INVALID_HANDLE_VALUE) {
        return -1;
    }

    BYTE reply_buffer[2048];
    DWORD result = Icmp6SendEcho2(
        hIcmp,
        NULL,
        NULL,
        NULL,
        &destAddr,
        &destAddr,
        send_data,
        (WORD) payload_size,
        NULL,
        reply_buffer,
        (DWORD) sizeof(reply_buffer),
        timeout
    );

    if (result == 0) {
        IcmpCloseHandle(hIcmp);

        return 0;
    }

    PICMPV6_ECHO_REPLY reply = (PICMPV6_ECHO_REPLY) reply_buffer;

    /*
    printf("Reply from %s: bytes=%d status=0x%lx rtt=%lums\n",
            ipv6_address,
            reply->DataSize,
            reply->Status,
            reply->RoundTripTime);
    */

    IcmpCloseHandle(hIcmp);

    return reply->RoundTripTime;
}

#endif