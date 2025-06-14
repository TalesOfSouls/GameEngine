/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_STDLIB_PERFECT_HASH_MAP_H
#define COMS_STDLIB_PERFECT_HASH_MAP_H

#include "Types.h"
#include "HashMap.h"
#include "../hash/GeneralHash.h"
#include "../memory/RingMemory.h"

typedef uint64 (*PerfectHashFunction)(const char* key, int32 seed);

const PerfectHashFunction PERFECT_HASH_FUNCTIONS[] = {
    hash_djb2_seeded,
    hash_sdbm_seeded,
    hash_lose_lose_seeded,
    hash_polynomial_rolling_seeded,
    hash_fnv1a_seeded,
    hash_oat_seeded,
    hash_ejb_seeded
};

#define PERFECT_HASH_MAP_MAX_KEY_LENGTH 28

struct PerfectHashEntryInt32 {
    char key[PERFECT_HASH_MAP_MAX_KEY_LENGTH];
    int32 value;
};

struct PerfectHashEntryInt64 {
    char key[PERFECT_HASH_MAP_MAX_KEY_LENGTH];
    int64 value;
};

struct PerfectHashEntryInt32Int32 {
    char key[PERFECT_HASH_MAP_MAX_KEY_LENGTH];
    int32 value;
    int32 value2;
};

struct PerfectHashEntryUIntPtr {
    char key[PERFECT_HASH_MAP_MAX_KEY_LENGTH];
    uintptr_t value;
};

struct PerfectHashEntryVoidP {
    char key[PERFECT_HASH_MAP_MAX_KEY_LENGTH];
    void* value;
};

struct PerfectHashEntryFloat {
    char key[PERFECT_HASH_MAP_MAX_KEY_LENGTH];
    f32 value;
};

struct PerfectHashEntryStr {
    char key[PERFECT_HASH_MAP_MAX_KEY_LENGTH];
    char value[PERFECT_HASH_MAP_MAX_KEY_LENGTH];
};

struct PerfectHashEntry {
    char key[PERFECT_HASH_MAP_MAX_KEY_LENGTH];
    byte* value;
};

// Currently we assume that a perfect hash map doesn't change after initialization (incl. inserting all elements)
struct PerfectHashMap {
    int32 hash_seed;
    PerfectHashFunction hash_function;
    uint32 entry_size;

    uint32 map_count;
    byte* hash_entries;
};

// The ref hash map is used if the value size is dynamic per element (e.g. files, cache data etc.)
struct PerfectHashMapRef {
    PerfectHashMap hm;

    int32 data_pos;
    int32 data_size;
    byte* data;
};

PerfectHashMap* perfect_hashmap_prepare(PerfectHashMap* hm, const char** keys, int32 key_count, int32 seed_tries, RingMemory* ring)
{
    int32* indices = (int32 *) ring_get_memory(ring, hm->map_count * sizeof(int32), 4);
    bool is_unique = false;

    for (uint32 i = 0; i < ARRAY_COUNT(PERFECT_HASH_FUNCTIONS); ++i) {
        int32 seed;
        int32 c = 0;

        while (!is_unique && c < seed_tries) {
            is_unique = true;
            seed = rand();
            memset(indices, 0, hm->map_count * sizeof(int32));

            for (int32 j = 0; j < key_count; ++j) {
                int32 index = (PERFECT_HASH_FUNCTIONS[i])(keys[j], seed) % hm->map_count;
                if (indices[index]) {
                    is_unique = false;
                    break;
                } else {
                    indices[index] = 1;
                }
            }

            ++c;
        }

        if (is_unique) {
            hm->hash_seed = seed;
            hm->hash_function = PERFECT_HASH_FUNCTIONS[i];

            return hm;
        }
    }

    ASSERT_SIMPLE(false);
    LOG_1("[ERROR] Couldn't create perfect hashmap");

    return NULL;
}

