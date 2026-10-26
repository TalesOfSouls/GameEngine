/**
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_DATABASE_SQLITE_STATEMENT_H
#define COMS_DATABASE_SQLITE_STATEMENT_H

#include "../../stdlib/Stdlib.h"
#include "SqliteDatabase.h"

/**
 * Prepared statement wrapper
 *
 * Prepared statements are the only efficient and safe way to talk to SQLite:
 * the query is parsed once and all values are bound as parameters which
 * prevents SQL injection and avoids re-parsing for every request.
 *
 * A statement belongs to exactly one connection and must not be shared between
 * threads.
 */

struct SqliteStatement {
    sqlite3_stmt* stmt;
    sqlite3* con;
};

/**
 * Prepare a statement
 *
 * @param DatabaseConnection* con      Connection
 * @param const char*         sql      SQL
 * @param SqliteStatement*    statement Prepared statement
 *
 * @return int32 0 on success, sqlite error code otherwise
 */
inline
int32 db_stmt_prepare(
    DatabaseConnection* const __restrict con,
    const char* const __restrict sql,
    SqliteStatement* const __restrict statement
) NO_EXCEPT
{
    ASSERT_TRUE(con->handle);
    ASSERT_TRUE(statement);

    statement->stmt = NULL;
    statement->con = (sqlite3 *) con->handle;

    const int32 rc = sqlite3_prepare_v2(
        (sqlite3 *) con->handle,
        sql,
        -1,
        &statement->stmt,
        NULL
    );

    if (rc != SQLITE_OK) {
        LOG_1("[ERROR] SQLite prepare failed: %s", {DATA_TYPE_CHAR_STR, sqlite3_errmsg((sqlite3 *) con->handle)});
        statement->stmt = NULL;
    }

    return rc;
}

/**
 * Finalize a statement
 *
 * @param SqliteStatement* statement Statement
 */
inline
void db_stmt_finalize(SqliteStatement* const __restrict statement) NO_EXCEPT
{
    if (statement && statement->stmt) {
        sqlite3_finalize(statement->stmt);
        statement->stmt = NULL;
    }
}

/**
 * Reset a statement so it can be executed again
 *
 * @param SqliteStatement* statement Statement
 *
 * @return int32 0 on success
 */
inline
int32 db_stmt_reset(SqliteStatement* const __restrict statement) NO_EXCEPT
{
    if (!statement || !statement->stmt) {
        return SQLITE_MISUSE;
    }

    return sqlite3_reset(statement->stmt);
}

/**
 * Remove all bound values from a statement
 *
 * @param SqliteStatement* statement Statement
 *
 * @return int32 0 on success
 */
inline
int32 db_stmt_clear_bindings(SqliteStatement* const __restrict statement) NO_EXCEPT
{
    if (!statement || !statement->stmt) {
        return SQLITE_MISUSE;
    }

    return sqlite3_clear_bindings(statement->stmt);
}

/**
 * Bind a 64 bit integer
 *
 * @param SqliteStatement* statement Statement
 * @param int32            index     1 based parameter index
 * @param int64            value     Value
 *
 * @return int32 0 on success
 */
FORCE_INLINE
int32 db_stmt_bind_int64(SqliteStatement* const __restrict statement, int32 index, int64 value) NO_EXCEPT
{
    return statement->stmt ? sqlite3_bind_int64(statement->stmt, index, value) : SQLITE_MISUSE;
}

/**
 * Bind a 32 bit integer
 *
 * @param SqliteStatement* statement Statement
 * @param int32            index     1 based parameter index
 * @param int32            value     Value
 *
 * @return int32 0 on success
 */
FORCE_INLINE
int32 db_stmt_bind_int32(SqliteStatement* const __restrict statement, int32 index, int32 value) NO_EXCEPT
{
    return statement->stmt ? sqlite3_bind_int(statement->stmt, index, value) : SQLITE_MISUSE;
}

/**
 * Bind a text value
 *
 * The value is copied by SQLite (SQLITE_TRANSIENT) which allows the caller to
 * pass temporary buffers.
 *
 * @param SqliteStatement* statement Statement
 * @param int32            index     1 based parameter index
 * @param const char*      value     Text
 * @param int32            length    Text length or -1 for null terminated
 *
 * @return int32 0 on success
 */
FORCE_INLINE
int32 db_stmt_bind_text(
    SqliteStatement* const __restrict statement,
    int32 index,
    const char* const __restrict value,
    int32 length = -1
) NO_EXCEPT
{
    return statement->stmt
        ? sqlite3_bind_text(statement->stmt, index, value, length, SQLITE_TRANSIENT)
        : SQLITE_MISUSE;
}

