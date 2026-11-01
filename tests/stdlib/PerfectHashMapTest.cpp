#include "../TestFramework.h"
#include "../../stdlib/PerfectHashMap.h"
#include "../../utils/StringUtils.h"

/**
 * PerfectHashMap tests
 *
 * The perfect hash map is a static map: prepare() searches a (hash function, seed) pair
 * that maps every key to its own slot, afterwards insert()/entry_get()/delete_entry() are
 * O(1) and never resize.
 *
 * The tests use the real API (alloc/init + prepare + the typed insert overloads) and cover
 * the seeded hash functions / the seed search algorithm the map is built on.
 *
 * NOTE: long keys are hashed completely but only their last PERFECT_HASH_MAP_MAX_KEY_LENGTH - 1
 * characters are stored (perfect_hashmap_key_tail). Two keys that only share their tail are
 * therefore different entries in the map, the tail is just the stored/comparison part.
 */

#define TEST_PERFECT_MAP_MAX_KEYS 32
#define TEST_PERFECT_MAP_KEY_BUFFER 48
#define TEST_PERFECT_MAP_TABLE_SIZE 256
#define TEST_PERFECT_MAP_SEED_TRIES 1000

// Same list as PERFECT_HASH_FUNCTIONS
typedef uint64 (*TestPerfectHashFunction)(const char* key, int32 seed) NO_EXCEPT;

static const TestPerfectHashFunction TEST_PERFECT_HASH_FUNCTIONS[] = {
    hash_djb2_seeded,
    hash_sdbm_seeded,
    hash_lose_lose_seeded,
    hash_polynomial_rolling_seeded,
    hash_fnv1a_seeded,
    hash_oat_seeded,
    hash_ejb_seeded
};

static const char* test_perfect_map_key_pointers[TEST_PERFECT_MAP_MAX_KEYS] = {0};

static void test_perfect_map_fill_keys(char keys[][TEST_PERFECT_MAP_KEY_BUFFER], int32 count, const char* prefix) {
    for (int32 i = 0; i < count; ++i) {
        sprintf_fast(keys[i], TEST_PERFECT_MAP_KEY_BUFFER, "%s_%d", prefix, i);
    }
}

static const char** test_perfect_map_keys(char keys[][TEST_PERFECT_MAP_KEY_BUFFER], int32 count) {
    for (int32 i = 0; i < count; ++i) {
        test_perfect_map_key_pointers[i] = keys[i];
    }

    return test_perfect_map_key_pointers;
}

// Slot of a key as PerfectHashMap computes it
static uint32 test_perfect_map_index(const PerfectHashMap* hm, const char* key) {
    return (uint32) (hm->hash_function(key, hm->hash_seed) % hm->map_count);
}

// Typed view of the entry of a key (the map stores raw bytes, the type is defined by the insert overload)
template <typename T>
static T* test_perfect_map_entry(const PerfectHashMap* hm, const char* key) {
    return (T *) (hm->hash_entries + hm->entry_size * test_perfect_map_index(hm, key));
}

static PerfectHashMap* test_perfect_map_prepare(
    PerfectHashMap* hm,
    int32 map_count,
    int32 element_size,
    const char** keys,
    int32 key_count,
    RingMemory* ring
) {
    perfect_hashmap_alloc(hm, map_count, element_size);

    return perfect_hashmap_prepare(hm, keys, key_count, TEST_PERFECT_MAP_SEED_TRIES, ring);
}

