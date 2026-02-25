/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_ENTITY_COMPONENT_SYSTEM_H
#define COMS_ENTITY_COMPONENT_SYSTEM_H

#include "../stdlib/Stdlib.h"
#include "../memory/ChunkMemory.h"
#include "../thread/ThreadDefines.h"

// Entities can be directly accessed by their id
// highest byte = entity type, lower bytes = id in respective ecs
struct EntityComponentSystem {
    int32 entity_type_count;
    int32 component_type_count;

    ChunkMemory* entities;
    ChunkMemory* components;

    uint64 ram_size;
    uint64 vram_size;
    uint64 entity_count;
    uint64 component_count;

    // @question Do we want to add a mutex to assets. This way we don't have to lock the entire ams.
    mutex* entity_mutex;
    mutex* component_mutex;
};

#endif