#include "../TestFramework.h"

// ThrdChunkMemory.cpp/ThrdHashMapT.cpp are not self contained:
// they need the logging, allocator, BufferMemory and MemoryArena declarations first
// (ChunkMemory.cpp pulls in BufferMemory.cpp, BitUtils.h and MemoryArena.h)
#include "../../log/Log.h"
#include "../../system/Allocator.h"
#include "../../log/PerformanceProfiler.h"
#include "../../memory/ChunkMemory.cpp"

#include "../../stdlib/ThrdHashMapT.cpp"
#include "../../utils/StringUtils.h"

/**
 * ThrdHashMapT tests
 *
 * The map is a lock free chained hash map on top of a ThrdChunkMemoryT. The tests
 * cover the single threaded semantics (insert/get/remove/reserve/full map/dump+load)
 * plus one small deterministic multi threaded insert case.
 */

typedef ThrdHashMapT<ThrdHashEntryStrT<int32>> ThrdStrMap;
typedef ThrdHashMapT<ThrdHashEntryT<int32, int32>> ThrdIntMap;

static void test_thrd_hashmap_bucket_count() {
    const int32 capacities[] = {1, 2, 8, 16, 100, 1000, 65535};

    for (int32 i = 0; i < ARRAY_COUNT(capacities); ++i) {
        const int32 bucket_count = hashmap_bucket_count(capacities[i]);

        // Power of 2
        TEST_EQUALS(bucket_count & (bucket_count - 1), 0);
        TEST_GREATER_THAN(bucket_count, 0);

        // At least 4 buckets per 5 elements (max load factor 0.8)
        TEST_TRUE(bucket_count * 4 >= capacities[i] * 5);

        // Not absurdly large
        TEST_TRUE(bucket_count <= capacities[i] * 8 + 16);
    }

    // Small capacities still get the minimum bucket count
    TEST_EQUALS(hashmap_bucket_count(1), 16);
    TEST_EQUALS(hashmap_bucket_count(8), 16);
    TEST_EQUALS(hashmap_bucket_count(16), 32);
}

static void test_thrd_hashmap_alloc() {
    ThrdStrMap hm = {0};
    hashmap_alloc(&hm, 8, 8);

    TEST_EQUALS(hm.buf.capacity, 8);
    TEST_NOT_EQUALS(hm.buf.memory, NULL);
    TEST_NOT_EQUALS(hm.buckets, NULL);
    TEST_EQUALS(hm.bucket_count, hashmap_bucket_count(8));
    TEST_TRUE(((uintptr_t) hm.buckets & (ASSUMED_CACHE_LINE_SIZE - 1)) == 0);

    // The capacity is aligned up to 2 so the 4 byte alignment is guaranteed
    TEST_EQUALS(hashmap_size(&hm), hm.buf.capacity * (int64) sizeof(ThrdHashEntryStrT<int32>) + hm.bucket_count * (int64) sizeof(uint16));

    // All buckets are empty
    for (int32 i = 0; i < hm.bucket_count; ++i) {
        TEST_EQUALS(hm.buckets[i].load(memory_order_relaxed), 0);
    }

    hashmap_free(&hm);
    TEST_EQUALS(hm.buf.capacity, 0);
    TEST_EQUALS(hm.buf.memory, NULL);
    TEST_EQUALS(hm.buckets, NULL);
    TEST_EQUALS(hm.bucket_count, 0);
}

static void test_thrd_hashmap_insert_get() {
    ThrdStrMap hm = {0};
    hashmap_alloc(&hm, 8, 8);

    ThrdHashEntryStrT<int32>* entry = hashmap_insert(&hm, "test1", 1);
    TEST_NOT_EQUALS(entry, NULL);
    TEST_EQUALS(entry->value, 1);
    TEST_EQUALS(strcmp(entry->key, "test1"), 0);

    entry = hashmap_entry_get(&hm, "test1");
    TEST_NOT_EQUALS(entry, NULL);
    TEST_EQUALS(entry->value, 1);
    TEST_NOT_EQUALS(entry->hash, 0);

    // A key with the same hash has to compare equal
    TEST_EQUALS(hashmap_entry_get(&hm, "test1")->hash, hashmap_entry_get(&hm, "test1")->hash);

    hashmap_insert(&hm, "test2", 2);
    hashmap_insert(&hm, "test3", 3);

    TEST_EQUALS(hashmap_entry_get(&hm, "test2")->value, 2);
    TEST_EQUALS(hashmap_entry_get(&hm, "test3")->value, 3);

    // All entries are still reachable
    TEST_EQUALS(hashmap_entry_get(&hm, "test1")->value, 1);
    TEST_EQUALS(hashmap_entry_get(&hm, "missing"), NULL);
    TEST_EQUALS(hashmap_entry_get(&hm, ""), NULL);
    TEST_EQUALS(hashmap_entry_get_any(&hm, "missing"), NULL);

    // The stored pointer is stable (entries never move)
    ThrdHashEntryStrT<int32>* first = hashmap_entry_get(&hm, "test1");
    hashmap_insert(&hm, "test4", 4);
    TEST_EQUALS(hashmap_entry_get(&hm, "test1"), first);
    TEST_EQUALS(first->value, 1);

    hashmap_free(&hm);
}

