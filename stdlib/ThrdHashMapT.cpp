/**
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_STDLIB_THRD_HASH_MAPT_C
#define COMS_STDLIB_THRD_HASH_MAPT_C

#include "ThrdHashMapT.h"
#include "../memory/ThrdChunkMemoryT.cpp"
#include "../hash/GeneralHash.h"
#include "HashMapHash.h"

/**
 * Implementation notes
 *
 *  - Entries live in a ThrdChunkMemoryT slot pool and never move (stable pointers)
 *  - A separate power of 2 bucket array holds the chain heads -> index = hash & (bucket_count - 1)
 *  - Insert pushes the new entry to the front of its chain with a single CAS (no chain walk)
 *  - Every string entry stores its hash -> chain walks compare 4 bytes before comparing the key
 *  - The string key is hashed and measured in one pass (no strlen, no hash function pointer call)
 *  - Remove only unlinks + frees the removed entry, other entries are never moved/copied
 */

template <typename V>
FORCE_INLINE
uint32 hashmap_entry_hash(const ThrdHashEntryStrT<V>* const entry) NO_EXCEPT
{
    return entry->hash;
}

template <typename K, typename V>
FORCE_INLINE
uint32 hashmap_entry_hash(const ThrdHashEntryT<K, V>* const entry) NO_EXCEPT
{
    return hashmap_int_hash(entry->key);
}

// Power of 2 bucket count with a max load factor of 0.8
FORCE_INLINE
int32 hashmap_bucket_count(int32 capacity) NO_EXCEPT
{
    int32 count = 16;
    while (count < capacity + capacity / 4) {
        count <<= 1;
    }

    return count;
}

template <typename T>
FORCE_INLINE
atomic<uint16>* hashmap_bucket(const ThrdHashMapT<T>* const hm, uint32 hash) NO_EXCEPT
{
    return &hm->buckets[hash & (uint32) (hm->bucket_count - 1)];
}

/**
 * Memory required by hashmap_init(hm, count, byte* buf, alignment)
 */
template <typename T>
FORCE_INLINE
size_t hashmap_required_size(int32 count, int32 alignment = sizeof(size_t)) NO_EXCEPT
{
    const size_t array_count = ceil_div_pow2<(int32) (sizeof(size_t) * 8)>(count);

    return alignment
        + count * sizeof(T)
        + 2 * sizeof(size_t) * array_count
        + sizeof(size_t) * 2
        + ASSUMED_CACHE_LINE_SIZE
        + hashmap_bucket_count(count) * sizeof(uint16);
}

template <typename T>
inline
void hashmap_alloc(ThrdHashMapT<T>* const hm, int32 capacity, int32 max_capacity, int32 alignment = sizeof(size_t)) NO_EXCEPT
{
    // This ensures 4 byte alignment
    capacity = ALIGN_UP(capacity, 2);
    max_capacity = ALIGN_UP(max_capacity, 2);
    ASSERT_TRUE(capacity <= 0xFFFF);

    LOG_1("[INFO] Allocate HashMapT for %n elements", {DATA_TYPE_INT32, &capacity});
    chunk_alloc(&hm->buf, capacity, max_capacity, alignment);

    hm->bucket_count = hashmap_bucket_count(capacity);
    const size_t bucket_size = hm->bucket_count * sizeof(uint16);
    hm->buckets = (atomic<uint16> *) platform_alloc_aligned(bucket_size, bucket_size, ASSUMED_CACHE_LINE_SIZE);
    memset((void *) hm->buckets, 0, bucket_size);
}

template <typename T>
FORCE_INLINE
void hashmap_free(ThrdHashMapT<T>* const hm) NO_EXCEPT
{
    chunk_free(&hm->buf);
    platform_aligned_free((void **) &hm->buckets);
    hm->buckets = NULL;
    hm->bucket_count = 0;
}

