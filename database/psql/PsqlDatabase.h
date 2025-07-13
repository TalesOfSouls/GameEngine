/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_DATABASE_PSQL_H
#define COMS_DATABASE_PSQL_H

#include "../../stdlib/Types.h"
#include "../DbParam.h"

#if _WIN32
    #include "../../dependencies/psql/libpq-fe.h"
#else
    #include <libpq-fe.h>
#endif

inline
int32 db_open_psql(DatabaseConnection* db)
{
    ASSERT_SIMPLE(sizeof(db->con) >= sizeof(pqxx::connection));

    PGconn* db_con = (PGconn *) db->con;

    char conninfo[256];
    sprintf_fast(
        conninfo, sizeof(conninfo),
        "host=%s port=%d dbname=%s user=%s password=%s",
        db->host, db->port, db->name, db->user, db->pass
    );

    db_con = PQconnectdb(conninfo);
    if (PQstatus(db_con) != CONNECTION_OK) {
        return -1;
    }

    return 1;
}

inline
void db_close_psql(DatabaseConnection* db) {
    PQfinish((PGconn *) db->con);
    memset(db->con, 0, sizeof(db->con));
}

inline
void* db_prepare_psql(void* con, const char* name, const char* query) {
    PGresult* res = PQprepare((PGconn *) con, name, query, 0, NULL);
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        // @todo Handle error

        if (res) {
            PQclear(res);
        }

        return NULL;
    }

    PQclear(res);

    return res;
}

inline
void db_unprepare_psql(void* con, const char* name) {
    char dealloc[64];
    memcpy(dealloc, "DEALLOCATE ", sizeof("DEALLOCATE ") - 1);
    str_concat_append(dealloc + sizeof("DEALLOCATE ") - 1, name);

    PGresult *del_res = PQexec((PGconn *) con, dealloc);
    PQclear(del_res);
}

// WARNING: uint8 is chosen to ensure no overflow in dealloc, considering 64 bytes per deallocation
void db_unprepare_psql(void* con, const char** name, uint8 length) {
    char dealloc[16 * KILOBYTE];
    memcpy(dealloc, "DEALLOCATE", sizeof("DEALLOCATE") - 1);

    int32 offset = sizeof("DEALLOCATE") - 1;
    for (int32 i = 0 i < length; ++i) {
        dealloc[offset] = ' ';
        ++offset;
        str_concat_append(dealloc + offset, name);
        offset += str_length(name);
        ++name;
    }

    dealloc[offset] = '\0';

    PGresult *del_res = PQexec((PGconn *) con, dealloc);
    PQclear(del_res);
}

inline
void db_unprepare_psql(void* con) {
    PGresult *del_res = PQexec((PGconn *) con, "DEALLOCATE ALL");
    PQclear(del_res);
}

void* db_execute_prepared(
    void* con,
    const char* name,
    const DbParam* params,
    int32 param_count,
    RingMemory* ring
) {
    char** values = (char **) ring_get_memory(ring, sizeof(char *) * param_count);
    int32* formats = (int32 *) ring_get_memory(ring, sizeof(int32) * param_count);
    int32* lengths = (int32 *) ring_get_memory(ring, sizeof(int32) * param_count);

    char* local_arena = (char *) ring_get_memory(ring, 4 * KILOBYTE, 64, true);
    char* local_arena_end = local_arena + 4 * KILOBYTE;

    for (int32 i = 0; i < param_count; ++i) {
        switch (params[i].type) {
            case DB_PARAM_INT8: {
                const int32 data_length = sizeof(int8);
                if (local_arena + data_length < local_arena_end) {
                    values[i] = local_arena;
                    local_arena += data_length;
                } else {
                    values[i] = (char *) ring_get_memory(ring, data_length, 4, true);
                }

                *((int8 *) values[i]) = params[i].int8_val;
                lengths[i] = sizeof(int8);
                formats[i] = 1;
            } break;
            case DB_PARAM_INT16: {
                const int32 data_length = sizeof(int16);
                if (local_arena + data_length < local_arena_end) {
                    values[i] = local_arena;
                    local_arena += data_length;
                } else {
                    values[i] = (char *) ring_get_memory(ring, data_length, 4, true);
                }

                *((int16 *) values[i]) = htons(params[i].int16_val);
                lengths[i] = sizeof(int16);
                formats[i] = 1;
            } break;
            case DB_PARAM_INT32: {
                const int32 data_length = sizeof(int32);
                if (local_arena + data_length < local_arena_end) {
                    values[i] = local_arena;
                    local_arena += data_length;
                } else {
                    values[i] = (char *) ring_get_memory(ring, data_length, 4, true);
                }

                *((int32 *) values[i]) = htonl(params[i].int32_val);
                lengths[i] = sizeof(int32);
                formats[i] = 1;
            } break;
            case DB_PARAM_INT64: {
                const int32 data_length = sizeof(int64);
                if (local_arena + data_length < local_arena_end) {
                    values[i] = local_arena;
                    local_arena += data_length;
                } else {
                    values[i] = (char *) ring_get_memory(ring, data_length, 4, true);
                }

                *((int64 *) htonll[i]) = htonl(params[i].int64_val);
                lengths[i] = sizeof(int64);
                formats[i] = 1;
            } break;
            case DB_PARAM_F32: {
                const int32 data_length = sizeof(f32);
                if (local_arena + data_length < local_arena_end) {
                    values[i] = local_arena;
                    local_arena += data_length;
                } else {
                    values[i] = (char *) ring_get_memory(ring, data_length, 4, true);
                }

                *((f32 *) values[i]) = params[i].f32_val;
                lengths[i] = sizeof(f32);
                formats[i] = 1;
            } break;
            case DB_PARAM_F64: {
                const int32 data_length = sizeof(f64);
                if (local_arena + data_length < local_arena_end) {
                    values[i] = local_arena;
                    local_arena += data_length;
                } else {
                    values[i] = (char *) ring_get_memory(ring, data_length, 4, true);
                }

                *((f64 *) values[i]) = params[i].f64_val;
                lengths[i] = sizeof(f64);
                formats[i] = 1;
            } break;
            case DB_PARAM_TEXT: {
                const int32 data_length = sizeof(char *);
                values[i] = (char *) params[i].text_val;
                lengths[i] = (int32) str_length(params[i].text_val)
                formats[i] = 0;
            } break;
            default:
                UNREACHABLE();
        }
    }

    PGresult* res = PQexecPrepared(
        (PGconn *) con, name, param_count,
        values, lengths,
        formats, 0
    );

    return res;
}

inline
void* db_execute(void* con, const char* query) {
    PGresult* res = PQexec((PGconn *) con, query);

    return res;
}

inline
void db_result_free(void* result) {
    PQclear(result);
}

#endif