static void test_thrd_hashmap_duplicate() {
    ThrdStrMap hm = {0};
    hashmap_alloc(&hm, 8, 8);

    // The map doesn't check for duplicates: the newest entry is the chain head
    TEST_NOT_EQUALS(hashmap_insert(&hm, "dup", 1), NULL);
    TEST_NOT_EQUALS(hashmap_insert(&hm, "dup", 2), NULL);
    TEST_EQUALS(hashmap_entry_get(&hm, "dup")->value, 2);

    // Removing one entry exposes the older duplicate
    hashmap_remove(&hm, "dup");
    TEST_NOT_EQUALS(hashmap_entry_get(&hm, "dup"), NULL);
    TEST_EQUALS(hashmap_entry_get(&hm, "dup")->value, 1);

    hashmap_remove(&hm, "dup");
    TEST_EQUALS(hashmap_entry_get(&hm, "dup"), NULL);

    hashmap_free(&hm);
}

static void test_thrd_hashmap_remove() {
    ThrdStrMap hm = {0};
    hashmap_alloc(&hm, 8, 8);

    hashmap_insert(&hm, "test1", 1);
    hashmap_insert(&hm, "test2", 2);
    hashmap_insert(&hm, "test3", 3);

    hashmap_remove(&hm, "test2");
    TEST_EQUALS(hashmap_entry_get(&hm, "test2"), NULL);
    TEST_EQUALS(hashmap_entry_get(&hm, "test1")->value, 1);
    TEST_EQUALS(hashmap_entry_get(&hm, "test3")->value, 3);

    // Removing a missing key is a no-op
    hashmap_remove(&hm, "missing");
    TEST_NOT_EQUALS(hashmap_entry_get(&hm, "test1"), NULL);

    // The freed slot is reused (no leak of chunk elements)
    hashmap_insert(&hm, "test4", 4);
    TEST_EQUALS(hashmap_entry_get(&hm, "test4")->value, 4);

    // Remove by entry pointer
    ThrdHashEntryStrT<int32>* entry = hashmap_entry_get(&hm, "test1");
    hashmap_remove_entry(&hm, entry);
    TEST_EQUALS(hashmap_entry_get(&hm, "test1"), NULL);
    TEST_NOT_EQUALS(hashmap_entry_get(&hm, "test3"), NULL);

    // Remove the rest
    hashmap_remove(&hm, "test3");
    hashmap_remove(&hm, "test4");
    TEST_EQUALS(hashmap_entry_get(&hm, "test3"), NULL);
    TEST_EQUALS(hashmap_entry_get(&hm, "test4"), NULL);

    hashmap_free(&hm);
}

static void test_thrd_hashmap_reserve() {
    ThrdStrMap hm = {0};
    hashmap_alloc(&hm, 8, 8);

    ThrdHashEntryStrT<int32>* reserved = hashmap_reserve(&hm, "reserved");
    TEST_NOT_EQUALS(reserved, NULL);
    TEST_EQUALS(reserved->value, 0);

    // The entry exists but isn't published as complete yet
    TEST_EQUALS(hashmap_entry_get(&hm, "reserved"), NULL);
    TEST_EQUALS(hashmap_entry_get_any(&hm, "reserved"), reserved);

    reserved->value = 42;
    hashmap_mark_completed(&hm, "reserved");

    TEST_NOT_EQUALS(hashmap_entry_get(&hm, "reserved"), NULL);
    TEST_EQUALS(hashmap_entry_get(&hm, "reserved")->value, 42);

    // get_reserve returns the existing entry instead of reserving a new one
    ThrdHashEntryStrT<int32>* existing = hashmap_get_reserve(&hm, "reserved");
    TEST_EQUALS(existing, reserved);
    TEST_EQUALS(existing->value, 42);

    // get_reserve creates + zero initializes a missing entry
    ThrdHashEntryStrT<int32>* created = hashmap_get_reserve(&hm, "created");
    TEST_NOT_EQUALS(created, NULL);
    TEST_NOT_EQUALS(created, reserved);
    TEST_EQUALS(created->value, 0);

    hashmap_free(&hm);
}

