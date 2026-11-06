#include "../TestFramework.h"
#include "../../stdlib/HashMapT.cpp"
#include "../../memory/RingMemory.cpp"

/**
 * Hash map tests
 *
 * The old HashMap (runtime entry size) was replaced by HashMapT<T> (compile time entry type),
 * so the tests use the entry types directly (@see HashMapT.h)
 */

static void test_hashmap_hash_mix() {
    // Deterministic
    TEST_EQUALS(hashmap_hash_mix(0), hashmap_hash_mix(0));
    TEST_EQUALS(hashmap_hash_mix(1234), hashmap_hash_mix(1234));

    // Different inputs -> different hashes
    TEST_NOT_EQUALS(hashmap_hash_mix(0), hashmap_hash_mix(1));
    TEST_NOT_EQUALS(hashmap_hash_mix(1), hashmap_hash_mix(2));

    // All bits have to influence the result (avalanche)
    TEST_NOT_EQUALS(hashmap_hash_mix(0), hashmap_hash_mix(0x00010000));
    TEST_NOT_EQUALS(hashmap_hash_mix(0), hashmap_hash_mix(0x80000000));
}

static void test_hashmap_str_hash() {
    const char* key = "test1";
    int32 length = 0;
    const uint32 hash = hashmap_str_hash(&key, &length);

    TEST_EQUALS(length, 5);
    TEST_EQUALS(strcmp(key, "test1"), 0);
    TEST_EQUALS(hash, hashmap_str_hash(&key, &length)); // deterministic

    // Different keys -> different hashes
    const char* key2 = "test2";
    int32 length2 = 0;
    TEST_NOT_EQUALS(hash, hashmap_str_hash(&key2, &length2));

    // Keys longer than HASH_MAP_MAX_KEY_LENGTH - 1 are truncated to the tail
    const char* long_key = "0123456789ABCDEFGHIJKLMNOP";
    int32 long_length = 0;
    hashmap_str_hash(&long_key, &long_length);

    TEST_EQUALS(long_length, HASH_MAP_MAX_KEY_LENGTH - 1);
    TEST_EQUALS(strlen(long_key), HASH_MAP_MAX_KEY_LENGTH - 1);

    // The truncated key is the tail of the original key
    const char* full_key = "0123456789ABCDEFGHIJKLMNOP";
    TEST_EQUALS(strcmp(long_key, full_key + (strlen(full_key) - (HASH_MAP_MAX_KEY_LENGTH - 1))), 0);
}

static void test_hashmap_int_hash() {
    // Deterministic
    TEST_EQUALS(hashmap_int_hash((int32) 1), hashmap_int_hash((int32) 1));

    TEST_NOT_EQUALS(hashmap_int_hash((int32) 1), hashmap_int_hash((int32) 2));
    TEST_NOT_EQUALS(hashmap_int_hash((int32) 0), hashmap_int_hash((int32) 1));

    // The same value in different integer types doesn't have to be the same hash,
    // but the hash of a type has to be stable
    TEST_EQUALS(hashmap_int_hash((uint64) 1234), hashmap_int_hash((uint64) 1234));
}

static void test_hashmap_hash_range() {
    // Every count (not only powers of 2) has to produce a valid index
    const uint32 counts[] = {3, 5, 7, 8, 9, 16, 17, 64, 100, 1000, 65535};

    for (uint32 c = 0; c < ARRAY_COUNT(counts); ++c) {
        for (uint32 i = 0; i < 1000; ++i) {
            const uint32 index = hashmap_hash_range(hashmap_hash_mix(i), counts[c]);
            if (index >= counts[c]) {
                TEST_TRUE(false);
                break;
            }
        }
    }

    // The first/last bucket has to be reachable
    TEST_EQUALS(hashmap_hash_range(0, 10), 0);
    TEST_EQUALS(hashmap_hash_range(0xFFFFFFFF, 10), 9);
}

static void test_hashmap_alloc() {
    HashMapT<HashEntryStrT<int32>> hm = {0};
    hashmap_alloc(&hm, 3, 3);

    TEST_TRUE(hm.buf.capacity > 0);
    TEST_EQUALS(hm.buf.last_pos, -1);
    TEST_EQUALS(*hm.buf.free, 0);
    TEST_EQUALS(hashmap_size(&hm), hm.buf.capacity * (int32) sizeof(HashEntryStrT<int32>));

    hashmap_free(&hm);
    TEST_EQUALS(hm.buf.capacity, 0);
    TEST_EQUALS(hm.buf.memory, NULL);
}

