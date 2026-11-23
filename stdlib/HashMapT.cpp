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
#include "HashMapHash.h"
#include "../utils/SimpleString.h"

/**
 * Bucket index = hashmap_hash_range(hash, capacity), the head of bucket i is stored in slot i (entry->head)
 * Insert pushes the new entry to the front of its chain (no chain walk)
 * New entries are preferably placed in the slot of their own bucket -> head + entry often in one cache line
 * String entries store a 16 bit hash tag -> chain walks compare 2 bytes before comparing the key
 * String keys are hashed/compared 8 bytes at a time, the length/SimpleString versions skip the length calculation
 * Remove only unlinks + frees the removed entry, other entries are never moved/copied
 */

// Tag stored in string entries (uses the low bits, the bucket index uses the high bits of the hash)
FORCE_INLINE
uint16 hashmap_hash_tag(uint32 hash) NO_EXCEPT
{
    return (uint16) hash;
}

template <typename T>
FORCE_INLINE
uint16* hashmap_head(const HashMapT<T>* const hm, uint32 hash) NO_EXCEPT
{
    return &hm->buf.memory[hashmap_hash_range(hash, (uint32) hm->buf.capacity)].head;
}

// The bucket heads are part of the entries -> the entry memory must be zeroed
template <typename T>
FORCE_INLINE
void hashmap_clear_entries(HashMapT<T>* const hm) NO_EXCEPT
{
    memset((void *) hm->buf.memory, 0, hm->buf.capacity * sizeof(T));
}

template <typename T>
inline
void hashmap_alloc(HashMapT<T>* const hm, int32 capacity, int32 max_capacity, int32 alignment = sizeof(size_t)) NO_EXCEPT
{
    // This ensures 4 byte alignment
    capacity = ALIGN_UP(capacity, 2);
    max_capacity = ALIGN_UP(max_capacity, 2);
    ASSERT_TRUE(capacity <= 0xFFFF);

    LOG_1("[INFO] Allocate HashMapT for %n elements", {DATA_TYPE_INT32, &capacity});
    chunk_alloc(&hm->buf, capacity, max_capacity, alignment);
    hashmap_clear_entries(hm);
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
    ASSERT_TRUE(capacity <= 0xFFFF);

    LOG_1("[INFO] Allocate HashMapT for %n elements", {DATA_TYPE_INT32, &capacity});
    chunk_alloc(&hm->buf, mem, capacity, max_capacity, alignment);
    hashmap_clear_entries(hm);
}

template <typename T>
inline
void hashmap_init(HashMapT<T>* const hm, int32 count, byte* const buf, int32 alignment = sizeof(size_t)) NO_EXCEPT
{
    ASSERT_TRUE(count <= 0xFFFF);
    LOG_1("[INFO] Create HashMapT for %n elements", {DATA_TYPE_INT32, &count});
    chunk_init(&hm->buf, buf, count, alignment);

    ASSERT_MEM_ZERO(
        hm->buf.memory,
        count * sizeof(T)
            + ceil_div_pow2<(int32) sizeof(size_t) * 8>(count) * sizeof(hm->buf.free)
    );

    hashmap_clear_entries(hm);
}