template <typename T>
inline
void hashmap_alloc(ThrdHashMapT<T>* const hm, MemoryArena* mem, int32 capacity, int32 max_capacity, int32 alignment = sizeof(size_t)) NO_EXCEPT
{
    // This ensures 4 byte alignment
    capacity = ALIGN_UP(capacity, 2);
    max_capacity = ALIGN_UP(max_capacity, 2);
    ASSERT_TRUE(capacity <= 0xFFFF);

    LOG_1("[INFO] Allocate HashMapT for %n elements", {DATA_TYPE_INT32, &capacity});
    chunk_alloc(&hm->buf, mem, capacity, max_capacity, alignment);

    hm->bucket_count = hashmap_bucket_count(capacity);
    const size_t bucket_size = hm->bucket_count * sizeof(uint16);
    MemoryArena* arena = mem_arena_add(mem, bucket_size, bucket_size, ASSUMED_CACHE_LINE_SIZE);
    hm->buckets = (atomic<uint16> *) arena->memory;
    memset((void *) hm->buckets, 0, bucket_size);
}

// WARNING: buf must be at least hashmap_required_size<T>(count, alignment) large
template <typename T>
inline
void hashmap_init(ThrdHashMapT<T>* const hm, int32 count, byte* const buf, int32 alignment = sizeof(size_t)) NO_EXCEPT
{
    ASSERT_TRUE(count <= 0xFFFF);
    LOG_1("[INFO] Create HashMapT for %n elements", {DATA_TYPE_INT32, &count});
    chunk_init(&hm->buf, buf, count, alignment);

    ASSERT_MEM_ZERO(
        hm->buf.memory,
        count * sizeof(T)
            + ceil_div_pow2<(int32) sizeof(size_t) * 8>(count) * sizeof(hm->buf.free)
    );

    // The bucket array is placed directly behind the chunk memory
    const size_t array_count = ceil_div_pow2<(int32) (sizeof(size_t) * 8)>(count);
    hm->bucket_count = hashmap_bucket_count(count);
    hm->buckets = (atomic<uint16> *) ALIGN_UP(
        (uintptr_t) (hm->buf.completeness + array_count),
        (uintptr_t) ASSUMED_CACHE_LINE_SIZE
    );
    memset((void *) hm->buckets, 0, hm->bucket_count * sizeof(uint16));
}

template <typename T>
inline
void hashmap_init(ThrdHashMapT<T>* const hm, int32 count, BufferMemory* const buf, int32 alignment = sizeof(size_t)) NO_EXCEPT
{
    ASSERT_TRUE(count <= 0xFFFF);
    LOG_1("[INFO] Create HashMapT for %n elements", {DATA_TYPE_INT32, &count});
    chunk_init(&hm->buf, buf, count, alignment);

    ASSERT_MEM_ZERO(
        hm->buf.memory,
        count * sizeof(T)
            + ceil_div_pow2<(int32) sizeof(size_t) * 8>(count) * sizeof(hm->buf.free)
    );

    hm->bucket_count = hashmap_bucket_count(count);
    hm->buckets = (atomic<uint16> *) memory_get(buf, hm->bucket_count * sizeof(uint16), ASSUMED_CACHE_LINE_SIZE);
    memset((void *) hm->buckets, 0, hm->bucket_count * sizeof(uint16));
}

template <typename T>
FORCE_INLINE
void hashmap_free(ThrdHashMapT<T>* const hm, MemoryArena* mem) NO_EXCEPT
{
    chunk_free(&hm->buf, mem);
    mem_arena_remove(mem, hm->buckets);
    hm->buckets = NULL;
    hm->bucket_count = 0;
}

template <typename T>
FORCE_INLINE
int64 hashmap_size(const ThrdHashMapT<T>* const hm) NO_EXCEPT
{
    return hm->buf.capacity * sizeof(T) + hm->bucket_count * sizeof(uint16);
}

/**
 * Publishes an element by pushing it to the front of its bucket chain
 * All entry data must be written before calling this (release)
 */
