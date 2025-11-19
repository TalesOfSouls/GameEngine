#include "../TestFramework.h"
#include "../../stdlib/HashMap.h"

static void test_hashmap_alloc() {
    HashMap hm = {};
    hashmap_alloc(&hm, 3, sizeof(HashEntryInt32));

    TEST_TRUE(hm.buf.count > 0);

    hashmap_free(&hm);
    TEST_EQUALS(hm.buf.size, 0);
    TEST_EQUALS(hm.buf.memory, NULL);
}

static void test_hashmap_insert_int32() {
    HashMap hm = {};
    hashmap_alloc(&hm, 3, sizeof(HashEntryInt32));

    HashEntryInt32* entry;

    hashmap_insert(&hm, "test1", 1);
    entry = (HashEntryInt32 *) hashmap_get_entry(&hm, "test1");
    TEST_NOT_EQUALS(entry, NULL);
    TEST_EQUALS(entry->value, 1);

    hashmap_insert(&hm, "test2", 2);
    entry = (HashEntryInt32 *) hashmap_get_entry(&hm, "test2");
    TEST_NOT_EQUALS(entry, NULL);
    TEST_EQUALS(entry->value, 2);

    hashmap_insert(&hm, "test3", 3);
    entry = (HashEntryInt32 *) hashmap_get_entry(&hm, "test3");
    TEST_NOT_EQUALS(entry, NULL);
    TEST_EQUALS(entry->value, 3);

    entry = (HashEntryInt32 *) hashmap_get_entry(&hm, "invalid");
    TEST_EQUALS(entry, NULL);

    hashmap_free(&hm);
}

static void test_hashmap_remove() {
    HashMap hm = {};
    hashmap_alloc(&hm, 3, sizeof(HashEntryInt32));

    HashEntryInt32* entry;

    hashmap_insert(&hm, "test1", 1);
    entry = (HashEntryInt32 *) hashmap_get_entry(&hm, "test1");
    TEST_NOT_EQUALS(entry, NULL);
    TEST_EQUALS(entry->value, 1);

    hashmap_insert(&hm, "test2", 2);
    entry = (HashEntryInt32 *) hashmap_get_entry(&hm, "test2");
    TEST_NOT_EQUALS(entry, NULL);
    TEST_EQUALS(entry->value, 2);

    hashmap_remove(&hm, "test2");
    entry = (HashEntryInt32 *) hashmap_get_entry(&hm, "test2");
    TEST_EQUALS(entry, NULL);

    entry = (HashEntryInt32 *) hashmap_get_entry(&hm, "test1");
    TEST_NOT_EQUALS(entry, NULL);
    TEST_EQUALS(entry->value, 1);

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

    int64 dump_size = hashmap_dump(&hm_dump, out, MEMBER_SIZEOF(HashEntryInt32, value));
    int64 load_size = hashmap_load(&hm_load, out, MEMBER_SIZEOF(HashEntryInt32, value));
    TEST_EQUALS(dump_size, load_size);
    TEST_MEMORY_EQUALS(hm_dump.buf.memory, hm_load.buf.memory, hm_dump.buf.size);

    hashmap_free(&hm_dump);
    hashmap_free(&hm_load);

    ring_free(&ring);
}

#if PERFORMANCE_TEST
// Testing our hash map implementation vs a hash map implementation mentioned in most books
#define MAX_TEST_KEY_LENGTH 26
typedef struct {
    char key[MAX_TEST_KEY_LENGTH];
    int32_t value;
    int used;   // 0 = empty, 1 = used, -1 = tombstone (removed)
} HashEntryStd;

typedef struct {
    HashEntryStd* entries;
    uint32_t capacity;
    uint32_t count;
    HashMapHashFunction hash_function;
} HashMapStd;

// ------------------ Core HashMap Functions ------------------

HashMapStd* hashmap_test_create(uint32_t capacity) {
    HashMapStd* map = (HashMapStd*) platform_alloc(sizeof(HashMapStd));
    map->capacity = capacity;
    map->count = 0;
    map->entries = (HashEntryStd*) platform_alloc(capacity * sizeof(HashEntryStd));
    map->hash_function = NULL;
    return map;
}

void hashmap_test_free(HashMapStd* map) {
    platform_free((void **) &map->entries);
    platform_free((void **) &map);
}

