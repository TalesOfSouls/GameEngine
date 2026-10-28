/**
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_STDLIB_HASH_MAPT_H
#define COMS_STDLIB_HASH_MAPT_H

#include "Stdlib.h"
#include "../memory/ChunkMemoryT.h"

// If a hash key is longer than the max key length, we use the last N - 1 characters of that key
// (the last byte is reserved for the null terminator)
// The key length is chosen to result in a 32 byte entry for the common case: HashEntryStrT<int32>
#define HASH_MAP_MAX_KEY_LENGTH 22

typedef uint64 (*HashMapHashFunction)(const void* data) NO_EXCEPT;

/**
 * WARNING: Never overwrite a whole entry (e.g. memset/memcpy of T), the head member belongs to the bucket
 *          with the same index as the slot and NOT to the entry stored in this slot.
 */
template <typename V>
struct HashEntryStrT {
    char key[HASH_MAP_MAX_KEY_LENGTH];

    // Id + 1 of the next element in the same bucket (0 = end of chain)
    uint16 next;

    // Id + 1 of the first element of bucket[slot index] (0 = empty bucket)
    uint16 head;

    // 16 bit of the key hash, filters out almost all non-matching entries before the key comparison
    uint16 tag;

    V value;
};

template <typename K, typename V>
struct HashEntryT {
    K key;

    // Id + 1 of the next element in the same bucket (0 = end of chain)
    uint16 next;

    // Id + 1 of the first element of bucket[slot index] (0 = empty bucket)
    uint16 head;

    V value;
};

/**
 * Hash map with separate chaining, all data is stored in a ChunkMemoryT (no additional memory required)
 *
 * Every slot i of the chunk memory contains the entry stored in that slot AND the head of bucket i.
 * The bucket of a key is hashmap_hash_range(hash, capacity) -> works with any capacity (not only 2^n).
 * Chains only contain entries of the same bucket, entries never move (pointers stay valid until removed).
 * Since the bucket heads are part of the chunk memory, dump/load doesn't need to rebuild anything.
 *
 * WARNING: Max capacity is 65535 elements since the chain uses uint16 links
 */
template <typename T>
struct HashMapT {
    ChunkMemoryT<T> buf;

    // @deprecated Kept for compatibility, no longer used
    // String keys use an inlined one-pass djb2 + length (hashmap_str_hash)
    // Other keys use hashmap_int_hash (the old code hashed the key bytes as a null terminated string)
    HashMapHashFunction hash_function;
};

#endif