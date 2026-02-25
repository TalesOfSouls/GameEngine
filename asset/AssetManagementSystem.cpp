/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_ASSET_MANAGEMENT_SYSTEM_C
#define COMS_ASSET_MANAGEMENT_SYSTEM_C

#include "../stdlib/Stdlib.h"
#include "Asset.h"
#include "../memory/ChunkMemory.h"
#include "../utils/Assert.h"
#include "../utils/BitUtils.h"
#include "../stdlib/HashMap.cpp"
#include "../log/DebugMemory.h"
#include "../thread/Atomic.h"

#include "AssetManagementSystem.h"

inline
void ams_create(AssetManagementSystem* const ams, BufferMemory* const buf, int32 asset_component_count, int32 count) NO_EXCEPT
{
    LOG_1("[INFO] Create AMS for %n assets", {DATA_TYPE_INT32, &count});
    hashmap_create(&ams->hash_map, count, sizeof(HashEntry) + sizeof(Asset), buf);
    ams->asset_component_count = asset_component_count;
    ams->asset_components = (AssetComponent *) buffer_get_memory(buf, asset_component_count * sizeof(AssetComponent), ASSUMED_CACHE_LINE_SIZE);

    //memset(ams->asset_components, 0, asset_component_count * sizeof(AssetComponent));
    memset(ams->asset_components, 0, align_up(asset_component_count * sizeof(AssetComponent), ASSUMED_CACHE_LINE_SIZE));
}

// Different AMS components can have different chunk sizes
// This is useful for components that only contain textures, audio, 3d object data, etc. to avoid massive unused data or very small memory chunks for all situations which makes chunk allocation slower
inline
void ams_component_create(AssetComponent* ac, BufferMemory* const buf, int32 chunk_size, int32 count) NO_EXCEPT
{
    ASSERT_TRUE(chunk_size);
    LOG_1(
        "[INFO] Create AMS Component for %n assets and %n B",
        {DATA_TYPE_INT32, &count}, {DATA_TYPE_UINT32, &chunk_size}
    );

    chunk_init(&ac->asset_memory, buf, count, chunk_size, ASSUMED_CACHE_LINE_SIZE);
    mutex_init(&ac->mtx, NULL);
}

inline
void ams_component_alloc(AssetComponent* ac, int32 chunk_size, int32 count, int32 max_count) NO_EXCEPT
{
    ASSERT_TRUE(chunk_size);
    LOG_1(
        "[INFO] Create AMS Component for %n assets and %n B",
        {DATA_TYPE_INT32, &count}, {DATA_TYPE_UINT32, &chunk_size}
    );

    chunk_alloc(&ac->asset_memory, count, max_count, chunk_size, ASSUMED_CACHE_LINE_SIZE);
    mutex_init(&ac->mtx, NULL);
}

inline
void ams_component_alloc(AssetComponent* ac, MemoryArena* mem, int32 chunk_size, int32 count, int32 max_count) NO_EXCEPT
{
    ASSERT_TRUE(chunk_size);
    LOG_1(
        "[INFO] Create AMS Component for %n assets and %n B",
        {DATA_TYPE_INT32, &count}, {DATA_TYPE_UINT32, &chunk_size}
    );

    chunk_alloc(&ac->asset_memory, mem, count, max_count, chunk_size, ASSUMED_CACHE_LINE_SIZE);
    mutex_init(&ac->mtx, NULL);
}

inline
void ams_component_create(AssetComponent* ac, byte* buf, int32 chunk_size, int32 count) NO_EXCEPT
{
    ASSERT_TRUE(chunk_size);
    LOG_1(
        "[INFO] Create AMS Component for %n assets and %n B",
        {DATA_TYPE_INT32, &count}, {DATA_TYPE_UINT32, &chunk_size}
    );

    ac->asset_memory.capacity = count;
    ac->asset_memory.chunk_size = chunk_size;
    ac->asset_memory.last_pos = 0;
    ac->asset_memory.alignment = sizeof(size_t);
    ac->asset_memory.memory = buf;
    ac->asset_memory.free = (uint64 *) (ac->asset_memory.memory + ac->asset_memory.chunk_size * count);

    mutex_init(&ac->mtx, NULL);
}

FORCE_INLINE
void ams_component_free(AssetComponent* ac) NO_EXCEPT
{
    chunk_free(&ac->asset_memory);
    mutex_destroy(&ac->mtx);
}