static void test_hashmap_insert_str() {
    HashMapT<HashEntryStrT<int32>> hm = {0};
    hashmap_alloc(&hm, 8, 8);

    HashEntryStrT<int32>* entry = hashmap_insert(&hm, "test1", 1);
    TEST_NOT_EQUALS(entry, NULL);
    TEST_EQUALS(entry->value, 1);
    TEST_EQUALS(strcmp(entry->key, "test1"), 0);

    entry = hashmap_entry_get(&hm, "test1");
    TEST_NOT_EQUALS(entry, NULL);
    TEST_EQUALS(entry->value, 1);

    hashmap_insert(&hm, "test2", 2);
    entry = hashmap_entry_get(&hm, "test2");
    TEST_NOT_EQUALS(entry, NULL);
    TEST_EQUALS(entry->value, 2);

    hashmap_insert(&hm, "test3", 3);
    entry = hashmap_entry_get(&hm, "test3");
    TEST_NOT_EQUALS(entry, NULL);
    TEST_EQUALS(entry->value, 3);

    // Every entry is still reachable
    TEST_EQUALS(hashmap_entry_get(&hm, "test1")->value, 1);
    TEST_EQUALS(hashmap_entry_get(&hm, "test2")->value, 2);

    entry = hashmap_entry_get(&hm, "invalid");
    TEST_EQUALS(entry, NULL);

    // A removed entry is really gone
    TEST_EQUALS(hashmap_entry_get(&hm, ""), NULL);

    hashmap_free(&hm);
}

static void test_hashmap_remove() {
    HashMapT<HashEntryStrT<int32>> hm = {0};
    hashmap_alloc(&hm, 8, 8);

    hashmap_insert(&hm, "test1", 1);
    hashmap_insert(&hm, "test2", 2);

    hashmap_remove(&hm, "test2");
    TEST_EQUALS(hashmap_entry_get(&hm, "test2"), NULL);

    // The other entries are unaffected
    HashEntryStrT<int32>* entry = hashmap_entry_get(&hm, "test1");
    TEST_NOT_EQUALS(entry, NULL);
    TEST_EQUALS(entry->value, 1);

    // Remove a non existing entry
    hashmap_remove(&hm, "invalid");
    TEST_NOT_EQUALS(hashmap_entry_get(&hm, "test1"), NULL);

    // The freed slot can be reused
    hashmap_insert(&hm, "test3", 3);
    entry = hashmap_entry_get(&hm, "test3");
    TEST_NOT_EQUALS(entry, NULL);
    TEST_EQUALS(entry->value, 3);

    // Remove all entries
    hashmap_remove(&hm, "test1");
    hashmap_remove(&hm, "test3");

    TEST_EQUALS(hashmap_entry_get(&hm, "test1"), NULL);
    TEST_EQUALS(hashmap_entry_get(&hm, "test3"), NULL);

    hashmap_free(&hm);
}

static void test_hashmap_reserve() {
    HashMapT<HashEntryStrT<int32>> hm = {0};
    hashmap_alloc(&hm, 8, 8);

    HashEntryStrT<int32>* entry = hashmap_reserve(&hm, "test1");
    TEST_NOT_EQUALS(entry, NULL);
    TEST_EQUALS(entry->value, 0);

    entry->value = 42;
    TEST_EQUALS(hashmap_entry_get(&hm, "test1")->value, 42);

    // get_reserve returns the existing entry instead of creating a new one
    HashEntryStrT<int32>* reserved = hashmap_get_reserve(&hm, "test1");
    TEST_EQUALS(reserved, hashmap_entry_get(&hm, "test1"));
    TEST_EQUALS(reserved->value, 42);

    // get_reserve creates + zero initializes a missing entry
    HashEntryStrT<int32>* created = hashmap_get_reserve(&hm, "test2");
    TEST_NOT_EQUALS(created, NULL);
    TEST_EQUALS(created->value, 0);

    hashmap_free(&hm);
}

