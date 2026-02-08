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
#include "../hash/GeneralHash.h"
#include "../memory/RingMemory.h"
#include "../memory/BufferMemory.h"
#include "../memory/ChunkMemory.h"
#include "../utils/StringUtils.h"
#include "../stdlib/Simd.h"
#include "../system/Allocator.h"

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
    // @question We might want to align the ChunkMemory memory to 8byte, currently it's either 4 or 8 byte depending on the length
    ChunkMemory buf;

    // In our hash map implementation we have longer chains since we don't have separate buckets as when using a table array
    // What I mean is that a chain in this implementation could even contain hashs that result in different indices
    // It's probably a mix of a open and a chain implementation
    // This is why a well distributed hash function is key for this implementation
    HashMapHashFunction hash_function;
};

// @performance We might want to provide an element alignment and memory start alignment (= e.g. cache line size)
FORCE_INLINE
void hashmap_alloc(HashMap* const hm, int32 count, int32 element_size, int32 alignment = 32) NO_EXCEPT
{
    // This ensures 4 byte alignment
    count = align_up(count, 2);

    LOG_1("[INFO] Allocate HashMap for %n elements with %n B per element", {DATA_TYPE_INT32, &count}, {DATA_TYPE_INT32, &element_size});
    hm->hash_function = hash_djb2;
    chunk_alloc(&hm->buf, count, element_size, alignment);
}

FORCE_INLINE
void hashmap_free(HashMap* const hm) NO_EXCEPT
{
    chunk_free(&hm->buf);
}

// WARNING: element_size = element size + remaining HashEntry data size
// count ideally should be a power of 2 for better data alignment
inline
void hashmap_create(HashMap* const hm, int32 count, int32 element_size, RingMemory* const ring, int32 alignment = 32) NO_EXCEPT
{
    ASSERT_TRUE(ring);

    // This ensures 4 byte alignment
    count = align_up(count, 2);

    LOG_1("[INFO] Create HashMap for %n elements with %n B per element", {DATA_TYPE_INT32, &count}, {DATA_TYPE_INT32, &element_size});
    const uint64 hm_size = chunk_size_total(count, element_size, alignment);
    byte* const data = ring_get_memory(
        ring,
        hm_size,
        alignment
    );

    hm->hash_function = hash_djb2;
    chunk_init(&hm->buf, data, count, element_size, alignment);

    ASSERT_MEM_ZERO(data, hm_size);
}

// WARNING: element_size = element size + remaining HashEntry data size
// count ideally should be a power of 2 for better data alignment
inline
void hashmap_create(HashMap* const hm, int32 count, int32 element_size, BufferMemory* const buf, int32 alignment = 32) NO_EXCEPT
{
    ASSERT_TRUE(buf);

    LOG_1("[INFO] Create HashMap for %n elements with %n B per element", {DATA_TYPE_INT32, &count}, {DATA_TYPE_INT32, &element_size});
    const uint64 hm_size = chunk_size_total(count, element_size, alignment);
    byte* const data = buffer_get_memory(
        buf,
        hm_size,
        alignment
    );

    hm->hash_function = hash_djb2;
    chunk_init(&hm->buf, data, count, element_size, alignment);

    ASSERT_MEM_ZERO(data, hm_size);
}

// WARNING: element_size = element size + remaining HashEntry data size
// count ideally should be a power of 2 for better data alignment
inline
void hashmap_create(HashMap* const hm, int32 count, int32 element_size, byte* const buf, int32 alignment = 32) NO_EXCEPT
{
    LOG_1("[INFO] Create HashMap for %n elements with %n B per element", {DATA_TYPE_INT32, &count}, {DATA_TYPE_INT32, &element_size});
    hm->hash_function = hash_djb2;
    chunk_init(&hm->buf, buf, count, element_size, alignment);

    ASSERT_MEM_ZERO(
        buf,
        count * element_size
        + ceil_div(count, (int32) sizeof(uint_max) * 8) * sizeof(hm->buf.free)
    );
}

// Calculates how large a hashmap will be
FORCE_INLINE
int64 hashmap_size(int32 count, int32 element_size, int32 alignment = 32) NO_EXCEPT
{
    return chunk_size_total(count, element_size, alignment); // elements
}

FORCE_INLINE
int64 hashmap_size(const HashMap* const hm) NO_EXCEPT
{
    return hm->buf.size;
}

/////////////////////////////
// string key
/////////////////////////////
HashEntryInt32* hashmap_insert(HashMap* const __restrict hm, const char* __restrict key, int32 value) NO_EXCEPT
{
    const int32 index = hm->hash_function((void *) key) % hm->buf.capacity;

    // This is either the place where we insert or the start of the chain we have to follow
    const int32 new_index = chunk_reserve_one((uint_max *) hm->buf.free, hm->buf.capacity, index);
    if (new_index < 0) {
        return NULL;
    }

    // This is either the place where we insert or the start of the chain we have to follow
    HashEntryInt32* entry = (HashEntryInt32 *) chunk_get_element(&hm->buf, index);
    if (index != new_index) {
        // Find the previous chain element
        HashEntryInt32* prev = entry;
        while (prev->next) {
            prev = (HashEntryInt32 *) chunk_get_element(&hm->buf, prev->next - 1);
        }

        prev->next = (uint16) (new_index + 1);
        entry = (HashEntryInt32 *) chunk_get_element(&hm->buf, new_index);
    }

    ASSERT_TRUE(((uintptr_t) entry) % 32 == 0);

    // Ensure key length
    // +4 because this Entry can store a longer key
    str_move_to_pos(&key, -((int32) sizeof(entry->key)));
    strncpy(entry->key, key, sizeof(entry->key));
    entry->key[sizeof(entry->key) - 1] = '\0';

    entry->value = value;
    entry->next = 0;

    return entry;
}

