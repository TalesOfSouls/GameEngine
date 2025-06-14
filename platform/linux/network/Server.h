/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_PLATFORM_LINUX_NETWORK_SERVER_H
#define COMS_PLATFORM_LINUX_NETWORK_SERVER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/tcp.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>

#include "../../../stdlib/Types.h"
#include "../../../network/SocketConnection.h"
#include "../../../utils/EndianUtils.h"

inline
bool socket_non_blocking(SocketConnection* con)
{
    int32 flags = fcntl(con->sd, F_GETFL, 0);
    if (flags < 0) {
        return false;
    }

    return fcntl(con->sd, F_SETFL, flags | O_NONBLOCK) >= 0;
}

inline
void socket_no_delay(SocketConnection* con)
{
    int32 nodelay = 1;
    setsockopt(con->sd, IPPROTO_TCP, TCP_NODELAY, (char*)&nodelay, sizeof(nodelay));
}

// WARNING: requires `sudo setcap cap_net_raw=eip /path/to/your_program`
int32 socket_server_raw_create(SocketConnection* con) {
    con->sd = socket(AF_INET6, SOCK_RAW, 255);
    if (con->sd < 0) {
        con->sd = 0;
        LOG_1("[ERROR] Socket failed");

        return -1;
    }

    int32 flags;
    if ((flags = fcntl(con->sd, F_GETFL, 0)) < 0) {
        close(con->sd);
        con->sd = 0;
        LOG_1("[ERROR] Fcntl F_GETFL failed");

        return -2;
    }

    if (fcntl(con->sd, F_SETFL, flags | O_NONBLOCK) < 0) {
        close(con->sd);
        con->sd = 0;
        LOG_1("[ERROR] Fcntl O_NONBLOCK failed");

        return -3;
    }

    memset(&con->addr, 0, sizeof(con->addr));
    con->addr.sin6_family = AF_INET6;
    con->addr.sin6_addr = in6addr_any;
    con->addr.sin6_port = htons(con->port);

    if (bind(con->sd, (sockaddr *) &con->addr, sizeof(con->addr)) < 0) {
        close(con->sd);
        con->sd = 0;

        if (errno == EADDRINUSE) {
            LOG_1("[ERROR] Port already in use");
            return -4;
        } else {
            LOG_1("[ERROR] Binding failed");
            return -5;
        }
    }

    return 0;
}

// WARNING: requires `sudo setcap cap_net_raw=eip /path/to/your_program`
int32 socket_server_udp_raw_create(SocketConnection* con) {
    con->sd = socket(AF_INET6, SOCK_RAW, IPPROTO_UDP);
    if (con->sd < 0) {
        con->sd = 0;
        LOG_1("[ERROR] Socket failed");

        return -1;
    }

    int32 flags;
    if ((flags = fcntl(con->sd, F_GETFL, 0)) < 0) {
        close(con->sd);
        con->sd = 0;
        LOG_1("[ERROR] Fcntl F_GETFL failed");

        return -2;
    }

    if (fcntl(con->sd, F_SETFL, flags | O_NONBLOCK) < 0) {
        close(con->sd);
        con->sd = 0;
        LOG_1("[ERROR] Fcntl O_NONBLOCK failed");

        return -3;
    }

    memset(&con->addr, 0, sizeof(con->addr));
    con->addr.sin6_family = AF_INET6;
    con->addr.sin6_addr = in6addr_any;
    con->addr.sin6_port = htons(con->port);

    if (bind(con->sd, (sockaddr *) &con->addr, sizeof(con->addr)) < 0) {
        close(con->sd);
        con->sd = 0;

        if (errno == EADDRINUSE) {
            LOG_1("[ERROR] Port already in use");
            return -4;
        } else {
            LOG_1("[ERROR] Binding failed");
            return -5;
        }
    }

    return 0;
}

int32 socket_server_udp_create(SocketConnection* con) {
    con->sd = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
    if (con->sd < 0) {
        con->sd = 0;
        LOG_1("[ERROR] Socket failed");

        return -1;
    }

    int32 flags;
    if ((flags = fcntl(con->sd, F_GETFL, 0)) < 0) {
        close(con->sd);
        con->sd = 0;
        LOG_1("[ERROR] Fcntl F_GETFL failed");

        return -2;
    }

    if (fcntl(con->sd, F_SETFL, flags | O_NONBLOCK) < 0) {
        close(con->sd);
        con->sd = 0;
        LOG_1("[ERROR] Fcntl O_NONBLOCK failed");

        return -3;
    }

    memset(&con->addr, 0, sizeof(con->addr));
    con->addr.sin6_family = AF_INET6;
    con->addr.sin6_addr = in6addr_any;
    con->addr.sin6_port = htons(con->port);

    if (bind(con->sd, (sockaddr *) &con->addr, sizeof(con->addr)) < 0) {
        close(con->sd);
        con->sd = 0;

        if (errno == EADDRINUSE) {
            LOG_1("[ERROR] Port already in use");
            return -4;
        } else {
            LOG_1("[ERROR] Binding failed");
            return -5;
        }
    }

    return 0;
}