// Many entries -> collisions, chain walking and full capacity
static void test_hashmap_many() {
    HashMapT<HashEntryStrT<int32>> hm = {0};
    hashmap_alloc(&hm, 256, 256);

    char key[32];

    for (int32 i = 0; i < 256; ++i) {
        sprintf_fast(key, sizeof(key), "key_%d", i);
        TEST_NOT_EQUALS(hashmap_insert(&hm, key, i), NULL);
    }

    // The map is full now
    TEST_EQUALS(hashmap_insert(&hm, "overflow", 1), NULL);

    for (int32 i = 0; i < 256; ++i) {
        sprintf_fast(key, sizeof(key), "key_%d", i);
        HashEntryStrT<int32>* entry = hashmap_entry_get(&hm, key);
        TEST_NOT_EQUALS(entry, NULL);
        TEST_EQUALS(entry->value, i);
    }

    // Removing half of the entries keeps the other half reachable
    for (int32 i = 0; i < 256; i += 2) {
        sprintf_fast(key, sizeof(key), "key_%d", i);
        hashmap_remove(&hm, key);
    }

    for (int32 i = 0; i < 256; ++i) {
        sprintf_fast(key, sizeof(key), "key_%d", i);
        HashEntryStrT<int32>* entry = hashmap_entry_get(&hm, key);

        if (i % 2 == 0) {
            TEST_EQUALS(entry, NULL);
        } else {
            TEST_NOT_EQUALS(entry, NULL);
            TEST_EQUALS(entry->value, i);
        }
    }

    hashmap_free(&hm);
}

// Long keys are stored truncated (last HASH_MAP_MAX_KEY_LENGTH - 1 characters)
static void test_hashmap_long_keys() {
    HashMapT<HashEntryStrT<int32>> hm = {0};
    hashmap_alloc(&hm, 8, 8);

    hashmap_insert(&hm, "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ", 1);

    // The same tail -> the same stored key (and therefore the same entry)
    HashEntryStrT<int32>* entry = hashmap_entry_get(&hm, "QQQQQQQQQQQABCDEFGHIJKLMNOPQRSTUVWXYZ");
    TEST_NOT_EQUALS(entry, NULL);
    TEST_EQUALS(entry->value, 1);

    hashmap_free(&hm);
}

static void test_hashmap_int_key() {
    HashMapT<HashEntryT<int32, int32>> hm = {0};
    hashmap_alloc(&hm, 16, 16);

    for (int32 i = 0; i < 16; ++i) {
        HashEntryT<int32, int32>* entry = hashmap_insert(&hm, i * 7, i * 100);
        TEST_NOT_EQUALS(entry, NULL);
        TEST_EQUALS(entry->key, i * 7);
        TEST_EQUALS(entry->value, i * 100);
    }

    for (int32 i = 0; i < 16; ++i) {
        HashEntryT<int32, int32>* entry = hashmap_entry_get(&hm, i * 7);
        TEST_NOT_EQUALS(entry, NULL);
        TEST_EQUALS(entry->value, i * 100);
    }

    hashmap_remove(&hm, 7);
    TEST_EQUALS(hashmap_entry_get(&hm, 7), NULL);
    TEST_NOT_EQUALS(hashmap_entry_get(&hm, 14), NULL);

    hashmap_free(&hm);
}

static void test_hashmap_dump_load() {
    HashMapT<HashEntryStrT<int32>> hm_dump = {0};
    hashmap_alloc(&hm_dump, 8, 8);

    hashmap_insert(&hm_dump, "test1", 1);
    hashmap_insert(&hm_dump, "test2", 2);
    hashmap_insert(&hm_dump, "test3", 3);

    RingMemory ring;
    ring_alloc(&ring, 1024 * 1024, 1024 * 1024);
    byte* out = memory_get(&ring, 1024 * 1024, sizeof(size_t));

    const int64 dump_size = hashmap_dump(&hm_dump, out);

    HashMapT<HashEntryStrT<int32>> hm_load = {0};
    hashmap_alloc(&hm_load, 8, 8);

    const int64 load_size = hashmap_load(&hm_load, out);

    TEST_EQUALS(dump_size, load_size);
    TEST_MEMORY_EQUALS(hm_dump.buf.memory, hm_load.buf.memory, hm_dump.buf.capacity * sizeof(HashEntryStrT<int32>));

    // The loaded map is usable
    TEST_EQUALS(hashmap_entry_get(&hm_load, "test1")->value, 1);
    TEST_EQUALS(hashmap_entry_get(&hm_load, "test2")->value, 2);
    TEST_EQUALS(hashmap_entry_get(&hm_load, "test3")->value, 3);
    TEST_EQUALS(hashmap_entry_get(&hm_load, "test4"), NULL);

    hashmap_free(&hm_dump);
    hashmap_free(&hm_load);

    ring_free(&ring);
}

#if PERFORMANCE_TEST
#include "../../utils/RandomUtils.h"

// Testing our hash map implementation vs a hash map implementation mentioned in most books
#define MAX_TEST_KEY_LENGTH 26

typedef struct {
    char key[MAX_TEST_KEY_LENGTH];
    int32 value;
    int used;   // 0 = empty, 1 = used, -1 = tombstone (removed)
} HashEntryStd;

