/**
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_STDLIB_THRD_HASH_MAPT_H
#define COMS_STDLIB_THRD_HASH_MAPT_H

#include "Stdlib.h"
#include "HashMapT.h"
#include "../memory/ThrdChunkMemoryT.h"

// If a hash key is longer than the max key length, we use the last N - 1 characters of that key
// (the last byte is reserved for the null terminator)
#define HASH_MAP_MAX_KEY_LENGTH 22

template <typename V>
struct ThrdHashEntryStrT {
    char key[HASH_MAP_MAX_KEY_LENGTH];

    // Id + 1 of the next element in the same bucket (0 = end of chain)
    atomic<uint16> next;

    // Full hash of the (stored) key.
    // Allows us to skip the string comparison for almost all non-matching chain entries
    // and to find the bucket of an entry without re-hashing the key (e.g. remove by entry)
    uint32 hash;

    V value;
};

template <typename K, typename V>
struct ThrdHashEntryT {
    K key;
    atomic<uint16> next;
    V value;
};

/**
 * Thread safe hash map (lock free insert/lookup)
 *
 * The data is stored in a ThrdChunkMemoryT which acts as a slot pool.
 * Entries never move after insertion, which means pointers to entries/values stay valid until the entry is removed.
 * (e.g. the AMS hands out Asset* pointers that live inside the hash map entries)
 *
 * The bucket array is a separate power of 2 array containing the head (id + 1) of every chain.
 * Chains only contain entries with the same bucket index (classic separate chaining)
 *
 * WARNING: Max capacity is 65535 elements since the chain uses uint16 links
 */
template <typename T>
struct ThrdHashMapT {
    // Contains the actual entries (key, next, value)
    ThrdChunkMemoryT<T> buf;

    // Head (element id + 1) of every chain, 0 = empty bucket
    atomic<uint16>* buckets;

    // Always a power of 2 (>= buf.capacity)
    int32 bucket_count;

    // @deprecated Kept for compatibility, no longer used
    // String keys use an inlined one-pass djb2 + length (hashmap_str_hash)
    // Other keys use hashmap_int_hash (the old code hashed the key bytes as a null terminated string)
    HashMapHashFunction hash_function;
};

#endif
