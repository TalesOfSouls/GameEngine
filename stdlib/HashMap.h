/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_STDLIB_HASH_MAP_H
#define COMS_STDLIB_HASH_MAP_H

#include "Stdlib.h"
#include "../memory/ChunkMemory.h"

// @question Could this get simplified by using simple templates for HashEntry?
//          So far nothing feels really good when trying. Sure it's possible but never nice.

// If a hash key is longer than the max key length, we use the last N characters of that key
// The key length is currently chosen to result in 32 byte size for the common case: HashEntryInt32
#define HASH_MAP_MAX_KEY_LENGTH 22

/////////////////////////////
// string key
/////////////////////////////
// Next below always represents the index in the chunk memory of the next entry with the same hash (not the byte offset)
struct HashEntryInt32 {
    char key[HASH_MAP_MAX_KEY_LENGTH];
    uint16 next;

    // We make this int64 despite the wrong struct name
    // This is because we want at least 32 byte elements for our hash entries
    int64 value;
};

// This struct is often used for hash maps that are implemented with dynamic length content
// value  = stores the offset into the buffer array
// value2 = stores the length of the data
struct HashEntryInt32Int32 {
    char key[HASH_MAP_MAX_KEY_LENGTH];
    uint16 next;
    int32 value;
    int32 value2;
};

struct HashEntryInt64 {
    char key[HASH_MAP_MAX_KEY_LENGTH];
    uint16 next;
    int64 value;
};

struct HashEntryUIntPtr {
    char key[HASH_MAP_MAX_KEY_LENGTH];
    uint16 next;
    uintptr_t value;
};

struct HashEntryVoidP {
    char key[HASH_MAP_MAX_KEY_LENGTH];
    uint16 next;
    void* value;
};

struct HashEntryFloat {
    char key[HASH_MAP_MAX_KEY_LENGTH];
    uint16 next;
    f32 value;
};

struct HashEntryStr {
    char key[HASH_MAP_MAX_KEY_LENGTH];
    uint16 next;
    char value[HASH_MAP_MAX_KEY_LENGTH + 8];
};

struct HashEntry {
    char key[HASH_MAP_MAX_KEY_LENGTH];
    uint16 next;
    byte* value;
};

/////////////////////////////
// uint32 key
/////////////////////////////
struct HashEntryInt32KeyInt32 {
    uint32 key;
    uint32 next;
    int32 value;
};

struct HashEntryInt64KeyInt32 {
    uint32 key;
    uint32 next;
    int64 value;
};

struct HashEntryUIntPtrKeyInt32 {
    uint32 key;
    uint32 next;
    uintptr_t value;
};

struct HashEntryVoidPKeyInt32 {
    uint32 key;
    uint32 next;
    void* value;
};

struct HashEntryFloatKeyInt32 {
    uint32 key;
    uint32 next;
    f64 value;
};

struct HashEntryStrKeyInt32 {
    uint32 key;
    uint32 next;
    char value[24];
};

struct HashEntryKeyInt32 {
    uint32 key;
    uint32 next;
    byte* value;
};

/////////////////////////////
// uint64 key
/////////////////////////////
struct HashEntryInt32KeyInt64 {
    uint64 key;
    uint32 next;
    int32 value;
};

struct HashEntryInt64KeyInt64 {
    uint64 key;
    uint32 next;
    int64 value;
};

struct HashEntryUIntPtrKeyInt64 {
    uint64 key;
    uint32 next;
    uintptr_t value;
};

struct HashEntryVoidPKeyInt64 {
    uint64 key;
    uint32 next;
    void* value;
};

struct HashEntryFloatKeyInt64 {
    uint64 key;
    uint32 next;
    f64 value;
};

struct HashEntryStrKeyInt64 {
    uint64 key;
    uint32 next;
    char value[16];
};

struct HashEntryKeyInt64 {
    uint64 key;
    uint32 next;
    byte* value;
};

typedef uint64 (*HashMapHashFunction)(const void* data);

// @performance This hash map implementation is approx. 15-25% slower than a "normal" chained hash map
//  We still keep it for now because the slow part (the chunk_reserve) could potentially be improved
//  However, a optimized build makes this hashmap equally fast or even faster? Are we sure?
struct HashMap {
    // Contains the actual data of the hash map (sometimes)
    // Careful, some hash map implementations don't store the value in here but an offset for use in another array
    // In such a case this doesn't store the actual data but the hash entry which in return can simply contain
    // a pointer or index in some arbitrary other array/memory.
    // For such cases we have some additional pointer/offset chasing to do BUT we can handle hash collisions much faster
    // because iterating through the hash map entries is faster since they might be already in L3 or L2 cache.
    ChunkMemory buf;

    // In our hash map implementation we have longer chains since we don't have separate buckets as when using a table array
    // What I mean is that a chain in this implementation could even contain hashs that result in different indices
    // It's probably a mix of a open and a chain implementation
    // This is why a well distributed hash function is key for this implementation
    HashMapHashFunction hash_function;
};

#endif