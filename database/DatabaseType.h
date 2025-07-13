/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_DATABASE_TYPE_H
#define COMS_DATABASE_TYPE_H

enum DatabaseType : byte {
    DB_TYPE_UNKNOWN,
    DB_TYPE_SQLITE,
    DB_TYPE_MARIA,
    DB_TYPE_PSQL,
    DB_TYPE_MSSQL
};

DatabaseType db_type_from_string(const char* str)
{
    if (str_compare(str, "sqlite", sizeof("sqlite") - 1) == 0) {
        return DB_TYPE_SQLITE;
    } else if (str_compare(str, "mariadb", sizeof("mariadb") - 1) == 0) {
        return DB_TYPE_MARIA;
    } else if (str_compare(str, "mysql", sizeof("mysql") - 1) == 0) {
        return DB_TYPE_MARIA;
    } else if (str_compare(str, "psql", sizeof("psql") - 1) == 0) {
        return DB_TYPE_PSQL;
    } else if (str_compare(str, "mssql", sizeof("mssql") - 1) == 0) {
        return DB_TYPE_MSSQL;
    }

    return DB_TYPE_UNKNOWN;
}

#endif