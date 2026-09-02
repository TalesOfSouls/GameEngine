/**
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_ASSET_MANAGEMENT_SYSTEM_C
#define COMS_ASSET_MANAGEMENT_SYSTEM_C

#include "../stdlib/Stdlib.h"
#include "../memory/ThrdChunkMemory.cpp"
#include "../utils/BitUtils.h"
#include "../stdlib/ThrdHashMapT.cpp"
#include "../log/DebugMemory.h"
#include "../thread/Atomic.h"
#include "Asset.h"
#include "AssetManagementSystem.h"

inline
void ams_create(AssetManagementSystem* const ams, BufferMemory* const buf, int32 asset_component_count, int32 count) NO_EXCEPT
{
    LOG_1("[INFO] Create AMS for %n assets", {DATA_TYPE_INT32, &count});
    hashmap_init(&ams->hash_map, count, buf);
    ams->asset_component_count = asset_component_count;
    ams->asset_components = (AssetComponent *) memory_get(
        buf,
        asset_component_count * sizeof(AssetComponent),
        alignof(AssetComponent)
    );

    //memset(ams->asset_components, 0, asset_component_count * sizeof(AssetComponent));
    memset(ams->asset_components, 0, align_up(asset_component_count * sizeof(AssetComponent), alignof(AssetComponent)));
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
}

inline
void ams_component_create(AssetComponent* ac, byte* buf, int32 chunk_size, int32 count) NO_EXCEPT
{
    ASSERT_TRUE(chunk_size);
    LOG_1(
        "[INFO] Create AMS Component for %n assets and %n B",
        {DATA_TYPE_INT32, &count}, {DATA_TYPE_UINT32, &chunk_size}
    );

    chunk_init(&ac->asset_memory, buf, count, chunk_size, sizeof(size_t));
}

FORCE_INLINE
void ams_component_free(AssetComponent* ac) NO_EXCEPT
{
    chunk_free(&ac->asset_memory);
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

/**
 * We are marking the Asset and the asset data as completely loaded
 */
FORCE_INLINE
void ams_set_loaded(AssetManagementSystem* const ams, Asset* const asset) NO_EXCEPT
{
    chunk_mark_complete(&ams->hash_map.buf, asset);

    if (asset->self) {
        // We need to find the AssetComponent based on the memory position
        // @performance Do we even need to mark it completed? Isn't marking the asset enough?
        for (int i = 0; i < ams->asset_component_count; ++i) {
            if ((uintptr_t) asset->self >= (uintptr_t) ams->asset_components[i].asset_memory.memory
                && (uintptr_t) asset->self < ((uintptr_t) ams->asset_components[i].asset_memory.memory) + ams->asset_components[i].asset_memory.size
            ) {
                chunk_mark_complete(&ams->asset_components[i].asset_memory, (void *) asset->self);
                break;
            }
        }
    }
}

FORCE_INLINE
bool ams_is_loaded(const AssetManagementSystem* const ams, Asset* const asset) NO_EXCEPT
{
    if (!asset || !chunk_is_complete(&ams->hash_map.buf, asset)) {
        return false;
    }

    if (asset->self) {
        // We need to find the AssetComponent based on the memory position
        // @performance Do we even need to mark it completed? Isn't marking the asset enough?
        for (int i = 0; i < ams->asset_component_count; ++i) {
            if ((uintptr_t) asset->self >= (uintptr_t) ams->asset_components[i].asset_memory.memory
                && (uintptr_t) asset->self < ((uintptr_t) ams->asset_components[i].asset_memory.memory) + ams->asset_components[i].asset_memory.size
            ) {
                return chunk_is_complete(&ams->asset_components[i].asset_memory, asset->self);
            }
        }
    }

    return true;
}

FORCE_INLINE
bool ams_is_in_vram(Asset* const asset) NO_EXCEPT
{
    return asset && (asset->state & ASSET_MEMORY_STATE_IN_VRAM);
}