FORCE_INLINE
void ams_free(AssetManagementSystem* const ams) NO_EXCEPT
{
    for (int32 i = 0; i < ams->asset_component_count; ++i) {
        ams_component_free(&ams->asset_components[i]);
    }
}

FORCE_INLINE
uint16 ams_calculate_chunks(const AssetComponent* ac, int32 byte_size, int32 overhead) NO_EXCEPT
{
    return (uint16) ceil_div(byte_size + overhead, ac->asset_memory.chunk_size);
}

FORCE_INLINE
void thrd_ams_set_loaded(Asset* const asset) NO_EXCEPT
{
    atomic_set_release(&asset->is_loaded, 1);
}

FORCE_INLINE
bool thrd_ams_is_loaded(Asset* const asset) NO_EXCEPT
{
    return asset && atomic_get_acquire(&asset->is_loaded) > 0;
}

FORCE_INLINE
bool thrd_ams_is_in_vram(Asset* const asset) NO_EXCEPT
{
    return asset && atomic_get_acquire(&asset->is_loaded)
        && (asset->state & ASSET_STATE_IN_VRAM);
}

inline
Asset* ams_get_asset(AssetManagementSystem* const ams, const char* key) NO_EXCEPT
{
    HashEntry* const entry = hashmap_get_entry(&ams->hash_map, key);

    DEBUG_MEMORY_READ(
        (uint64) (entry ? ((Asset *) entry->value)->self : 0),
        entry ? ((Asset *) entry->value)->ram_size : 0
    );

    return entry ? (Asset *) entry->value : NULL;
}

// @performance We could probably avoid locking by adding a atomic flag to indicate if the value is valid
inline
Asset* thrd_ams_get_asset(AssetManagementSystem* const ams, const char* key) NO_EXCEPT
{
    HashEntry* const entry = hashmap_get_entry(&ams->hash_map, key);

    if (!entry || atomic_get_acquire(&((Asset *) entry->value)->is_loaded) <= 0) {
        return NULL;
    }

    DEBUG_MEMORY_READ(
        (uint64) (entry ? ((Asset *) entry->value)->self : 0),
        entry ? ((Asset *) entry->value)->ram_size : 0
    );

    return (Asset *) entry->value;
}

Asset* thrd_ams_get_asset_wait(AssetManagementSystem* const ams, const char* key) NO_EXCEPT
{
    HashEntry* const entry = hashmap_get_entry(&ams->hash_map, key);
    if (!entry) {
        return NULL;
    }

    int32 state = 0;
    while (!(state = atomic_get_acquire(&((Asset *) entry->value)->is_loaded))) {}
    if (state < 0) {
        // Marked for removal
        // @question Consider to change the state and return the asset?
        return NULL;
    }

    DEBUG_MEMORY_READ(
        (uint64) (entry ? ((Asset *) entry->value)->self : 0),
        entry ? ((Asset *) entry->value)->ram_size : 0
    );

    return (Asset *) entry->value;
}

Asset* thrd_ams_get_reserve_asset_wait(AssetManagementSystem* const ams, byte type, const char* name, uint32 size, uint32 overhead = 0) NO_EXCEPT
{
    // @bug Isn't hashmap_get_reserve unsafe for threading?
    HashEntry* const entry = hashmap_get_reserve(&ams->hash_map, name);
    Asset* const asset = (Asset *) entry->value;

    if (asset->self) {
        int32 state = 0;
        while (!(state = atomic_get_acquire(&((Asset *) entry->value)->is_loaded))) {}
        if (state > 0) {
            return asset;
        }
    }

    AssetComponent* const ac = &ams->asset_components[type];
    const uint16 elements = ams_calculate_chunks(ac, size, overhead);
    const int32 free_data = chunk_reserve(&ac->asset_memory, elements);

    byte* const data = chunk_get_element(&ac->asset_memory, free_data);

    asset->component_id = type;
    asset->self = data;
    asset->size = elements; // Crucial for freeing
    asset->ram_size = ac->asset_memory.chunk_size * elements;

    ac->vram_size += asset->vram_size;
    ac->ram_size += asset->ram_size;
    ++ac->asset_count;

    DEBUG_MEMORY_RESERVE((uintptr_t) asset, asset->ram_size);

    return asset;
}