template <typename T>
inline
void hashmap_init(HashMapT<T>* const hm, int32 count, BufferMemory* const buf, int32 alignment = sizeof(size_t)) NO_EXCEPT
{
    ASSERT_TRUE(count <= 0xFFFF);
    LOG_1("[INFO] Create HashMapT for %n elements", {DATA_TYPE_INT32, &count});
    chunk_init(&hm->buf, buf, count, alignment);

    ASSERT_MEM_ZERO(
        hm->buf.memory,
        count * sizeof(T)
            + ceil_div_pow2<(int32) sizeof(size_t) * 8>(count) * sizeof(hm->buf.free)
    );

    hashmap_clear_entries(hm);
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

/**
 * Reserves a slot for a new entry of the bucket (preferably the slot of the bucket itself)
 * and pushes it to the front of the bucket chain
 *
 * Only next is set, the caller has to fill the key/value
 */
template <typename T>
FORCE_INLINE
T* hashmap_slot_create(HashMapT<T>* const hm, uint32 hash) NO_EXCEPT
{
    const int32 bucket = (int32) hashmap_hash_range(hash, (uint32) hm->buf.capacity);

    const int32 id = chunk_reserve_one(hm->buf.free, hm->buf.capacity, bucket);
    if (id < 0) {
        return NULL;
    }

    uint16* const head = &hm->buf.memory[bucket].head;
    T* const entry = &hm->buf.memory[id];

    // WARNING: Don't touch entry->head, it belongs to bucket[id]
    entry->next = *head;
    *head = (uint16) (id + 1);

    DEBUG_MEMORY_WRITE((uintptr_t) entry, sizeof(T));

    return entry;
}

/**
 * Finds a string entry
 *
 * @param const char*   key     Stored key part (created by hashmap_str_key)
 * @param int32         length  Length of the stored key part
 */
template <typename T>
FORCE_INLINE
T* hashmap_str_find(const HashMapT<T>* const hm, const char* key, int32 length, uint32 hash) NO_EXCEPT
{
    const uint16 tag = hashmap_hash_tag(hash);
    uint16 next = *hashmap_head(hm, hash);

    while (next) {
        T* const entry = &hm->buf.memory[next - 1];

        // The tag check filters out almost all non-matching entries
        // The compare includes the \0 of the stored key -> no partial matches
        if (entry->tag == tag && hashmap_str_equals(entry->key, key, length)) {
            DEBUG_MEMORY_READ((uintptr_t) entry, sizeof(T));
            return entry;
        }

        next = entry->next;
    }

    return NULL;
}

// The key doesn't have to be \0 terminated
template <typename T>
FORCE_INLINE
T* hashmap_str_slot_create(HashMapT<T>* const hm, const char* key, int32 length, uint32 hash) NO_EXCEPT
{
    T* const entry = hashmap_slot_create(hm, hash);
    if (!entry) {
        return NULL;
    }

    memcpy(entry->key, key, length);
    entry->key[length] = '\0';
    entry->tag = hashmap_hash_tag(hash);

    return entry;
}

// Removes the entry from its bucket chain and frees the slot, nothing else is moved
template <typename T>
FORCE_INLINE
void hashmap_unlink_free(HashMapT<T>* const hm, uint32 hash, T* const entry) NO_EXCEPT
{
    const uint16 link = (uint16) (entry - hm->buf.memory + 1);

    uint16* next = hashmap_head(hm, hash);
    while (*next) {
        if (*next == link) {
            *next = entry->next;
            entry->next = 0;

            chunk_free_element(&hm->buf, (uint32) (link - 1));

            return;
        }

        next = &hm->buf.memory[*next - 1].next;
    }
}

/**
 * String key functions
 *
 * Every function exists in 3 versions:
 *      const char* key                 \0 terminated key (the length is measured)
 *      const char* key, int32 length   key with known length (doesn't have to be \0 terminated)
 *      SimpleString<C> key             same as the length version
 * The length versions skip the length calculation -> prefer them if the length is already known
 */

template <typename T, typename V>
T* hashmap_insert(HashMapT<T>* const hm, const char* key, int32 length, V value) NO_EXCEPT
{
    hashmap_str_key(&key, &length);
    const uint32 hash = hashmap_str_hash(key, length);

    T* const entry = hashmap_str_slot_create(hm, key, length, hash);
    if (!entry) {
        return NULL;
    }

    entry->value = value;

    return entry;
}

template <typename T, typename V>
T* hashmap_insert(HashMapT<T>* const hm, const char* key, int32 length, V* value) NO_EXCEPT
{
    hashmap_str_key(&key, &length);
    const uint32 hash = hashmap_str_hash(key, length);

    T* const entry = hashmap_str_slot_create(hm, key, length, hash);
    if (!entry) {
        return NULL;
    }

    if (value) {
        memcpy(&entry->value, value, sizeof(V));
    } else {
        memset(&entry->value, 0, sizeof(entry->value));
    }

    return entry;
}

template <typename T, typename V>
FORCE_INLINE
T* hashmap_insert(HashMapT<T>* const hm, const char* key, V value) NO_EXCEPT
{
    return hashmap_insert(hm, key, hashmap_str_length(key), value);
}

template <typename T, typename V>
FORCE_INLINE
T* hashmap_insert(HashMapT<T>* const hm, const char* key, V* value) NO_EXCEPT
{
    return hashmap_insert(hm, key, hashmap_str_length(key), value);
}

template <typename T, typename C, typename V>
FORCE_INLINE
T* hashmap_insert(HashMapT<T>* const hm, SimpleString<C> key, V value) NO_EXCEPT
{
    return hashmap_insert(hm, (const char *) key.str, key.length, value);
}

// The value is zero initialized (the slot may contain data of a previously removed element)
template <typename T>
T* hashmap_reserve(HashMapT<T>* const hm, const char* key, int32 length) NO_EXCEPT
{
    hashmap_str_key(&key, &length);
    const uint32 hash = hashmap_str_hash(key, length);

    T* const entry = hashmap_str_slot_create(hm, key, length, hash);
    if (!entry) {
        return NULL;
    }

    memset(&entry->value, 0, sizeof(entry->value));

    return entry;
}

template <typename T>
FORCE_INLINE
T* hashmap_reserve(HashMapT<T>* const hm, const char* key) NO_EXCEPT
{
    return hashmap_reserve(hm, key, hashmap_str_length(key));
}

template <typename T, typename C>
FORCE_INLINE
T* hashmap_reserve(HashMapT<T>* const hm, SimpleString<C> key) NO_EXCEPT
{
    return hashmap_reserve(hm, (const char *) key.str, key.length);
}

// The value is zero initialized (the slot may contain data of a previously removed element)
template <
    typename T, typename K,
    enable_if_t<!is_convertible_v<K, const char*>, int> = 0
>
T* hashmap_reserve(HashMapT<T>* const hm, K key) NO_EXCEPT
{
    T* const entry = hashmap_slot_create(hm, hashmap_int_hash(key));
    if (!entry) {
        return NULL;
    }

    entry->key = (K) key;
    memset(&entry->value, 0, sizeof(entry->value));

    return entry;
}

template <typename T>
T* hashmap_get_reserve(HashMapT<T>* const hm, const char* key, int32 length) NO_EXCEPT
{
    hashmap_str_key(&key, &length);
    const uint32 hash = hashmap_str_hash(key, length);

    T* const entry = hashmap_str_find(hm, key, length, hash);
    if (entry) {
        return entry;
    }

    T* const new_entry = hashmap_str_slot_create(hm, key, length, hash);
    if (!new_entry) {
        return NULL;
    }

    memset(&new_entry->value, 0, sizeof(new_entry->value));

    return new_entry;
}

template <typename T>
FORCE_INLINE
T* hashmap_get_reserve(HashMapT<T>* const hm, const char* key) NO_EXCEPT
{
    return hashmap_get_reserve(hm, key, hashmap_str_length(key));
}

template <typename T, typename C>
FORCE_INLINE
T* hashmap_get_reserve(HashMapT<T>* const hm, SimpleString<C> key) NO_EXCEPT
{
    return hashmap_get_reserve(hm, (const char *) key.str, key.length);
}

template <typename T>
inline
T* hashmap_entry_get(const HashMapT<T>* const hm, const char* key, int32 length) NO_EXCEPT
{
    hashmap_str_key(&key, &length);

    return hashmap_str_find(hm, key, length, hashmap_str_hash(key, length));
}

template <typename T>
FORCE_INLINE
T* hashmap_entry_get(const HashMapT<T>* const hm, const char* key) NO_EXCEPT
{
    return hashmap_entry_get(hm, key, hashmap_str_length(key));
}

// Same hm type as the int key version -> partial ordering picks this version for SimpleString keys
template <typename T, typename C>
FORCE_INLINE
T* hashmap_entry_get(HashMapT<T>* const hm, SimpleString<C> key) NO_EXCEPT
{
    return hashmap_entry_get((const HashMapT<T>*) hm, (const char *) key.str, key.length);
}

template <typename T>
void hashmap_remove(HashMapT<T>* const hm, const char* key, int32 length) NO_EXCEPT
{
    hashmap_str_key(&key, &length);
    const uint32 hash = hashmap_str_hash(key, length);

    T* const entry = hashmap_str_find(hm, key, length, hash);
    if (entry) {
        hashmap_unlink_free(hm, hash, entry);
    }
}

template <typename T>
FORCE_INLINE
void hashmap_remove(HashMapT<T>* const hm, const char* key) NO_EXCEPT
{
    hashmap_remove(hm, key, hashmap_str_length(key));
}

template <typename T, typename C>
FORCE_INLINE
void hashmap_remove(HashMapT<T>* const hm, SimpleString<C> key) NO_EXCEPT
{
    hashmap_remove(hm, (const char *) key.str, key.length);
}

template <
    typename T, typename K, typename V,
    enable_if_t<!is_convertible_v<K, const char*>, int> = 0
>
T* hashmap_insert(HashMapT<T>* const hm, K key, V value) NO_EXCEPT
{
    T* const entry = hashmap_slot_create(hm, hashmap_int_hash(key));
    if (!entry) {
        return NULL;
    }

    entry->key = key;
    entry->value = value;

    return entry;
}

template <typename T, typename K>
FORCE_INLINE
T* hashmap_int_find(const HashMapT<T>* const hm, const K& key, uint32 hash) NO_EXCEPT
{
    uint16 next = *hashmap_head(hm, hash);

    while (next) {
        T* const entry = &hm->buf.memory[next - 1];
        if (entry->key == key) {
            DEBUG_MEMORY_READ((uintptr_t) entry, sizeof(T));
            return entry;
        }

        next = entry->next;
    }

    return NULL;
}

template <
    typename T, typename K,
    enable_if_t<!is_convertible_v<K, const char*>, int> = 0
>
T* hashmap_entry_get(HashMapT<T>* const hm, K key) NO_EXCEPT
{
    return hashmap_int_find(hm, key, hashmap_int_hash(key));
}

template <
    typename T, typename K,
    enable_if_t<!is_convertible_v<K, const char*>, int> = 0
>
void hashmap_remove(HashMapT<T>* const hm, K key) NO_EXCEPT
{
    const uint32 hash = hashmap_int_hash(key);

    T* const entry = hashmap_int_find(hm, key, hash);
    if (entry) {
        hashmap_unlink_free(hm, hash, entry);
    }
}

/**
 * Binary representation:
 *
 * 00 01 02 03 = capacity
 * 04 05 06 07 = last_pos
 * 08 09 0A 0B = free_offset
 * 0C .. .. .. = hash map data (incl. the bucket heads)
 */
template <typename T>
int64 hashmap_dump(const HashMapT<T>* const hm, byte* data, MAYBE_UNUSED int32 steps = 8) NO_EXCEPT
{
    LOG_1("[INFO] Dump HashMapT");
    const byte* const start = data;

    // Dump Chunk memory
    data += chunk_dump(&hm->buf, data);

    // @bug change endian of hashmap next/head/tag and values?
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

    // Load chunk memory (incl. the bucket heads)
    data += chunk_load(&hm->buf, data);

    // @bug change endian of hashmap next/head/tag and values?
    // Since we just dump the chunk memory we may have different endian between the saving and the loading system
    // An additional problem is that the elements can have different types and therefore memory layout

    PSEUDO_USE(steps);

    return data - start;
}

#endif
