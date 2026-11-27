/**
 * Platform independent socket functions
 *
 * Platform specific (platform/.../network/Socket.cpp):
 *      socket_init() / socket_cleanup()    socket library (Windows: WSAStartup/WSACleanup, reference counted)
 *      socket_close()
 *      socket_receive_timeout_set()        timeout of blocking receives
 *      socket_udp_connreset_ignore()       Windows: ICMP "port unreachable" doesn't abort the next receive
 *
 * The plain BSD functions (sendto, recvfrom, send, recv) work on all platforms,
 * only cast the buffer to (const char *) / (char *) for Windows.
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_NETWORK_SOCKET_C
#define COMS_NETWORK_SOCKET_C

#include "../stdlib/Stdlib.h"
#include "../utils/StringUtils.h"
#include "Socket.h"

#if _WIN32
    #include "../platform/win32/network/Socket.cpp"
#elif __linux__
    #include "../platform/linux/network/Socket.cpp"
#endif

inline
bool network_ipv6_is_usable() NO_EXCEPT
{
    return network_is_ipv6_enabled_in_os()
        && network_has_ipv6_address()
        && network_has_ipv6_default_route();
}

/**
 * Resolves a host (name or ip) for UDP and creates a matching socket
 *
 * @param const char*   host    Hostname or ip
 * @param uint16        port    Port
 * @param addrinfo**    result  Output: resolved address (freeaddrinfo() by the caller if a socket is returned)
 *
 * @return socketid SOCKET_INVALID on failure
 */
static inline
socketid socket_udp_resolve(const char* const __restrict host, uint16 port, addrinfo** const __restrict result) NO_EXCEPT
{
    char port_text[21];
    int_to_str((int64) port, port_text);

    addrinfo hints = {};
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = IPPROTO_UDP;

    *result = NULL;
    if (getaddrinfo(host, port_text, &hints, result) != 0 || !*result) {
        return SOCKET_INVALID;
    }

    const socketid sd = (socketid) socket((*result)->ai_family, (*result)->ai_socktype, (*result)->ai_protocol);
    if (sd == SOCKET_INVALID) {
        freeaddrinfo(*result);
        *result = NULL;
    }

    return sd;
}

/**
 * Creates a "connected" UDP socket (use send/recv, the OS drops datagrams of other senders)
 *
 * @param const char*   host    Hostname or ip
 * @param uint16        port    Port
 *
 * @return socketid SOCKET_INVALID on failure
 */
inline
socketid socket_udp_connect(const char* const host, uint16 port) NO_EXCEPT
{
    addrinfo* result;
    socketid sd = socket_udp_resolve(host, port, &result);
    if (sd == SOCKET_INVALID) {
        return SOCKET_INVALID;
    }

    if (connect(sd, result->ai_addr, (int32) result->ai_addrlen) != 0) {
        socket_close(&sd);
    }

    // @question do we really need this dynamic allocation is so nasty
    freeaddrinfo(result);

    return sd;
}

#endif