static void test_perfect_hashmap_key_tail() {
    // Short keys are returned as is (the old implementation moved the pointer before the string)
    const char* short_key = "k0";
    TEST_EQUALS(perfect_hashmap_key_tail(short_key), short_key);
    TEST_EQUALS(strcmp(perfect_hashmap_key_tail(short_key), "k0"), 0);

    const char empty_key[] = "";
    TEST_EQUALS(perfect_hashmap_key_tail(empty_key), empty_key);

    // A key of exactly the maximum length (27 characters) is not truncated
    const char* exact_key = "0123456789ABCDEFGHIJKLMNOPQ";   // 27 chars
    TEST_EQUALS((int32) strlen(exact_key), PERFECT_HASH_MAP_MAX_KEY_LENGTH - 1);
    TEST_EQUALS(perfect_hashmap_key_tail(exact_key), exact_key);

    // 28 characters are already truncated
    const char* one_too_long = "0123456789ABCDEFGHIJKLMNOPQR";   // 28 chars
    TEST_NOT_EQUALS(perfect_hashmap_key_tail(one_too_long), one_too_long);
    TEST_EQUALS(strcmp(perfect_hashmap_key_tail(one_too_long), "123456789ABCDEFGHIJKLMNOPQR"), 0);

    // Long keys are cut down to their tail
    const char* long_key = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";   // 36 chars
    const char* tail = perfect_hashmap_key_tail(long_key);
    TEST_EQUALS((int32) strlen(tail), PERFECT_HASH_MAP_MAX_KEY_LENGTH - 1);
    TEST_EQUALS(strcmp(tail, "9ABCDEFGHIJKLMNOPQRSTUVWXYZ"), 0);

    // The tail is inside the original key
    TEST_TRUE(tail >= long_key);
    TEST_TRUE(tail < long_key + strlen(long_key));
}

static void test_perfect_hashmap_alloc_size() {
    PerfectHashMap hm = {0};
    perfect_hashmap_alloc(&hm, 32, sizeof(PerfectHashEntryInt32));

    TEST_EQUALS(hm.map_count, 32);
    TEST_EQUALS(hm.entry_size, sizeof(PerfectHashEntryInt32));
    TEST_NOT_EQUALS(hm.hash_entries, NULL);
    TEST_EQUALS(perfect_hashmap_size(&hm), 32 * (int64) sizeof(PerfectHashEntryInt32));
    TEST_EQUALS(perfect_hashmap_size(32, sizeof(PerfectHashEntryInt32)), 32 * (int64) sizeof(PerfectHashEntryInt32));

    // The entry memory is zero initialized -> every key is empty
    for (int32 i = 0; i < 32; ++i) {
        TEST_EQUALS(hm.hash_entries[i * hm.entry_size], 0);
    }

    // The memory is aligned like a platform allocation
    TEST_TRUE(((uintptr_t) hm.hash_entries & (sizeof(size_t) - 1)) == 0);

    perfect_hashmap_free(&hm);
    TEST_EQUALS(hm.hash_entries, NULL);
}

