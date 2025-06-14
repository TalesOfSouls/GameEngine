#include "../TestFramework.h"
#include "../../stdlib/HashMap.h"

static void test_hashmap_alloc() {
    HashMap hm = {};
    hashmap_alloc(&hm, 3, sizeof(HashEntryInt32));

    ASSERT_NOT_EQUALS(hm.table, NULL);
    ASSERT_TRUE(hm.buf.count > 0);

    hashmap_free(&hm);
    ASSERT_EQUALS(hm.buf.size, 0);
    ASSERT_EQUALS(hm.buf.memory, NULL);
}

static void test_hashmap_insert_int32() {
    HashMap hm = {};
    hashmap_alloc(&hm, 3, sizeof(HashEntryInt32));

    HashEntryInt32* entry;

    hashmap_insert(&hm, "test1", 1);
    entry = (HashEntryInt32 *) hashmap_get_entry(&hm, "test1");
    ASSERT_NOT_EQUALS(entry, NULL);
    ASSERT_EQUALS(entry->value, 1);

    hashmap_insert(&hm, "test2", 2);
    entry = (HashEntryInt32 *) hashmap_get_entry(&hm, "test2");
    ASSERT_NOT_EQUALS(entry, NULL);
    ASSERT_EQUALS(entry->value, 2);

    hashmap_insert(&hm, "test3", 3);
    entry = (HashEntryInt32 *) hashmap_get_entry(&hm, "test3");
    ASSERT_NOT_EQUALS(entry, NULL);
    ASSERT_EQUALS(entry->value, 3);

    entry = (HashEntryInt32 *) hashmap_get_entry(&hm, "invalid");
    ASSERT_EQUALS(entry, NULL);

    hashmap_free(&hm);
}

static void test_hashmap_dump_load() {
    RingMemory ring;
    ring_alloc(&ring, 10 * MEGABYTE, 64);

    HashMap hm_dump = {};
    hashmap_alloc(&hm_dump, 3, sizeof(HashEntryInt32));

    hashmap_insert(&hm_dump, "test1", 1);
    hashmap_insert(&hm_dump, "test2", 2);
    hashmap_insert(&hm_dump, "test3", 3);

    HashMap hm_load = {};
    hashmap_alloc(&hm_load, 3, sizeof(HashEntryInt32));

    byte* out = ring_get_memory(&ring, 1024 * 1024);

    int64 dump_size = hashmap_dump(&hm_dump, out);
    int64 load_size = hashmap_load(&hm_load, out);
    ASSERT_EQUALS(dump_size, load_size);
    ASSERT_MEMORY_EQUALS(hm_dump.table, hm_load.table, sizeof(uint16) * hm_dump.buf.count);
    ASSERT_MEMORY_EQUALS(hm_dump.buf.memory, hm_load.buf.memory, hm_dump.buf.size);

    hashmap_free(&hm_dump);
    hashmap_free(&hm_load);

    ring_free(&ring);
}

#ifdef UBER_TEST
    #ifdef main
        #undef main
    #endif
    #define main StdlibHashMapTest
#endif

int main() {
    TEST_INIT(25);

    TEST_RUN(test_hashmap_alloc);
    TEST_RUN(test_hashmap_insert_int32);
    TEST_RUN(test_hashmap_dump_load);

    TEST_FINALIZE();

    return 0;
}