HashEntryInt64* hashmap_insert(HashMap* const __restrict hm, const char* __restrict key, int64 value) NO_EXCEPT
{
    const int32 index = hm->hash_function((void *) key) % hm->buf.capacity;

    // This is either the place where we insert or the start of the chain we have to follow
    const int32 new_index = chunk_reserve_one((uint_max *) hm->buf.free, hm->buf.capacity, index);
    if (new_index < 0) {
        return NULL;
    }

    HashEntryInt64* entry = (HashEntryInt64 *) chunk_get_element(&hm->buf, index);
    if (index != new_index) {
        // Find the previous chain element
        HashEntryInt64* prev = entry;
        while (prev->next) {
            prev = (HashEntryInt64*) chunk_get_element(&hm->buf, prev->next - 1);
        }

        prev->next = (uint16) (new_index + 1);
        entry = (HashEntryInt64 *) chunk_get_element(&hm->buf, new_index);
    }

    ASSERT_TRUE(((uintptr_t) entry) % 32 == 0);

    // Ensure key length
    str_move_to_pos(&key, -HASH_MAP_MAX_KEY_LENGTH);
    strncpy(entry->key, key, HASH_MAP_MAX_KEY_LENGTH);
    entry->key[HASH_MAP_MAX_KEY_LENGTH - 1] = '\0';

    entry->value = value;
    entry->next = 0;

    return entry;
}

HashEntryInt32Int32* hashmap_insert(HashMap* const __restrict hm, const char* __restrict key, int32 value1, int32 value2) NO_EXCEPT
{
    const int32 index = hm->hash_function((void *) key) % hm->buf.capacity;

    // This is either the place where we insert or the start of the chain we have to follow
    const int32 new_index = chunk_reserve_one((uint_max *) hm->buf.free, hm->buf.capacity, index);
    if (new_index < 0) {
        return NULL;
    }

    // This is either the place where we insert or the start of the chain we have to follow
    HashEntryInt32Int32* entry = (HashEntryInt32Int32 *) chunk_get_element(&hm->buf, index);
    if (index != new_index) {
        // Find the previous chain element
        HashEntryInt32Int32* prev = entry;
        while (prev->next) {
            prev = (HashEntryInt32Int32 *) chunk_get_element(&hm->buf, prev->next - 1);
        }

        prev->next = (uint16) (new_index + 1);
        entry = (HashEntryInt32Int32 *) chunk_get_element(&hm->buf, new_index);
    }

    ASSERT_TRUE(((uintptr_t) entry) % 32 == 0);

    // Ensure key length
    str_move_to_pos(&key, -HASH_MAP_MAX_KEY_LENGTH);
    strncpy(entry->key, key, HASH_MAP_MAX_KEY_LENGTH);
    entry->key[HASH_MAP_MAX_KEY_LENGTH - 1] = '\0';

    entry->value = value1;
    entry->value2 = value2;
    entry->next = 0;

    return entry;
}

HashEntryUIntPtr* hashmap_insert(HashMap* const __restrict hm, const char* __restrict key, uintptr_t value) NO_EXCEPT
{
    const int32 index = hm->hash_function((void *) key) % hm->buf.capacity;

    // This is either the place where we insert or the start of the chain we have to follow
    const int32 new_index = chunk_reserve_one((uint_max *) hm->buf.free, hm->buf.capacity, index);
    if (new_index < 0) {
        return NULL;
    }

    // This is either the place where we insert or the start of the chain we have to follow
    HashEntryUIntPtr* entry = (HashEntryUIntPtr *) chunk_get_element(&hm->buf, index);
    if (index != new_index) {
        // Find the previous chain element
        HashEntryUIntPtr* prev = entry;
        while (prev->next) {
            prev = (HashEntryUIntPtr*) chunk_get_element(&hm->buf, prev->next - 1);
        }

        prev->next = (uint16) (new_index + 1);
        entry = (HashEntryUIntPtr *) chunk_get_element(&hm->buf, new_index);
    }

    ASSERT_TRUE(((uintptr_t) entry) % 32 == 0);

    // Ensure key length
    str_move_to_pos(&key, -HASH_MAP_MAX_KEY_LENGTH);
    strncpy(entry->key, key, HASH_MAP_MAX_KEY_LENGTH);
    entry->key[HASH_MAP_MAX_KEY_LENGTH - 1] = '\0';

    entry->value = value;
    entry->next = 0;

    return entry;
}