static void test_perfect_hashmap_prepare_int32() {
    RingMemory ring;
    ring_alloc(&ring, 1024 * 1024, 1024 * 1024);

    char keys[TEST_PERFECT_MAP_MAX_KEYS][TEST_PERFECT_MAP_KEY_BUFFER] = {{0}};
    const int32 key_count = 16;
    test_perfect_map_fill_keys(keys, key_count, "asset");

    PerfectHashMap hm = {0};
    PerfectHashMap* prepared = test_perfect_map_prepare(
        &hm, TEST_PERFECT_MAP_TABLE_SIZE, sizeof(PerfectHashEntryInt32),
        test_perfect_map_keys(keys, key_count), key_count, &ring
    );

    TEST_NOT_EQUALS(prepared, NULL);
    TEST_EQUALS(prepared, &hm);
    TEST_NOT_EQUALS(hm.hash_function, NULL);

    // Every key has its own slot
    bool used[TEST_PERFECT_MAP_TABLE_SIZE] = {false};
    for (int32 i = 0; i < key_count; ++i) {
        const uint32 index = test_perfect_map_index(&hm, keys[i]);
        TEST_TRUE(index < (uint32) hm.map_count);
        TEST_FALSE(used[index]);
        used[index] = true;
    }

    // Insert all keys
    for (int32 i = 0; i < key_count; ++i) {
        perfect_hashmap_insert(&hm, keys[i], (int32) (i * 3));
    }

    // Every inserted key is reachable and stores key + value
    for (int32 i = 0; i < key_count; ++i) {
        PerfectHashEntry* entry = perfect_hashmap_entry_get(&hm, keys[i]);
        TEST_NOT_EQUALS(entry, NULL);
        TEST_EQUALS(strcmp(entry->key, keys[i]), 0);

        PerfectHashEntryInt32* typed = test_perfect_map_entry<PerfectHashEntryInt32>(&hm, keys[i]);
        TEST_EQUALS(typed->value, i * 3);
        TEST_EQUALS(strcmp(typed->key, keys[i]), 0);
    }

    // A key that isn't in the map returns NULL
    TEST_EQUALS(perfect_hashmap_entry_get(&hm, "missing_0"), NULL);

    // NOTE: the empty key is NOT tested here on purpose. The map has no occupancy flag:
    // an unused slot has key[0] == '\0', so looking up "" matches the first unused slot of
    // the table and returns a pointer to that (empty) entry instead of NULL.
    // The same marker is used to delete entries (perfect_hashmap_delete_entry).

    // Overwriting a slot keeps the map consistent
    perfect_hashmap_insert(&hm, keys[0], 999);
    TEST_EQUALS(perfect_hashmap_entry_get(&hm, keys[0])->key[0], (int64) keys[0][0]);
    TEST_EQUALS(test_perfect_map_entry<PerfectHashEntryInt32>(&hm, keys[0])->value, 999);

    perfect_hashmap_free(&hm);
    ring_free(&ring);
}