HashEntryStd* open_test_insert(HashMapStd* map, const char* key, int32_t value) {
    uint64_t hash = map->hash_function ? map->hash_function((void *) key) : hash_djb2(key);
    uint32_t index = hash % map->capacity;

    str_move_to_pos(&key, -((int32) MAX_TEST_KEY_LENGTH));

    for (uint32_t i = 0; i < map->capacity; i++) {
        uint32_t probe = (index + i) % map->capacity;
        HashEntryStd* entry = &map->entries[probe];

        if (entry->used == 0 || entry->used == -1) {
            str_copy(entry->key, key, MAX_TEST_KEY_LENGTH - 1);
            entry->key[MAX_TEST_KEY_LENGTH - 1] = '\0';
            entry->value = value;
            entry->used = 1;
            ++map->count;
            return entry;
        }/* else if (entry->used == 1 && str_compare(entry->key, key) == 0) {
            entry->value = value; // update existing
            return entry;
        }*/
    }

    return NULL;
}

HashEntryStd* hashmap_test_get(HashMapStd* map, const char* key) {
    uint64_t hash = map->hash_function ? map->hash_function((void *) key) : hash_djb2(key);
    uint32_t index = hash % map->capacity;

    for (uint32_t i = 0; i < map->capacity; i++) {
        uint32_t probe = (index + i) % map->capacity;
        HashEntryStd* entry = &map->entries[probe];

        if (entry->used == 0) {
            return NULL; // not found
        } else if (entry->used == 1 && str_compare(entry->key, key) == 0) {
            return entry;
        }
    }
    return NULL;
}

void hashmap_test_remove(HashMapStd* map, const char* key) {
    uint64_t hash = map->hash_function ? map->hash_function((void *) key) : hash_djb2(key);
    uint32_t index = hash % map->capacity;

    for (uint32_t i = 0; i < map->capacity; i++) {
        uint32_t probe = (index + i) % map->capacity;
        HashEntryStd* entry = &map->entries[probe];

        if (entry->used == 0) {
            return; // not found
        } else if (entry->used == 1 && str_compare(entry->key, key) == 0) {
            entry->used = -1; // mark as tombstone
            map->count--;
            return;
        }
    }
}

// ------------------ Chained HashMap Structs ------------------

typedef struct ChainTestNode {
    char key[MAX_TEST_KEY_LENGTH];
    int32_t value;
    struct ChainTestNode* next;
} ChainTestNode;

typedef struct {
    ChainTestNode** buckets;       // Hash table buckets
    ChainTestNode* nodes_pool;     // Pre-allocated nodes
    ChainTestNode* free_list;      // Free nodes list
    uint32_t capacity;             // Number of buckets
    uint32_t max_nodes;            // Total nodes pre-allocated
    uint32_t count;                // Current number of used nodes
    HashMapHashFunction hash_function;
} ChainTestMap;

// ------------------ Core HashMap Functions ------------------

ChainTestMap* chain_test_create(uint32_t capacity, uint32_t max_nodes) {
    ChainTestMap* map = (ChainTestMap*) platform_alloc(sizeof(ChainTestMap));
    map->capacity = capacity;
    map->max_nodes = max_nodes;
    map->count = 0;
    map->buckets = (ChainTestNode**) platform_alloc(capacity * sizeof(ChainTestNode*));
    map->hash_function = NULL;

    // Pre-allocate nodes
    map->nodes_pool = (ChainTestNode*) platform_alloc(max_nodes * sizeof(ChainTestNode));

    // Initialize free list
    map->free_list = &map->nodes_pool[0];
    for (uint32_t i = 0; i < max_nodes - 1; i++) {
        map->nodes_pool[i].next = &map->nodes_pool[i + 1];
    }
    map->nodes_pool[max_nodes - 1].next = NULL;

    return map;
}

void chain_test_free(ChainTestMap* map) {
    platform_free((void **) &map->buckets);
    platform_free((void **) &map->nodes_pool);
    platform_free((void **) &map);
}

// Grab a node from the free list
static ChainTestNode* chain_test_allocate_node(ChainTestMap* map) {
    if (!map->free_list) return NULL; // No free nodes left
    ChainTestNode* node = map->free_list;
    map->free_list = node->next;
    node->next = NULL;
    return node;
}

// Return node to the free list
static void chain_test_free_node(ChainTestMap* map, ChainTestNode* node) {
    node->next = map->free_list;
    map->free_list = node;
}

// ------------------ HashMap Operations ------------------

