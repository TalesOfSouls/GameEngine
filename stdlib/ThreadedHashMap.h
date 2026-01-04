/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_STDLIB_THREADED_HASH_MAP_H
#define COMS_STDLIB_THREADED_HASH_MAP_H

#include "../stdlib/Stdlib.h"
#include "HashMap.h"

#include "../thread/Atomic.h"
#include "../thread/Semaphore.h"
#include "../thread/Thread.h"

struct ThreadedHashMap {
    void** table;
    ChunkMemory buf;

    mutex mtx;
};

// WARNING: element_size = element size + remaining HashEntry data size
inline
void thrd_hashmap_create(ThreadedHashMap* __restrict hm, int32 count, int32 element_size, RingMemory* const __restrict ring, int32 alignment = 32)
{
    hashmap_create((HashMap *) hm, count, element_size, ring, alignment);
    mutex_init(&hm->mtx, NULL);
}

// WARNING: element_size = element size + remaining HashEntry data size
inline
void thrd_hashmap_create(ThreadedHashMap* __restrict hm, int32 count, int32 element_size, BufferMemory* const __restrict buf, int32 alignment = 32)
{
    hashmap_create((HashMap *) hm, count, element_size, buf, alignment);
    mutex_init(&hm->mtx, NULL);
}

// WARNING: element_size = element size + remaining HashEntry data size
inline
void thrd_hashmap_create(ThreadedHashMap* __restrict hm, int32 count, int32 element_size, byte* const __restrict buf, int32 alignment = 32)
{
    hashmap_create((HashMap *) hm, count, element_size, buf, alignment);
    mutex_init(&hm->mtx, NULL);
}

inline
void thrd_hashmap_free(ThreadedHashMap* hm)
{
    mutex_destroy(&hm->mtx);
}

inline
void thrd_hashmap_insert(ThreadedHashMap* __restrict hm, const char* __restrict key, int32 value) {
    MutexGuard _guard(&hm->mtx);
    hashmap_insert((HashMap *) hm, key, value);
}

inline
void thrd_hashmap_insert(ThreadedHashMap* __restrict hm, const char* __restrict key, int64 value) {
    MutexGuard _guard(&hm->mtx);
    hashmap_insert((HashMap *) hm, key, value);
}

inline
void thrd_hashmap_insert(ThreadedHashMap* __restrict hm, const char* __restrict key, uintptr_t value) {
    MutexGuard _guard(&hm->mtx);
    hashmap_insert((HashMap *) hm, key, value);
}

inline
void thrd_hashmap_insert(ThreadedHashMap* __restrict hm, const char* __restrict key, void* __restrict value) {
    MutexGuard _guard(&hm->mtx);
    hashmap_insert((HashMap *) hm, key, value);
}

inline
void thrd_hashmap_insert(ThreadedHashMap* __restrict hm, const char* __restrict key, f32 value) {
    MutexGuard _guard(&hm->mtx);
    hashmap_insert((HashMap *) hm, key, value);
}

inline
void thrd_hashmap_insert(ThreadedHashMap* __restrict hm, const char* __restrict key, const char* __restrict value) {
    MutexGuard _guard(&hm->mtx);
    hashmap_insert((HashMap *) hm, key, value);
}

inline
void thrd_hashmap_insert(ThreadedHashMap* __restrict hm, const char* __restrict key, byte* __restrict value) {
    MutexGuard _guard(&hm->mtx);
    hashmap_insert((HashMap *) hm, key, value);
}

inline
void thrd_hashmap_get_entry(ThreadedHashMap* hm, HashEntry* entry, const char* key) {
    MutexGuard _guard(&hm->mtx);
    HashEntry* temp = hashmap_get_entry((HashMap *) hm, key);
    memcpy(entry, temp, hm->buf.chunk_size);
}

inline
void thrd_hashmap_get_entry(ThreadedHashMap* hm, HashEntry* entry, const char* key, uint64 index) {
    MutexGuard _guard(&hm->mtx);
    HashEntry* temp = hashmap_get_entry((HashMap *) hm, key, index);
    memcpy(entry, temp, hm->buf.chunk_size);
}

inline
void thrd_hashmap_remove(ThreadedHashMap* __restrict hm, const char* __restrict key) {
    MutexGuard _guard(&hm->mtx);
    hashmap_remove((HashMap *) hm, key);
}

#endif