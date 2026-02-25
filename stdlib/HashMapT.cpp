/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_STDLIB_HASH_MAPT_C
#define COMS_STDLIB_HASH_MAPT_C

#include "HashMap.cpp"
#include "../memory/ChunkMemoryT.h"

template <typename T>
FORCE_INLINE
void hashmap_alloc(HashMapT<T>* const hm, int32 capacity, int32 max_capacity, int32 alignment = 32) NO_EXCEPT
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
void hashmap_free(HashMapT<T>* const hm) NO_EXCEPT
{
    chunk_free(&hm->buf);
}

template <typename T>
FORCE_INLINE
void hashmap_alloc(HashMapT<T>* const hm, MemoryArena* mem, int32 capacity, int32 max_capacity, int32 alignment = 32) NO_EXCEPT
{
    // This ensures 4 byte alignment
    capacity = align_up(capacity, 2);
    max_capacity = align_up(max_capacity, 2);

    LOG_1("[INFO] Allocate HashMapT for %n elements", {DATA_TYPE_INT32, &capacity});
    hm->hash_function = hash_djb2;
    chunk_alloc(&hm->buf, mem, capacity, max_capacity, alignment);
}

template <typename T>
FORCE_INLINE
void hashmap_free(HashMapT<T>* const hm, MemoryArena* mem) NO_EXCEPT
{
    chunk_free(&hm->buf, mem);
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

    T* entry = (T *) chunk_get_element(&hm->buf, index);
    if (index != new_index) {
        // Find the previous chain element
        T* prev = entry;
        while (prev->next) {
            prev = (T *) chunk_get_element(&hm->buf, prev->next - 1);
        }

        prev->next = (uint16) (new_index + 1);
        entry = (T *) chunk_get_element(&hm->buf, new_index);
    }

    // Ensure key length
    str_move_to_pos(&key, -HASH_MAP_MAX_KEY_LENGTH);
    str_copy(entry->key, key, HASH_MAP_MAX_KEY_LENGTH);
    entry->key[HASH_MAP_MAX_KEY_LENGTH - 1] = '\0';

    entry->value = value;
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
    T* entry = (T *) chunk_get_element(&hm->buf, index);
    if (index != new_index) {
        // Find the previous chain element
        T* prev = entry;
        while (prev->next) {
            prev = (T*) chunk_get_element(&hm->buf, prev->next - 1);
        }

        prev->next = (uint16) (new_index + 1);
        entry = (T *) chunk_get_element(&hm->buf, new_index);
    }

    entry->value = (byte *) entry + sizeof(T);

    // Ensure key length
    str_move_to_pos(&key, -HASH_MAP_MAX_KEY_LENGTH);
    str_copy(entry->key, key, HASH_MAP_MAX_KEY_LENGTH);
    entry->key[HASH_MAP_MAX_KEY_LENGTH - 1] = '\0';

    entry->next = 0;

    return entry;
}

template <typename T>
T* hashmap_get_reserve(HashMapT<T>* const __restrict hm, const char* __restrict key) NO_EXCEPT
{
    const int32 index = hm->hash_function((void *) key) % hm->buf.capacity;

    // This is either the place where we insert or the start of the chain we have to follow
    T* entry = (T *) chunk_get_element(&hm->buf, index);

    // Ensure key length
    str_move_to_pos(&key, -HASH_MAP_MAX_KEY_LENGTH);

    T* prev = entry;
    while (true) {
        if (strcmp(prev->key, key) == 0) {
            DEBUG_MEMORY_READ((uintptr_t) prev, sizeof(T));
            return prev;
        }

        if (!prev->next) {
            break;
        }

        prev = (T *) chunk_get_element(&hm->buf, prev->next - 1);
    };

    const int32 new_index = chunk_reserve_one(&hm->buf);
    if (new_index < 0) {
        return NULL;
    }

    prev->next = (uint16) (new_index + 1);

    entry = (T *) chunk_get_element(&hm->buf, new_index);
    entry->value = (byte *) entry + sizeof(T);

    str_copy(entry->key, key, HASH_MAP_MAX_KEY_LENGTH);
    entry->key[HASH_MAP_MAX_KEY_LENGTH - 1] = '\0';

    return entry;
}

