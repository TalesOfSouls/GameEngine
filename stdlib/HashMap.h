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

#include "Types.h"
#include "../hash/GeneralHash.h"
#include "../memory/RingMemory.h"
#include "../memory/BufferMemory.h"
#include "../memory/ChunkMemory.h"
#include "../utils/StringUtils.h"
#include "../stdlib/Simd.h"
#include "../system/Allocator.h"

// If a hash key is longer than the max key length, we use the last N characters of that key
// The key length is currently chosen to result in 32 byte size for the common case: HashEntryInt32
#define HASH_MAP_MAX_KEY_LENGTH 26

/////////////////////////////
// string key
/////////////////////////////
// Next below always represents the index in the chunk memory of the next entry with the same hash (not the byte offset)
struct HashEntryInt32 {
    char key[HASH_MAP_MAX_KEY_LENGTH];
    uint16 next;
    int32 value;
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
    char value[HASH_MAP_MAX_KEY_LENGTH];
};

struct HashEntry {
    char key[HASH_MAP_MAX_KEY_LENGTH];
    uint16 next;
    byte* value;
};

/////////////////////////////
// int key
/////////////////////////////
struct HashEntryInt32KeyInt32 {
    int32 key;
    uint16 next;
    int32 value;
};

struct HashEntryInt64KeyInt32 {
    int32 key;
    uint16 next;
    int64 value;
};

struct HashEntryUIntPtrKeyInt32 {
    int32 key;
    uint16 next;
    uintptr_t value;
};

struct HashEntryVoidPKeyInt32 {
    int32 key;
    uint16 next;
    void* value;
};

struct HashEntryFloatKeyInt32 {
    int32 key;
    uint16 next;
    f32 value;
};

struct HashEntryStrKeyInt32 {
    int32 key;
    uint16 next;
    char value[HASH_MAP_MAX_KEY_LENGTH];
};

struct HashEntryKeyInt32 {
    int32 key;
    uint16 next;
    byte* value;
};

// HashMaps are limited to 4GB in total size
struct HashMap {
    // Contains the chunk memory index for a provided key/hash
    // Values are 1-indexed/offset since 0 means not used/found
    uint16* table;

    // Contains the actual data of the hash map
    // Careful, some hash map implementations don't store the value in here but an offset for use in another array
    // @question We might want to align the ChunkMemory memory to 8byte, currently it's either 4 or 8 byte depending on the length
    ChunkMemory buf;
};

// The ref hash map is used if the value size is dynamic per element (e.g. files, cache data etc.)
struct HashMapRef {
    HashMap hm;

    ChunkMemory data;
};

// @todo Change so the hashmap can grow or maybe even better create a static and dynamic version
inline
void hashmap_alloc(HashMap* hm, int32 count, int32 element_size, int32 alignment = 64)
{
    LOG_1("[INFO] Allocate HashMap for %n elements with %n B per element", {{LOG_DATA_INT32, &count}, {LOG_DATA_INT32, &element_size}});
    byte* data = (byte *) platform_alloc(
        count * (sizeof(uint16) + element_size)
        + CEIL_DIV(count, alignment) * sizeof(hm->buf.free)
    );

    hm->table = (uint16 *) data;
    chunk_init(&hm->buf, data + sizeof(uint16) * count, count, element_size, alignment);
}

inline
void hashmap_alloc(HashMapRef* hmr, int32 count, int32 data_element_size, int32 alignment = 64)
{
    int32 element_size = sizeof(HashEntryInt32Int32);
    LOG_1("[INFO] Allocate HashMap for %n elements with %n B per element", {{LOG_DATA_INT32, &count}, {LOG_DATA_INT32, &element_size}});
    byte* data = (byte *) platform_alloc_aligned(
        count * (sizeof(uint16) + element_size)
        + CEIL_DIV(count, alignment) * sizeof(hmr->hm.buf.free)
        + count * data_element_size,
        alignment
    );

    hmr->hm.table = (uint16 *) data;
    chunk_init(&hmr->hm.buf, data + sizeof(uint16) * count, count, element_size, alignment);
    chunk_init(&hmr->data, data + hmr->hm.buf.size, count, data_element_size, alignment);
}

inline
void hashmap_free(HashMap* hm)
{
    platform_aligned_free((void **) &hm->table);

    hm->table = NULL;
    hm->buf.size = 0;
    hm->buf.memory = NULL;
}