HashEntryVoidP* hashmap_insert(HashMap* const __restrict hm, const char* key, void* __restrict value) NO_EXCEPT
{
    const int32 index = hm->hash_function((void *) key) % hm->buf.capacity;

    // This is either the place where we insert or the start of the chain we have to follow
    const int32 new_index = chunk_reserve_one((uint_max *) hm->buf.free, hm->buf.capacity, index);
    if (new_index < 0) {
        return NULL;
    }

    // This is either the place where we insert or the start of the chain we have to follow
    HashEntryVoidP* entry = (HashEntryVoidP *) chunk_get_element(&hm->buf, index);
    if (index != new_index) {
        // Find the previous chain element
        HashEntryVoidP* prev = entry;
        while (prev->next) {
            prev = (HashEntryVoidP*) chunk_get_element(&hm->buf, prev->next - 1);
        }

        prev->next = (uint16) (new_index + 1);
        entry = (HashEntryVoidP *) chunk_get_element(&hm->buf, new_index);
    }

    ASSERT_TRUE(((uintptr_t) entry) % 32 == 0);

    // Ensure key length
    str_move_to_pos(&key, -HASH_MAP_MAX_KEY_LENGTH);
    strncpy(entry->key, key, HASH_MAP_MAX_KEY_LENGTH);
    entry->key[HASH_MAP_MAX_KEY_LENGTH - 1] = '\0';

    entry->value = value;
    entry->next = 0;

    return entry;
}

HashEntryFloat* hashmap_insert(HashMap* const __restrict hm, const char* __restrict key, f32 value) NO_EXCEPT
{
    const int32 index = hm->hash_function((void *) key) % hm->buf.capacity;

    // This is either the place where we insert or the start of the chain we have to follow
    const int32 new_index = chunk_reserve_one((uint_max *) hm->buf.free, hm->buf.capacity, index);
    if (new_index < 0) {
        return NULL;
    }

    // This is either the place where we insert or the start of the chain we have to follow
    HashEntryFloat* entry = (HashEntryFloat *) chunk_get_element(&hm->buf, index);
    if (index != new_index) {
        // Find the previous chain element
        HashEntryFloat* prev = entry;
        while (prev->next) {
            prev = (HashEntryFloat*) chunk_get_element(&hm->buf, prev->next - 1);
        }

        prev->next = (uint16) (new_index + 1);
        entry = (HashEntryFloat *) chunk_get_element(&hm->buf, new_index);
    }

    ASSERT_TRUE(((uintptr_t) entry) % 32 == 0);

    // Ensure key length
    str_move_to_pos(&key, -((int32) sizeof(entry->key)));
    strncpy(entry->key, key, sizeof(entry->key));
    entry->key[sizeof(entry->key) - 1] = '\0';

    entry->value = value;
    entry->next = 0;

    return entry;
}

HashEntryStr* hashmap_insert(HashMap* const __restrict hm, const char* __restrict key, const char* __restrict value) NO_EXCEPT
{
    const int32 index = hm->hash_function((void *) key) % hm->buf.capacity;

    // This is either the place where we insert or the start of the chain we have to follow
    const int32 new_index = chunk_reserve_one((uint_max *) hm->buf.free, hm->buf.capacity, index);
    if (new_index < 0) {
        return NULL;
    }

    // This is either the place where we insert or the start of the chain we have to follow
    HashEntryStr* entry = (HashEntryStr *) chunk_get_element(&hm->buf, index);
    if (index != new_index) {
        // Find the previous chain element
        HashEntryStr* prev = entry;
        while (prev->next) {
            prev = (HashEntryStr*) chunk_get_element(&hm->buf, prev->next - 1);
        }

        prev->next = (uint16) (new_index + 1);
        entry = (HashEntryStr *) chunk_get_element(&hm->buf, new_index);
    }

    ASSERT_TRUE(((uintptr_t) entry) % 32 == 0);

    // Ensure key length
    str_move_to_pos(&key, -((int32) sizeof(entry->key)));
    strncpy(entry->key, key, sizeof(entry->key));
    entry->key[sizeof(entry->key) - 1] = '\0';

    strncpy(entry->value, value, sizeof(entry->value));
    entry->value[sizeof(entry->value) - 1] = '\0';

    entry->next = 0;

    return entry;
}

// This function adds the actual data immediately after the hash map entry
// This requires the chunks to have the correct size
HashEntry* hashmap_insert(HashMap* const __restrict hm, const char* __restrict key, const byte* __restrict value, size_t size = 0) NO_EXCEPT
{
    const int32 index = hm->hash_function((void *) key) % hm->buf.capacity;

    // This is either the place where we insert or the start of the chain we have to follow
    const int32 new_index = chunk_reserve_one((uint_max *) hm->buf.free, hm->buf.capacity, index);
    if (new_index < 0) {
        return NULL;
    }

    // This is either the place where we insert or the start of the chain we have to follow
    HashEntry* entry = (HashEntry *) chunk_get_element(&hm->buf, index);
    if (index != new_index) {
        // Find the previous chain element
        HashEntry* prev = entry;
        while (prev->next) {
            prev = (HashEntry*) chunk_get_element(&hm->buf, prev->next - 1);
        }

        prev->next = (uint16) (new_index + 1);
        entry = (HashEntry *) chunk_get_element(&hm->buf, new_index);
    }

    ASSERT_TRUE(((uintptr_t) entry) % 32 == 0);

    entry->value = (byte *) entry + sizeof(HashEntry);

    // Ensure key length
    str_move_to_pos(&key, -HASH_MAP_MAX_KEY_LENGTH);
    strncpy(entry->key, key, HASH_MAP_MAX_KEY_LENGTH);
    entry->key[HASH_MAP_MAX_KEY_LENGTH - 1] = '\0';

    memcpy(entry->value, value, size ? size : hm->buf.chunk_size - sizeof(HashEntry));

    entry->next = 0;

    return entry;
}