template <typename T>
inline
T* hashmap_get_entry(HashMapT<T>* const __restrict hm, const char* __restrict key) NO_EXCEPT
{
    const int32 index = hm->hash_function((void *) key) % hm->buf.capacity;
    if (chunk_is_free(&hm->buf, index)) {
        return NULL;
    }

    T* entry = (T *) chunk_get_element(&hm->buf, index);

    // Ensure key length
    str_move_to_pos(&key, -HASH_MAP_MAX_KEY_LENGTH);

    while (entry) {
        if (strcmp(entry->key, key) == 0) {
            DEBUG_MEMORY_READ((uintptr_t) entry, sizeof(T));
            return entry;
        }

        entry = entry->next ? (T *) chunk_get_element(&hm->buf, entry->next - 1) : NULL;
    }

    return NULL;
}

template <typename T>
void hashmap_remove(HashMapT<T>* const __restrict hm, const char* __restrict key) NO_EXCEPT
{
    const int32 index = hm->hash_function((void *) key) % hm->buf.capacity;

    T* entry = (T *) chunk_get_element(&hm->buf, index);
    T* prev = NULL;

    str_move_to_pos(&key, -HASH_MAP_MAX_KEY_LENGTH);

    while (entry) {
        if (strcmp(entry->key, key) == 0) {
            if (prev) {
                prev->next = entry->next;
            }

            chunk_free_element(&hm->buf, index);

            return;
        }

        prev = entry;
        entry = entry->next ? (T *) chunk_get_element(&hm->buf, entry->next - 1) : NULL;
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
    T* entry = (T *) chunk_get_element(&hm->buf, index);
    if (index != new_index) {
        // Find the previous chain element
        T* prev = entry;
        while (prev->next) {
            prev = (T *) chunk_get_element(&hm->buf, prev->next - 1);
        }

        prev->next = (uint16) (new_index + 1);
        entry = (T *) chunk_get_element(&hm->buf, new_index);
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
T* hashmap_get_entry(HashMapT<T>* const hm, K key) NO_EXCEPT
{
    const int32 index = hm->hash_function((void *) &key) % hm->buf.capacity;
    if (chunk_is_free(&hm->buf, index)) {
        return NULL;
    }

    T* entry = (T *) chunk_get_element(&hm->buf, index);

    while (entry) {
        if (entry->key == key) {
            DEBUG_MEMORY_READ((uintptr_t) entry, sizeof(T));
            return entry;
        }

        entry = entry->next ? (T *) chunk_get_element(&hm->buf, entry->next - 1) : NULL;
    }

    return NULL;
}

template <typename T, typename K>
void hashmap_remove(HashMapT<T>* const hm, K key) NO_EXCEPT
{
    const int32 index = hm->hash_function((void *) &key) % hm->buf.capacity;

    T* entry = (T *) chunk_get_element(&hm->buf, index);
    T* prev = NULL;

    while (entry) {
        if (entry->key == key) {
            if (prev) {
                prev->next = entry->next;
            }

            chunk_free_element(&hm->buf, index);

            return;
        }

        prev = entry;
        entry = entry->next ? (T *) chunk_get_element(&hm->buf, entry->next - 1) : NULL;
    }
}

template <typename T>
int64 hashmap_dump(const HashMapT<T>* const hm, byte* data, MAYBE_UNUSED int32 steps = 8) NO_EXCEPT
{
    LOG_1("[INFO] Dump HashMapT");
    const byte* const start = data;

    // Dump Chunk memory
    data += chunk_dump(&hm->buf, data);

    // @todo change endian of hashmap next "pointer" and values?

    PSEUDO_USE(steps);

    return data - start;
}

// WARNING: Requires hashmap_create first
template <typename T>
int64 hashmap_load(HashMapT<T>* const hm, const byte* data, MAYBE_UNUSED int32 steps = 8) NO_EXCEPT
{
    LOG_1("[INFO] Load HashMapT");
    const byte* const start = data;

    // Load chunk memory
    data += chunk_load(&hm->buf, data);

    // @todo change endian of hashmap next "pointer" and values?

    PSEUDO_USE(steps);

    return data - start;
}

#endif