inline
Asset* ams_asset_get(AssetManagementSystem* const ams, const char* key) NO_EXCEPT
{
    ThrdHashEntryStrT<Asset>* const entry = hashmap_entry_get(&ams->hash_map, key);

    if (!entry || !chunk_is_complete(&ams->hash_map.buf, entry)) {
        return NULL;
    }

    DEBUG_MEMORY_READ(
        (uintptr_t) (entry ? entry->value.self : 0),
        (entry ? entry->value.ram_size : 0)
    );

    return &entry->value;
}

Asset* ams_asset_get_wait(AssetManagementSystem* const ams, const char* key) NO_EXCEPT
{
    ThrdHashEntryStrT<Asset>* const entry = hashmap_entry_get(&ams->hash_map, key);
    if (!entry) {
        return NULL;
    }

    bool is_loaded;
    while (!(is_loaded = chunk_is_complete(&ams->hash_map.buf, entry))) {
        YieldProcessor();
    }

    if (!is_loaded) {
        // Marked for removal
        return NULL;
    }

    DEBUG_MEMORY_READ(
        (uintptr_t) (entry ? entry->value.self : 0),
        (entry ? entry->value.ram_size : 0)
    );

    return &entry->value;
}

Asset* ams_asset_get_reserve_wait(AssetManagementSystem* const ams, byte type, const char* name, uint32 size, uint32 overhead = 0) NO_EXCEPT
{
    ThrdHashEntryStrT<Asset>* const entry = hashmap_get_reserve(&ams->hash_map, name);
    Asset* const asset = &entry->value;

    if (asset->self) {
        bool is_loaded;
        while (!(is_loaded = chunk_is_complete(&ams->hash_map.buf, entry))) {
            YieldProcessor();
        }

        if (is_loaded) {
            return asset;
        }
    }

    AssetComponent* const ac = &ams->asset_components[type];
    const uint16 elements = ams_calculate_chunks(ac, size, overhead);
    const int32 free_data = chunk_reserve(&ac->asset_memory, elements);

    byte* const data = chunk_element_get(&ac->asset_memory, free_data);

    asset->component_id = type;
    asset->self = data;
    asset->chunk_count = elements; // Crucial for freeing
    asset->ram_size = ac->asset_memory.chunk_size * elements;

    ac->vram_size.fetch_add(asset->vram_size);
    ac->ram_size.fetch_add(asset->ram_size);
    ++ac->asset_count;

    DEBUG_MEMORY_WRITE((uintptr_t) asset, asset->ram_size);

    return asset;
}

inline
void ams_remove_asset_ram(AssetComponent* const ac, const Asset* const asset) NO_EXCEPT
{
    ac->ram_size.fetch_sub(asset->ram_size);
    chunk_free_elements(
        &ac->asset_memory,
        chunk_id_from_memory(
            (uintptr_t) ac->asset_memory.memory,
            (uintptr_t) asset->self,
            ac->asset_memory.chunk_size
        ),
        asset->chunk_count
    );
}

inline
void ams_remove_asset_ram(AssetManagementSystem* const ams, const Asset* const asset) NO_EXCEPT
{
    AssetComponent* const ac = &ams->asset_components[asset->component_id];
    ac->ram_size.fetch_sub(asset->ram_size);

    chunk_free_elements(
        &ac->asset_memory,
        chunk_id_from_memory(
            (uintptr_t) ac->asset_memory.memory,
            (uintptr_t) asset->self,
            ac->asset_memory.chunk_size
        ),
        asset->chunk_count
    );
}

inline
void ams_asset_remove(AssetManagementSystem* const ams, AssetComponent* ac, Asset* const asset, const char* name) NO_EXCEPT
{
    ac->vram_size.fetch_sub(asset->vram_size);
    ac->ram_size.fetch_sub(asset->ram_size);
    --ac->asset_count;

    chunk_free_elements(
        &ac->asset_memory,
        chunk_id_from_memory(
            (uintptr_t) ac->asset_memory.memory,
            (uintptr_t) asset->self,
            ac->asset_memory.chunk_size
        ),
        asset->chunk_count
    );

    hashmap_remove(&ams->hash_map, name);
}

