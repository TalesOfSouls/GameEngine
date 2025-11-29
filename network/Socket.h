/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_NETWORK_SOCKET_H
#define COMS_NETWORK_SOCKET_H

#if _WIN32
    #include "../platform/win32/network/Socket.h"
#elif __linux__
    #include "../platform/linux/network/Socket.h"
#endif

inline
bool network_ipv6_is_usable() NO_EXCEPT {
    return network_is_ipv6_enabled_in_os()
        && network_has_ipv6_address()
        && network_has_ipv6_default_route();
}

#endif