template <typename T>
FORCE_INLINE
void hashmap_link(ThrdHashMapT<T>* const hm, uint32 hash, int32 element) NO_EXCEPT
{
    atomic<uint16>* const bucket = hashmap_bucket(hm, hash);
    T* const entry = &hm->buf.memory[element];

    uint16 head = bucket->load(memory_order_acquire);
    do {
        entry->next.store(head, memory_order_relaxed);
    } while (!bucket->compare_exchange_weak(head, (uint16) (element + 1), memory_order_release, memory_order_acquire));
}

/**
 * Removes an element from its bucket chain (doesn't free the element)
 *
 * Inserts only modify the bucket head -> unlinking an inner element can't race with inserts
 * Unlinking the head uses a CAS and retries if a new element got pushed in the meantime
 *
 * WARNING: Not safe against concurrent removals in the same chain
 */
template <typename T>
inline
bool hashmap_unlink(ThrdHashMapT<T>* const hm, uint32 hash, int32 element) NO_EXCEPT
{
    atomic<uint16>* const bucket = hashmap_bucket(hm, hash);
    const uint16 link = (uint16) (element + 1);
    const T* const entry = &hm->buf.memory[element];

    while (true) {
        uint16 current = bucket->load(memory_order_acquire);
        if (current == link) {
            if (bucket->compare_exchange_strong(
                current, entry->next.load(memory_order_acquire),
                memory_order_acq_rel, memory_order_acquire
            )) {
                return true;
            }

            // Somebody pushed a new head
            continue;
        }

        while (current) {
            T* const prev = &hm->buf.memory[current - 1];
            const uint16 next = prev->next.load(memory_order_acquire);

            if (next == link) {
                prev->next.store(entry->next.load(memory_order_acquire), memory_order_release);
                return true;
            }

            current = next;
        }

        return false;
    }
}

/**
 * Finds a string entry (ignores the completeness state)
 *
 * @param key    Stored key part (@see hashmap_str_hash)
 * @param length Length of the stored key part
 */
template <typename T>
FORCE_INLINE
T* hashmap_str_find(const ThrdHashMapT<T>* const __restrict hm, const char* key, int32 length, uint32 hash) NO_EXCEPT
{
    uint16 next = hashmap_bucket(hm, hash)->load(memory_order_acquire);

    while (next) {
        T* const entry = &hm->buf.memory[next - 1];

        // The hash check filters out almost all non-matching entries
        // The memcmp includes the \0 -> no partial matches
        if (entry->hash == hash && memcmp(entry->key, key, length + 1) == 0) {
            DEBUG_MEMORY_READ((uintptr_t) entry, sizeof(T));
            return entry;
        }

        next = entry->next.load(memory_order_acquire);
    }

    return NULL;
}

/**
 * Reserves a slot and fills the key + hash
 * The entry is NOT linked yet
 */
template <typename T>
FORCE_INLINE
T* hashmap_str_slot_create(ThrdHashMapT<T>* const __restrict hm, const char* key, int32 length, uint32 hash, int32* element) NO_EXCEPT
{
    const int32 id = chunk_reserve_one(&hm->buf);
    if (id < 0) {
        return NULL;
    }

    T* const entry = &hm->buf.memory[id];

    // Includes the \0
    memcpy(entry->key, key, length + 1);
    entry->hash = hash;

    *element = id;

    return entry;
}

template <typename T, typename V>
T* hashmap_insert(ThrdHashMapT<T>* const __restrict hm, const char* key, V value) NO_EXCEPT
{
    int32 length;
    const uint32 hash = hashmap_str_hash(&key, &length);

    int32 element;
    T* const entry = hashmap_str_slot_create(hm, key, length, hash, &element);
    if (!entry) {
        return NULL;
    }

    entry->value = value;

    // Mark as complete before publishing -> no reader can see a half written entry
    chunk_mark_complete(&hm->buf, element);
    hashmap_link(hm, hash, element);

    return entry;
}