// Same code as above with the difference that we are using a fixed length key array instead of an array of pointers
PerfectHashMap* perfect_hashmap_prepare(PerfectHashMap* hm, const char* keys, int32 key_count, int32 key_length, int32 seed_tries, RingMemory* ring)
{
    int32* indices = (int32 *) ring_get_memory(ring, hm->map_count * sizeof(int32), 4);
    bool is_unique = false;

    for (uint32 i = 0; i < ARRAY_COUNT(PERFECT_HASH_FUNCTIONS); ++i) {
        int32 seed;
        int32 c = 0;

        while (!is_unique && c < seed_tries) {
            is_unique = true;
            seed = rand();
            memset(indices, 0, hm->map_count * sizeof(int32));

            for (int32 j = 0; j < key_count; ++j) {
                int32 index = (PERFECT_HASH_FUNCTIONS[i])(&keys[j * key_length], seed) % hm->map_count;
                if (indices[index]) {
                    is_unique = false;
                    break;
                } else {
                    indices[index] = 1;
                }
            }

            ++c;
        }

        if (is_unique) {
            hm->hash_seed = seed;
            hm->hash_function = PERFECT_HASH_FUNCTIONS[i];

            return hm;
        }
    }

    ASSERT_SIMPLE(false);
    LOG_1("[INFO] Couldn't create perfect hashmap");

    return NULL;
}

void perfect_hashmap_alloc(PerfectHashMap* hm, int32 count, int32 element_size, int32 alignment = 64)
{
    LOG_1("[INFO] Allocating PerfectHashMap for %n elements with %n B per element", {{LOG_DATA_INT32, &count}, {LOG_DATA_INT32, &element_size}});
    hm->map_count = count;
    hm->entry_size = element_size;
    hm->hash_entries = (byte *) platform_alloc_aligned(count * element_size, alignment);
}

void perfect_hashmap_alloc(PerfectHashMapRef* hmr, int32 count, int32 total_data_size, int32 alignment = 64)
{
    hmr->hm.entry_size = sizeof(PerfectHashEntryInt32Int32);
    LOG_1("[INFO] Allocating PerfectHashMap for %n elements with %n B per element", {{LOG_DATA_INT32, &count}, {LOG_DATA_INT32, &hmr->hm.entry_size}});
    hmr->hm.map_count = count;
    hmr->hm.hash_entries = (byte *) platform_alloc_aligned(
        count * hmr->hm.entry_size
        + total_data_size,
        alignment
    );

    hmr->data_pos = 0;
    hmr->data_size = total_data_size;
    hmr->data = hmr->hm.hash_entries + count * hmr->hm.entry_size;
}

void perfect_hashmap_free(PerfectHashMap* hm) {
    platform_aligned_free((void **) &hm->hash_entries);
}

void perfect_hashmap_free(PerfectHashMapRef* hmr) {
    platform_aligned_free((void **) &hmr->hm.hash_entries);
}

// WARNING: element_size = element size + remaining HashEntry data size
void perfect_hashmap_create(PerfectHashMap* hm, int32 count, int32 element_size, BufferMemory* buf)
{
    LOG_1("[INFO] Create PerfectHashMap for %n elements with %n B per element", {{LOG_DATA_INT32, &count}, {LOG_DATA_INT32, &element_size}});
    hm->map_count = count;
    hm->entry_size = element_size;
    hm->hash_entries = buffer_get_memory(
        buf,
        count * element_size,
        64, true
    );
}

// WARNING: element_size = element size + remaining HashEntry data size
void perfect_hashmap_create(PerfectHashMap* hm, int32 count, int32 element_size, byte* buf)
{
    LOG_1("[INFO] Create PerfectHashMap for %n elements with %n B per element", {{LOG_DATA_INT32, &count}, {LOG_DATA_INT32, &element_size}});
    hm->map_count = count;
    hm->entry_size = element_size;
    hm->hash_entries = buf;
}

// Calculates how large a hashmap will be
inline
int64 perfect_hashmap_size(int32 count, int32 element_size)
{
    return count * element_size;
}

inline
int64 perfect_hashmap_size(const PerfectHashMap* hm)
{
    return hm->entry_size * hm->map_count;
}

