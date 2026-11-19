/**
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_PLATFORM_LINUX_NETWORK_SOCKET_H
#define COMS_PLATFORM_LINUX_NETWORK_SOCKET_H

#include "../../../stdlib/Stdlib.h"

#include <sys/socket.h>
#include <sys/time.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>

typedef int32 socketid;

#define SOCKET_INVALID (-1)

#endif