ChainTestNode* chain_test_insert(ChainTestMap* map, const char* key, int32_t value) {
    uint64_t hash = map->hash_function ? map->hash_function((void*)key) : hash_djb2(key);
    uint32_t index = hash % map->capacity;

    str_move_to_pos(&key, -((int32) MAX_TEST_KEY_LENGTH));

    ChainTestNode* node = map->buckets[index];
    while (node) {
        /*
        if (str_compare(node->key, key) == 0) {
            node->value = value; // Update existing
            return node;
        }
        */
        node = node->next;
    }

    // Allocate from pre-allocated pool
    ChainTestNode* new_node = chain_test_allocate_node(map);
    if (!new_node) return NULL; // Pool exhausted

    str_copy(new_node->key, key, MAX_TEST_KEY_LENGTH - 1);
    new_node->key[MAX_TEST_KEY_LENGTH - 1] = '\0';
    new_node->value = value;
    new_node->next = map->buckets[index];
    map->buckets[index] = new_node;
    ++map->count;

    return new_node;
}

ChainTestNode* chain_test_get(ChainTestMap* map, const char* key) {
    uint64_t hash = map->hash_function ? map->hash_function((void*)key) : hash_djb2(key);
    uint32_t index = hash % map->capacity;

    ChainTestNode* node = map->buckets[index];
    while (node) {
        if (str_compare(node->key, key) == 0) return node;
        node = node->next;
    }
    return NULL;
}

void chain_test_remove(ChainTestMap* map, const char* key) {
    uint64_t hash = map->hash_function ? map->hash_function((void*)key) : hash_djb2(key);
    uint32_t index = hash % map->capacity;

    ChainTestNode* node = map->buckets[index];
    ChainTestNode* prev = NULL;

    while (node) {
        if (str_compare(node->key, key) == 0) {
            if (prev) {
                prev->next = node->next;
            } else {
                map->buckets[index] = node->next;
            }
            chain_test_free_node(map, node); // Return to free list
            map->count--;
            return;
        }
        prev = node;
        node = node->next;
    }
}

#define HASH_MAP_MAX_COUNT (1 << 13)
#define HASH_MAP_TEST_COUNT (1 << 12)

#include "../../utils/RandomUtils.h"

static void _my_hashmap(MAYBE_UNUSED volatile void* val) {
    HashMap map = {};
    hashmap_alloc(&map, HASH_MAP_MAX_COUNT, sizeof(HashEntryInt32));
    char test_key[20];

    for (int32 i = 0; i < HASH_MAP_TEST_COUNT; ++i) {
        random_string("ABCDEF0123456789", 16, test_key, 19);

        hashmap_insert(&map, test_key, 1);
        MAYBE_UNUSED HashEntry* entry = hashmap_get_entry(&map, test_key);
    }

    hashmap_free(&map);
}

static void _open_hashmap(MAYBE_UNUSED volatile void* val) {
    HashMapStd* map = hashmap_test_create(HASH_MAP_MAX_COUNT);
    char test_key[20];

    for (int32 i = 0; i < HASH_MAP_TEST_COUNT; ++i) {
        random_string("ABCDEF0123456789", 16, test_key, 19);

        open_test_insert(map, test_key, 1);
        MAYBE_UNUSED HashEntryStd* entry = hashmap_test_get(map, test_key);
    }

    hashmap_test_free(map);
}

static void _chained_hashmap(MAYBE_UNUSED volatile void* val) {
    ChainTestMap* map = chain_test_create(HASH_MAP_MAX_COUNT, HASH_MAP_MAX_COUNT);
    char test_key[20];

    for (int32 i = 0; i < HASH_MAP_TEST_COUNT; ++i) {
        random_string("ABCDEF0123456789", 16, test_key, 19);

        chain_test_insert(map, test_key, 1);
        MAYBE_UNUSED ChainTestNode* entry = chain_test_get(map, test_key);
    }

    chain_test_free(map);
}

static void test_hash_map_performance() {
    COMPARE_FUNCTION_TEST_TIME(_my_hashmap, _open_hashmap, -500.0);
    COMPARE_FUNCTION_TEST_TIME(_my_hashmap, _chained_hashmap, -500.0);
    //COMPARE_FUNCTION_TEST_CYCLE(_my_hashmap, _open_hashmap, 5.0);
}
#endif

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
    TEST_RUN(test_hashmap_remove);
    TEST_RUN(test_hashmap_dump_load);

    #if PERFORMANCE_TEST
        TEST_RUN(test_hash_map_performance);
    #endif

    TEST_FINALIZE();

    return 0;
}