// @bug the insert functions don't handle too long keys like the HashMap does
inline
void perfect_hashmap_insert(PerfectHashMap* hm, const char* key, int32 value) noexcept {
    int32 index = hm->hash_function(key, hm->hash_seed) % hm->map_count;
    PerfectHashEntryInt32* entry = (PerfectHashEntryInt32 *) (hm->hash_entries + hm->entry_size * index);

    // Ensure key length
    str_move_to_pos(&key, -PERFECT_HASH_MAP_MAX_KEY_LENGTH);
    str_copy_short(entry->key, key, PERFECT_HASH_MAP_MAX_KEY_LENGTH);
    entry->key[PERFECT_HASH_MAP_MAX_KEY_LENGTH - 1] = '\0';

    entry->value = value;
}

inline
void perfect_hashmap_insert(PerfectHashMap* hm, const char* key, int64 value) noexcept {
    int32 index = hm->hash_function(key, hm->hash_seed) % hm->map_count;
    PerfectHashEntryInt64* entry = (PerfectHashEntryInt64 *) (hm->hash_entries + hm->entry_size * index);

    // Ensure key length
    str_move_to_pos(&key, -PERFECT_HASH_MAP_MAX_KEY_LENGTH);
    str_copy_short(entry->key, key, PERFECT_HASH_MAP_MAX_KEY_LENGTH);
    entry->key[PERFECT_HASH_MAP_MAX_KEY_LENGTH - 1] = '\0';

    entry->value = value;
}

inline
void perfect_hashmap_insert(PerfectHashMap* hm, const char* key, int32 value1, int32 value2) noexcept {
    int32 index = hm->hash_function(key, hm->hash_seed) % hm->map_count;
    PerfectHashEntryInt32Int32* entry = (PerfectHashEntryInt32Int32 *) (hm->hash_entries + hm->entry_size * index);

    // Ensure key length
    str_move_to_pos(&key, -PERFECT_HASH_MAP_MAX_KEY_LENGTH);
    str_copy_short(entry->key, key, PERFECT_HASH_MAP_MAX_KEY_LENGTH);
    entry->key[PERFECT_HASH_MAP_MAX_KEY_LENGTH - 1] = '\0';

    entry->value = value1;
    entry->value2 = value2;
}

inline
void perfect_hashmap_insert(PerfectHashMapRef* hmr, const char* key, byte* data, int32 data_size) noexcept {
    if (hmr->data_pos + data_size > hmr->data_size) {
        ASSERT_SIMPLE(hmr->data_pos + data_size <= hmr->data_size);
        return;
    }

    // Insert data
    // NOTE: The data and the hash map entry are in two separate memory areas
    memcpy(hmr->data + hmr->data_pos, data, data_size);

    // Handle hash map entry
    int32 index = hmr->hm.hash_function(key, hmr->hm.hash_seed) % hmr->hm.map_count;
    PerfectHashEntryInt32Int32* entry = (PerfectHashEntryInt32Int32 *) (hmr->hm.hash_entries + hmr->hm.entry_size * index);

    // Ensure key length
    str_move_to_pos(&key, -PERFECT_HASH_MAP_MAX_KEY_LENGTH);
    str_copy_short(entry->key, key, PERFECT_HASH_MAP_MAX_KEY_LENGTH);
    entry->key[PERFECT_HASH_MAP_MAX_KEY_LENGTH - 1] = '\0';

    entry->value = hmr->data_pos;
    entry->value2 = data_size;
    hmr->data_pos += data_size;
}

inline
void perfect_hashmap_insert(PerfectHashMap* hm, const char* key, uintptr_t value) noexcept {
    int32 index = hm->hash_function(key, hm->hash_seed) % hm->map_count;
    PerfectHashEntryUIntPtr* entry = (PerfectHashEntryUIntPtr *) (hm->hash_entries + hm->entry_size * index);

    // Ensure key length
    str_move_to_pos(&key, -PERFECT_HASH_MAP_MAX_KEY_LENGTH);
    str_copy_short(entry->key, key, PERFECT_HASH_MAP_MAX_KEY_LENGTH);
    entry->key[PERFECT_HASH_MAP_MAX_KEY_LENGTH - 1] = '\0';

    entry->value = value;
}

