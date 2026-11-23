/**
 * Platform independent socket types (socketid, SOCKET_INVALID)
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_NETWORK_SOCKET_H
#define COMS_NETWORK_SOCKET_H

#include "../stdlib/Stdlib.h"

#if _WIN32
    #include "../platform/win32/network/Socket.h"
#elif __linux__
    #include "../platform/linux/network/Socket.h"
#endif

#endif
