/**
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_NETWORK_PING_H
#define COMS_NETWORK_PING_H

#if _WIN32
    #include "../platform/win32/network/Ping.h"
#elif __linux__
    #include "../platform/linux/network/Ping.h"
#endif

#endif