// WARNING: element_size = element size + remaining HashEntry data size
inline
void hashmap_create(HashMap* hm, int32 count, int32 element_size, RingMemory* ring, int32 alignment = 64) noexcept
{
    LOG_1("[INFO] Create HashMap for %n elements with %n B per element", {{LOG_DATA_INT32, &count}, {LOG_DATA_INT32, &element_size}});
    byte* data = ring_get_memory(
        ring,
        count * (sizeof(uint16) + element_size)
        + CEIL_DIV(count, alignment) * sizeof(hm->buf.free)
    );

    hm->table = (uint16 *) data;
    chunk_init(&hm->buf, data + sizeof(uint16) * count, count, element_size, alignment);
}

// WARNING: element_size = element size + remaining HashEntry data size
inline
void hashmap_create(HashMap* hm, int32 count, int32 element_size, BufferMemory* buf, int32 alignment = 64) noexcept
{
    LOG_1("[INFO] Create HashMap for %n elements with %n B per element", {{LOG_DATA_INT32, &count}, {LOG_DATA_INT32, &element_size}});
    byte* data = buffer_get_memory(
        buf,
        count * (sizeof(uint16) + element_size)
        + CEIL_DIV(count, alignment) * sizeof(hm->buf.free)
    );

    hm->table = (uint16 *) data;
    chunk_init(&hm->buf, data + sizeof(uint16) * count, count, element_size, alignment);
}

// WARNING: element_size = element size + remaining HashEntry data size
inline
void hashmap_create(HashMap* hm, int32 count, int32 element_size, byte* buf, int32 alignment = 64) noexcept
{
    LOG_1("[INFO] Create HashMap for %n elements with %n B per element", {{LOG_DATA_INT32, &count}, {LOG_DATA_INT32, &element_size}});
    hm->table = (uint16 *) buf;
    chunk_init(&hm->buf, buf + sizeof(uint16) * count, count, element_size, alignment);
}

inline
void hashmap_update_data_pointer(HashMap* hm, byte* data) noexcept
{
    hm->table = (uint16 *) data;
    hm->buf.memory = data + sizeof(uint16) * hm->buf.count;
}

// Calculates how large a hashmap will be
inline
int64 hashmap_size(int count, int32 element_size) noexcept
{
    return count * sizeof(element_size) // table
        + count * element_size // elements
        + sizeof(uint64) * CEIL_DIV(count, 64); // free
}

inline
int64 hashmap_size(const HashMap* hm) noexcept
{
    return hm->buf.count * sizeof(uint16) + hm->buf.size;
}

/////////////////////////////
// string key
/////////////////////////////
// @performance We could greatly improve the hashmap performance by ensuring that a uniquely hashed entry always starts at a cacheline
//      If another hash results in the same index that should be at the cacheline + 32 position (if 32 bit size)
//      This would ensure for those elements that if there is one collision we at least don't have to read another cache line
//      Of course for more than 1 collision we would still need to load multiple cachelines, but ideally that shouldn't happen too often
void hashmap_insert(HashMap* hm, const char* key, int32 value) noexcept {
    uint64 index = hash_djb2(key) % hm->buf.count;

    int32 element = chunk_reserve(&hm->buf, 1);
    HashEntryInt32* entry = (HashEntryInt32 *) chunk_get_element(&hm->buf, element, true);

    ASSERT_SIMPLE(((uintptr_t) entry) % 32 == 0);

    // Ensure key length
    str_move_to_pos(&key, -HASH_MAP_MAX_KEY_LENGTH);
    str_copy_short(entry->key, key, HASH_MAP_MAX_KEY_LENGTH);
    entry->key[HASH_MAP_MAX_KEY_LENGTH - 1] = '\0';

    entry->value = value;
    entry->next = 0;

    uint16* target = &hm->table[index];
    while (*target) {
        HashEntryInt32* tmp = (HashEntryInt32*) chunk_get_element(&hm->buf, *target - 1, false);
        target = &tmp->next;
    }
    *target = (uint16) (element + 1);
}

void hashmap_insert(HashMap* hm, const char* key, int64 value) noexcept {
    uint64 index = hash_djb2(key) % hm->buf.count;

    int32 element = chunk_reserve(&hm->buf, 1);
    HashEntryInt64* entry = (HashEntryInt64 *) chunk_get_element(&hm->buf, element, true);

    // Ensure key length
    str_move_to_pos(&key, -HASH_MAP_MAX_KEY_LENGTH);
    str_copy_short(entry->key, key, HASH_MAP_MAX_KEY_LENGTH);
    entry->key[HASH_MAP_MAX_KEY_LENGTH - 1] = '\0';

    entry->value = value;
    entry->next = 0;

    uint16* target = &hm->table[index];
    while (*target) {
        HashEntryInt64* tmp = (HashEntryInt64*) chunk_get_element(&hm->buf, *target - 1, false);
        target = &tmp->next;
    }
    *target = (uint16) (element + 1);
}