inline
void perfect_hashmap_insert(PerfectHashMap* hm, const char* key, void* value) noexcept {
    int32 index = hm->hash_function(key, hm->hash_seed) % hm->map_count;
    PerfectHashEntryVoidP* entry = (PerfectHashEntryVoidP *) (hm->hash_entries + hm->entry_size * index);

    // Ensure key length
    str_move_to_pos(&key, -PERFECT_HASH_MAP_MAX_KEY_LENGTH);
    str_copy_short(entry->key, key, PERFECT_HASH_MAP_MAX_KEY_LENGTH);
    entry->key[PERFECT_HASH_MAP_MAX_KEY_LENGTH - 1] = '\0';

    entry->value = value;
}

inline
void perfect_hashmap_insert(PerfectHashMap* hm, const char* key, f32 value) noexcept {
    int32 index = hm->hash_function(key, hm->hash_seed) % hm->map_count;
    PerfectHashEntryFloat* entry = (PerfectHashEntryFloat *) (hm->hash_entries + hm->entry_size * index);

    // Ensure key length
    str_move_to_pos(&key, -PERFECT_HASH_MAP_MAX_KEY_LENGTH);
    str_copy_short(entry->key, key, PERFECT_HASH_MAP_MAX_KEY_LENGTH);
    entry->key[PERFECT_HASH_MAP_MAX_KEY_LENGTH - 1] = '\0';

    entry->value = value;
}

inline
void perfect_hashmap_insert(PerfectHashMap* hm, const char* key, const char* value) noexcept {
    int32 index = hm->hash_function(key, hm->hash_seed) % hm->map_count;
    PerfectHashEntryStr* entry = (PerfectHashEntryStr *) (hm->hash_entries + hm->entry_size * index);

    // Ensure key length
    str_move_to_pos(&key, -PERFECT_HASH_MAP_MAX_KEY_LENGTH);
    str_copy_short(entry->key, key, PERFECT_HASH_MAP_MAX_KEY_LENGTH);
    entry->key[PERFECT_HASH_MAP_MAX_KEY_LENGTH - 1] = '\0';

    str_copy_short(entry->value, value, PERFECT_HASH_MAP_MAX_KEY_LENGTH);
}

inline
void perfect_hashmap_insert(PerfectHashMap* hm, const char* key, const byte* value) noexcept {
    int32 index = hm->hash_function(key, hm->hash_seed) % hm->map_count;
    PerfectHashEntryStr* entry = (PerfectHashEntryStr *) (hm->hash_entries + hm->entry_size * index);

    // Ensure key length
    str_move_to_pos(&key, -PERFECT_HASH_MAP_MAX_KEY_LENGTH);
    str_copy_short(entry->key, key, PERFECT_HASH_MAP_MAX_KEY_LENGTH);
    entry->key[PERFECT_HASH_MAP_MAX_KEY_LENGTH - 1] = '\0';

    memcpy(entry->value, value, hm->entry_size - sizeof(PerfectHashEntry));
}

inline
PerfectHashEntry* perfect_hashmap_get_entry(const PerfectHashMap* hm, const char* key) noexcept {
    int32 index = hm->hash_function(key, hm->hash_seed) % hm->map_count;
    PerfectHashEntry* entry = (PerfectHashEntry *) (hm->hash_entries + hm->entry_size * index);

    str_move_to_pos(&key, -HASH_MAP_MAX_KEY_LENGTH);

    return str_compare(entry->key, key) == 0 ? entry : NULL;
}

inline
byte* perfect_hashmap_get_value(const PerfectHashMapRef* hmr, const char* key) noexcept {
    int32 index = hmr->hm.hash_function(key, hmr->hm.hash_seed) % hmr->hm.map_count;
    PerfectHashEntryInt32Int32* entry = (PerfectHashEntryInt32Int32 *) (hmr->hm.hash_entries + hmr->hm.entry_size * index);

    str_move_to_pos(&key, -HASH_MAP_MAX_KEY_LENGTH);

    return str_compare(entry->key, key) == 0 ? hmr->data + entry->value : NULL;
}

