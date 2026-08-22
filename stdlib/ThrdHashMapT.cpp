/**
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_STDLIB_THRD_HASH_MAPT_C
#define COMS_STDLIB_THRD_HASH_MAPT_C

#include "HashMapT.cpp"
#include "ThrdHashMapT.h"
#include "../memory/ThrdChunkMemoryT.cpp"

template <typename T>
inline
void hashmap_alloc(ThrdHashMapT<T>* const hm, int32 capacity, int32 max_capacity, int32 alignment = sizeof(size_t)) NO_EXCEPT
{
    // This ensures 4 byte alignment
    capacity = align_up(capacity, 2);
    max_capacity = align_up(max_capacity, 2);

    LOG_1("[INFO] Allocate HashMapT for %n elements", {DATA_TYPE_INT32, &capacity});
    hm->hash_function = hash_djb2;
    chunk_alloc(&hm->buf, capacity, max_capacity, alignment);
}

template <typename T>
FORCE_INLINE
void hashmap_free(ThrdHashMapT<T>* const hm) NO_EXCEPT
{
    chunk_free(&hm->buf);
}

template <typename T>
inline
void hashmap_alloc(ThrdHashMapT<T>* const hm, MemoryArena* mem, int32 capacity, int32 max_capacity, int32 alignment = sizeof(size_t)) NO_EXCEPT
{
    // This ensures 4 byte alignment
    capacity = align_up(capacity, 2);
    max_capacity = align_up(max_capacity, 2);

    LOG_1("[INFO] Allocate HashMapT for %n elements", {DATA_TYPE_INT32, &capacity});
    hm->hash_function = hash_djb2;
    chunk_alloc(&hm->buf, mem, capacity, max_capacity, alignment);
}

template <typename T>
inline
void hashmap_init(ThrdHashMapT<T>* const hm, int32 count, byte* const buf, int32 alignment = sizeof(size_t)) NO_EXCEPT
{
    LOG_1("[INFO] Create HashMapT for %n elements", {DATA_TYPE_INT32, &count});
    hm->hash_function = hash_djb2;
    chunk_init(&hm->buf, buf, count, alignment);

    ASSERT_MEM_ZERO(
        hm->buf.memory,
        count * sizeof(T)
            + ceil_div(count, (int32) sizeof(size_t) * 8) * sizeof(hm->buf.free)
    );
}

template <typename T>
inline
void hashmap_init(ThrdHashMapT<T>* const hm, int32 count, BufferMemory* const buf, int32 alignment = sizeof(size_t)) NO_EXCEPT
{
    LOG_1("[INFO] Create HashMapT for %n elements", {DATA_TYPE_INT32, &count});
    hm->hash_function = hash_djb2;
    chunk_init(&hm->buf, buf, count, alignment);

    ASSERT_MEM_ZERO(
        hm->buf.memory,
        count * sizeof(T)
            + ceil_div(count, (int32) sizeof(size_t) * 8) * sizeof(hm->buf.free)
    );
}

template <typename T>
FORCE_INLINE
void hashmap_free(ThrdHashMapT<T>* const hm, MemoryArena* mem) NO_EXCEPT
{
    chunk_free(&hm->buf, mem);
}

template <typename T>
FORCE_INLINE
int64 hashmap_size(const ThrdHashMapT<T>* const hm) NO_EXCEPT
{
    return hm->buf.capacity * sizeof(T);
}

template <typename T, typename V>
T* hashmap_insert(ThrdHashMapT<T>* const __restrict hm, const char* __restrict key, V value) NO_EXCEPT
{
    const int32 index = hm->hash_function((void *) key) % hm->buf.capacity;

    // This is either the place where we insert or the start of the chain we have to follow
    const int32 new_index = chunk_reserve_one(hm->buf.free, hm->buf.capacity, index);
    if (new_index < 0) {
        return NULL;
    }

    T* entry = (T *) chunk_element_get(&hm->buf, new_index);

    // Ensure key length
    str_move_to_pos(&key, -HASH_MAP_MAX_KEY_LENGTH);
    strncpy(entry->key, key, HASH_MAP_MAX_KEY_LENGTH);
    entry->key[HASH_MAP_MAX_KEY_LENGTH - 1] = '\0';

    entry->value = value;
    entry->next.store(0);

    if (index != new_index) {
        // Find the previous chain element
        T* prev = (T *) chunk_element_get(&hm->buf, index);

        ATOMIC_EXCHANGE_RETRY:
        uint16 next = prev->next.load();
        while (next) {
            prev = (T *) chunk_element_get(&hm->buf, next - 1);
            next = prev->next.load();
        }

        uint16 expected = 0;
        if(!prev->next.compare_exchange_strong(expected, (uint16) (new_index + 1))) {
            goto ATOMIC_EXCHANGE_RETRY;
        }
    }

    chunk_mark_complete(&hm->buf, new_index);

    return entry;
}

template <typename T, typename V>
T* hashmap_insert(ThrdHashMapT<T>* const __restrict hm, const char* __restrict key, V* value) NO_EXCEPT
{
    const int32 index = hm->hash_function((void *) key) % hm->buf.capacity;

    // This is either the place where we insert or the start of the chain we have to follow
    const int32 new_index = chunk_reserve_one(hm->buf.free, hm->buf.capacity, index);
    if (new_index < 0) {
        return NULL;
    }

    T* entry = (T *) chunk_element_get(&hm->buf, new_index);

    // Ensure key length
    str_move_to_pos(&key, -HASH_MAP_MAX_KEY_LENGTH);
    strncpy(entry->key, key, HASH_MAP_MAX_KEY_LENGTH);
    entry->key[HASH_MAP_MAX_KEY_LENGTH - 1] = '\0';

    if (value) {
        memcpy(&entry->value, value, sizeof(V));
    }

    entry->next.store(0);

    if (index != new_index) {
        // Find the previous chain element
        T* prev = (T *) chunk_element_get(&hm->buf, index);

        ATOMIC_EXCHANGE_RETRY:
        uint16 next = prev->next.load();
        while (next) {
            prev = (T *) chunk_element_get(&hm->buf, next - 1);
            next = prev->next.load();
        }

        uint16 expected = 0;
        if(!prev->next.compare_exchange_strong(expected, (uint16) (new_index + 1))) {
            goto ATOMIC_EXCHANGE_RETRY;
        }
    }

    chunk_mark_complete(&hm->buf, new_index);

    return entry;
}

template <typename T>
T* hashmap_reserve(ThrdHashMapT<T>* const __restrict hm, const char* __restrict key) NO_EXCEPT
{
    const int32 index = hm->hash_function((void *) key) % hm->buf.capacity;

    // This is either the place where we insert or the start of the chain we have to follow
    const int32 new_index = chunk_reserve_one(hm->buf.free, hm->buf.capacity, index);
    if (new_index < 0) {
        return NULL;
    }

    // This is either the place where we insert or the start of the chain we have to follow
    T* entry = (T *) chunk_element_get(&hm->buf, new_index);

    // Ensure key length
    str_move_to_pos(&key, -HASH_MAP_MAX_KEY_LENGTH);
    strncpy(entry->key, key, HASH_MAP_MAX_KEY_LENGTH);
    entry->key[HASH_MAP_MAX_KEY_LENGTH - 1] = '\0';

    entry->next.store(0);

    if (index != new_index) {
        // Find the previous chain element
        T* prev = (T *) chunk_element_get(&hm->buf, index);

        ATOMIC_EXCHANGE_RETRY:
        uint16 next = prev->next.load();
        while (next) {
            prev = (T*) chunk_element_get(&hm->buf, next - 1);
            next = prev->next.load();
        }

        uint16 expected = 0;
        if(!prev->next.compare_exchange_strong(expected, (uint16) (new_index + 1))) {
            goto ATOMIC_EXCHANGE_RETRY;
        }
    }

    return entry;
}

template <typename T>
T* hashmap_get_reserve(ThrdHashMapT<T>* const __restrict hm, const char* __restrict key) NO_EXCEPT
{
    const int32 index = hm->hash_function((void *) key) % hm->buf.capacity;

    // This is either the place where we insert or the start of the chain we have to follow
    T* entry = (T *) chunk_element_get(&hm->buf, index);

    // Ensure key length
    str_move_to_pos(&key, -HASH_MAP_MAX_KEY_LENGTH);

    if (!chunk_is_free(&hm-buf, index)) {
        T* entry = (T *) chunk_element_get(&hm->buf, index);

        while (true) {
            if (strcmp(entry->key, key) == 0) {
                DEBUG_MEMORY_READ((uintptr_t) entry, sizeof(T));
                return entry;
            }

            const uint16 next = entry->next.load();
            if (!next) {
                break;
            }

            entry = (T *) chunk_element_get(&hm->buf, next - 1);
        };
    }

    return hashmap_reserve(hm, key);
}

template <typename T>
inline
T* hashmap_entry_get(const ThrdHashMapT<T>* const __restrict hm, const char* __restrict key) NO_EXCEPT
{
    const int32 index = hm->hash_function((void *) key) % hm->buf.capacity;
    if (chunk_is_free(&hm->buf, index)) {
        return NULL;
    }

    T* entry = (T *) chunk_element_get(&hm->buf, index);

    // Ensure key length
    str_move_to_pos(&key, -HASH_MAP_MAX_KEY_LENGTH);

    while (entry) {
        if (strcmp(entry->key, key) == 0) {
            DEBUG_MEMORY_READ((uintptr_t) entry, sizeof(T));
            return entry;
        }

        const uint16 next = entry->next.load();
        entry = next ? (T *) chunk_element_get(&hm->buf, next - 1) : NULL;
    }

    return NULL;
}

template <typename T>
void hashmap_remove(ThrdHashMapT<T>* const __restrict hm, const char* __restrict key) NO_EXCEPT
{
    const int32 index = hm->hash_function((void *) key) % hm->buf.capacity;

    str_move_to_pos(&key, -HASH_MAP_MAX_KEY_LENGTH);

    T* entry = (T *) chunk_element_get(&hm->buf, index);
    T* prev = NULL;
    int32 entry_index = index;

    while (entry) {
        if (strcmp(entry->key, key) == 0) {
            const uint16 next = entry->next.load();

            if (!prev && next) {
                const int32 succ_index = next - 1;
                T* succ = (T *) chunk_element_get(&hm->buf, succ_index);

                memcpy(entry->key, succ->key, HASH_MAP_MAX_KEY_LENGTH);
                entry->value = succ->value;
                entry->next.store(succ->next.load());

                chunk_free_element(&hm->buf, succ_index);
            } else {
                if (prev) {
                    prev->next.store(next);
                }

                chunk_free_element(&hm->buf, entry_index);
            }

            return;
        }

        prev = entry;
        const uint16 next = entry->next.load();
        if (!next) {
            break;
        }

        entry_index = next - 1;
        entry = (T *) chunk_element_get(&hm->buf, entry_index);
    }
}

template <
    typename T, typename K, typename V,
    enable_if_t<!is_convertible_v<K, const char*>, int> = 0
>
T* hashmap_insert(ThrdHashMapT<T>* const hm, K key, V value) NO_EXCEPT
{
    const int32 index = hm->hash_function((void *) &key) % hm->buf.capacity;

    const int32 new_index = chunk_reserve_one(hm->buf.free, hm->buf.capacity, index);
    if (new_index < 0) {
        return NULL;
    }

    T* entry = (T *) chunk_element_get(&hm->buf, new_index);

    entry->key = key;
    entry->value = value;

    // Initialize fully before publishing into the chain (see note above).
    entry->next.store(0);

    if (index != new_index) {
        T* prev = (T *) chunk_element_get(&hm->buf, index);

        ATOMIC_EXCHANGE_RETRY:
        uint16 next = prev->next.load();
        while (next) {
            prev = (T *) chunk_element_get(&hm->buf, next - 1);
            next = prev->next.load();
        }

        uint16 expected = 0;
        if (!prev->next.compare_exchange_strong(expected, (uint16) (new_index + 1))) {
            goto ATOMIC_EXCHANGE_RETRY;
        }
    }

    chunk_mark_complete(&hm->buf, new_index);

    return entry;
}

template <
    typename T, typename K,
    enable_if_t<!is_convertible_v<K, const char*>, int> = 0
>
T* hashmap_entry_get(ThrdHashMapT<T>* const hm, K key) NO_EXCEPT
{
    const int32 index = hm->hash_function((void *) &key) % hm->buf.capacity;
    if (chunk_is_free(&hm->buf, index)) {
        return NULL;
    }

    T* entry = (T *) chunk_element_get(&hm->buf, index);

    while (entry) {
        if (entry->key == key) {
            DEBUG_MEMORY_READ((uintptr_t) entry, sizeof(T));
            return entry;
        }

        const uint16 next = entry->next.load();
        entry = next ? (T *) chunk_element_get(&hm->buf, next - 1) : NULL;
    }

    return NULL;
}

template <typename T, typename K>
void hashmap_remove(ThrdHashMapT<T>* const hm, K key) NO_EXCEPT
{
    const int32 index = hm->hash_function((void *) &key) % hm->buf.capacity;

    T* entry = (T *) chunk_element_get(&hm->buf, index);
    T* prev = NULL;
    int32 entry_index = index;

    while (entry) {
        if (entry->key == key) {
            const uint16 next = entry->next.load();

            if (prev == NULL && next) {
                const int32 succ_index = next - 1;
                T* succ = (T *) chunk_element_get(&hm->buf, succ_index);

                entry->key = succ->key;
                entry->value = succ->value;
                entry->next.store(succ->next.load());

                chunk_free_element(&hm->buf, succ_index);
            } else {
                if (prev) {
                    prev->next.store(next);
                }

                chunk_free_element(&hm->buf, entry_index);
            }

            return;
        }

        prev = entry;
        const uint16 next = entry->next.load();
        if (!next) {
            break;
        }

        entry_index = next - 1;
        entry = (T *) chunk_element_get(&hm->buf, entry_index);
    }
}

/**
 * Binary representation:
 *
 * 00 01 02 03 = capacity
 * 04 05 06 07 = last_pos
 * 08 09 0A 0B = free_offset
 * 0C .. .. .. = hash map data
 */
template <typename T>
int64 hashmap_dump(const ThrdHashMapT<T>* const hm, byte* data, MAYBE_UNUSED int32 steps = 8) NO_EXCEPT
{
    LOG_1("[INFO] Dump HashMapT");
    const byte* const start = data;

    // Dump Chunk memory
    data += chunk_dump(&hm->buf, data);

    // @bug change endian of hashmap next "pointer" and values?
    // Since we just dump the chunk memory we may have different endian between the saving and the loading system
    // An additional problem is that the elements can have different types and therefore memory layout

    PSEUDO_USE(steps);

    return data - start;
}

// WARNING: Requires hashmap_init first
template <typename T>
int64 hashmap_load(ThrdHashMapT<T>* const hm, const byte* data, MAYBE_UNUSED int32 steps = 8) NO_EXCEPT
{
    LOG_1("[INFO] Load HashMapT");
    const byte* const start = data;

    // Load chunk memory
    data += chunk_load(&hm->buf, data);

    // @bug change endian of hashmap next "pointer" and values?
    // Since we just dump the chunk memory we may have different endian between the saving and the loading system
    // An additional problem is that the elements can have different types and therefore memory layout

    PSEUDO_USE(steps);

    return data - start;
}

#endif