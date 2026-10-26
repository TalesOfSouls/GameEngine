/**
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_DATABASE_CONNECTION_H
#define COMS_DATABASE_CONNECTION_H

#include "../stdlib/Stdlib.h"
#include "DatabaseType.h"

/**
 * Database connection
 *
 * `handle` contains the native connection object of the driver
 * (e.g. sqlite3* for SQLite, MYSQL* for MariaDB, ...).
 *
 * `con` is only kept for legacy code and must not be used by new code.
 */
struct DatabaseConnection {
    // @legacy Raw storage for the native handle
    byte con[32];

    uint32 id; // Internal id to identify the connection
    DatabaseType type;
    uint16 port;
    const char* host;
    const char* name;
    const char* user;
    const char* pass;

    // Native driver connection handle
    void* handle;
};

#endif
