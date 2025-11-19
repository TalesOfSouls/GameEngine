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

#include "../stdlib/Types.h"
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
void thrd_hashmap_create(ThreadedHashMap* __restrict hm, int32 count, int32 element_size, RingMemory* __restrict ring, int32 alignment = 32)
{
    hashmap_create((HashMap *) hm, count, element_size, ring, alignment);
    mutex_init(&hm->mtx, NULL);
}

// WARNING: element_size = element size + remaining HashEntry data size
inline
void thrd_hashmap_create(ThreadedHashMap* __restrict hm, int32 count, int32 element_size, BufferMemory* __restrict buf, int32 alignment = 32)
{
    hashmap_create((HashMap *) hm, count, element_size, buf, alignment);
    mutex_init(&hm->mtx, NULL);
}

// WARNING: element_size = element size + remaining HashEntry data size
inline
void thrd_hashmap_create(ThreadedHashMap* __restrict hm, int32 count, int32 element_size, byte* __restrict buf, int32 alignment = 32)
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
    mutex_lock(&hm->mtx);
    hashmap_insert((HashMap *) hm, key, value);
    mutex_unlock(&hm->mtx);
}

inline
void thrd_hashmap_insert(ThreadedHashMap* __restrict hm, const char* __restrict key, int64 value) {
    mutex_lock(&hm->mtx);
    hashmap_insert((HashMap *) hm, key, value);
    mutex_unlock(&hm->mtx);
}

inline
void thrd_hashmap_insert(ThreadedHashMap* __restrict hm, const char* __restrict key, uintptr_t value) {
    mutex_lock(&hm->mtx);
    hashmap_insert((HashMap *) hm, key, value);
    mutex_unlock(&hm->mtx);
}

inline
void thrd_hashmap_insert(ThreadedHashMap* __restrict hm, const char* __restrict key, void* __restrict value) {
    mutex_lock(&hm->mtx);
    hashmap_insert((HashMap *) hm, key, value);
    mutex_unlock(&hm->mtx);
}

inline
void thrd_hashmap_insert(ThreadedHashMap* __restrict hm, const char* __restrict key, f32 value) {
    mutex_lock(&hm->mtx);
    hashmap_insert((HashMap *) hm, key, value);
    mutex_unlock(&hm->mtx);
}

inline
void thrd_hashmap_insert(ThreadedHashMap* __restrict hm, const char* __restrict key, const char* __restrict value) {
    mutex_lock(&hm->mtx);
    hashmap_insert((HashMap *) hm, key, value);
    mutex_unlock(&hm->mtx);
}

inline
void thrd_hashmap_insert(ThreadedHashMap* __restrict hm, const char* __restrict key, byte* __restrict value) {
    mutex_lock(&hm->mtx);
    hashmap_insert((HashMap *) hm, key, value);
    mutex_unlock(&hm->mtx);
}

inline
void thrd_hashmap_get_entry(ThreadedHashMap* hm, HashEntry* entry, const char* key) {
    mutex_lock(&hm->mtx);
    HashEntry* temp = hashmap_get_entry((HashMap *) hm, key);
    memcpy(entry, temp, hm->buf.chunk_size);
    mutex_unlock(&hm->mtx);
}

inline
void thrd_hashmap_get_entry(ThreadedHashMap* hm, HashEntry* entry, const char* key, uint64 index) {
    mutex_lock(&hm->mtx);
    HashEntry* temp = hashmap_get_entry((HashMap *) hm, key, index);
    memcpy(entry, temp, hm->buf.chunk_size);
    mutex_unlock(&hm->mtx);
}

inline
void thrd_hashmap_remove(ThreadedHashMap* __restrict hm, const char* __restrict key) {
    mutex_lock(&hm->mtx);
    hashmap_remove((HashMap *) hm, key);
    mutex_unlock(&hm->mtx);
}

#endif