inline
void perfect_hashmap_delete_entry(PerfectHashMap* hm, const char* key) noexcept {
    int32 index = hm->hash_function(key, hm->hash_seed) % hm->map_count;
    PerfectHashEntry* entry = (PerfectHashEntry *) (hm->hash_entries + hm->entry_size * index);

    str_move_to_pos(&key, -HASH_MAP_MAX_KEY_LENGTH);

    if (str_compare(entry->key, key) != 0) {
        return;
    }

    // This depends on where we check if an element exists
    // If we change perfect_hashmap_get_entry this also needs changing
    *entry->key = '\0';
}

inline
int64 perfect_hashmap_dump(const PerfectHashMap* hm, byte* data)
{
    byte* start = data;

    *((int32 *) data) = SWAP_ENDIAN_LITTLE(hm->map_count);
    data += sizeof(hm->map_count);

    *((int32 *) data) = SWAP_ENDIAN_LITTLE(hm->hash_seed);
    data += sizeof(hm->hash_seed);

    for (uint32 i = 0; i < ARRAY_COUNT(PERFECT_HASH_FUNCTIONS); ++i) {
        if (hm->hash_function == PERFECT_HASH_FUNCTIONS[i]) {
            *((int32 *) data) = SWAP_ENDIAN_LITTLE((uint64) i);
            data += sizeof(i);

            break;
        }
    }

    *((int32 *) data) = SWAP_ENDIAN_LITTLE(hm->entry_size);
    data += sizeof(hm->entry_size);

    memcpy(data, hm->hash_entries, hm->map_count * hm->entry_size);
    data += hm->map_count * hm->entry_size;

    return (int64) (data - start);
}

// WARNING: Requires perfect_hashmap_create first
inline
int64 perfect_hashmap_load(PerfectHashMap* hm, const byte* data)
{
    const byte* start = data;

    hm->map_count = SWAP_ENDIAN_LITTLE(*((int32 *) data));
    data += sizeof(hm->map_count);

    hm->hash_seed = SWAP_ENDIAN_LITTLE(*((int32 *) data));
    data += sizeof(hm->hash_seed);

    hm->hash_function = PERFECT_HASH_FUNCTIONS[*((int32 *) data)];
    data += sizeof(int32);

    hm->entry_size = SWAP_ENDIAN_LITTLE(*((int32 *) data));
    data += sizeof(hm->entry_size);

    memcpy(hm->hash_entries, data, hm->map_count * hm->entry_size);
    data += hm->map_count * hm->entry_size;

    return (int64) (data - start);
}

// WARNiNG: Requires the phm to be initialized already incl. element count and element size etc.
inline
bool perfect_hashmap_from_hashmap(PerfectHashMap* phm, const HashMap* hm, int32 seed_tries, RingMemory* ring)
{
    char** keys = (char **) ring_get_memory(ring, sizeof(char *) * hm->buf.count, 8);

    // Find all keys
    int32 key_index = 0;
    uint32 chunk_id = 0;
    chunk_iterate_start(&hm->buf, chunk_id) {
        HashEntry* entry = (HashEntry *) chunk_get_element((ChunkMemory *) &hm->buf, chunk_id);
        keys[key_index++] = entry->key;
    } chunk_iterate_end;

    // Check if we can turn it into a perfect hash map
    const PerfectHashMap* is_perfect = perfect_hashmap_prepare(phm, (const char**) keys, key_index, seed_tries, ring);
    if (!is_perfect) {
        return false;
    }

    // Fill perfect hash map
    chunk_id = 0;
    chunk_iterate_start(&hm->buf, chunk_id) {
        const HashEntry* entry = (const HashEntry *) chunk_get_element((ChunkMemory *) &hm->buf, chunk_id);
        perfect_hashmap_insert(phm, entry->key, entry->value);
    } chunk_iterate_end;

    return true;
}

#endif