typedef struct {
    HashEntryStd* entries;
    uint32 capacity;
    uint32 count;
} HashMapStd;

static HashMapStd* hashmap_test_create(uint32 capacity) {
    HashMapStd* map = (HashMapStd *) platform_alloc_aligned(sizeof(HashMapStd));
    map->capacity = capacity;
    map->count = 0;
    map->entries = (HashEntryStd *) platform_alloc_aligned(capacity * sizeof(HashEntryStd));

    return map;
}

static void hashmap_test_free(HashMapStd* map) {
    platform_aligned_free((void **) &map->entries);
    platform_aligned_free((void **) &map);
}

static HashEntryStd* open_test_insert(HashMapStd* map, const char* key, int32 value) {
    const uint64 hash = hashmap_int_hash(key[0]);
    const uint32 index = (uint32) (hash % map->capacity);

    for (uint32 i = 0; i < map->capacity; i++) {
        const uint32 probe = (index + i) % map->capacity;
        HashEntryStd* entry = &map->entries[probe];

        if (entry->used == 0 || entry->used == -1) {
            strncpy(entry->key, key, MAX_TEST_KEY_LENGTH - 1);
            entry->key[MAX_TEST_KEY_LENGTH - 1] = '\0';
            entry->value = value;
            entry->used = 1;
            ++map->count;

            return entry;
        }
    }

    return NULL;
}

static HashEntryStd* hashmap_test_get(HashMapStd* map, const char* key) {
    const uint64 hash = hashmap_int_hash(key[0]);
    const uint32 index = (uint32) (hash % map->capacity);

    for (uint32 i = 0; i < map->capacity; i++) {
        const uint32 probe = (index + i) % map->capacity;
        HashEntryStd* entry = &map->entries[probe];

        if (entry->used == 0) {
            return NULL;
        } else if (entry->used == 1 && strcmp(entry->key, key) == 0) {
            return entry;
        }
    }

    return NULL;
}

#define HASH_MAP_MAX_COUNT (1 << 10)
#define HASH_MAP_TEST_COUNT (1 << 9)

static void _my_hashmap(MAYBE_UNUSED volatile void* val) {
    HashMapT<HashEntryStrT<int32>> map = {0};
    hashmap_alloc(&map, HASH_MAP_MAX_COUNT, HASH_MAP_MAX_COUNT);
    char test_key[20];

    size_t total = 0;

    for (int32 i = 0; i < HASH_MAP_TEST_COUNT; ++i) {
        random_string("ABCDEF0123456789", 16, test_key, 19);

        hashmap_insert(&map, test_key, 1);
        HashEntryStrT<int32>* entry = hashmap_entry_get(&map, test_key);
        total += entry->value;
    }

    TEST_TRUE(total);

    hashmap_free(&map);
}

static void _open_hashmap(MAYBE_UNUSED volatile void* val) {
    HashMapStd* map = hashmap_test_create(HASH_MAP_MAX_COUNT);
    char test_key[20];

    size_t total = 0;

    for (int32 i = 0; i < HASH_MAP_TEST_COUNT; ++i) {
        random_string("ABCDEF0123456789", 16, test_key, 19);

        open_test_insert(map, test_key, 1);
        HashEntryStd* entry = hashmap_test_get(map, test_key);
        total += entry->value;
    }

    TEST_TRUE(total);

    hashmap_test_free(map);
}

static void test_hash_map_performance() {
    COMPARE_FUNCTION_TEST_TIME(_my_hashmap, _open_hashmap, -500.0);
}
#endif

#ifdef UBER_TEST
    #ifdef main
        #undef main
    #endif
    #define main StdlibHashMapTest
#endif

int main() {
    TEST_INIT(64);

    TEST_RUN(test_hashmap_hash_mix);
    TEST_RUN(test_hashmap_str_hash);
    TEST_RUN(test_hashmap_int_hash);
    TEST_RUN(test_hashmap_hash_range);
    TEST_RUN(test_hashmap_alloc);
    TEST_RUN(test_hashmap_insert_str);
    TEST_RUN(test_hashmap_remove);
    TEST_RUN(test_hashmap_reserve);
    TEST_RUN(test_hashmap_many);
    TEST_RUN(test_hashmap_long_keys);
    TEST_RUN(test_hashmap_int_key);
    TEST_RUN(test_hashmap_dump_load);

    #if PERFORMANCE_TEST
        TEST_RUN(test_hash_map_performance);
    #endif

    TEST_FINALIZE();

    return 0;
}