void ams_asset_remove(AssetManagementSystem* const ams, const char* name) NO_EXCEPT
{
    ThrdHashEntryStrT<Asset>* const entry = hashmap_entry_get(&ams->hash_map, name);

    Asset* const asset = &entry->value;
    AssetComponent* const ac = &ams->asset_components[asset->component_id];

    ac->vram_size.fetch_sub(asset->vram_size);
    ac->ram_size.fetch_sub(asset->ram_size);
    --ac->asset_count;

    chunk_free_elements(
        &ac->asset_memory,
        chunk_id_from_memory(
            (uintptr_t) ac->asset_memory.memory,
            (uintptr_t) asset->self,
            ac->asset_memory.chunk_size
        ),
        asset->chunk_count
    );

    hashmap_remove(&ams->hash_map, name);
}

void ams_asset_remove(AssetManagementSystem* const ams, const char* name, Asset* const asset) NO_EXCEPT
{
    AssetComponent* const ac = &ams->asset_components[asset->component_id];
    ac->vram_size.fetch_sub(asset->vram_size);
    ac->ram_size.fetch_sub(asset->ram_size);
    --ac->asset_count;

    chunk_free_elements(
        &ac->asset_memory,
        chunk_id_from_memory(
            (uintptr_t) ac->asset_memory.memory,
            (uintptr_t) asset->self,
            ac->asset_memory.chunk_size
        ),
        asset->chunk_count
    );

    hashmap_remove(&ams->hash_map, name);
}

// @todo implement defragment command to optimize memory layout since the memory layout will become fragmented over time

/**
 * Finds the best component match based on requested size
 */
static inline
byte ams_component_find_type(const AssetManagementSystem* const ams, uint32 size, uint32 overhead)
{
    const f32 required_size = (f32) (size + overhead);

    byte type = 0;

    for (int i = 1; i < AMS_TYPE_SIZE; ++i) {
        const f32 c_size = (f32) ams->asset_components[i].asset_memory.chunk_size;

        // @todo we should also check if there is still room available in this component
        //      or one step further how much of the total chunks this one would occupy and if it is > 20% go to next component

        // This is an extremely simple approach.
        // We only take the next larger component if the required size is above a certain threshold
        if (required_size < 0.33f * c_size) {
            return type;
        }

        type = (byte) i;
    }

    return type;
}

inline
Asset* ams_asset_reserve(
    AssetManagementSystem* const ams,
    const char* name,
    uint32 size, uint32 overhead = 0
) NO_EXCEPT
{
    const byte type = ams_component_find_type(ams, size, overhead);

    AssetComponent* const ac = &ams->asset_components[type];
    const uint16 elements = ams_calculate_chunks(ac, size, overhead);

    // Required for guard
    const int32 free_data = chunk_reserve(&ac->asset_memory, elements);
    if (free_data < 0) {
        ASSERT_TRUE(free_data >= 0);

        return NULL;
    }

    byte* const asset_data = chunk_element_get(&ac->asset_memory, free_data);

    Asset asset = {
        SMN(official_id) 0,
        SMN(ram_size) ac->asset_memory.chunk_size * elements,
        SMN(vram_size) 0,
        SMN(last_access) 0,
        SMN(chunk_count) elements,
        SMN(component_id) type,
        SMN(state) 0,
        SMN(is_persistent) false,
        SMN(reference_count) 0,
        SMN(self) asset_data
    };

    ac->vram_size.fetch_add(asset.vram_size);
    ac->ram_size.fetch_add(asset.ram_size);
    ++ac->asset_count;

    DEBUG_MEMORY_WRITE((uintptr_t) asset_data, asset.ram_size);

    ThrdHashEntryStrT<Asset>* const entry = hashmap_insert(&ams->hash_map, name, &asset);

    return entry ? &entry->value : NULL;
}

