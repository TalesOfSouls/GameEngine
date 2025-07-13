/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_DATABASE_POOL_H
#define COMS_DATABASE_POOL_H

#include "../stdlib/Types.h"
#include "../log/Log.h"
#include "../log/Stats.h"
#include "../log/PerformanceProfiler.h"
#include "../compiler/CompilerUtils.h"
#include "../thread/Atomic.h"
#include "../system/Allocator.h"
#include "DatabaseConnection.h"
#include "Database.h"
#include "../memory/ThreadedChunkMemory.h"

struct DatabasePool {
    // How many connections does this pool support?
    uint8 count;
    int16 pos = -1;

    DatabaseConnection* connections;

    // Bitfield showing which connections are free and which are in use
    alignas(8) atomic_64 uint64* free;
};

void db_pool_alloc(DatabasePool* pool, uint8 count) {
    ASSERT_SIMPLE(count);
    PROFILE(PROFILE_DB_POOL_ALLOC, NULL, false, true);
    LOG_1("[INFO] Allocating DatabasePool for %d connections", {{LOG_DATA_BYTE, &count}});

    uint64 size = count * sizeof(DatabaseConnection)
        + sizeof(uint64) * CEIL_DIV(count, 64) // free
        + 64 * 2; // overhead for alignment

    pool->connections = (DatabaseConnection *) platform_alloc_aligned(size, 64);
    pool->free = (uint64 *) ROUND_TO_NEAREST((uintptr_t) (pool->connections + count * sizeof(DatabaseConnection)), 64);
    pool->count = count;
}

void db_pool_add(DatabasePool* __restrict pool, DatabaseConnection* __restrict db) noexcept {
    db->id = ++pool->pos;
    memcpy(&pool->connections[pool->pos], db, sizeof(DatabaseConnection));
}

void db_pool_free(DatabasePool* pool) {
    LOG_1("[INFO] Freeing DatabasePool");

    for (int32 i = 0; i < pool->count; ++i) {
        db_close(&pool->connections[i]);
    }

    platform_aligned_free((void **) &pool->connections);
    pool->free = NULL;
    pool->count = 0;
}

// Returns free database connection or null if none could be found
// @todo implement db_pool_get_wait(pool, waittime)
inline
const DatabaseConnection* db_pool_get(DatabasePool* pool) noexcept {
    int32 id = thrd_chunk_get_unset(pool->free, (pool->count - 1) / 64, 0);

    return id >= 0 ? &pool->connections[id] : NULL;
}

// releases the database connection for use
FORCE_INLINE
void db_pool_release(DatabasePool* pool, int32 id) noexcept {
    thrd_chunk_set_unset(id, pool->free);
}

#endif