static void test_perfect_hashmap_insert_types() {
    RingMemory ring;
    ring_alloc(&ring, 1024 * 1024, 1024 * 1024);

    char keys[TEST_PERFECT_MAP_MAX_KEYS][TEST_PERFECT_MAP_KEY_BUFFER] = {{0}};
    const int32 key_count = 8;
    test_perfect_map_fill_keys(keys, key_count, "type");
    const char** key_pointers = test_perfect_map_keys(keys, key_count);

    int32 payload[key_count] = {0};
    for (int32 i = 0; i < key_count; ++i) {
        payload[i] = i * 7;
    }

    // int64
    {
        PerfectHashMap hm = {0};
        TEST_NOT_EQUALS(test_perfect_map_prepare(&hm, 64, sizeof(PerfectHashEntryInt64), key_pointers, key_count, &ring), NULL);

        for (int32 i = 0; i < key_count; ++i) {
            perfect_hashmap_insert(&hm, keys[i], (int64) 1000000007LL * (i + 1));
        }

        for (int32 i = 0; i < key_count; ++i) {
            TEST_NOT_EQUALS(perfect_hashmap_entry_get(&hm, keys[i]), NULL);
            TEST_EQUALS(
                (long long) test_perfect_map_entry<PerfectHashEntryInt64>(&hm, keys[i])->value,
                (long long) (1000000007LL * (i + 1))
            );
            TEST_EQUALS((int32) sizeof(PerfectHashEntryInt64), (int32) hm.entry_size);
        }

        perfect_hashmap_free(&hm);
    }

    // int32 + int32
    {
        PerfectHashMap hm = {0};
        TEST_NOT_EQUALS(test_perfect_map_prepare(&hm, 64, sizeof(PerfectHashEntryInt32Int32), key_pointers, key_count, &ring), NULL);

        for (int32 i = 0; i < key_count; ++i) {
            perfect_hashmap_insert(&hm, keys[i], i, i * i);
        }

        for (int32 i = 0; i < key_count; ++i) {
            PerfectHashEntryInt32Int32* entry = test_perfect_map_entry<PerfectHashEntryInt32Int32>(&hm, keys[i]);
            TEST_EQUALS(entry->value, i);
            TEST_EQUALS(entry->value2, i * i);
            TEST_NOT_EQUALS(perfect_hashmap_entry_get(&hm, keys[i]), NULL);
        }

        perfect_hashmap_free(&hm);
    }

    // uintptr_t
    {
        PerfectHashMap hm = {0};
        TEST_NOT_EQUALS(test_perfect_map_prepare(&hm, 64, sizeof(PerfectHashEntryUIntPtr), key_pointers, key_count, &ring), NULL);

        for (int32 i = 0; i < key_count; ++i) {
            perfect_hashmap_insert(&hm, keys[i], (uintptr_t) (0x1000 + i));
        }

        for (int32 i = 0; i < key_count; ++i) {
            TEST_EQUALS(
                (long long) test_perfect_map_entry<PerfectHashEntryUIntPtr>(&hm, keys[i])->value,
                (long long) (0x1000 + i)
            );
        }

        perfect_hashmap_free(&hm);
    }

    // void*
    {
        PerfectHashMap hm = {0};
        TEST_NOT_EQUALS(test_perfect_map_prepare(&hm, 64, sizeof(PerfectHashEntryVoidP), key_pointers, key_count, &ring), NULL);

        for (int32 i = 0; i < key_count; ++i) {
            perfect_hashmap_insert(&hm, keys[i], (void *) &payload[i]);
        }

        for (int32 i = 0; i < key_count; ++i) {
            TEST_EQUALS(test_perfect_map_entry<PerfectHashEntryVoidP>(&hm, keys[i])->value, &payload[i]);
            TEST_EQUALS(perfect_hashmap_entry_get(&hm, keys[i])->value, (byte *) &payload[i]);
        }

        perfect_hashmap_free(&hm);
    }

    // f32
    {
        PerfectHashMap hm = {0};
        TEST_NOT_EQUALS(test_perfect_map_prepare(&hm, 64, sizeof(PerfectHashEntryFloat), key_pointers, key_count, &ring), NULL);

        for (int32 i = 0; i < key_count; ++i) {
            perfect_hashmap_insert(&hm, keys[i], (f32) i * 0.5f);
        }

        for (int32 i = 0; i < key_count; ++i) {
            TEST_EQUALS_WITH_DELTA(test_perfect_map_entry<PerfectHashEntryFloat>(&hm, keys[i])->value, (f32) i * 0.5f, 0.000001);
        }

        perfect_hashmap_free(&hm);
    }

    // const char*
    {
        PerfectHashMap hm = {0};
        TEST_NOT_EQUALS(test_perfect_map_prepare(&hm, 64, sizeof(PerfectHashEntryStr), key_pointers, key_count, &ring), NULL);

        char values[key_count][32] = {{0}};
        for (int32 i = 0; i < key_count; ++i) {
            sprintf_fast(values[i], 32, "value_%d", i);
            perfect_hashmap_insert(&hm, keys[i], (const char *) values[i]);
        }

        for (int32 i = 0; i < key_count; ++i) {
            TEST_EQUALS(strcmp(test_perfect_map_entry<PerfectHashEntryStr>(&hm, keys[i])->value, values[i]), 0);
        }

        perfect_hashmap_free(&hm);
    }

    ring_free(&ring);
}

