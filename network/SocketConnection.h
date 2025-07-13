/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_NETWORK_SOCKET_CONNECTION_H
#define COMS_NETWORK_SOCKET_CONNECTION_H

#include "../stdlib/Types.h"

#if _WIN32
    #include <winsock2.h>
    #include <ws2ipdef.h>

    typedef SOCKET socketid;
#else
    #include <netdb.h>
    #include <unistd.h>
    #include <arpa/inet.h>

    typedef int32 socketid;
#endif

struct SocketConnection {
    socketid sd;
    sockaddr_in6 addr;
    uint16 port;

    // used for epoll
    int32 fd;
};

#endif