// This is perfect to directly fill the data instead of copying over
// Usually only makes sense for large data
HashEntry* hashmap_reserve(HashMap* const __restrict hm, const char* __restrict key) NO_EXCEPT
{
    const int32 index = hm->hash_function((void *) key) % hm->buf.capacity;

    // This is either the place where we insert or the start of the chain we have to follow
    const int32 new_index = chunk_reserve_one((uint_max *) hm->buf.free, hm->buf.capacity, index);
    if (new_index < 0) {
        return NULL;
    }

    // This is either the place where we insert or the start of the chain we have to follow
    HashEntry* entry = (HashEntry *) chunk_get_element(&hm->buf, index);
    if (index != new_index) {
        // Find the previous chain element
        HashEntry* prev = entry;
        while (prev->next) {
            prev = (HashEntry*) chunk_get_element(&hm->buf, prev->next - 1);
        }

        prev->next = (uint16) (new_index + 1);
        entry = (HashEntry *) chunk_get_element(&hm->buf, new_index);
    }

    entry->value = (byte *) entry + sizeof(HashEntry);

    // Ensure key length
    str_move_to_pos(&key, -HASH_MAP_MAX_KEY_LENGTH);
    strncpy(entry->key, key, HASH_MAP_MAX_KEY_LENGTH);
    entry->key[HASH_MAP_MAX_KEY_LENGTH - 1] = '\0';

    entry->next = 0;

    return entry;
}

// Returns existing element or element to be filled
// Usefull if we want to create new element if it doesn't exist or return the existing element
HashEntry* hashmap_get_reserve(HashMap* const __restrict hm, const char* __restrict key) NO_EXCEPT
{
    const int32 index = hm->hash_function((void *) key) % hm->buf.capacity;

    // This is either the place where we insert or the start of the chain we have to follow
    HashEntry* entry = (HashEntry *) chunk_get_element(&hm->buf, index);

    // Ensure key length
    str_move_to_pos(&key, -HASH_MAP_MAX_KEY_LENGTH);

    HashEntry* prev = entry;
    while (true) {
        if (strcmp(prev->key, key) == 0) {
            DEBUG_MEMORY_READ((uintptr_t) prev, sizeof(HashEntry));
            return prev;
        }

        if (!prev->next) {
            break;
        }

        prev = (HashEntry *) chunk_get_element(&hm->buf, prev->next - 1);
    };

    const int32 new_index = chunk_reserve_one(&hm->buf);
    if (new_index < 0) {
        return NULL;
    }

    prev->next = (uint16) (new_index + 1);

    entry = (HashEntry *) chunk_get_element(&hm->buf, new_index);
    entry->value = (byte *) entry + sizeof(HashEntry);

    strncpy(entry->key, key, HASH_MAP_MAX_KEY_LENGTH);
    entry->key[HASH_MAP_MAX_KEY_LENGTH - 1] = '\0';

    return entry;
}

inline
HashEntry* hashmap_get_entry(HashMap* const __restrict hm, const char* __restrict key) NO_EXCEPT
{
    const int32 index = hm->hash_function((void *) key) % hm->buf.capacity;
    if (chunk_is_free(&hm->buf, index)) {
        return NULL;
    }

    HashEntry* entry = (HashEntry *) chunk_get_element(&hm->buf, index);

    // Ensure key length
    str_move_to_pos(&key, -HASH_MAP_MAX_KEY_LENGTH);

    while (entry) {
        if (strcmp(entry->key, key) == 0) {
            DEBUG_MEMORY_READ((uintptr_t) entry, sizeof(HashEntry));
            return entry;
        }

        entry = entry->next ? (HashEntry *) chunk_get_element(&hm->buf, entry->next - 1) : NULL;
    }

    return NULL;
}

void hashmap_remove(HashMap* const __restrict hm, const char* __restrict key) NO_EXCEPT
{
    const int32 index = hm->hash_function((void *) key) % hm->buf.capacity;

    HashEntry* entry = (HashEntry *) chunk_get_element(&hm->buf, index);
    HashEntry* prev = NULL;

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
        entry = entry->next ? (HashEntry *) chunk_get_element(&hm->buf, entry->next - 1) : NULL;
    }
}