int32 socket_server_http_create(SocketConnection* con, int32 max_connections = 5)
{
    con->sd = socket(AF_INET6, SOCK_STREAM | SOCK_NONBLOCK, 0);
    if (con->sd < 0) {
        con->sd = 0;
        LOG_1("[ERROR] Socket failed");

        return -1;
    }

    int32 opt = 1;
    if (setsockopt(con->sd, SOL_SOCKET, SO_REUSEADDR, (const char*) &opt, sizeof(opt)) < 0) {
        close(con->sd);
        con->sd = 0;
        LOG_1("[ERROR] Setsockopt SO_REUSEADDR failed");

        return -2;
    }

    // For multi-process servers
    opt = 1;
    if (setsockopt(con->sd, SOL_SOCKET, SO_REUSEPORT, (const char*) &opt, sizeof(opt)) < 0) {
        close(con->sd);
        con->sd = 0;
        LOG_1("[ERROR] Setsockopt SO_REUSEPORT failed");

        return -1;
    }

    opt = 1;
    if (setsockopt(con->sd, SOL_SOCKET, SO_KEEPALIVE, (const char*) &opt, sizeof(opt)) < 0) {
        close(con->sd);
        con->sd = 0;
        LOG_1("[ERROR] Setsockopt SO_KEEPALIVE failed");

        return -1;
    }

    opt = 1;
    if (setsockopt(con->sd, IPPROTO_TCP, TCP_NODELAY, (const char*) &opt, sizeof(opt)) < 0) {
        close(con->sd);
        con->sd = 0;
        LOG_1("[ERROR] Setsockopt TCP_NODELAY failed");

        return -1;
    }

    /*
    // Configure IPv6 to also accept IPv4 connections
    opt = 0;
    setsockopt(con->sd, IPPROTO_IPV6, IPV6_V6ONLY, (const char*) &opt, sizeof(opt));
    */

    memset(&con->addr, 0, sizeof(con->addr));
    con->addr.sin6_family = AF_INET6;
    con->addr.sin6_addr = in6addr_any;
    con->addr.sin6_port = htons(con->port);

    if (bind(con->sd, (struct sockaddr *) &con->addr, sizeof(con->addr)) < 0) {
        close(con->sd);
        con->sd = 0;

        if (errno == EADDRINUSE) {
            LOG_1("[ERROR] Port already in use");
            return -3;
        } else {
            LOG_1("[ERROR] Binding failed");
            return -4;
        }
    }

    if (listen(con->sd, max_connections) < 0) {
        close(con->sd);
        con->sd = 0;

        LOG_1("[ERROR] Listen failed");

        return -5;
    }

    con->fd = epoll_create1(0);
    if (con->fd < 0) {
        close(con->sd);
        LOG_1("[ERROR] Epoll failed");

        return -6;
    }

    return 0;
}

int32 socket_server_websocket_create(SocketConnection* con) {
    con->sd = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
    if (con->sd < 0) {
        con->sd = 0;
        LOG_1("[ERROR] Socket failed");

        return -1;
    }

    int32 flags;
    if ((flags = fcntl(con->sd, F_GETFL, 0)) < 0) {
        close(con->sd);
        con->sd = 0;
        LOG_1("[ERROR] Fcntl F_GETFL failed");

        return -2;
    }

    if (fcntl(con->sd, F_SETFL, flags | O_NONBLOCK) < 0) {
        close(con->sd);
        con->sd = 0;
        LOG_1("[ERROR] Fcntl O_NONBLOCK failed");

        return -3;
    }

    int opt = 1;
    setsockopt(con->sd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    memset(&con->addr, 0, sizeof(con->addr));
    con->addr.sin6_family = AF_INET6;
    con->addr.sin6_addr = in6addr_any;
    con->addr.sin6_port = htons(con->port);

    if (bind(con->sd, (sockaddr*)&con->addr, sizeof(con->addr)) < 0) {
        close(con->sd);
        con->sd = 0;

        if (errno == EADDRINUSE) {
            LOG_1("[ERROR] Port already in use");
            return -4;
        } else {
            LOG_1("[ERROR] Binding failed");
            return -5;
        }
    }

    if (listen(con->sd, SOMAXCONN) < 0) {
        close(con->sd);
        con->sd = 0;

        LOG_1("[ERROR] Listen failed");
    }

    return true;
}

void socket_connection_close(SocketConnection* con) {
    socket_close(con->sd);

    if (con->fd) {
        socket_close(con->fd);
    }
}

#endif