/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_NETWORK_SERVER_H
#define COMS_NETWORK_SERVER_H

#if _WIN32
    #include "../platform/win32/network/Server.h"
#elif __linux__
    #include "../platform/linux/network/Server.h"
#endif

#endif