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

#include "../../../stdlib/Types.h"
#include "../../../network/SocketConnection.h"
#include "../../../utils/EndianUtils.h"

#pragma comment(lib, "Ws2_32.lib")

void socket_client_udp_create(SocketConnection* con, uint16 port = 0) {
    WSADATA wsaData;

    // Initialize Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        return;
    }

    // Create socket
    con->sd = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
    if (con->sd == INVALID_SOCKET) {
        WSACleanup();
        return;
    }

    // Set socket to non-blocking mode
    u_long mode = 1;
    if (ioctlsocket(con->sd, FIONBIO, &mode) != NO_ERROR) {
        closesocket(con->sd);
        WSACleanup();
        return;
    }

    // Bind socket
    con->addr.sin6_family = AF_INET6;
    con->addr.sin6_addr = in6addr_any;
    con->addr.sin6_port = SWAP_ENDIAN_BIG(port); // 0 = OS decides the port

    if (bind(con->sd, (struct sockaddr*) &con->addr, sizeof(con->addr)) == SOCKET_ERROR) {
        closesocket(con->sd);
        WSACleanup();
        return;
    }
}

#endif