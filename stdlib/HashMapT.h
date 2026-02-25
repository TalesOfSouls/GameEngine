/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_STDLIB_HASH_MAPT_H
#define COMS_STDLIB_HASH_MAPT_H

#include "HashMap.h"
#include "../memory/ChunkMemoryT.h"

template <typename V>
struct HashEntryStrT {
    char key[HASH_MAP_MAX_KEY_LENGTH];
    uint16 next;
    V value;
};

template <typename K, typename V>
struct HashEntryT {
    K key;
    uint16 next;
    V value;
};

// @performance This hash map implementation is approx. 25% faster than the none-template version in debug build
//              However, the optimized build has similar performance albite a little bit faster
template <typename T>
struct HashMapT {
    // Contains the actual data of the hash map (sometimes)
    // Careful, some hash map implementations don't store the value in here but an offset for use in another array
    // In such a case this doesn't store the actual data but the hash entry which in return can simply contain
    // a pointer or index in some arbitrary other array/memory.
    // For such cases we have some additional pointer/offset chasing to do BUT we can handle hash collisions much faster
    // because iterating through the hash map entries is faster since they might be already in L3 or L2 cache.
    ChunkMemoryT<T> buf;

    // In our hash map implementation we have longer chains since we don't have separate buckets as when using a table array
    // What I mean is that a chain in this implementation could even contain hashs that result in different indices
    // It's probably a mix of a open and a chain implementation
    // This is why a well distributed hash function is key for this implementation
    HashMapHashFunction hash_function;
};

#endif