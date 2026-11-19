/**
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_PLATFORM_WIN32_NETWORK_SOCKET_H
#define COMS_PLATFORM_WIN32_NETWORK_SOCKET_H

#include "../../../stdlib/Stdlib.h"
#include <winsock2.h>
#include <ws2ipdef.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

typedef SOCKET socketid;

#define SOCKET_INVALID INVALID_SOCKET

#endif