/////////////////////////////
// uint32 key
/////////////////////////////
HashEntryInt32KeyInt32* hashmap_insert(HashMap* const hm, uint32 key, int32 value) NO_EXCEPT
{
    const int32 index = hm->hash_function((void *) &key) % hm->buf.capacity;

    // This is either the place where we insert or the start of the chain we have to follow
    const int32 new_index = chunk_reserve_one((uint_max *) hm->buf.free, hm->buf.capacity, index);
    if (new_index < 0) {
        return NULL;
    }

    // This is either the place where we insert or the start of the chain we have to follow
    HashEntryInt32KeyInt32* entry = (HashEntryInt32KeyInt32 *) chunk_get_element(&hm->buf, index);
    if (index != new_index) {
        // Find the previous chain element
        HashEntryInt32KeyInt32* prev = entry;
        while (prev->next) {
            prev = (HashEntryInt32KeyInt32 *) chunk_get_element(&hm->buf, prev->next - 1);
        }

        prev->next = new_index + 1;
        entry = (HashEntryInt32KeyInt32 *) chunk_get_element(&hm->buf, new_index);
    }

    entry->key = key;
    entry->value = value;
    entry->next = 0;

    return entry;
}

HashEntryInt64KeyInt32* hashmap_insert(HashMap* const hm, uint32 key, int64 value) NO_EXCEPT
{
    const int32 index = hm->hash_function((void *) &key) % hm->buf.capacity;

    // This is either the place where we insert or the start of the chain we have to follow
    const int32 new_index = chunk_reserve_one((uint_max *) hm->buf.free, hm->buf.capacity, index);
    if (new_index < 0) {
        return NULL;
    }

    // This is either the place where we insert or the start of the chain we have to follow
    HashEntryInt64KeyInt32* entry = (HashEntryInt64KeyInt32 *) chunk_get_element(&hm->buf, index);
    if (index != new_index) {
        // Find the previous chain element
        HashEntryInt64KeyInt32* prev = entry;
        while (prev->next) {
            prev = (HashEntryInt64KeyInt32 *) chunk_get_element(&hm->buf, prev->next - 1);
        }

        prev->next = new_index + 1;
        entry = (HashEntryInt64KeyInt32 *) chunk_get_element(&hm->buf, new_index);
    }

    entry->key = key;
    entry->value = value;
    entry->next = 0;

    return entry;
}

HashEntryUIntPtrKeyInt32* hashmap_insert(HashMap* const hm, uint32 key, uintptr_t value) NO_EXCEPT
{
    const int32 index = hm->hash_function((void *) &key) % hm->buf.capacity;

    // This is either the place where we insert or the start of the chain we have to follow
    const int32 new_index = chunk_reserve_one((uint_max *) hm->buf.free, hm->buf.capacity, index);
    if (new_index < 0) {
        return NULL;
    }

    // This is either the place where we insert or the start of the chain we have to follow
    HashEntryUIntPtrKeyInt32* entry = (HashEntryUIntPtrKeyInt32 *) chunk_get_element(&hm->buf, index);
    if (index != new_index) {
        // Find the previous chain element
        HashEntryUIntPtrKeyInt32* prev = entry;
        while (prev->next) {
            prev = (HashEntryUIntPtrKeyInt32 *) chunk_get_element(&hm->buf, prev->next - 1);
        }

        prev->next = new_index + 1;
        entry = (HashEntryUIntPtrKeyInt32 *) chunk_get_element(&hm->buf, new_index);
    }

    entry->key = key;
    entry->value = value;
    entry->next = 0;

    return entry;
}

HashEntryVoidPKeyInt32* hashmap_insert(HashMap* const __restrict hm, uint32 key, void* __restrict value) NO_EXCEPT
{
    const int32 index = hm->hash_function((void *) &key) % hm->buf.capacity;

    // This is either the place where we insert or the start of the chain we have to follow
    const int32 new_index = chunk_reserve_one((uint_max *) hm->buf.free, hm->buf.capacity, index);
    if (new_index < 0) {
        return NULL;
    }

    // This is either the place where we insert or the start of the chain we have to follow
    HashEntryVoidPKeyInt32* entry = (HashEntryVoidPKeyInt32 *) chunk_get_element(&hm->buf, index);
    if (index != new_index) {
        // Find the previous chain element
        HashEntryVoidPKeyInt32* prev = entry;
        while (prev->next) {
            prev = (HashEntryVoidPKeyInt32 *) chunk_get_element(&hm->buf, prev->next - 1);
        }

        prev->next = new_index + 1;
        entry = (HashEntryVoidPKeyInt32 *) chunk_get_element(&hm->buf, new_index);
    }

    entry->key = key;
    entry->value = value;
    entry->next = 0;

    return entry;
}

HashEntryFloatKeyInt32* hashmap_insert(HashMap* const hm, uint32 key, f32 value) NO_EXCEPT
{
    const int32 index = hm->hash_function((void *) &key) % hm->buf.capacity;

    // This is either the place where we insert or the start of the chain we have to follow
    const int32 new_index = chunk_reserve_one((uint_max *) hm->buf.free, hm->buf.capacity, index);
    if (new_index < 0) {
        return NULL;
    }

    // This is either the place where we insert or the start of the chain we have to follow
    HashEntryFloatKeyInt32* entry = (HashEntryFloatKeyInt32 *) chunk_get_element(&hm->buf, index);
    if (index != new_index) {
        // Find the previous chain element
        HashEntryFloatKeyInt32* prev = entry;
        while (prev->next) {
            prev = (HashEntryFloatKeyInt32 *) chunk_get_element(&hm->buf, prev->next - 1);
        }

        prev->next = new_index + 1;
        entry = (HashEntryFloatKeyInt32 *) chunk_get_element(&hm->buf, new_index);
    }

    entry->key = key;
    entry->value = value;
    entry->next = 0;

    return entry;
}

