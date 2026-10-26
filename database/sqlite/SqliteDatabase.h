/**
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_DATABASE_SQLITE_H
#define COMS_DATABASE_SQLITE_H

#include "../../stdlib/Stdlib.h"
#include "../../log/Log.h"
#include "../DatabaseConnection.h"

/**
 * SQLite driver
 *
 * The header expects sqlite3.h to be available on the include path.
 * Linux:  -lsqlite3 provided by the distribution (libsqlite3-dev)
 * Windows: add the sqlite source/include directory to the include path
 *          (e.g. ../EngineDependencies/sqlite/src)
 *
 * Threading: every connection is opened with SQLITE_OPEN_NOMUTEX since the
 * application is expected to use one connection per thread (or serialize
 * access). This avoids the internal mutex overhead.
 */

#include <sqlite3.h>

/**
 * Get the native sqlite handle of a connection
 *
 * @param DatabaseConnection* con Connection
 *
 * @return sqlite3* Native handle or NULL
 */
FORCE_INLINE
sqlite3* db_sqlite_handle(DatabaseConnection* const __restrict con) NO_EXCEPT
{
    return (sqlite3 *) con->handle;
}

/**
 * Open a SQLite database
 *
 * @param DatabaseConnection* con       Connection
 * @param const char*         path      Database file path
 * @param bool                read_only Open read only (allows sharing a WAL database with a writer)
 *
 * @return int32 0 on success, sqlite error code otherwise
 */
inline
int32 db_sqlite_open(
    DatabaseConnection* const __restrict con,
    const char* const __restrict path,
    bool read_only = false
) NO_EXCEPT
{
    ASSERT_TRUE(con);
    ASSERT_TRUE(path);

    const int32 flags = read_only
        ? SQLITE_OPEN_READONLY | SQLITE_OPEN_NOMUTEX
        : SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_NOMUTEX;

    sqlite3* handle = NULL;
    const int32 rc = sqlite3_open_v2(path, &handle, flags, NULL);

    if (rc != SQLITE_OK) {
        if (handle) {
            sqlite3_close(handle);
        }

        con->handle = NULL;

        return rc;
    }

    con->handle = handle;
    con->type = DB_TYPE_SQLITE;
    con->host = path;

    // Wait instead of failing immediately when another connection is writing
    sqlite3_busy_timeout(handle, 5000);

    return 0;
}

/**
 * Close a SQLite database
 *
 * @param DatabaseConnection* con Connection
 */
inline
void db_sqlite_close(DatabaseConnection* const __restrict con) NO_EXCEPT
{
    if (!con->handle) {
        return;
    }

    sqlite3_close((sqlite3 *) con->handle);
    con->handle = NULL;
}

/**
 * Execute one or multiple SQL statements without a result
 *
 * @param DatabaseConnection* con Connection
 * @param const char*         sql SQL
 *
 * @return int32 0 on success, sqlite error code otherwise
 */
inline
int32 db_sqlite_exec(DatabaseConnection* const __restrict con, const char* const __restrict sql) NO_EXCEPT
{
    ASSERT_TRUE(con->handle);

    char* error = NULL;
    const int32 rc = sqlite3_exec((sqlite3 *) con->handle, sql, NULL, NULL, &error);

    if (error) {
        LOG_1("[ERROR] SQLite: %s", {DATA_TYPE_CHAR_STR, error});
        sqlite3_free(error);
    }

    return rc;
}

/**
 * Execute a SQL statement that returns a single integer value
 *
 * @param DatabaseConnection* con      Connection
 * @param const char*         sql      SQL query
 * @param int64               fallback Value returned if the query fails
 *
 * @return int64 First column of the first row
 */
inline
int64 db_sqlite_query_int64(
    DatabaseConnection* const __restrict con,
    const char* const __restrict sql,
    int64 fallback = 0
) NO_EXCEPT
{
    ASSERT_TRUE(con->handle);

    sqlite3_stmt* stmt = NULL;
    if (sqlite3_prepare_v2((sqlite3 *) con->handle, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return fallback;
    }

    int64 result = fallback;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        result = sqlite3_column_int64(stmt, 0);
    }

    sqlite3_finalize(stmt);

    return result;
}

/**
 * Last error message of a connection
 *
 * @param DatabaseConnection* con Connection
 *
 * @return const char* Error message
 */
inline
const char* db_sqlite_error(DatabaseConnection* const __restrict con) NO_EXCEPT
{
    return con->handle ? sqlite3_errmsg((sqlite3 *) con->handle) : "no connection";
}

/**
 * Amount of rows changed by the last statement
 *
 * @param DatabaseConnection* con Connection
 *
 * @return int32 Changed rows
 */
FORCE_INLINE
int32 db_sqlite_changes(DatabaseConnection* const __restrict con) NO_EXCEPT
{
    return con->handle ? sqlite3_changes((sqlite3 *) con->handle) : 0;
}

/**
 * Row id of the last inserted row
 *
 * @param DatabaseConnection* con Connection
 *
 * @return int64 Row id
 */
FORCE_INLINE
int64 db_sqlite_last_insert_rowid(DatabaseConnection* const __restrict con) NO_EXCEPT
{
    return con->handle ? sqlite3_last_insert_rowid((sqlite3 *) con->handle) : 0;
}

/**
 * Enable WAL mode
 *
 * WAL allows concurrent readers while one writer is active which is exactly
 * what the auth server needs (network threads read, the database thread writes).
 *
 * @param DatabaseConnection* con Connection
 *
 * @return bool True if WAL is enabled
 */
inline
bool db_sqlite_enable_wal(DatabaseConnection* const __restrict con) NO_EXCEPT
{
    if (db_sqlite_exec(con, "PRAGMA journal_mode=WAL;") != 0) {
        return false;
    }

    // NORMAL is durable enough for a game server and much faster than FULL
    db_sqlite_exec(con, "PRAGMA synchronous=NORMAL;");

    return true;
}

/**
 * Read a PRAGMA value as integer
 *
 * @param DatabaseConnection* con      Connection
 * @param const char*         name     Pragma name (without PRAGMA and semicolon)
 * @param int64               fallback Value returned on failure
 *
 * @return int64 Pragma value
 */
inline
int64 db_sqlite_pragma_int64(
    DatabaseConnection* const __restrict con,
    const char* const __restrict name,
    int64 fallback = 0
) NO_EXCEPT
{
    char sql[128];
    sprintf_fast(sql, sizeof(sql) - 1, "PRAGMA %s;", name);

    return db_sqlite_query_int64(con, sql, fallback);
}

/**
 * Legacy open function used by db_open()
 *
 * @param DatabaseConnection* db Connection, db->host contains the path
 *
 * @return int32 0 on success
 */
inline
int32 db_open_sqlite(DatabaseConnection* const db) NO_EXCEPT
{
    return db_sqlite_open(db, db->host, false);
}

/**
 * Legacy close function used by db_close()
 *
 * @param DatabaseConnection* db Connection
 */
inline
void db_close_sqlite(DatabaseConnection* const db) NO_EXCEPT
{
    db_sqlite_close(db);
}

#endif