template <typename T, typename V>
T* hashmap_insert(ThrdHashMapT<T>* const __restrict hm, const char* key, V* value) NO_EXCEPT
{
    int32 length;
    const uint32 hash = hashmap_str_hash(&key, &length);

    int32 element;
    T* const entry = hashmap_str_slot_create(hm, key, length, hash, &element);
    if (!entry) {
        return NULL;
    }

    if (value) {
        memcpy(&entry->value, value, sizeof(V));
    } else {
        memset(&entry->value, 0, sizeof(entry->value));
    }

    // Mark as complete before publishing -> no reader can see a half written entry
    chunk_mark_complete(&hm->buf, element);
    hashmap_link(hm, hash, element);

    return entry;
}

// Marks a hashmap_reserve element as completed
template <typename T>
void hashmap_mark_completed(ThrdHashMapT<T>* const __restrict hm, const char* key) NO_EXCEPT
{
    int32 length;
    const uint32 hash = hashmap_str_hash(&key, &length);

    T* const entry = hashmap_str_find(hm, key, length, hash);
    if (entry) {
        chunk_mark_complete(&hm->buf, (int32) (entry - hm->buf.memory));
    }
}

/**
 * Reserves an entry for key, the entry is NOT marked as complete
 * The value is zero initialized (the slot may contain data of a previously removed element)
 */
template <typename T>
T* hashmap_reserve(ThrdHashMapT<T>* const __restrict hm, const char* key) NO_EXCEPT
{
    int32 length;
    const uint32 hash = hashmap_str_hash(&key, &length);

    int32 element;
    T* const entry = hashmap_str_slot_create(hm, key, length, hash, &element);
    if (!entry) {
        return NULL;
    }

    memset(&entry->value, 0, sizeof(entry->value));
    hashmap_link(hm, hash, element);

    return entry;
}

/**
 * Returns the existing entry (complete or not) or reserves a new one
 */
template <typename T>
T* hashmap_get_reserve(ThrdHashMapT<T>* const __restrict hm, const char* key) NO_EXCEPT
{
    int32 length;
    const uint32 hash = hashmap_str_hash(&key, &length);

    T* const entry = hashmap_str_find(hm, key, length, hash);
    if (entry) {
        return entry;
    }

    int32 element;
    T* const new_entry = hashmap_str_slot_create(hm, key, length, hash, &element);
    if (!new_entry) {
        return NULL;
    }

    memset(&new_entry->value, 0, sizeof(new_entry->value));
    hashmap_link(hm, hash, element);

    return new_entry;
}

/**
 * Returns the entry for key (incomplete entries are treated as not found)
 */
template <typename T>
inline
T* hashmap_entry_get(const ThrdHashMapT<T>* const __restrict hm, const char* key) NO_EXCEPT
{
    int32 length;
    const uint32 hash = hashmap_str_hash(&key, &length);

    T* const entry = hashmap_str_find(hm, key, length, hash);
    if (!entry || !chunk_is_complete(&hm->buf, (uint32) (entry - hm->buf.memory))) {
        return NULL;
    }

    return entry;
}

/**
 * Returns the entry for key even if it isn't marked as complete yet
 */
template <typename T>
inline
T* hashmap_entry_get_any(const ThrdHashMapT<T>* const __restrict hm, const char* key) NO_EXCEPT
{
    int32 length;
    const uint32 hash = hashmap_str_hash(&key, &length);

    return hashmap_str_find(hm, key, length, hash);
}

/**
 * Removes an entry by pointer (no key hashing/comparison required)
 * Other entries are never moved -> pointers to other entries stay valid
 */
template <typename T>
inline
void hashmap_remove_entry(ThrdHashMapT<T>* const hm, T* const entry) NO_EXCEPT
{
    const int32 element = (int32) (entry - hm->buf.memory);
    ASSERT_TRUE(element >= 0 && element < hm->buf.capacity);

    if (hashmap_unlink(hm, hashmap_entry_hash(entry), element)) {
        chunk_free_element(&hm->buf, (uint32) element);
    }
}