inline
void ams_remove_asset(AssetManagementSystem* const ams, AssetComponent* ac, Asset* const asset, const char* name) NO_EXCEPT
{
    // @todo remove from vram

    asset->is_loaded = 0;
    ac->vram_size -= asset->vram_size;
    ac->ram_size -= asset->ram_size;
    --ac->asset_count;

    hashmap_remove(&ams->hash_map, name);
    chunk_free_elements(
        &ac->asset_memory,
        chunk_id_from_memory(
            (uintptr_t) ac->asset_memory.memory,
            (uintptr_t) asset->self,
            ac->asset_memory.chunk_size
        ),
        asset->size
    );
}

inline
void ams_remove_asset_ram(AssetComponent* const ac, const Asset* const asset) NO_EXCEPT
{
    ac->ram_size -= asset->ram_size;
    chunk_free_elements(
        &ac->asset_memory,
        chunk_id_from_memory(
            (uintptr_t) ac->asset_memory.memory,
            (uintptr_t) asset->self,
            ac->asset_memory.chunk_size
        ),
        asset->size
    );
}

// @todo It would be nice if we could remove the asset by passing it as a parameter instead of the name
// The problem is there is no correlation between asset data (e.g. internal_id) and global hashmap (e.g. element_id)
// This means we would have to iterate all hashmap entries and remove it this way, which is very slow
inline
void ams_remove_asset(AssetManagementSystem* const ams, const char* name) NO_EXCEPT
{
    // @todo remove from vram

    Asset* const asset = ams_get_asset(ams, name);
    AssetComponent* const ac = &ams->asset_components[asset->component_id];

    asset->is_loaded = 0;
    ac->vram_size -= asset->vram_size;
    ac->ram_size -= asset->ram_size;
    --ac->asset_count;

    hashmap_remove(&ams->hash_map, name);
    chunk_free_elements(
        &ac->asset_memory,
        chunk_id_from_memory(
            (uintptr_t) ac->asset_memory.memory,
            (uintptr_t) asset->self,
            ac->asset_memory.chunk_size
        ),
        asset->size
    );
}

inline
void ams_remove_asset(AssetManagementSystem* const ams, Asset* const asset, const char* name) NO_EXCEPT
{
    // @todo remove from vram

    AssetComponent* const ac = &ams->asset_components[asset->component_id];

    asset->is_loaded = 0;
    ac->vram_size -= asset->vram_size;
    ac->ram_size -= asset->ram_size;
    --ac->asset_count;

    hashmap_remove(&ams->hash_map, name);
    chunk_free_elements(
        &ac->asset_memory,
        chunk_id_from_memory(
            (uintptr_t) ac->asset_memory.memory,
            (uintptr_t) asset->self,
            ac->asset_memory.chunk_size
        ),
        asset->size
    );
}

inline
void ams_remove_asset_ram(AssetManagementSystem* const ams, const Asset* const asset) NO_EXCEPT
{
    AssetComponent* const ac = &ams->asset_components[asset->component_id];
    ac->ram_size -= asset->ram_size;

    chunk_free_elements(
        &ac->asset_memory,
        chunk_id_from_memory(
            (uintptr_t) ac->asset_memory.memory,
            (uintptr_t) asset->self,
            ac->asset_memory.chunk_size
        ),
        asset->size
    );
}

inline
void thrd_ams_remove_asset(AssetManagementSystem* const ams, AssetComponent* ac, Asset* const asset, const char* name) NO_EXCEPT
{
    // @todo remove from vram

    asset->is_loaded = 0;
    ac->vram_size -= asset->vram_size;
    ac->ram_size -= asset->ram_size;
    --ac->asset_count;

    atomic_set_release(&asset->is_loaded, 0);
    hashmap_remove(&ams->hash_map, name);
    chunk_free_elements(
        &ac->asset_memory,
        chunk_id_from_memory(
            (uintptr_t) ac->asset_memory.memory,
            (uintptr_t) asset->self,
            ac->asset_memory.chunk_size
        ),
        asset->size
    );
}

void thrd_ams_remove_asset(AssetManagementSystem* const ams, const char* name) NO_EXCEPT
{
    HashEntry* const entry = hashmap_get_entry(&ams->hash_map, name);
    Asset* const asset = (Asset *) entry->value;
    atomic_set_release(&asset->is_loaded, -1);
    hashmap_remove(&ams->hash_map, name);

    AssetComponent* const ac = &ams->asset_components[asset->component_id];
    chunk_free_elements(
        &ac->asset_memory,
        chunk_id_from_memory(
            (uintptr_t) ac->asset_memory.memory,
            (uintptr_t) asset->self,
            ac->asset_memory.chunk_size
        ),
        asset->size
    );

    ac->vram_size -= asset->vram_size;
    ac->ram_size -= asset->ram_size;
    --ac->asset_count;
}

