/**
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_STDLIB_HASH_MAPT_C
#define COMS_STDLIB_HASH_MAPT_C

#include "HashMapT.h"
#include "../memory/ChunkMemoryT.cpp"
#include "../hash/GeneralHash.h"

template <typename T>
inline
void hashmap_alloc(HashMapT<T>* const hm, int32 capacity, int32 max_capacity, int32 alignment = sizeof(size_t)) NO_EXCEPT
{
    // This ensures 4 byte alignment
    capacity = ALIGN_UP(capacity, 2);
    max_capacity = ALIGN_UP(max_capacity, 2);

    LOG_1("[INFO] Allocate HashMapT for %n elements", {DATA_TYPE_INT32, &capacity});
    hm->hash_function = hash_djb2;
    chunk_alloc(&hm->buf, capacity, max_capacity, alignment);
}

template <typename T>
FORCE_INLINE
void hashmap_free(HashMapT<T>* const hm) NO_EXCEPT
{
    chunk_free(&hm->buf);
}

template <typename T>
inline
void hashmap_alloc(HashMapT<T>* const hm, MemoryArena* mem, int32 capacity, int32 max_capacity, int32 alignment = sizeof(size_t)) NO_EXCEPT
{
    // This ensures 4 byte alignment
    capacity = ALIGN_UP(capacity, 2);
    max_capacity = ALIGN_UP(max_capacity, 2);

    LOG_1("[INFO] Allocate HashMapT for %n elements", {DATA_TYPE_INT32, &capacity});
    hm->hash_function = hash_djb2;
    chunk_alloc(&hm->buf, mem, capacity, max_capacity, alignment);
}

template <typename T>
inline
void hashmap_init(HashMapT<T>* const hm, int32 count, byte* const buf, int32 alignment = sizeof(size_t)) NO_EXCEPT
{
    LOG_1("[INFO] Create HashMapT for %n elements", {DATA_TYPE_INT32, &count});
    hm->hash_function = hash_djb2;
    chunk_init(&hm->buf, buf, count, alignment);

    ASSERT_MEM_ZERO(
        hm->buf.memory,
        count * sizeof(T)
            + ceil_div_pow2<(int32) sizeof(size_t) * 8>(count) * sizeof(hm->buf.free)
    );
}

template <typename T>
inline
void hashmap_init(HashMapT<T>* const hm, int32 count, BufferMemory* const buf, int32 alignment = sizeof(size_t)) NO_EXCEPT
{
    LOG_1("[INFO] Create HashMapT for %n elements", {DATA_TYPE_INT32, &count});
    hm->hash_function = hash_djb2;
    chunk_init(&hm->buf, buf, count, alignment);

    ASSERT_MEM_ZERO(
        hm->buf.memory,
        count * sizeof(T)
            + ceil_div_pow2<(int32) sizeof(size_t) * 8>(count) * sizeof(hm->buf.free)
    );
}

template <typename T>
FORCE_INLINE
void hashmap_free(HashMapT<T>* const hm, MemoryArena* mem) NO_EXCEPT
{
    chunk_free(&hm->buf, mem);
}

template <typename T>
FORCE_INLINE
int64 hashmap_size(const HashMapT<T>* const hm) NO_EXCEPT
{
    return hm->buf.capacity * sizeof(T);
}

template <typename T, typename V>
T* hashmap_insert(HashMapT<T>* const __restrict hm, const char* __restrict key, V value) NO_EXCEPT
{
    const int32 index = hm->hash_function((void *) key) % hm->buf.capacity;

    // This is either the place where we insert or the start of the chain we have to follow
    const int32 new_index = chunk_reserve_one(hm->buf.free, hm->buf.capacity, index);
    if (new_index < 0) {
        return NULL;
    }

    T* entry = (T *) chunk_element_get(&hm->buf, index);
    if (index != new_index) {
        // Find the previous chain element
        T* prev = entry;
        while (prev->next) {
            prev = (T *) chunk_element_get(&hm->buf, prev->next - 1);
        }

        prev->next = (uint16) (new_index + 1);
        entry = (T *) chunk_element_get(&hm->buf, new_index);
    }

    // Ensure key length
    str_move_to_pos(&key, -HASH_MAP_MAX_KEY_LENGTH);
    strncpy(entry->key, key, HASH_MAP_MAX_KEY_LENGTH);
    entry->key[HASH_MAP_MAX_KEY_LENGTH - 1] = '\0';

    entry->value = value;
    entry->next = 0;

    return entry;
}

template <typename T, typename V>
T* hashmap_insert(HashMapT<T>* const __restrict hm, const char* __restrict key, V* value) NO_EXCEPT
{
    const int32 index = hm->hash_function((void *) key) % hm->buf.capacity;

    // This is either the place where we insert or the start of the chain we have to follow
    const int32 new_index = chunk_reserve_one(hm->buf.free, hm->buf.capacity, index);
    if (new_index < 0) {
        return NULL;
    }

    T* entry = (T *) chunk_element_get(&hm->buf, index);
    if (index != new_index) {
        // Find the previous chain element
        T* prev = entry;
        while (prev->next) {
            prev = (T *) chunk_element_get(&hm->buf, prev->next - 1);
        }

        prev->next = (uint16) (new_index + 1);
        entry = (T *) chunk_element_get(&hm->buf, new_index);
    }

    // Ensure key length
    str_move_to_pos(&key, -HASH_MAP_MAX_KEY_LENGTH);
    strncpy(entry->key, key, HASH_MAP_MAX_KEY_LENGTH);
    entry->key[HASH_MAP_MAX_KEY_LENGTH - 1] = '\0';

    if (value) {
        memcpy(&entry->value, value, sizeof(V));
    }

    entry->next = 0;

    return entry;
}

template <typename T>
T* hashmap_reserve(HashMapT<T>* const __restrict hm, const char* __restrict key) NO_EXCEPT
{
    const int32 index = hm->hash_function((void *) key) % hm->buf.capacity;

    // This is either the place where we insert or the start of the chain we have to follow
    const int32 new_index = chunk_reserve_one(hm->buf.free, hm->buf.capacity, index);
    if (new_index < 0) {
        return NULL;
    }

    // This is either the place where we insert or the start of the chain we have to follow
    T* entry = (T *) chunk_element_get(&hm->buf, index);
    if (index != new_index) {
        // Find the previous chain element
        T* prev = entry;
        while (prev->next) {
            prev = (T*) chunk_element_get(&hm->buf, prev->next - 1);
        }

        prev->next = (uint16) (new_index + 1);
        entry = (T *) chunk_element_get(&hm->buf, new_index);
    }

    // Ensure key length
    str_move_to_pos(&key, -HASH_MAP_MAX_KEY_LENGTH);
    strncpy(entry->key, key, HASH_MAP_MAX_KEY_LENGTH);
    entry->key[HASH_MAP_MAX_KEY_LENGTH - 1] = '\0';

    entry->next = 0;

    return entry;
}

template <typename T, typename K>
T* hashmap_reserve(HashMapT<T>* const __restrict hm, K key) NO_EXCEPT
{
    // @performance Consider to force a direct conversion (void *) key
    //              This is obviously insane but it should work as long as the hash_function
    //              knows what it is doing
    const int32 index = hm->hash_function((void *) &key) % hm->buf.capacity;

    // This is either the place where we insert or the start of the chain we have to follow
    const int32 new_index = chunk_reserve_one(hm->buf.free, hm->buf.capacity, index);
    if (new_index < 0) {
        return NULL;
    }

    // This is either the place where we insert or the start of the chain we have to follow
    T* entry = (T *) chunk_element_get(&hm->buf, index);
    if (index != new_index) {
        // Find the previous chain element
        T* prev = entry;
        while (prev->next) {
            prev = (T*) chunk_element_get(&hm->buf, prev->next - 1);
        }

        prev->next = (uint16) (new_index + 1);
        entry = (T *) chunk_element_get(&hm->buf, new_index);
    }

    entry->key = (K) key;
    entry->next = 0;

    return entry;
}

template <typename T>
T* hashmap_get_reserve(HashMapT<T>* const __restrict hm, const char* __restrict key) NO_EXCEPT
{
    const int32 index = hm->hash_function((void *) key) % hm->buf.capacity;

    // Ensure key length
    str_move_to_pos(&key, -HASH_MAP_MAX_KEY_LENGTH);

    T* prev = NULL;

    // Only walk the bucket if the head slot is actually occupied - otherwise
    // we'd be dereferencing an uninitialized entry (garbage key/next).
    if (!chunk_is_free(&hm->buf, index)) {
        T* entry = (T *) chunk_element_get(&hm->buf, index);
        prev = entry;

        while (true) {
            if (strcmp(prev->key, key) == 0) {
                DEBUG_MEMORY_READ((uintptr_t) prev, sizeof(T));
                return prev;
            }

            if (!prev->next) {
                break;
            }

            prev = (T *) chunk_element_get(&hm->buf, prev->next - 1);
        };
    }

    const int32 new_index = chunk_reserve_one(hm->buf.free, hm->buf.capacity, index);
    if (new_index < 0) {
        return NULL;
    }

    T* entry = (T *) chunk_element_get(&hm->buf, new_index);

    strncpy(entry->key, key, HASH_MAP_MAX_KEY_LENGTH);
    entry->key[HASH_MAP_MAX_KEY_LENGTH - 1] = '\0';
    entry->next = 0;

    if (prev) {
        // Bucket head was occupied - append the new node to the tail of the chain
        prev->next = (uint16) (new_index + 1);
    }
    // else: index == new_index and chunk_reserve_one placed the entry
    // directly at the bucket head, nothing further to link.

    return entry;
}

template <typename T>
inline
T* hashmap_entry_get(const HashMapT<T>* const __restrict hm, const char* __restrict key) NO_EXCEPT
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

        entry = entry->next ? (T *) chunk_element_get(&hm->buf, entry->next - 1) : NULL;
    }

    return NULL;
}

template <typename T>
void hashmap_remove(HashMapT<T>* const __restrict hm, const char* __restrict key) NO_EXCEPT
{
    const int32 index = hm->hash_function((void *) key) % hm->buf.capacity;

    T* entry = (T *) chunk_element_get(&hm->buf, index);
    T* prev = NULL;
    int32 entry_index = index;

    str_move_to_pos(&key, -HASH_MAP_MAX_KEY_LENGTH);

    while (entry) {
        if (strcmp(entry->key, key) == 0) {
            if (prev == NULL && entry->next) {
                // Removing the bucket head, but the chain continues: splice the
                // successor's data into the head slot and free the successor's
                // slot instead, so the rest of the chain stays reachable.
                const int32 succ_index = entry->next - 1;
                T* succ = (T *) chunk_element_get(&hm->buf, succ_index);

                strncpy(entry->key, succ->key, HASH_MAP_MAX_KEY_LENGTH);
                memcpy(&entry->value, &succ->value, sizeof(T));
                entry->next = succ->next;
                chunk_free_element(&hm->buf, succ_index);
            } else {
                if (prev) {
                    prev->next = entry->next;
                }

                entry->key[0] = '\0';
                chunk_free_element(&hm->buf, entry_index);
            }

            return;
        }

        prev = entry;
        if (!entry->next) {
            break;
        }

        entry_index = entry->next - 1;
        entry = (T *) chunk_element_get(&hm->buf, entry_index);
    }
}

template <
    typename T, typename K, typename V,
    enable_if_t<!is_convertible_v<K, const char*>, int> = 0
>
T* hashmap_insert(HashMapT<T>* const hm, K key, V value) NO_EXCEPT
{
    const int32 index = hm->hash_function((void *) &key) % hm->buf.capacity;

    // This is either the place where we insert or the start of the chain we have to follow
    const int32 new_index = chunk_reserve_one(hm->buf.free, hm->buf.capacity, index);
    if (new_index < 0) {
        return NULL;
    }

    // This is either the place where we insert or the start of the chain we have to follow
    T* entry = (T *) chunk_element_get(&hm->buf, index);
    if (index != new_index) {
        // Find the previous chain element
        T* prev = entry;
        while (prev->next) {
            prev = (T *) chunk_element_get(&hm->buf, prev->next - 1);
        }

        prev->next = (uint16) (new_index + 1);
        entry = (T *) chunk_element_get(&hm->buf, new_index);
    }

    entry->key = key;
    entry->value = value;
    entry->next = 0;

    return entry;
}

template <
    typename T, typename K,
    enable_if_t<!is_convertible_v<K, const char*>, int> = 0
>
T* hashmap_entry_get(HashMapT<T>* const hm, K key) NO_EXCEPT
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

        entry = entry->next ? (T *) chunk_element_get(&hm->buf, entry->next - 1) : NULL;
    }

    return NULL;
}

template <typename T, typename K>
void hashmap_remove(HashMapT<T>* const hm, K key) NO_EXCEPT
{
    const int32 index = hm->hash_function((void *) &key) % hm->buf.capacity;

    T* entry = (T *) chunk_element_get(&hm->buf, index);
    T* prev = NULL;
    int32 entry_index = index;

    while (entry) {
        if (entry->key == key) {
            if (prev == NULL && entry->next) {
                const int32 succ_index = entry->next - 1;
                T* succ = (T *) chunk_element_get(&hm->buf, succ_index);

                entry->key = succ->key;
                entry->value = succ->value;
                entry->next = succ->next;

                chunk_free_element(&hm->buf, succ_index);
            } else {
                if (prev) {
                    prev->next = entry->next;
                }

                chunk_free_element(&hm->buf, entry_index);
            }

            return;
        }

        prev = entry;
        if (!entry->next) {
            break;
        }

        entry_index = entry->next - 1;
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
int64 hashmap_dump(const HashMapT<T>* const hm, byte* data, MAYBE_UNUSED int32 steps = 8) NO_EXCEPT
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
int64 hashmap_load(HashMapT<T>* const hm, const byte* data, MAYBE_UNUSED int32 steps = 8) NO_EXCEPT
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