static void test_perfect_hashmap_delete() {
    RingMemory ring;
    ring_alloc(&ring, 1024 * 1024, 1024 * 1024);

    char keys[TEST_PERFECT_MAP_MAX_KEYS][TEST_PERFECT_MAP_KEY_BUFFER] = {{0}};
    const int32 key_count = 8;
    test_perfect_map_fill_keys(keys, key_count, "del");

    PerfectHashMap hm = {0};
    TEST_NOT_EQUALS(
        test_perfect_map_prepare(&hm, 64, sizeof(PerfectHashEntryInt32), test_perfect_map_keys(keys, key_count), key_count, &ring),
        NULL
    );

    for (int32 i = 0; i < key_count; ++i) {
        perfect_hashmap_insert(&hm, keys[i], i);
    }

    // Delete one entry -> only that key is gone
    perfect_hashmap_delete_entry(&hm, keys[3]);
    TEST_EQUALS(perfect_hashmap_entry_get(&hm, keys[3]), NULL);

    for (int32 i = 0; i < key_count; ++i) {
        if (i == 3) {
            continue;
        }

        TEST_NOT_EQUALS(perfect_hashmap_entry_get(&hm, keys[i]), NULL);
        TEST_EQUALS(test_perfect_map_entry<PerfectHashEntryInt32>(&hm, keys[i])->value, i);
    }

    // Deleting again (or a key that was never inserted) is a no-op
    perfect_hashmap_delete_entry(&hm, keys[3]);
    perfect_hashmap_delete_entry(&hm, "never_inserted");
    TEST_EQUALS(perfect_hashmap_entry_get(&hm, keys[3]), NULL);
    TEST_NOT_EQUALS(perfect_hashmap_entry_get(&hm, keys[4]), NULL);

    // The slot can be reused
    perfect_hashmap_insert(&hm, keys[3], 300);
    TEST_NOT_EQUALS(perfect_hashmap_entry_get(&hm, keys[3]), NULL);
    TEST_EQUALS(test_perfect_map_entry<PerfectHashEntryInt32>(&hm, keys[3])->value, 300);

    perfect_hashmap_free(&hm);
    ring_free(&ring);
}

static void test_perfect_hashmap_long_keys() {
    RingMemory ring;
    ring_alloc(&ring, 1024 * 1024, 1024 * 1024);

    char keys[TEST_PERFECT_MAP_MAX_KEYS][TEST_PERFECT_MAP_KEY_BUFFER] = {{0}};
    const int32 key_count = 4;

    // Short keys
    strcpy(keys[0], "a");
    strcpy(keys[1], "ab");

    // Keys longer than the stored maximum (27 characters)
    strcpy(keys[2], "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ");
    strcpy(keys[3], "xyz_0123456789ABCDEFGHIJKLMNOPQRSTUV");

    PerfectHashMap hm = {0};
    TEST_NOT_EQUALS(
        test_perfect_map_prepare(&hm, 64, sizeof(PerfectHashEntryInt32), test_perfect_map_keys(keys, key_count), key_count, &ring),
        NULL
    );

    for (int32 i = 0; i < key_count; ++i) {
        perfect_hashmap_insert(&hm, keys[i], i);
    }

    for (int32 i = 0; i < key_count; ++i) {
        TEST_NOT_EQUALS(perfect_hashmap_entry_get(&hm, keys[i]), NULL);
        TEST_EQUALS(test_perfect_map_entry<PerfectHashEntryInt32>(&hm, keys[i])->value, i);
    }

    // Short keys are stored unchanged
    TEST_EQUALS(strcmp(test_perfect_map_entry<PerfectHashEntryInt32>(&hm, "a")->key, "a"), 0);
    TEST_EQUALS(strcmp(test_perfect_map_entry<PerfectHashEntryInt32>(&hm, "ab")->key, "ab"), 0);

    // Long keys only keep their tail
    const char* tail = perfect_hashmap_key_tail(keys[2]);
    TEST_EQUALS((int32) strlen(tail), PERFECT_HASH_MAP_MAX_KEY_LENGTH - 1);
    TEST_EQUALS(strcmp(test_perfect_map_entry<PerfectHashEntryInt32>(&hm, keys[2])->key, tail), 0);
    TEST_EQUALS(strcmp(tail, "9ABCDEFGHIJKLMNOPQRSTUVWXYZ"), 0);

    // The stored key is always terminated
    PerfectHashEntryInt32* long_entry = test_perfect_map_entry<PerfectHashEntryInt32>(&hm, keys[2]);
    TEST_EQUALS(long_entry->key[PERFECT_HASH_MAP_MAX_KEY_LENGTH - 1], 0);

    perfect_hashmap_free(&hm);
    ring_free(&ring);
}