void hashmap_insert(HashMap* hm, const char* key, int32 value1, int32 value2) noexcept {
    uint64 index = hash_djb2(key) % hm->buf.count;

    int32 element = chunk_reserve(&hm->buf, 1);
    HashEntryInt32Int32* entry = (HashEntryInt32Int32 *) chunk_get_element(&hm->buf, element, true);

    // Ensure key length
    str_move_to_pos(&key, -HASH_MAP_MAX_KEY_LENGTH);
    str_copy_short(entry->key, key, HASH_MAP_MAX_KEY_LENGTH);
    entry->key[HASH_MAP_MAX_KEY_LENGTH - 1] = '\0';

    entry->value = value1;
    entry->value2 = value2;
    entry->next = 0;

    uint16* target = &hm->table[index];
    while (*target) {
        HashEntryInt32Int32* tmp = (HashEntryInt32Int32*) chunk_get_element(&hm->buf, *target - 1, false);
        target = &tmp->next;
    }
    *target = (uint16) (element + 1);
}

void hashmap_insert(HashMapRef* hmr, const char* key, byte* data, int32 data_size) noexcept {
    // Data chunk
    int32 chunk_count = (int32) ((data_size + hmr->data.chunk_size - 1) / hmr->data.chunk_size);
    int32 chunk_offset = chunk_reserve(&hmr->data, chunk_count);

    if (chunk_offset < 0) {
        ASSERT_SIMPLE(chunk_offset >= 0);
        return;
    }

    // Insert Data
    // NOTE: The data and the hash map entry are in two separate memory areas
    byte* data_mem = chunk_get_element(&hmr->data, chunk_offset);
    memcpy(data_mem, data, data_size);

    // Handle hash map entry
    uint64 index = hash_djb2(key) % hmr->hm.buf.count;

    int32 element = chunk_reserve(&hmr->hm.buf, 1);
    HashEntryInt32Int32* entry = (HashEntryInt32Int32 *) chunk_get_element(&hmr->hm.buf, element, true);

    // Ensure key length
    str_move_to_pos(&key, -HASH_MAP_MAX_KEY_LENGTH);
    str_copy_short(entry->key, key, HASH_MAP_MAX_KEY_LENGTH);
    entry->key[HASH_MAP_MAX_KEY_LENGTH - 1] = '\0';

    entry->value = chunk_offset;
    entry->value2 = chunk_count;
    entry->next = 0;

    uint16* target = &hmr->hm.table[index];
    while (*target) {
        HashEntryInt32Int32* tmp = (HashEntryInt32Int32*) chunk_get_element(&hmr->hm.buf, *target - 1, false);
        target = &tmp->next;
    }
    *target = (uint16) (element + 1);
}

void hashmap_insert(HashMap* hm, const char* key, uintptr_t value) noexcept {
    uint64 index = hash_djb2(key) % hm->buf.count;

    int32 element = chunk_reserve(&hm->buf, 1);
    HashEntryUIntPtr* entry = (HashEntryUIntPtr *) chunk_get_element(&hm->buf, element, true);

    // Ensure key length
    str_move_to_pos(&key, -HASH_MAP_MAX_KEY_LENGTH);
    str_copy_short(entry->key, key, HASH_MAP_MAX_KEY_LENGTH);
    entry->key[HASH_MAP_MAX_KEY_LENGTH - 1] = '\0';

    entry->value = value;
    entry->next = 0;

    uint16* target = &hm->table[index];
    while (*target) {
        HashEntryUIntPtr* tmp = (HashEntryUIntPtr*) chunk_get_element(&hm->buf, *target - 1, false);
        target = &tmp->next;
    }
    *target = (uint16) (element + 1);
}

void hashmap_insert(HashMap* hm, const char* key, void* value) noexcept {
    uint64 index = hash_djb2(key) % hm->buf.count;

    int32 element = chunk_reserve(&hm->buf, 1);
    HashEntryVoidP* entry = (HashEntryVoidP *) chunk_get_element(&hm->buf, element, true);

    // Ensure key length
    str_move_to_pos(&key, -HASH_MAP_MAX_KEY_LENGTH);
    str_copy_short(entry->key, key, HASH_MAP_MAX_KEY_LENGTH);
    entry->key[HASH_MAP_MAX_KEY_LENGTH - 1] = '\0';

    entry->value = value;
    entry->next = 0;

    uint16* target = &hm->table[index];
    while (*target) {
        HashEntryVoidP* tmp = (HashEntryVoidP*) chunk_get_element(&hm->buf, *target - 1, false);
        target = &tmp->next;
    }
    *target = (uint16) (element + 1);
}

