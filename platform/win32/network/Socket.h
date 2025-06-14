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

#include "../../../stdlib/Types.h"
#include <winsock2.h>
#include <ws2ipdef.h>

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

#endif