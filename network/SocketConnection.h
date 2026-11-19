/**
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_NETWORK_SOCKET_CONNECTION_H
#define COMS_NETWORK_SOCKET_CONNECTION_H

#include "../stdlib/Stdlib.h"
#include "Socket.h"

struct SocketConnection {
    socketid sd;
    sockaddr_in6 addr;
    uint16 port;

    // used for epoll
    int32 fd;
};

#endif