void hashmap_insert(HashMap* hm, const char* key, f32 value) noexcept {
    uint64 index = hash_djb2(key) % hm->buf.count;

    int32 element = chunk_reserve(&hm->buf, 1);
    HashEntryFloat* entry = (HashEntryFloat *) chunk_get_element(&hm->buf, element, true);

    // Ensure key length
    str_move_to_pos(&key, -HASH_MAP_MAX_KEY_LENGTH);
    str_copy_short(entry->key, key, HASH_MAP_MAX_KEY_LENGTH);
    entry->key[HASH_MAP_MAX_KEY_LENGTH - 1] = '\0';

    entry->value = value;
    entry->next = 0;

    uint16* target = &hm->table[index];
    while (*target) {
        HashEntryFloat* tmp = (HashEntryFloat*) chunk_get_element(&hm->buf, *target - 1, false);
        target = &tmp->next;
    }
    *target = (uint16) (element + 1);
}

void hashmap_insert(HashMap* hm, const char* key, const char* value) noexcept {
    uint64 index = hash_djb2(key) % hm->buf.count;

    int32 element = chunk_reserve(&hm->buf, 1);
    HashEntryStr* entry = (HashEntryStr *) chunk_get_element(&hm->buf, element, true);

    // Ensure key length
    str_move_to_pos(&key, -HASH_MAP_MAX_KEY_LENGTH);
    str_copy_short(entry->key, key, HASH_MAP_MAX_KEY_LENGTH);
    entry->key[HASH_MAP_MAX_KEY_LENGTH - 1] = '\0';

    str_copy_short(entry->value, value, HASH_MAP_MAX_KEY_LENGTH);
    entry->value[HASH_MAP_MAX_KEY_LENGTH - 1] = '\0';

    entry->next = 0;

    uint16* target = &hm->table[index];
    while (*target) {
        HashEntryStr* tmp = (HashEntryStr*) chunk_get_element(&hm->buf, *target - 1, false);
        target = &tmp->next;
    }
    *target = (uint16) (element + 1);
}

HashEntry* hashmap_insert(HashMap* hm, const char* key, const byte* value) noexcept {
    uint64 index = hash_djb2(key) % hm->buf.count;

    int32 element = chunk_reserve(&hm->buf, 1);
    HashEntry* entry = (HashEntry *) chunk_get_element(&hm->buf, element, true);

    entry->value = (byte *) entry + sizeof(HashEntry);

    // Ensure key length
    str_move_to_pos(&key, -HASH_MAP_MAX_KEY_LENGTH);
    str_copy_short(entry->key, key, HASH_MAP_MAX_KEY_LENGTH);
    entry->key[HASH_MAP_MAX_KEY_LENGTH - 1] = '\0';

    memcpy(entry->value, value, hm->buf.chunk_size - sizeof(HashEntry));

    entry->next = 0;

    uint16* target = &hm->table[index];
    while (*target) {
        HashEntry* tmp = (HashEntry*) chunk_get_element(&hm->buf, *target - 1, false);
        target = &tmp->next;
    }
    *target = (uint16) (element + 1);

    return entry;
}

HashEntry* hashmap_reserve(HashMap* hm, const char* key) noexcept {
    uint64 index = hash_djb2(key) % hm->buf.count;

    int32 element = chunk_reserve(&hm->buf, 1);
    HashEntry* entry = (HashEntry *) chunk_get_element(&hm->buf, element, true);

    entry->value = (byte *) entry + sizeof(HashEntry);

    // Ensure key length
    str_move_to_pos(&key, -HASH_MAP_MAX_KEY_LENGTH);
    str_copy_short(entry->key, key, HASH_MAP_MAX_KEY_LENGTH);
    entry->key[HASH_MAP_MAX_KEY_LENGTH - 1] = '\0';

    entry->next = 0;

    uint16* target = &hm->table[index];
    while (*target) {
        HashEntry* tmp = (HashEntry*) chunk_get_element(&hm->buf, *target - 1, false);
        target = &tmp->next;
    }
    *target = (uint16) (element + 1);

    return entry;
}