HashEntryStrKeyInt32* hashmap_insert(HashMap* const __restrict hm, uint32 key, const char* __restrict value) NO_EXCEPT
{
    const int32 index = hm->hash_function((void *) &key) % hm->buf.capacity;

    // This is either the place where we insert or the start of the chain we have to follow
    const int32 new_index = chunk_reserve_one((uint_max *) hm->buf.free, hm->buf.capacity, index);
    if (new_index < 0) {
        return NULL;
    }

    // This is either the place where we insert or the start of the chain we have to follow
    HashEntryStrKeyInt32* entry = (HashEntryStrKeyInt32 *) chunk_get_element(&hm->buf, index);
    if (index != new_index) {
        // Find the previous chain element
        HashEntryStrKeyInt32* prev = entry;
        while (prev->next) {
            prev = (HashEntryStrKeyInt32 *) chunk_get_element(&hm->buf, prev->next - 1);
        }

        prev->next = new_index + 1;
        entry = (HashEntryStrKeyInt32 *) chunk_get_element(&hm->buf, new_index);
    }

    entry->key = key;

    strncpy(entry->value, value, HASH_MAP_MAX_KEY_LENGTH);
    entry->value[HASH_MAP_MAX_KEY_LENGTH - 1] = '\0';

    entry->next = 0;

    return entry;
}

HashEntryKeyInt32* hashmap_insert(HashMap* const __restrict hm, uint32 key, const byte* __restrict value) NO_EXCEPT
{
    const int32 index = hm->hash_function((void *) &key) % hm->buf.capacity;

    // This is either the place where we insert or the start of the chain we have to follow
    const int32 new_index = chunk_reserve_one((uint_max *) hm->buf.free, hm->buf.capacity, index);
    if (new_index < 0) {
        return NULL;
    }

    // This is either the place where we insert or the start of the chain we have to follow
    HashEntryKeyInt32* entry = (HashEntryKeyInt32 *) chunk_get_element(&hm->buf, index);
    if (index != new_index) {
        // Find the previous chain element
        HashEntryKeyInt32* prev = entry;
        while (prev->next) {
            prev = (HashEntryKeyInt32 *) chunk_get_element(&hm->buf, prev->next - 1);
        }

        prev->next = new_index + 1;
        entry = (HashEntryKeyInt32 *) chunk_get_element(&hm->buf, new_index);
    }

    entry->key = key;
    entry->value = (byte *) entry + sizeof(HashEntryKeyInt32);

    memcpy(entry->value, value, hm->buf.chunk_size - sizeof(HashEntryKeyInt32));

    entry->next = 0;

    return entry;
}

HashEntryKeyInt32* hashmap_get_entry(HashMap* const hm, uint32 key) NO_EXCEPT
{
    const int32 index = hm->hash_function((void *) &key) % hm->buf.capacity;
    if (chunk_is_free(&hm->buf, index)) {
        return NULL;
    }

    HashEntryKeyInt32* entry = (HashEntryKeyInt32 *) chunk_get_element(&hm->buf, index);

    while (entry) {
        if (entry->key == key) {
            DEBUG_MEMORY_READ((uintptr_t) entry, sizeof(HashEntryKeyInt32));
            return entry;
        }

        entry = (HashEntryKeyInt32 *) chunk_get_element(&hm->buf, entry->next - 1);
    }

    return NULL;
}

// @performance If we had a doubly linked list we could delete keys much easier
// However that would make insertion slower
// Maybe we create a nother hashmap that is doubly linked
void hashmap_remove(HashMap* const hm, uint32 key) NO_EXCEPT
{
    const int32 index = hm->hash_function((void *) &key) % hm->buf.capacity;

    HashEntryKeyInt32* entry = (HashEntryKeyInt32 *) chunk_get_element(&hm->buf, index);
    HashEntryKeyInt32* prev = NULL;

    while (entry) {
        if (entry->key == key) {
            if (prev) {
                prev->next = entry->next;
            }

            chunk_free_element(&hm->buf, index);

            return;
        }

        prev = entry;
        entry = (HashEntryKeyInt32 *) chunk_get_element(&hm->buf, entry->next - 1);
    }
}

/////////////////////////////
// uint64 key
/////////////////////////////
HashEntryInt32KeyInt64* hashmap_insert(HashMap* const hm, uint64 key, int32 value) NO_EXCEPT
{
    const int32 index = hm->hash_function((void *) key) % hm->buf.capacity;

    // This is either the place where we insert or the start of the chain we have to follow
    const int32 new_index = chunk_reserve_one((uint_max *) hm->buf.free, hm->buf.capacity, index);
    if (new_index < 0) {
        return NULL;
    }

    // This is either the place where we insert or the start of the chain we have to follow
    HashEntryInt32KeyInt64* entry = (HashEntryInt32KeyInt64 *) chunk_get_element(&hm->buf, index);
    if (index != new_index) {
        // Find the previous chain element
        HashEntryInt32KeyInt64* prev = entry;
        while (prev->next) {
            prev = (HashEntryInt32KeyInt64*) chunk_get_element(&hm->buf, prev->next - 1);
        }

        prev->next = new_index + 1;
        entry = (HashEntryInt32KeyInt64 *) chunk_get_element(&hm->buf, new_index);
    }

    entry->key = key;
    entry->value = value;
    entry->next = 0;

    return entry;
}