void thrd_ams_remove_asset(AssetManagementSystem* const ams, const char* name, Asset* const asset) NO_EXCEPT
{
    atomic_set_release(&asset->is_loaded, -1);
    hashmap_remove(&ams->hash_map, name);

    AssetComponent* const ac = &ams->asset_components[asset->component_id];
    chunk_free_elements(
        &ac->asset_memory,
        chunk_id_from_memory(
            (uintptr_t) ac->asset_memory.memory,
            (uintptr_t) asset->self,
            ac->asset_memory.chunk_size
        ),
        asset->size
    );

    ac->vram_size -= asset->vram_size;
    ac->ram_size -= asset->ram_size;
    --ac->asset_count;
}

// @todo implement defragment command to optimize memory layout since the memory layout will become fragmented over time

Asset* ams_reserve_asset(AssetManagementSystem* const ams, byte type, const char* name, uint32 size, uint32 overhead = 0) NO_EXCEPT
{
    AssetComponent* const ac = &ams->asset_components[type];
    const uint16 elements = ams_calculate_chunks(ac, size, overhead);

    const int32 free_data = chunk_reserve(&ac->asset_memory, elements);
    if (free_data < 0) {
        ASSERT_TRUE(free_data >= 0);
        return NULL;
    }

    byte* const asset_data = chunk_get_element(&ac->asset_memory, free_data);
    Asset* const asset = (Asset *) hashmap_reserve(&ams->hash_map, name)->value;

    asset->component_id = type;
    asset->self = asset_data;
    asset->size = elements; // Crucial for freeing
    asset->ram_size = ac->asset_memory.chunk_size * elements;

    ac->vram_size += asset->vram_size;
    ac->ram_size += asset->ram_size;
    ++ac->asset_count;

    DEBUG_MEMORY_RESERVE((uintptr_t) asset, asset->ram_size);

    return asset;
}

inline
Asset* thrd_ams_reserve_asset(AssetManagementSystem* const ams, byte type, const char* name, uint32 size, uint32 overhead = 0) NO_EXCEPT
{
    AssetComponent* const ac = &ams->asset_components[type];
    const uint16 elements = ams_calculate_chunks(ac, size, overhead);

    // Required for guard
    // alternatively we could manually call _guard.unlock();
    MutexGuard _guard(&ams->asset_components[type].mtx);
    const int32 free_data = chunk_reserve(&ac->asset_memory, elements);
    if (free_data < 0) {
        ASSERT_TRUE(free_data >= 0);
        _guard.unlock();

        return NULL;
    }
    _guard.unlock();

    byte* const asset_data = chunk_get_element(&ac->asset_memory, free_data);

    Asset asset = {
        0, // .official_id =
        ac->asset_memory.chunk_size * elements, // .ram_size =
        0, // .vram_size =
        0, // .last_access =
        elements, // .size =
        0, // .is_loaded =
        type, // .componentid =
        0, // .state =
        0, // .reference_count =
        asset_data // .self =
    };

    ac->vram_size += asset.vram_size;
    ac->ram_size += asset.ram_size;
    ++ac->asset_count;

    DEBUG_MEMORY_RESERVE((uintptr_t) asset_data, asset.ram_size);

    return (Asset *) hashmap_insert(&ams->hash_map, name, (byte *) &asset)->value;
}