/**
 * Bind NULL
 *
 * @param SqliteStatement* statement Statement
 * @param int32            index     1 based parameter index
 *
 * @return int32 0 on success
 */
FORCE_INLINE
int32 db_stmt_bind_null(SqliteStatement* const __restrict statement, int32 index) NO_EXCEPT
{
    return statement->stmt ? sqlite3_bind_null(statement->stmt, index) : SQLITE_MISUSE;
}

/**
 * Execute a prepared statement
 *
 * @param SqliteStatement* statement Statement
 *
 * @return int32 SQLITE_ROW, SQLITE_DONE or an error code
 */
FORCE_INLINE
int32 db_stmt_step(SqliteStatement* const __restrict statement) NO_EXCEPT
{
    return statement->stmt ? sqlite3_step(statement->stmt) : SQLITE_MISUSE;
}

/**
 * Check if a step returned a row
 *
 * @param int32 rc Step result
 *
 * @return bool True if a row is available
 */
FORCE_INLINE
bool db_stmt_is_row(int32 rc) NO_EXCEPT
{
    return rc == SQLITE_ROW;
}

/**
 * Check if a step completed successfully
 *
 * @param int32 rc Step result
 *
 * @return bool True if the statement completed
 */
FORCE_INLINE
bool db_stmt_is_done(int32 rc) NO_EXCEPT
{
    return rc == SQLITE_DONE;
}

/**
 * Read an integer column
 *
 * @param SqliteStatement* statement Statement
 * @param int32            column    0 based column index
 *
 * @return int64 Column value
 */
FORCE_INLINE
int64 db_stmt_column_int64(const SqliteStatement* const __restrict statement, int32 column) NO_EXCEPT
{
    return sqlite3_column_int64(statement->stmt, column);
}

/**
 * Read a 32 bit integer column
 *
 * @param SqliteStatement* statement Statement
 * @param int32            column    0 based column index
 *
 * @return int32 Column value
 */
FORCE_INLINE
int32 db_stmt_column_int32(const SqliteStatement* const __restrict statement, int32 column) NO_EXCEPT
{
    return sqlite3_column_int(statement->stmt, column);
}

/**
 * Read a text column
 *
 * The pointer is owned by SQLite and only valid until the next step/reset.
 *
 * @param SqliteStatement* statement Statement
 * @param int32            column    0 based column index
 *
 * @return const char* Column text (never NULL, empty string for NULL values)
 */
FORCE_INLINE
const char* db_stmt_column_text(const SqliteStatement* const __restrict statement, int32 column) NO_EXCEPT
{
    const unsigned char* const text = sqlite3_column_text(statement->stmt, column);

    return text ? (const char *) text : "";
}

/**
 * Amount of bytes in a text column
 *
 * @param SqliteStatement* statement Statement
 * @param int32            column    0 based column index
 *
 * @return int32 Byte count
 */
FORCE_INLINE
int32 db_stmt_column_bytes(const SqliteStatement* const __restrict statement, int32 column) NO_EXCEPT
{
    return sqlite3_column_bytes(statement->stmt, column);
}

/**
 * Check if a column is NULL
 *
 * @param SqliteStatement* statement Statement
 * @param int32            column    0 based column index
 *
 * @return bool True if the column is NULL
 */
FORCE_INLINE
bool db_stmt_column_is_null(const SqliteStatement* const __restrict statement, int32 column) NO_EXCEPT
{
    return sqlite3_column_type(statement->stmt, column) == SQLITE_NULL;
}

/**
 * Copy a text column into a fixed size buffer
 *
 * @param SqliteStatement* statement  Statement
 * @param int32            column     0 based column index
 * @param char*            out        Destination
 * @param int32            out_length Destination size (including null terminator)
 *
 * @return int32 Amount of copied characters
 */
inline
int32 db_stmt_column_text_copy(
    const SqliteStatement* const __restrict statement,
    int32 column,
    char* const __restrict out,
    int32 out_length
) NO_EXCEPT
{
    if (!out || out_length <= 0) {
        return 0;
    }

    const char* const text = db_stmt_column_text(statement, column);
    int32 length = db_stmt_column_bytes(statement, column);

    if (length > out_length - 1) {
        length = out_length - 1;
    }

    memcpy(out, text, (size_t) length);
    out[length] = '\0';

    return length;
}

/**
 * Execute a statement without reading rows (e.g. INSERT/UPDATE/DELETE)
 *
 * @param SqliteStatement* statement Statement
 *
 * @return bool True if the statement completed successfully
 */
inline
bool db_stmt_execute(SqliteStatement* const __restrict statement) NO_EXCEPT
{
    const int32 rc = db_stmt_step(statement);
    db_stmt_reset(statement);

    return rc == SQLITE_DONE || rc == SQLITE_ROW;
}

#endif
