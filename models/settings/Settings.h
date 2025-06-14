/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_MODELS_SETTINGS_H
#define COMS_MODELS_SETTINGS_H

#include "../../stdlib/Types.h"

#if _WIN32
    #include <Ws2ipdef.h>
#elif __linux__
    #include <arpa/inet.h>
#endif

struct SIMDSettings {
    byte simd_version;
    int32 simd_step_size;
    bool supports_abm;
};

struct NetworkSettings {
    in6_addr network_hostname;
    uint16 network_port;
};

struct ServerInfoSettings {
    char server_name[24];
    in6_addr server_hostname;
    uint16 server_port;
};

#endif