// @todo Find a way to handle manual ram/vram changes
// Either implement a ams_update(AssetManagementSystem* const ams, Asset* const asset) function
// Or set .has_changed = true (even if garbage collection gets set) and call this func somewhere (maybe thread?)
// Perform general ams update (stats and garbage collection)
// We perform multiple things in one iteration to reduce the iteration costs
// @todo don't use uint64 for time, use uint32 and use relative time to start of program
void thrd_ams_update(AssetManagementSystem* const ams, uint64 time, uint64 dt) NO_EXCEPT
{
    PROFILE(PROFILE_AMS_UPDATE);
    for (int32 i = 0; i < ams->asset_component_count; ++i) {
        ams->asset_components[i].vram_size = 0;
        ams->asset_components[i].ram_size = 0;
        ams->asset_components[i].asset_count = 0;
    }

    // Iterate the hash map to find all assets
    uint32 chunk_id = 0;
    chunk_iterate_start(&ams->hash_map.buf, chunk_id) {
        HashEntry* const entry = (HashEntry *) chunk_get_element(&ams->hash_map.buf, chunk_id);
        Asset* const asset = (Asset *) entry->value;

        if (!thrd_ams_is_loaded(asset)) {
            continue;
        }

        ams->asset_components[asset->component_id].vram_size += asset->vram_size;
        ams->asset_components[asset->component_id].ram_size += asset->ram_size;
        ++ams->asset_components[asset->component_id].asset_count;

        if ((asset->state & ASSET_STATE_RAM_GC) || (asset->state & ASSET_STATE_VRAM_GC)) {
            // @todo Currently we cannot really delete based on last access since we are not updating the last_access reliably
            // This is usually the case for global/static assets such as font, ui_asset = ui vertices, ...
            // The reason for this is that we store a reference to those assets in a global struct
            // One solution could be to manually update the last_access at the end of every frame (shouldn't be THAT many assets)?
            // Maybe we can even implement a scene specific post_scene() function that does this for scene specific assets e.g. post_scene_scene1();
            if ((asset->state & ASSET_STATE_RAM_GC)
                && (asset->state & ASSET_STATE_VRAM_GC)
                && time - asset->last_access <= dt
            ) {
                // @performance Ideally we would like to pass the entry to delete
                // The problem is the hashmap_delete function can't work with entries directly since it is not a doubly linked list
                thrd_ams_remove_asset(ams, &ams->asset_components[asset->component_id], asset, entry->key);
            } else if ((asset->state & ASSET_STATE_RAM_GC)
                && time - asset->last_access <= dt
            ) {
                ams_remove_asset_ram(&ams->asset_components[asset->component_id], asset);
            } else if ((asset->state & ASSET_STATE_VRAM_GC)
                && time - asset->last_access <= dt
            ) {
                ams->asset_components[asset->component_id].vram_size -= asset->vram_size;
            }
        }
    } chunk_iterate_end;
}

Asset* ams_insert_asset(AssetManagementSystem* const ams, Asset* const asset_temp, const char* name) NO_EXCEPT
{
    AssetComponent* const ac = &ams->asset_components[asset_temp->component_id];

    const int32 free_data = chunk_reserve(&ac->asset_memory, asset_temp->size);
    if (free_data < 0) {
        ASSERT_TRUE(free_data >= 0);
        return NULL;
    }

    byte* const asset_data = chunk_get_element(&ac->asset_memory, free_data);

    asset_temp->self = asset_data;
    asset_temp->ram_size = ac->asset_memory.chunk_size * asset_temp->size;

    ac->vram_size += asset_temp->vram_size;
    ac->ram_size += asset_temp->ram_size;
    ++ac->asset_count;

    Asset* const asset = (Asset *) hashmap_insert(&ams->hash_map, name, (byte *) asset_temp)->value;
    DEBUG_MEMORY_RESERVE((uintptr_t) asset->self, asset->ram_size);

    return asset;
}

inline
Asset* thrd_ams_insert_asset(AssetManagementSystem* const ams, Asset* const asset_temp, const char* name) NO_EXCEPT
{
    AssetComponent* const ac = &ams->asset_components[asset_temp->component_id];

    MutexGuard _guard(&ams->asset_components[asset_temp->component_id].mtx);
    const int32 free_data = chunk_reserve(&ac->asset_memory, asset_temp->size);
    if (free_data < 0) {
        ASSERT_TRUE(free_data >= 0);
        _guard.unlock();

        return NULL;
    }
    _guard.unlock();

    byte* const asset_data = chunk_get_element(&ac->asset_memory, free_data);
    memcpy(asset_data, asset_temp->self, sizeof(Asset));

    asset_temp->self = asset_data;
    asset_temp->ram_size = ac->asset_memory.chunk_size * asset_temp->size;

    ac->vram_size += asset_temp->vram_size;
    ac->ram_size += asset_temp->ram_size;
    ++ac->asset_count;

    Asset* const asset = (Asset *) hashmap_insert(&ams->hash_map, name, (byte *) asset_temp)->value;
    DEBUG_MEMORY_RESERVE((uintptr_t) asset->self, asset->ram_size);

    atomic_set_release(&asset->is_loaded, 1);

    return asset;
}

#endif