static void test_thrd_hashmap_full() {
    ThrdStrMap hm = {0};
    hashmap_alloc(&hm, 4, 4);

    char key[32];
    for (int32 i = 0; i < 4; ++i) {
        sprintf_fast(key, sizeof(key), "key_%d", i);
        TEST_NOT_EQUALS(hashmap_insert(&hm, key, i), NULL);
    }

    // The map is full
    TEST_EQUALS(hashmap_insert(&hm, "overflow", 99), NULL);
    TEST_EQUALS(hashmap_entry_get(&hm, "overflow"), NULL);

    // All four entries are still reachable
    for (int32 i = 0; i < 4; ++i) {
        sprintf_fast(key, sizeof(key), "key_%d", i);
        ThrdHashEntryStrT<int32>* entry = hashmap_entry_get(&hm, key);
        TEST_NOT_EQUALS(entry, NULL);
        TEST_EQUALS(entry->value, i);
    }

    // Freeing one element allows exactly one new element
    hashmap_remove(&hm, "key_0");
    TEST_NOT_EQUALS(hashmap_insert(&hm, "overflow", 99), NULL);
    TEST_EQUALS(hashmap_entry_get(&hm, "overflow")->value, 99);
    TEST_EQUALS(hashmap_insert(&hm, "overflow2", 100), NULL);

    hashmap_free(&hm);
}

static void test_thrd_hashmap_int_key() {
    ThrdIntMap hm = {0};
    hashmap_alloc(&hm, 16, 16);

    for (int32 i = 0; i < 16; ++i) {
        ThrdHashEntryT<int32, int32>* entry = hashmap_insert(&hm, i * 7, i * 100);
        TEST_NOT_EQUALS(entry, NULL);
        TEST_EQUALS(entry->key, i * 7);
        TEST_EQUALS(entry->value, i * 100);
    }

    for (int32 i = 0; i < 16; ++i) {
        ThrdHashEntryT<int32, int32>* entry = hashmap_entry_get(&hm, i * 7);
        TEST_NOT_EQUALS(entry, NULL);
        TEST_EQUALS(entry->value, i * 100);
    }

    // Missing keys
    TEST_EQUALS(hashmap_entry_get(&hm, 1), NULL);
    TEST_EQUALS(hashmap_entry_get(&hm, -1), NULL);

    hashmap_remove(&hm, 7);
    TEST_EQUALS(hashmap_entry_get(&hm, 7), NULL);
    TEST_NOT_EQUALS(hashmap_entry_get(&hm, 14), NULL);
    TEST_EQUALS(hashmap_entry_get(&hm, 14)->value, 200);

    hashmap_remove(&hm, 7);
    TEST_EQUALS(hashmap_insert(&hm, 7, 700)->value, 700);
    TEST_EQUALS(hashmap_entry_get(&hm, 7)->value, 700);

    hashmap_free(&hm);
}

static void test_thrd_hashmap_long_key() {
    ThrdStrMap hm = {0};
    hashmap_alloc(&hm, 8, 8);

    // Only the last HASH_MAP_MAX_KEY_LENGTH - 1 characters are stored/hashed
    const char* long_key = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    TEST_NOT_EQUALS(hashmap_insert(&hm, long_key, 7), NULL);

    const int32 key_length = (int32) strlen(long_key);
    const char* stored = long_key + (key_length - (HASH_MAP_MAX_KEY_LENGTH - 1));

    ThrdHashEntryStrT<int32>* entry = hashmap_entry_get(&hm, stored);
    TEST_NOT_EQUALS(entry, NULL);
    TEST_EQUALS(entry->value, 7);
    TEST_EQUALS(strlen(entry->key), HASH_MAP_MAX_KEY_LENGTH - 1);
    TEST_EQUALS(strcmp(entry->key, stored), 0);

    // A different prefix with the same tail maps to the same entry
    TEST_EQUALS(hashmap_entry_get(&hm, "QQQQQQQQQABCDEFGHIJKLMNOPQRSTUVWXYZ"), entry);

    hashmap_free(&hm);
}