// Returns existing element or element to be filled
HashEntry* hashmap_get_reserve(HashMap* hm, const char* key) noexcept
{
    uint64 index = hash_djb2(key) % hm->buf.count;
    HashEntry* entry = (HashEntry *) chunk_get_element(&hm->buf, hm->table[index] - 1, false);

    str_move_to_pos(&key, -HASH_MAP_MAX_KEY_LENGTH);

    while (entry != NULL) {
        if (str_compare(entry->key, key) == 0) {
            DEBUG_MEMORY_READ((uintptr_t) entry, sizeof(HashEntry));
            return entry;
        }

        if (!entry->next) {
            break;
        }

        entry = (HashEntry *) chunk_get_element(&hm->buf, entry->next - 1, false);
    }

    int32 element = chunk_reserve(&hm->buf, 1);
    HashEntry* entry_new = (HashEntry *) chunk_get_element(&hm->buf, element, true);

    entry_new->value = (byte *) entry_new + sizeof(HashEntry);

    // Ensure key length
    str_move_to_pos(&key, -HASH_MAP_MAX_KEY_LENGTH);
    str_copy_short(entry_new->key, key, HASH_MAP_MAX_KEY_LENGTH);
    entry_new->key[HASH_MAP_MAX_KEY_LENGTH - 1] = '\0';

    if (entry) {
        entry->next = (uint16) (element + 1);
    } else {
        hm->table[index] = (uint16) (element + 1);
    }

    return entry_new;
}

inline
HashEntry* hashmap_get_entry_by_element(HashMap* hm, uint32 element) noexcept
{
    return (HashEntry *) chunk_get_element(&hm->buf, element - 1, false);
}

HashEntry* hashmap_get_entry(HashMap* hm, const char* key) noexcept {
    uint64 index = hash_djb2(key) % hm->buf.count;
    HashEntry* entry = (HashEntry *) chunk_get_element(&hm->buf, hm->table[index] - 1, false);

    str_move_to_pos(&key, -HASH_MAP_MAX_KEY_LENGTH);

    while (entry != NULL) {
        if (str_compare(entry->key, key) == 0) {
            DEBUG_MEMORY_READ((uintptr_t) entry, sizeof(HashEntry));
            return entry;
        }

        entry = (HashEntry *) chunk_get_element(&hm->buf, entry->next - 1, false);
    }

    return NULL;
}

byte* hashmap_get_value(HashMapRef* hmr, const char* key) noexcept {
    uint64 index = hash_djb2(key) % hmr->hm.buf.count;
    HashEntryInt32Int32* entry = (HashEntryInt32Int32 *) chunk_get_element(&hmr->hm.buf, hmr->hm.table[index] - 1, false);

    str_move_to_pos(&key, -HASH_MAP_MAX_KEY_LENGTH);

    while (entry != NULL) {
        if (str_compare(entry->key, key) == 0) {
            DEBUG_MEMORY_READ((uintptr_t) entry, sizeof(HashEntryInt32Int32));
            return chunk_get_element(&hmr->data, entry->value);
        }

        entry = (HashEntryInt32Int32 *) chunk_get_element(&hmr->hm.buf, entry->next - 1, false);
    }

    return NULL;
}

uint32 hashmap_get_element(const HashMap* hm, const char* key) noexcept {
    uint64 index = hash_djb2(key) % hm->buf.count;
    const HashEntry* entry = (const HashEntry *) chunk_get_element((ChunkMemory *) &hm->buf, hm->table[index] - 1, false);

    uint32 element_id = hm->table[index];

    str_move_to_pos(&key, -HASH_MAP_MAX_KEY_LENGTH);

    while (entry != NULL) {
        if (str_compare(entry->key, key) == 0) {
            DEBUG_MEMORY_READ((uintptr_t) entry, sizeof(HashEntry));
            return element_id;
        }

        element_id = entry->next;
        entry = (const HashEntry *) chunk_get_element((ChunkMemory *) &hm->buf, entry->next - 1, false);
    }

    return 0;
}

inline
uint32 hashmap_get_element_by_entry(const HashMap* hm, const HashEntry* entry) noexcept
{
    return chunk_id_from_memory(&hm->buf, (byte *) entry) + 1;
}

// This function only saves one step (omission of the hash function)
// The reason for this is in some cases we can use compile time hashing
HashEntry* hashmap_get_entry(HashMap* hm, const char* key, uint64 hash) noexcept {
    hash %= hm->buf.count;
    HashEntry* entry = (HashEntry *) chunk_get_element(&hm->buf, hm->table[hash] - 1, false);

    str_move_to_pos(&key, -HASH_MAP_MAX_KEY_LENGTH);

    while (entry != NULL) {
        if (str_compare(entry->key, key) == 0) {
            DEBUG_MEMORY_READ((uintptr_t) entry, sizeof(HashEntry));
            return entry;
        }

        entry = (HashEntry *) chunk_get_element(&hm->buf, entry->next - 1, false);
    }

    return NULL;
}