HashEntryInt64KeyInt64* hashmap_insert(HashMap* const hm, uint64 key, int64 value) NO_EXCEPT
{
    const int32 index = hm->hash_function((void *) key) % hm->buf.capacity;

    // This is either the place where we insert or the start of the chain we have to follow
    const int32 new_index = chunk_reserve_one((uint_max *) hm->buf.free, hm->buf.capacity, index);
    if (new_index < 0) {
        return NULL;
    }

    // This is either the place where we insert or the start of the chain we have to follow
    HashEntryInt64KeyInt64* entry = (HashEntryInt64KeyInt64 *) chunk_get_element(&hm->buf, index);
    if (index != new_index) {
        // Find the previous chain element
        HashEntryInt64KeyInt64* prev = entry;
        while (prev->next) {
            prev = (HashEntryInt64KeyInt64*) chunk_get_element(&hm->buf, prev->next - 1);
        }

        prev->next = new_index + 1;
        entry = (HashEntryInt64KeyInt64 *) chunk_get_element(&hm->buf, new_index);
    }

    entry->key = key;
    entry->value = value;
    entry->next = 0;

    return entry;
}

HashEntryUIntPtrKeyInt64* hashmap_insert(HashMap* const hm, uint64 key, uintptr_t value) NO_EXCEPT
{
    const int32 index = hm->hash_function((void *) key) % hm->buf.capacity;

    // This is either the place where we insert or the start of the chain we have to follow
    const int32 new_index = chunk_reserve_one((uint_max *) hm->buf.free, hm->buf.capacity, index);
    if (new_index < 0) {
        return NULL;
    }

    // This is either the place where we insert or the start of the chain we have to follow
    HashEntryUIntPtrKeyInt64* entry = (HashEntryUIntPtrKeyInt64 *) chunk_get_element(&hm->buf, index);
    if (index != new_index) {
        // Find the previous chain element
        HashEntryUIntPtrKeyInt64* prev = entry;
        while (prev->next) {
            prev = (HashEntryUIntPtrKeyInt64*) chunk_get_element(&hm->buf, prev->next - 1);
        }

        prev->next = new_index + 1;
        entry = (HashEntryUIntPtrKeyInt64 *) chunk_get_element(&hm->buf, new_index);
    }

    entry->key = key;
    entry->value = value;
    entry->next = 0;

    return entry;
}

HashEntryVoidPKeyInt64* hashmap_insert(HashMap* const __restrict hm, uint64 key, void* __restrict value) NO_EXCEPT
{
    const int32 index = hm->hash_function((void *) key) % hm->buf.capacity;

    // This is either the place where we insert or the start of the chain we have to follow
    const int32 new_index = chunk_reserve_one((uint_max *) hm->buf.free, hm->buf.capacity, index);
    if (new_index < 0) {
        return NULL;
    }

    // This is either the place where we insert or the start of the chain we have to follow
    HashEntryVoidPKeyInt64* entry = (HashEntryVoidPKeyInt64 *) chunk_get_element(&hm->buf, index);
    if (index != new_index) {
        // Find the previous chain element
        HashEntryVoidPKeyInt64* prev = entry;
        while (prev->next) {
            prev = (HashEntryVoidPKeyInt64*) chunk_get_element(&hm->buf, prev->next - 1);
        }

        prev->next = new_index + 1;
        entry = (HashEntryVoidPKeyInt64 *) chunk_get_element(&hm->buf, new_index);
    }

    entry->key = key;
    entry->value = value;
    entry->next = 0;

    return entry;
}

HashEntryFloatKeyInt64* hashmap_insert(HashMap* const hm, uint64 key, f32 value) NO_EXCEPT
{
    const int32 index = hm->hash_function((void *) key) % hm->buf.capacity;

    // This is either the place where we insert or the start of the chain we have to follow
    const int32 new_index = chunk_reserve_one((uint_max *) hm->buf.free, hm->buf.capacity, index);
    if (new_index < 0) {
        return NULL;
    }

    // This is either the place where we insert or the start of the chain we have to follow
    HashEntryFloatKeyInt64* entry = (HashEntryFloatKeyInt64 *) chunk_get_element(&hm->buf, index);
    if (index != new_index) {
        // Find the previous chain element
        HashEntryFloatKeyInt64* prev = entry;
        while (prev->next) {
            prev = (HashEntryFloatKeyInt64*) chunk_get_element(&hm->buf, prev->next - 1);
        }

        prev->next = new_index + 1;
        entry = (HashEntryFloatKeyInt64 *) chunk_get_element(&hm->buf, new_index);
    }

    entry->key = key;
    entry->value = value;
    entry->next = 0;

    return entry;
}

