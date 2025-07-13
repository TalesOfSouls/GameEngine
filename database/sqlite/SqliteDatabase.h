/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_DATABASE_SQLITE_H
#define COMS_DATABASE_SQLITE_H

#include "../../stdlib/Types.h"

#if _WIN32
    #include "../../dependencies/sqlite/src/sqlite3.h"
#else
    #include <sqlite3.h>
#endif

inline
int32 db_open_sqlite(DatabaseConnection* db)
{
    ASSERT_SIMPLE(sizeof(db->con) >= sizeof(sqlite3*));

    int32 rc;
    rc = sqlite3_open(db->host, (sqlite3 **) &db->con);

    db->con = rc;

    if (!rc) {
        return -1;
    }

    return rc;
}

inline
void db_close_sqlite(DatabaseConnection* db)
{
    sqlite3_close((sqlite3 *) db->con);
}

#endif