// @performance If we had a doubly linked list we could delete keys much easier
// However that would make insertion slower
// Maybe we create a nother hashmap that is doubly linked
void hashmap_remove(HashMap* hm, const char* key) noexcept {
    uint64 index = hash_djb2(key) % hm->buf.count;
    HashEntry* entry = (HashEntry *) chunk_get_element(&hm->buf, hm->table[index] - 1, false);
    HashEntry* prev = NULL;

    uint32 element_id = hm->table[index];

    str_move_to_pos(&key, -HASH_MAP_MAX_KEY_LENGTH);

    while (entry != NULL) {
        if (str_compare(entry->key, key) == 0) {
            if (prev == NULL) {
                hm->table[index] = entry->next;
            } else {
                prev->next = entry->next;
            }

            chunk_free_elements(&hm->buf, element_id - 1);

            return;
        }

        element_id = entry->next;
        prev = entry;
        entry = (HashEntry *) chunk_get_element(&hm->buf, entry->next - 1, false);
    }
}

/////////////////////////////
// int key
/////////////////////////////
void hashmap_insert(HashMap* hm, int32 key, int32 value) noexcept {
    uint64 index = ((uint32) key) % hm->buf.count;

    int32 element = chunk_reserve(&hm->buf, 1);
    HashEntryInt32KeyInt32* entry = (HashEntryInt32KeyInt32 *) chunk_get_element(&hm->buf, element, true);

    entry->key = key;
    entry->value = value;
    entry->next = 0;

    uint16* target = &hm->table[index];
    while (*target) {
        HashEntryInt32KeyInt32* tmp = (HashEntryInt32KeyInt32*) chunk_get_element(&hm->buf, *target - 1, false);
        target = &tmp->next;
    }
    *target = (uint16) (element + 1);
}

void hashmap_insert(HashMap* hm, int32 key, int64 value) noexcept {
    uint64 index = ((uint32) key) % hm->buf.count;

    int32 element = chunk_reserve(&hm->buf, 1);
    HashEntryInt64KeyInt32* entry = (HashEntryInt64KeyInt32 *) chunk_get_element(&hm->buf, element, true);

    entry->key = key;
    entry->value = value;
    entry->next = 0;

    uint16* target = &hm->table[index];
    while (*target) {
        HashEntryInt64KeyInt32* tmp = (HashEntryInt64KeyInt32*) chunk_get_element(&hm->buf, *target - 1, false);
        target = &tmp->next;
    }
    *target = (uint16) (element + 1);
}

void hashmap_insert(HashMap* hm, int32 key, uintptr_t value) noexcept {
    uint64 index = ((uint32) key) % hm->buf.count;

    int32 element = chunk_reserve(&hm->buf, 1);
    HashEntryUIntPtrKeyInt32* entry = (HashEntryUIntPtrKeyInt32 *) chunk_get_element(&hm->buf, element, true);

    entry->key = key;
    entry->value = value;
    entry->next = 0;

    uint16* target = &hm->table[index];
    while (*target) {
        HashEntryUIntPtrKeyInt32* tmp = (HashEntryUIntPtrKeyInt32*) chunk_get_element(&hm->buf, *target - 1, false);
        target = &tmp->next;
    }
    *target = (uint16) (element + 1);
}

void hashmap_insert(HashMap* hm, int32 key, void* value) noexcept {
    uint64 index = ((uint32) key) % hm->buf.count;

    int32 element = chunk_reserve(&hm->buf, 1);
    HashEntryVoidPKeyInt32* entry = (HashEntryVoidPKeyInt32 *) chunk_get_element(&hm->buf, element, true);

    entry->key = key;
    entry->value = value;
    entry->next = 0;

    uint16* target = &hm->table[index];
    while (*target) {
        HashEntryVoidPKeyInt32* tmp = (HashEntryVoidPKeyInt32*) chunk_get_element(&hm->buf, *target - 1, false);
        target = &tmp->next;
    }
    *target = (uint16) (element + 1);
}

void hashmap_insert(HashMap* hm, int32 key, f32 value) noexcept {
    uint64 index = ((uint32) key) % hm->buf.count;

    int32 element = chunk_reserve(&hm->buf, 1);
    HashEntryFloatKeyInt32* entry = (HashEntryFloatKeyInt32 *) chunk_get_element(&hm->buf, element, true);

    entry->key = key;
    entry->value = value;
    entry->next = 0;

    uint16* target = &hm->table[index];
    while (*target) {
        HashEntryFloatKeyInt32* tmp = (HashEntryFloatKeyInt32*) chunk_get_element(&hm->buf, *target - 1, false);
        target = &tmp->next;
    }
    *target = (uint16) (element + 1);
}