HashEntryStrKeyInt64* hashmap_insert(HashMap* const __restrict hm, uint64 key, const char* __restrict value) NO_EXCEPT
{
    const int32 index = hm->hash_function((void *) key) % hm->buf.capacity;

    // This is either the place where we insert or the start of the chain we have to follow
    const int32 new_index = chunk_reserve_one((uint_max *) hm->buf.free, hm->buf.capacity, index);
    if (new_index < 0) {
        return NULL;
    }

    // This is either the place where we insert or the start of the chain we have to follow
    HashEntryStrKeyInt64* entry = (HashEntryStrKeyInt64 *) chunk_get_element(&hm->buf, index);
    if (index != new_index) {
        // Find the previous chain element
        HashEntryStrKeyInt64* prev = entry;
        while (prev->next) {
            prev = (HashEntryStrKeyInt64*) chunk_get_element(&hm->buf, prev->next - 1);
        }

        prev->next = new_index + 1;
        entry = (HashEntryStrKeyInt64 *) chunk_get_element(&hm->buf, new_index);
    }

    entry->key = key;

    strncpy(entry->value, value, HASH_MAP_MAX_KEY_LENGTH);
    entry->value[sizeof(entry->value) - 1] = '\0';
    entry->next = 0;

    return entry;
}

HashEntryKeyInt64* hashmap_insert(HashMap* const __restrict hm, uint64 key, const byte* __restrict value) NO_EXCEPT
{
    const int32 index = hm->hash_function((void *) key) % hm->buf.capacity;

    // This is either the place where we insert or the start of the chain we have to follow
    const int32 new_index = chunk_reserve_one((uint_max *) hm->buf.free, hm->buf.capacity, index);
    if (new_index < 0) {
        return NULL;
    }

    // This is either the place where we insert or the start of the chain we have to follow
    HashEntryKeyInt64* entry = (HashEntryKeyInt64 *) chunk_get_element(&hm->buf, index);
    if (index != new_index) {
        // Find the previous chain element
        HashEntryKeyInt64* prev = entry;
        while (prev->next) {
            prev = (HashEntryKeyInt64*) chunk_get_element(&hm->buf, prev->next - 1);
        }

        prev->next = new_index + 1;
        entry = (HashEntryKeyInt64 *) chunk_get_element(&hm->buf, new_index);
    }

    entry->key = key;
    entry->value = (byte *) entry + sizeof(HashEntryKeyInt64);

    memcpy(entry->value, value, hm->buf.chunk_size - sizeof(HashEntryKeyInt64));

    entry->next = 0;

    return entry;
}

HashEntryKeyInt64* hashmap_get_entry(HashMap* const hm, uint64 key) NO_EXCEPT
{
    const int32 index = hm->hash_function((void *) key) % hm->buf.capacity;
    if (chunk_is_free(&hm->buf, index)) {
        return NULL;
    }

    HashEntryKeyInt64* entry = (HashEntryKeyInt64 *) chunk_get_element(&hm->buf, index);

    while (entry) {
        if (entry->key == key) {
            DEBUG_MEMORY_READ((uintptr_t) entry, sizeof(HashEntryKeyInt64));
            return entry;
        }

        entry = (HashEntryKeyInt64 *) chunk_get_element(&hm->buf, entry->next - 1);
    }

    return NULL;
}

// @performance If we had a doubly linked list we could delete keys much easier
// However that would make insertion slower
// Maybe we create a nother hashmap that is doubly linked
void hashmap_remove(HashMap* const hm, uint64 key) NO_EXCEPT
{
    const int32 index = hm->hash_function((void *) key) % hm->buf.capacity;

    HashEntryKeyInt64* entry = (HashEntryKeyInt64 *) chunk_get_element(&hm->buf, index);
    HashEntryKeyInt64* prev = NULL;

    while (entry) {
        if (entry->key == key) {
            if (prev) {
                prev->next = entry->next;
            }

            chunk_free_element(&hm->buf, index);

            return;
        }

        prev = entry;
        entry = (HashEntryKeyInt64 *) chunk_get_element(&hm->buf, entry->next - 1);
    }
}

// @question Shouldn't we also store the chunk size etc? Currently not done and expected to be correctly initialized.
int64 hashmap_dump(const HashMap* const hm, byte* data, MAYBE_UNUSED int32 value_size, MAYBE_UNUSED int32 steps = 8) NO_EXCEPT
{
    LOG_1("[INFO] Dump HashMap");
    const byte* const start = data;

    // Dump Chunk memory
    data += chunk_dump(&hm->buf, data);

    // @todo change endian of hashmap next "pointer" and values?

    PSEUDO_USE(steps);
    PSEUDO_USE(value_size);

    return data - start;
}

// WARNING: Requires hashmap_create first
int64 hashmap_load(HashMap* const hm, const byte* data, MAYBE_UNUSED int32 value_size, MAYBE_UNUSED int32 steps = 8) NO_EXCEPT
{
    LOG_1("[INFO] Load HashMap");
    const byte* const start = data;

    // Load chunk memory
    data += chunk_load(&hm->buf, data);

    // @todo change endian of hashmap next "pointer" and values?

    PSEUDO_USE(steps);
    PSEUDO_USE(value_size);

    return data - start;
}

#endif