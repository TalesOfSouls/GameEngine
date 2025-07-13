/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_DATABASE_H
#define COMS_DATABASE_H

#include "../stdlib/Types.h"

#include "DatabaseType.h"
#include "DatabaseConnection.h"

#if DB_MYSQL || DB_MARIA
#else
    int32 db_open_maria(void*) { return 0; };
    void db_close_maria(void*) {};
#endif

#if DB_PSQL
    #include "psql/PsqlDatabase.h"
#else
    int32 db_open_psql(void*) { return 0; };
    void db_close_psql(void*) {};
#endif

#if DB_SQLITE
    #include "sqlite/SqliteDatabase.h"
#else
    int32 db_open_sqlite(void*) { return 0; };
    void db_close_sqlite(void*) {};
#endif

inline
int32 db_open(DatabaseConnection* db)
{
    switch (db->type) {
        case DB_TYPE_SQLITE:
            return db_open_sqlite(db);
        case DB_TYPE_MARIA:
            return 0;
        case DB_TYPE_PSQL:
            return db_open_psql(db);
        case DB_TYPE_MSSQL:
            return 0;
        case DB_TYPE_UNKNOWN:
            return 0;
        default:
            UNREACHABLE();
    }
}

inline
void db_close(DatabaseConnection* db)
{
    switch (db->type) {
        case DB_TYPE_SQLITE: {
            db_close_sqlite(db);
            return;
        }
        case DB_TYPE_MARIA: {
            return;
        }
        case DB_TYPE_PSQL: {
            db_close_psql(db);
            return;
        }
        case DB_TYPE_MSSQL: {
            return;
        }
        case DB_TYPE_UNKNOWN:
            return;
        default:
            UNREACHABLE();
    }
}

#endif