void hashmap_insert(HashMap* hm, int32 key, const char* value) noexcept {
    uint64 index = ((uint32) key) % hm->buf.count;

    int32 element = chunk_reserve(&hm->buf, 1);
    HashEntryStrKeyInt32* entry = (HashEntryStrKeyInt32 *) chunk_get_element(&hm->buf, element, true);

    entry->key = key;

    str_copy_short(entry->value, value, HASH_MAP_MAX_KEY_LENGTH);
    entry->value[HASH_MAP_MAX_KEY_LENGTH - 1] = '\0';

    entry->next = 0;

    uint16* target = &hm->table[index];
    while (*target) {
        HashEntryStrKeyInt32* tmp = (HashEntryStrKeyInt32*) chunk_get_element(&hm->buf, *target - 1, false);
        target = &tmp->next;
    }
    *target = (uint16) (element + 1);
}

void hashmap_insert(HashMap* hm, int32 key, const byte* value) noexcept {
    uint64 index = ((uint32) key) % hm->buf.count;

    int32 element = chunk_reserve(&hm->buf, 1);
    HashEntryKeyInt32* entry = (HashEntryKeyInt32 *) chunk_get_element(&hm->buf, element, true);

    entry->key = key;
    entry->value = (byte *) entry + sizeof(HashEntryKeyInt32);

    memcpy(entry->value, value, hm->buf.chunk_size - sizeof(HashEntryKeyInt32));

    entry->next = 0;

    uint16* target = &hm->table[index];
    while (*target) {
        HashEntryKeyInt32* tmp = (HashEntryKeyInt32*) chunk_get_element(&hm->buf, *target - 1, false);
        target = &tmp->next;
    }
    *target = (uint16) (element + 1);
}

HashEntryKeyInt32* hashmap_get_entry(HashMap* hm, int32 key) noexcept {
    uint64 index = ((uint32) key) % hm->buf.count;
    HashEntryKeyInt32* entry = (HashEntryKeyInt32 *) chunk_get_element(&hm->buf, hm->table[index] - 1, false);

    while (entry != NULL) {
        if (entry->key == key) {
            DEBUG_MEMORY_READ((uintptr_t) entry, sizeof(HashEntryKeyInt32));
            return entry;
        }

        entry = (HashEntryKeyInt32 *) chunk_get_element(&hm->buf, entry->next - 1, false);
    }

    return NULL;
}

// This function only saves one step (omission of the hash function)
// The reason for this is in some cases we can use compile time hashing
HashEntryKeyInt32* hashmap_get_entry(HashMap* hm, int32 key, uint64 hash) noexcept {
    hash %= hm->buf.count;
    HashEntryKeyInt32* entry = (HashEntryKeyInt32 *) chunk_get_element(&hm->buf, hm->table[hash] - 1, false);

    while (entry != NULL) {
        if (entry->key == key) {
            DEBUG_MEMORY_READ((uintptr_t) entry, sizeof(HashEntryKeyInt32));
            return entry;
        }

        entry = (HashEntryKeyInt32 *) chunk_get_element(&hm->buf, entry->next - 1, false);
    }

    return NULL;
}

// @performance If we had a doubly linked list we could delete keys much easier
// However that would make insertion slower
// Maybe we create a nother hashmap that is doubly linked
void hashmap_remove(HashMap* hm, int32 key) noexcept {
    uint64 index = ((uint32) key) % hm->buf.count;
    HashEntryKeyInt32* entry = (HashEntryKeyInt32 *) chunk_get_element(&hm->buf, hm->table[index] - 1, false);
    HashEntryKeyInt32* prev = NULL;

    uint32 element_id = hm->table[index];

    while (entry != NULL) {
        if (entry->key == key) {
            if (prev == NULL) {
                hm->table[index] = entry->next;
            } else {
                prev->next = entry->next;
            }

            chunk_free_elements(&hm->buf, element_id - 1);

            return;
        }

        element_id = entry->next;
        prev = entry;
        entry = (HashEntryKeyInt32 *) chunk_get_element(&hm->buf, entry->next - 1, false);
    }
}

inline
int32 hashmap_value_size(const HashMap* hm) noexcept
{
    return (uint32) (
        hm->buf.chunk_size
            - sizeof(char) * HASH_MAP_MAX_KEY_LENGTH // key
            - sizeof(uint16) // next element
        );
}