static void test_perfect_hashmap_prepare_failure() {
    // A perfect hash is impossible if there are more keys than slots -> prepare returns NULL
    RingMemory ring;
    ring_alloc(&ring, 1024 * 1024, 1024 * 1024);

    char keys[TEST_PERFECT_MAP_MAX_KEYS][TEST_PERFECT_MAP_KEY_BUFFER] = {{0}};
    const int32 key_count = 8;
    test_perfect_map_fill_keys(keys, key_count, "fail");

    PerfectHashMap hm = {0};
    perfect_hashmap_alloc(&hm, 4, sizeof(PerfectHashEntryInt32));

    // ASSERT_THROW() crashes a debug build on purpose
    #if !DEBUG
        TEST_EQUALS(perfect_hashmap_prepare(&hm, test_perfect_map_keys(keys, key_count), key_count, 10, &ring), NULL);
    #endif

    perfect_hashmap_free(&hm);
    ring_free(&ring);
}

static void test_perfect_hashmap_dump_load() {
    RingMemory ring;
    ring_alloc(&ring, 1024 * 1024, 1024 * 1024);

    char keys[TEST_PERFECT_MAP_MAX_KEYS][TEST_PERFECT_MAP_KEY_BUFFER] = {{0}};
    const int32 key_count = 12;
    test_perfect_map_fill_keys(keys, key_count, "dump");

    PerfectHashMap hm = {0};
    TEST_NOT_EQUALS(
        test_perfect_map_prepare(&hm, 128, sizeof(PerfectHashEntryInt32), test_perfect_map_keys(keys, key_count), key_count, &ring),
        NULL
    );

    for (int32 i = 0; i < key_count; ++i) {
        perfect_hashmap_insert(&hm, keys[i], i * 11);
    }

    byte dump_buffer[8192] = {0};
    const int64 dump_size = perfect_hashmap_dump(&hm, dump_buffer);

    // 4 * int32 header (map_count, hash_seed, hash function index, entry_size) + entries
    TEST_EQUALS(dump_size, 16 + hm.map_count * (int64) hm.entry_size);
    TEST_TRUE(dump_size <= (int64) sizeof(dump_buffer));

    // Load into a second, separately initialized instance
    byte load_buffer[8192] = {0};
    PerfectHashMap loaded = {0};
    perfect_hashmap_init(&loaded, hm.map_count, hm.entry_size, load_buffer);

    TEST_EQUALS(loaded.hash_entries, load_buffer);

    const int64 load_size = perfect_hashmap_load(&loaded, dump_buffer);
    TEST_EQUALS(load_size, dump_size);
    TEST_EQUALS(loaded.map_count, hm.map_count);
    TEST_EQUALS(loaded.entry_size, hm.entry_size);
    TEST_EQUALS(loaded.hash_seed, hm.hash_seed);
    TEST_EQUALS(loaded.hash_function, hm.hash_function);

    // The loaded map has to contain the same entries
    for (int32 i = 0; i < key_count; ++i) {
        TEST_NOT_EQUALS(perfect_hashmap_entry_get(&loaded, keys[i]), NULL);
        TEST_EQUALS(test_perfect_map_entry<PerfectHashEntryInt32>(&loaded, keys[i])->value, i * 11);
        TEST_EQUALS(strcmp(test_perfect_map_entry<PerfectHashEntryInt32>(&loaded, keys[i])->key, keys[i]), 0);
    }

    TEST_EQUALS(perfect_hashmap_entry_get(&loaded, "dump_missing"), NULL);
    TEST_MEMORY_EQUALS(hm.hash_entries, loaded.hash_entries, hm.map_count * hm.entry_size);

    perfect_hashmap_free(&hm);
    ring_free(&ring);
}