static void test_thrd_hashmap_dump_load() {
    ThrdStrMap dump_map = {0};
    hashmap_alloc(&dump_map, 8, 8);

    hashmap_insert(&dump_map, "test1", 1);
    hashmap_insert(&dump_map, "test2", 2);
    hashmap_insert(&dump_map, "test3", 3);
    hashmap_remove(&dump_map, "test2");

    byte buffer[2048] = {0};
    const int64 dump_size = hashmap_dump(&dump_map, buffer);
    TEST_GREATER_THAN(dump_size, 0);
    TEST_TRUE(dump_size <= (int64) sizeof(buffer));

    // The required size for hashmap_init has to fit in the same buffer layout
    TEST_TRUE(hashmap_required_size<ThrdHashEntryStrT<int32>>(8) <= sizeof(buffer));

    byte memory[2048] = {0};
    ThrdStrMap load_map = {0};
    hashmap_init(&load_map, 8, memory, sizeof(size_t));

    TEST_EQUALS(load_map.buf.capacity, 8);
    TEST_EQUALS(load_map.bucket_count, hashmap_bucket_count(8));

    const int64 load_size = hashmap_load(&load_map, buffer);
    TEST_EQUALS(load_size, dump_size);

    // The loaded entries have to be reachable (and the removed one has to stay removed)
    ThrdHashEntryStrT<int32>* entry = hashmap_entry_get(&load_map, "test1");
    TEST_NOT_EQUALS(entry, NULL);
    TEST_EQUALS(entry->value, 1);
    TEST_EQUALS(hashmap_entry_get(&load_map, "test3")->value, 3);
    TEST_EQUALS(hashmap_entry_get(&load_map, "test2"), NULL);

    // The loaded map can be modified
    TEST_NOT_EQUALS(hashmap_insert(&load_map, "test4", 4), NULL);
    TEST_EQUALS(hashmap_entry_get(&load_map, "test4")->value, 4);
    hashmap_remove(&load_map, "test1");
    TEST_EQUALS(hashmap_entry_get(&load_map, "test1"), NULL);

    hashmap_free(&dump_map);
}

struct ThrdHashMapThreadParam {
    ThrdStrMap* map;

    int32 thread_id;
    int32 key_count;

    int32 inserted;
    int32 verified;
};

static DWORD WINAPI thrd_hashmap_insert_job(void* data) {
    ThrdHashMapThreadParam* param = (ThrdHashMapThreadParam *) data;

    char key[32];
    for (int32 i = 0; i < param->key_count; ++i) {
        sprintf_fast(key, sizeof(key), "t%d_k%d", param->thread_id, i);

        ThrdHashEntryStrT<int32>* entry = hashmap_insert(param->map, key, i);
        if (entry != NULL) {
            ++param->inserted;
        }
    }

    return 0;
}

static void test_thrd_hashmap_multithreaded() {
    ThrdStrMap hm = {0};
    hashmap_alloc(&hm, 256, 256);

    enum { THREAD_COUNT = 2, KEYS_PER_THREAD = 128 };

    ThrdHashMapThreadParam params[THREAD_COUNT] = {0};
    HANDLE threads[THREAD_COUNT] = {0};

    for (int32 i = 0; i < THREAD_COUNT; ++i) {
        params[i].map = &hm;
        params[i].thread_id = i;
        params[i].key_count = KEYS_PER_THREAD;

        threads[i] = CreateThread(NULL, 0, thrd_hashmap_insert_job, &params[i], 0, NULL);
        TEST_NOT_EQUALS(threads[i], NULL);
    }

    for (int32 i = 0; i < THREAD_COUNT; ++i) {
        TEST_EQUALS(WaitForSingleObject(threads[i], 30000), WAIT_OBJECT_0);
        CloseHandle(threads[i]);
    }

    char key[32];

    // Every thread inserted all of its keys and every key is readable afterwards
    for (int32 thread_id = 0; thread_id < THREAD_COUNT; ++thread_id) {
        TEST_EQUALS(params[thread_id].inserted, KEYS_PER_THREAD);

        for (int32 i = 0; i < KEYS_PER_THREAD; ++i) {
            sprintf_fast(key, sizeof(key), "t%d_k%d", thread_id, i);

            ThrdHashEntryStrT<int32>* entry = hashmap_entry_get(&hm, key);
            if (entry != NULL && entry->value == i) {
                ++params[thread_id].verified;
            }
        }

        TEST_EQUALS(params[thread_id].verified, KEYS_PER_THREAD);
    }

    hashmap_free(&hm);
}

#ifdef UBER_TEST
    #ifdef main
        #undef main
    #endif
    #define main StdlibThrdHashMapTTest
#endif

int main() {
    TEST_INIT(96);

    TEST_RUN(test_thrd_hashmap_bucket_count);
    TEST_RUN(test_thrd_hashmap_alloc);
    TEST_RUN(test_thrd_hashmap_insert_get);
    TEST_RUN(test_thrd_hashmap_duplicate);
    TEST_RUN(test_thrd_hashmap_remove);
    TEST_RUN(test_thrd_hashmap_reserve);
    TEST_RUN(test_thrd_hashmap_full);
    TEST_RUN(test_thrd_hashmap_int_key);
    TEST_RUN(test_thrd_hashmap_long_key);
    TEST_RUN(test_thrd_hashmap_dump_load);
    TEST_RUN(test_thrd_hashmap_multithreaded);

    TEST_FINALIZE();

    return 0;
}