// @question Shouldn't we also store the chunk size etc? Currently not done and expected to be correctly initialized.
inline
int64 hashmap_dump(const HashMap* hm, byte* data, [[maybe_unused]] int32 steps = 8)
{
    LOG_1("[INFO] Dump HashMap");
    *((uint32 *) data) = SWAP_ENDIAN_LITTLE(hm->buf.count);
    data += sizeof(hm->buf.count);

    // Dump the table content
    memcpy(data, hm->table, sizeof(uint16) * hm->buf.count);
    SWAP_ENDIAN_LITTLE_SIMD(
        (uint16 *) data,
        (uint16 *) data,
        sizeof(uint16) * hm->buf.count / 2, // everything is 2 bytes -> easy to swap
        steps
    );
    data += sizeof(uint16) * hm->buf.count;

    // @bug what if Int32 key?
    int32 value_size = hashmap_value_size(hm);

    // Dumb hash map content = buffer memory
    // Since we are using ChunkMemory we can be smart about it and iterate the chunk memory instead of performing pointer chasing
    int32 free_index = 0;
    int32 bit_index = 0;
    for (uint32 i = 0; i < hm->buf.count; ++i) {
        if (hm->buf.free[free_index] & (1ULL << bit_index)) {
            HashEntry* entry = (HashEntry *) chunk_get_element((ChunkMemory *) &hm->buf, i);

            // key
            memcpy(data, entry->key, sizeof(entry->key));
            data += sizeof(entry->key);

            // next "pointer"
            *((uint16 *) data) = SWAP_ENDIAN_LITTLE(entry->next);
            data += sizeof(entry->next);

            // We just assume that 4 or 8 bytes = int -> endian handling
            if (value_size == 4) {
                *((int32 *) data) = SWAP_ENDIAN_LITTLE(((HashEntryInt32 *) entry)->value);
            } else if (value_size == 8) {
                *((int64 *) data) = SWAP_ENDIAN_LITTLE(((HashEntryInt64 *) entry)->value);
            } else {
                if (entry->value) {
                    memcpy(data, entry->value, value_size);
                } else {
                    memset(data, 0, value_size);
                }
            }
            data += value_size;
        } else {
            // No entry defined -> NULL
            memset(data, 0, hm->buf.chunk_size);
            data += hm->buf.chunk_size;
        }

        ++bit_index;
        if (bit_index > 63) {
            bit_index = 0;
            ++free_index;
        }
    }

    // dump free array
    memcpy(data, hm->buf.free, sizeof(uint64) * CEIL_DIV(hm->buf.count, 64));

    LOG_1("[INFO] Dumped HashMap: %n B", {{LOG_DATA_UINT64, (void *) &hm->buf.size}});

    return sizeof(hm->buf.count) // hash map count = buffer count
        + hm->buf.count * sizeof(uint16) // table content
        + hm->buf.size; // hash map content + free array
}

// WARNING: Requires hashmap_create first
inline
int64 hashmap_load(HashMap* hm, const byte* data, [[maybe_unused]] int32 steps = 8)
{
    LOG_1("[INFO] Load HashMap");
    uint64 count = SWAP_ENDIAN_LITTLE(*((uint32 *) data));
    data += sizeof(uint32);

    // Load the table content
    memcpy(hm->table, data, sizeof(uint16) * count);
    SWAP_ENDIAN_LITTLE_SIMD(
        (uint16 *) hm->table,
        (uint16 *) hm->table,
        sizeof(uint16) * count / 2, // everything is 2 bytes -> easy to swap
        steps
    );
    data += sizeof(uint16) * count;

    // This loop here is why it is important to already have an initialized hashmap
    // @question Do we maybe want to change this and not require an initalized hashmap?
    memcpy(hm->buf.memory, data, hm->buf.size);
    data += hm->buf.chunk_size * hm->buf.count;

    // @question don't we have to possibly endian swap check the free array as well?
    memcpy(hm->buf.free, data, sizeof(uint64) * CEIL_DIV(hm->buf.count, 64));

    // @bug what if Int32 key?
    int32 value_size = hashmap_value_size(hm);

    // Switch endian AND turn offsets to pointers
    uint32 chunk_id = 0;
    chunk_iterate_start(&hm->buf, chunk_id) {
        HashEntry* entry = (HashEntry *) chunk_get_element((ChunkMemory *) &hm->buf, chunk_id);

        // key is already loaded with the memcpy
        // @question Do we even want to use memcpy? We are re-checking all the values here anyways

        // next "pointer"
        entry->next = SWAP_ENDIAN_LITTLE(entry->next);

        if (value_size == 4) {
            ((HashEntryInt32 *) entry)->value = SWAP_ENDIAN_LITTLE(((HashEntryInt32 *) entry)->value);
        } else if (value_size == 8) {
            ((HashEntryInt64 *) entry)->value = SWAP_ENDIAN_LITTLE(((HashEntryInt64 *) entry)->value);
        }
    } chunk_iterate_end;

    LOG_1("[INFO] Loaded HashMap: %n B", {{LOG_DATA_UINT64, &hm->buf.size}});

    // How many bytes was read from data
    return sizeof(hm->buf.count) // hash map count = buffer count
        + hm->buf.count * sizeof(uint16) // table content
        + hm->buf.size;
}

#endif