// @todo Find a way to handle manual ram/vram changes
// Either implement a ams_update(AssetManagementSystem* const ams, Asset* const asset) function
// Or set .has_changed = true (even if garbage collection gets set) and call this func somewhere (maybe thread?)
// Perform general ams update (stats and garbage collection)
// We perform multiple things in one iteration to reduce the iteration costs
// @todo don't use uint64 for time, use uint32 and use relative time to start of program
void ams_update(AssetManagementSystem* const ams, uint64 time, uint64 dt) NO_EXCEPT
{
    PROFILE_DEBUG(PROFILE_AMS_UPDATE);
    for (int32 i = 0; i < ams->asset_component_count; ++i) {
        ams->asset_components[i].vram_size.store(0);
        ams->asset_components[i].ram_size.store(0);
        ams->asset_components[i].asset_count.store(0);
    }

    // Iterate the hash map to find all assets
    int32 chunk_id = 0;
    chunk_iterate_start(&ams->hash_map.buf, chunk_id) {
        ThrdHashEntryStrT<Asset>* const entry = (ThrdHashEntryStrT<Asset> *) chunk_element_get(&ams->hash_map.buf, chunk_id);
        Asset* const asset = &entry->value;

        if (!ams_is_loaded(ams, asset)) {
            continue;
        }

        ams->asset_components[asset->component_id].vram_size.fetch_add(asset->vram_size);
        ams->asset_components[asset->component_id].ram_size.fetch_add(asset->ram_size);
        ++ams->asset_components[asset->component_id].asset_count;

        if ((asset->state & ASSET_MEMORY_STATE_RAM_GC) || (asset->state & ASSET_MEMORY_STATE_VRAM_GC)) {
            // @todo Currently we cannot really delete based on last access since we are not updating the last_access reliably
            // This is usually the case for global/static assets such as font, ui_asset = ui vertices, ...
            // The reason for this is that we store a reference to those assets in a global struct
            // One solution could be to manually update the last_access at the end of every frame (shouldn't be THAT many assets)?
            // Maybe we can even implement a scene specific post_scene() function that does this for scene specific assets e.g. post_scene_scene1();
            if ((asset->state & ASSET_MEMORY_STATE_RAM_GC)
                && (asset->state & ASSET_MEMORY_STATE_VRAM_GC)
                && time - asset->last_access <= dt
            ) {
                // @performance Ideally we would like to pass the entry to delete
                // The problem is the hashmap_delete function can't work with entries directly since it is not a doubly linked list
                ams_asset_remove(ams, &ams->asset_components[asset->component_id], asset, entry->key);
            } else if ((asset->state & ASSET_MEMORY_STATE_RAM_GC)
                && time - asset->last_access <= dt
            ) {
                ams_remove_asset_ram(&ams->asset_components[asset->component_id], asset);
            } else if ((asset->state & ASSET_MEMORY_STATE_VRAM_GC)
                && time - asset->last_access <= dt
            ) {
                ams->asset_components[asset->component_id].vram_size -= asset->vram_size;
            }
        }
    } chunk_iterate_end;
}

inline
Asset* ams_asset_insert(AssetManagementSystem* const ams, Asset* const asset_temp, const char* name) NO_EXCEPT
{
    AssetComponent* const ac = &ams->asset_components[asset_temp->component_id];

    const int32 free_data = chunk_reserve(&ac->asset_memory, asset_temp->chunk_count);
    if (free_data < 0) {
        ASSERT_THROW();

        return NULL;
    }

    byte* const asset_data = chunk_element_get(&ac->asset_memory, free_data);
    memcpy(asset_data, asset_temp->self, sizeof(Asset));
    chunk_mark_complete(&ac->asset_memory, free_data);

    asset_temp->self = asset_data;
    asset_temp->ram_size = ac->asset_memory.chunk_size * asset_temp->chunk_count;

    ac->vram_size.fetch_add(asset_temp->vram_size);
    ac->ram_size.fetch_add(asset_temp->ram_size);
    ++ac->asset_count;

    Asset* const asset = (Asset *) &hashmap_insert(&ams->hash_map, name, asset_temp)->value;
    DEBUG_MEMORY_WRITE((uintptr_t) asset->self, asset->ram_size);

    return asset;
}

#endif