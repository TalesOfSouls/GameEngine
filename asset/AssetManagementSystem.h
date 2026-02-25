/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_ASSET_MANAGEMENT_SYSTEM_H
#define COMS_ASSET_MANAGEMENT_SYSTEM_H

#include "../stdlib/Stdlib.h"
#include "../memory/ChunkMemory.h"
#include "../stdlib/HashMap.h"
#include "../thread/ThreadDefines.h"

enum AssetManagementType {
    AMS_TYPE_SMALL,
    AMS_TYPE_MEDIUM,
    AMS_TYPE_LARGE,
    AMS_TYPE_VERY_LARGE,
    AMS_TYPE_SIZE
};

// The major asset types should have their own asset component system
// All other entities are grouped together in one asset component system
struct AssetComponent {
    // This is were the actual asset data is stored
    ChunkMemory asset_memory;

    uint64 ram_size;
    uint64 vram_size;
    uint64 asset_count;

    // @question Do we want to add a mutex to assets. This way we don't have to lock the entire ams.
    mutex mtx;
};

// @performance This doesn't really have anything to do with the AMS but how we currently operate
// We have everything on the gpu also in RAM. This is probably stupid and needs to be changed.
// Once core stuff is on the gpu, it should be removed from RAM (at least after n seconds)
struct AssetManagementSystem {
    // Used to find an asset in any asset component
    // @performance Change to HashMapT<???>
    HashMap hash_map;

    int32 asset_component_count;
    AssetComponent* asset_components;
};

#endif