static void test_perfect_hashmap_ref() {
    RingMemory ring;
    ring_alloc(&ring, 1024 * 1024, 1024 * 1024);

    char keys[TEST_PERFECT_MAP_MAX_KEYS][TEST_PERFECT_MAP_KEY_BUFFER] = {{0}};
    const int32 key_count = 6;
    test_perfect_map_fill_keys(keys, key_count, "blob");

    // The ref map stores offset + size in the entry and the data in a separate area
    PerfectHashMapRef ref = {0};
    TEST_NOT_EQUALS(
        test_perfect_map_prepare(&ref.hm, 64, sizeof(PerfectHashEntryInt32Int32), test_perfect_map_keys(keys, key_count), key_count, &ring),
        NULL
    );

    byte data_area[512] = {0};
    ref.data = data_area;
    ref.data_pos = 0;
    ref.data_size = (int32) sizeof(data_area);

    byte values[key_count][8] = {{0}};
    for (int32 i = 0; i < key_count; ++i) {
        const char* text = "blob_value";
        memcpy(values[i], text, 8);
        values[i][6] = (byte) ('0' + i);

        perfect_hashmap_insert(&ref, keys[i], values[i], 8);
    }

    TEST_EQUALS(ref.data_pos, key_count * 8);

    for (int32 i = 0; i < key_count; ++i) {
        byte* value = perfect_hashmap_get_value(&ref, keys[i]);
        TEST_NOT_EQUALS(value, NULL);
        TEST_MEMORY_EQUALS(value, values[i], 8);
    }

    TEST_EQUALS(perfect_hashmap_get_value(&ref, "blob_missing"), NULL);

    // The offset/size pair points into the data area
    PerfectHashEntryInt32Int32* entry = test_perfect_map_entry<PerfectHashEntryInt32Int32>(&ref.hm, keys[2]);
    TEST_EQUALS(entry->value, 16);
    TEST_EQUALS(entry->value2, 8);
    TEST_MEMORY_EQUALS(ref.data + entry->value, values[2], 8);

    perfect_hashmap_free(&ref.hm);
    ring_free(&ring);
}

static void test_perfect_hashmap_from_hashmap() {
    RingMemory ring;
    ring_alloc(&ring, 1024 * 1024, 1024 * 1024);

    // Source hash map
    HashMapT<HashEntryStrT<int32>> source = {0};
    hashmap_alloc(&source, 8, 8);

    hashmap_insert(&source, "alpha", 11);
    hashmap_insert(&source, "beta", 22);
    hashmap_insert(&source, "gamma", 33);

    PerfectHashMap phm = {0};
    perfect_hashmap_alloc(&phm, 32, sizeof(PerfectHashEntryInt32));

    TEST_TRUE(perfect_hashmap_from_hashmap(&phm, &source, TEST_PERFECT_MAP_SEED_TRIES, &ring));
    TEST_NOT_EQUALS(phm.hash_function, NULL);

    // Every source entry has to be in the perfect hash map
    HashEntryStrT<int32>* alpha = hashmap_entry_get(&source, "alpha");
    TEST_NOT_EQUALS(alpha, NULL);
    TEST_EQUALS(test_perfect_map_entry<PerfectHashEntryInt32>(&phm, "alpha")->value, 11);
    TEST_EQUALS(test_perfect_map_entry<PerfectHashEntryInt32>(&phm, "beta")->value, 22);
    TEST_EQUALS(test_perfect_map_entry<PerfectHashEntryInt32>(&phm, "gamma")->value, 33);

    // A key that isn't part of the source map returns NULL
    TEST_EQUALS(perfect_hashmap_entry_get(&phm, "delta"), NULL);

    perfect_hashmap_free(&phm);
    hashmap_free(&source);
    ring_free(&ring);
}

static void test_perfect_hashmap_hash_functions() {
    // Every function of the table has to be deterministic
    for (int32 i = 0; i < ARRAY_COUNT(TEST_PERFECT_HASH_FUNCTIONS); ++i) {
        const TestPerfectHashFunction function = TEST_PERFECT_HASH_FUNCTIONS[i];

        const uint64 first = function("key_0", 1234);
        const uint64 second = function("key_0", 1234);
        TEST_EQUALS((long long) first, (long long) second);

        // The table of the map and the local copy have to agree
        TEST_EQUALS((long long) PERFECT_HASH_FUNCTIONS[i]("key_0", 1234), (long long) function("key_0", 1234));

        // Different keys have to produce different hashes
        TEST_NOT_EQUALS((long long) first, (long long) function("key_1", 1234));
        TEST_NOT_EQUALS((long long) function("key_1", 1234), (long long) function("key_2", 1234));

        // The empty key is allowed
        TEST_EQUALS((long long) function("", 1234), (long long) function("", 1234));
    }
}