template <typename T>
void hashmap_remove(ThrdHashMapT<T>* const __restrict hm, const char* key) NO_EXCEPT
{
    int32 length;
    const uint32 hash = hashmap_str_hash(&key, &length);

    T* const entry = hashmap_str_find(hm, key, length, hash);
    if (!entry) {
        return;
    }

    const int32 element = (int32) (entry - hm->buf.memory);
    if (hashmap_unlink(hm, hash, element)) {
        chunk_free_element(&hm->buf, (uint32) element);
    }
}

template <
    typename T, typename K, typename V,
    enable_if_t<!is_convertible_v<K, const char*>, int> = 0
>
T* hashmap_insert(ThrdHashMapT<T>* const hm, K key, V value) NO_EXCEPT
{
    const int32 element = chunk_reserve_one(&hm->buf);
    if (element < 0) {
        return NULL;
    }

    T* const entry = &hm->buf.memory[element];
    entry->key = key;
    entry->value = value;

    chunk_mark_complete(&hm->buf, element);
    hashmap_link(hm, hashmap_int_hash(key), element);

    return entry;
}

template <typename T, typename K>
FORCE_INLINE
T* hashmap_int_find(const ThrdHashMapT<T>* const hm, const K& key, uint32 hash) NO_EXCEPT
{
    uint16 next = hashmap_bucket(hm, hash)->load(memory_order_acquire);

    while (next) {
        T* const entry = &hm->buf.memory[next - 1];
        if (entry->key == key) {
            DEBUG_MEMORY_READ((uintptr_t) entry, sizeof(T));
            return entry;
        }

        next = entry->next.load(memory_order_acquire);
    }

    return NULL;
}

template <
    typename T, typename K,
    enable_if_t<!is_convertible_v<K, const char*>, int> = 0
>
T* hashmap_entry_get(ThrdHashMapT<T>* const hm, K key) NO_EXCEPT
{
    return hashmap_int_find(hm, key, hashmap_int_hash(key));
}

template <
    typename T, typename K,
    enable_if_t<!is_convertible_v<K, const char*>, int> = 0
>
void hashmap_remove(ThrdHashMapT<T>* const hm, K key) NO_EXCEPT
{
    const uint32 hash = hashmap_int_hash(key);

    T* const entry = hashmap_int_find(hm, key, hash);
    if (!entry) {
        return;
    }

    const int32 element = (int32) (entry - hm->buf.memory);
    if (hashmap_unlink(hm, hash, element)) {
        chunk_free_element(&hm->buf, (uint32) element);
    }
}

/**
 * Binary representation:
 *
 * 00 01 02 03 = capacity
 * 04 05 06 07 = last_pos
 * 08 09 0A 0B = free_offset
 * 0C .. .. .. = hash map data
 *
 * The bucket array is not dumped, it is rebuilt in hashmap_load()
 */
template <typename T>
int64 hashmap_dump(const ThrdHashMapT<T>* const hm, byte* data, MAYBE_UNUSED int32 steps = 8) NO_EXCEPT
{
    LOG_1("[INFO] Dump HashMapT");
    const byte* const start = data;

    // Dump Chunk memory
    data += chunk_dump(&hm->buf, data);

    // @bug change endian of hashmap values?
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

    // Rebuild the bucket chains (the chain links of the dump are meaningless)
    memset((void *) hm->buckets, 0, hm->bucket_count * sizeof(uint16));
    for (int32 i = 0; i < hm->buf.capacity; ++i) {
        if (!chunk_is_free(&hm->buf, (uint32) i)) {
            hashmap_link(hm, hashmap_entry_hash(&hm->buf.memory[i]), i);
        }
    }

    // @bug change endian of hashmap values?
    // Since we just dump the chunk memory we may have different endian between the saving and the loading system
    // An additional problem is that the elements can have different types and therefore memory layout

    PSEUDO_USE(steps);

    return data - start;
}

#endif
