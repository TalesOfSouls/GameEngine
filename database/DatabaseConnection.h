/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_DATABASE_CONNECTION_H
#define COMS_DATABASE_CONNECTION_H

#include "../stdlib/Types.h"
#include "DatabaseType.h"

struct DatabaseConnection {
    byte con[32];

    uint32 id; // Internal id to identify the connection
    DatabaseType type;
    uint16 port;
    const char* host;
    const char* name;
    const char* user;
    const char* pass;
};

#endif