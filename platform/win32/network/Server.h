/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_PLATFORM_WIN32_NETWORK_SERVER_H
#define COMS_PLATFORM_WIN32_NETWORK_SERVER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <winsock2.h>
#include <ws2tcpip.h>

#include "../../../network/SocketConnection.h"
#include "../../../utils/EndianUtils.h"

#pragma comment(lib, "Ws2_32.lib")

void socket_non_blocking(SocketConnection* con)
{
    u_long mode = 1;
    if (ioctlsocket(con->sd, FIONBIO, &mode) != NO_ERROR) {
        closesocket(con->sd);
    }
}

bool socket_server_udp_create(SocketConnection* con) {
    // Create socket
    con->sd = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
    if (con->sd == INVALID_SOCKET) {
        return false;
    }

    // Bind socket
    memset(&con->addr, 0, sizeof(con->addr));
    con->addr.sin6_family = AF_INET6;
    con->addr.sin6_addr = in6addr_any;
    con->addr.sin6_port = htons(con->port);

    if (bind(con->sd, (struct sockaddr *) &con->addr, sizeof(con->addr)) == SOCKET_ERROR) {
        closesocket(con->sd);
        return false;
    }

    return true;
}

bool socket_server_websocket_create(SocketConnection* con) {
    con->sd = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
    if (con->sd == INVALID_SOCKET) {
        con->sd = 0;
        return false;
    }

    // Set non-blocking mode (Windows version)
    unsigned long mode = 1; // 1 = non-blocking, 0 = blocking
    if (ioctlsocket(con->sd, FIONBIO, &mode) != 0) {
        closesocket(con->sd);
        con->sd = 0;
        return false;
    }

    // Set SO_REUSEADDR
    int opt = 1;
    if (setsockopt(con->sd, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt)) == SOCKET_ERROR) {
        closesocket(con->sd);
        con->sd = 0;
        return false;
    }

    memset(&con->addr, 0, sizeof(con->addr));
    con->addr.sin6_family = AF_INET6;
    con->addr.sin6_addr = in6addr_any;
    con->addr.sin6_port = htons(con->port);

    if (bind(con->sd, (SOCKADDR*)&con->addr, sizeof(con->addr)) == SOCKET_ERROR) {
        closesocket(con->sd);
        con->sd = 0;
        return false;
    }

    if (listen(con->sd, SOMAXCONN) == SOCKET_ERROR) {
        closesocket(con->sd);
        con->sd = 0;
        return false;
    }

    return true;
}

bool socket_server_http_create(SocketConnection* con)
{
    con->sd = socket(AF_INET6, SOCK_STREAM, 0);
    if (con->sd == INVALID_SOCKET) {
        return false;
    }

    memset(&con->addr, 0, sizeof(con->addr));
    con->addr.sin6_family = AF_INET6;
    con->addr.sin6_addr = in6addr_any;
    con->addr.sin6_port = htons(con->port);

    if (bind(con->sd, (struct sockaddr *) &con->addr, sizeof(con->addr)) == SOCKET_ERROR) {
        closesocket(con->sd);
        return false;
    }

    if (listen(con->sd, 5) < 0) {
        closesocket(con->sd);
        return false;
    }

    return true;
}

#endif