static void test_perfect_hashmap_hash_seed() {
    for (int32 i = 0; i < ARRAY_COUNT(TEST_PERFECT_HASH_FUNCTIONS); ++i) {
        const TestPerfectHashFunction function = TEST_PERFECT_HASH_FUNCTIONS[i];

        const uint64 seed_a = function("key_1", 1);
        const uint64 seed_b = function("key_1", 2);
        const uint64 seed_c = function("key_1", 3);

        // The seed has to influence the hash, otherwise the seed search can't work
        TEST_NOT_EQUALS((long long) seed_a, (long long) seed_b);
        TEST_NOT_EQUALS((long long) seed_b, (long long) seed_c);

        // Same seed -> same hash
        TEST_EQUALS((long long) function("key_1", 1), (long long) seed_a);
    }
}

static void test_perfect_hashmap_prepare_search() {
    char keys[TEST_PERFECT_MAP_MAX_KEYS][TEST_PERFECT_MAP_KEY_BUFFER] = {{0}};
    const int32 key_count = 16;
    const int32 map_count = TEST_PERFECT_MAP_TABLE_SIZE;
    test_perfect_map_fill_keys(keys, key_count, "search");

    // Same seed search as perfect_hashmap_prepare(), but with deterministic seeds
    // (the real implementation uses rand()) so the test is reproducible
    int32 found_function = -1;
    int32 found_seed = 0;
    int32 used_tries = 0;

    for (int32 function_index = 0; function_index < ARRAY_COUNT(TEST_PERFECT_HASH_FUNCTIONS) && found_function < 0; ++function_index) {
        const TestPerfectHashFunction function = TEST_PERFECT_HASH_FUNCTIONS[function_index];

        for (int32 attempt = 0; attempt < 1000; ++attempt) {
            const int32 seed = 1234567 + attempt * 7919;
            bool used[TEST_PERFECT_MAP_TABLE_SIZE] = {false};
            bool is_unique = true;

            for (int32 j = 0; j < key_count; ++j) {
                const uint32 index = (uint32) (function(keys[j], seed) % (uint64) map_count);
                if (used[index]) {
                    is_unique = false;
                    break;
                }

                used[index] = true;
            }

            ++used_tries;

            if (is_unique) {
                found_function = function_index;
                found_seed = seed;
                break;
            }
        }
    }

    // A collision free seed has to exist for a realistic key set
    TEST_TRUE(found_function >= 0);
    TEST_GREATER_THAN(found_seed, 0);
    TEST_GREATER_THAN(used_tries, 0);
}

#ifdef UBER_TEST
    #ifdef main
        #undef main
    #endif
    #define main StdlibPerfectHashMapTest
#endif

int main() {
    TEST_INIT(128);

    TEST_RUN(test_perfect_hashmap_key_tail);
    TEST_RUN(test_perfect_hashmap_alloc_size);
    TEST_RUN(test_perfect_hashmap_prepare_int32);
    TEST_RUN(test_perfect_hashmap_insert_types);
    TEST_RUN(test_perfect_hashmap_delete);
    TEST_RUN(test_perfect_hashmap_long_keys);
    TEST_RUN(test_perfect_hashmap_prepare_failure);
    TEST_RUN(test_perfect_hashmap_dump_load);
    TEST_RUN(test_perfect_hashmap_ref);
    TEST_RUN(test_perfect_hashmap_from_hashmap);
    TEST_RUN(test_perfect_hashmap_hash_functions);
    TEST_RUN(test_perfect_hashmap_hash_seed);
    TEST_RUN(test_perfect_hashmap_prepare_search);

    TEST_FINALIZE();

    return 0;
}
