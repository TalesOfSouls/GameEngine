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

#include <string.h>
#include "../stdlib/Types.h"
#include "../memory/ChunkMemory.h"
#include "../utils/TestUtils.h"
#include "../utils/BitUtils.h"
#include "../stdlib/HashMap.h"
#include "../log/DebugMemory.h"

#include "Entity.h"

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

inline
void ecs_create(EntityComponentSystem* ecs, BufferMemory* buf, int32 entity_count, int32 component_count)
{
    ecs->entity_type_count = entity_count;
    ecs->entities = (ChunkMemory *) buffer_get_memory(buf, sizeof(ChunkMemory) * entity_count, 64);

    ecs->component_type_count = component_count;
    ecs->components = (ChunkMemory *) buffer_get_memory(buf, sizeof(ChunkMemory) * component_count, 64);
}

inline
void ecs_entity_type_create(ChunkMemory* ec, BufferMemory* buf, int32 chunk_size, int32 count)
{
    ASSERT_SIMPLE(chunk_size);

    chunk_init(ec, buf, count, chunk_size, 64);
    //mutex_init(&ec->mtx, NULL);
}

inline
void ecs_component_type_create(ChunkMemory* ec, BufferMemory* buf, int32 chunk_size, int32 count)
{
    ASSERT_SIMPLE(chunk_size);

    chunk_init(ec, buf, count, chunk_size, 64);
    //mutex_init(&ec->mtx, NULL);
}

Entity* ecs_get_entity(EntityComponentSystem* ecs, int32 entity_id)
{
    int32 ecs_type = (entity_id >> 24) & 0xFF;
    int32 raw_id = entity_id & 0x00FFFFFF;

    int32 byte_index = raw_id / 64;
    int32 bit_index = raw_id & 63;

    return IS_BIT_SET_64_R2L(ecs->entities[ecs_type].free[byte_index], bit_index) ?
        (Entity *) chunk_get_element(&ecs->entities[ecs_type], raw_id)
        : NULL;
}

Entity* ecs_reserve_entity(EntityComponentSystem* ecs, uint32 entity_type)
{
    ChunkMemory* mem = &ecs->entities[entity_type];
    int32 free_entity = chunk_reserve(mem, 1);
    if (free_entity < 0) {
        ASSERT_SIMPLE(free_entity >= 0);
        return NULL;
    }

    Entity* entity = (Entity *) chunk_get_element(mem, free_entity);

    // @todo log entity stats (count, ram, vram)

    return entity;
}

Entity* ecs_insert_entity(EntityComponentSystem* ecs, Entity* entity_temp, int32 entity_type)
{
    ChunkMemory* mem = &ecs->entities[entity_type];
    int32 free_entity = chunk_reserve(mem, 1);
    if (free_entity < 0) {
        ASSERT_SIMPLE(free_entity >= 0);
        return NULL;
    }

    Entity* entity = (Entity *) chunk_get_element(mem, free_entity);
    memcpy(entity, entity_temp, mem->chunk_size);

    // @todo log entity stats (count, ram, vram)
    //DEBUG_MEMORY_RESERVE((uintptr_t) entity, entity->ram_size);

    return entity;
}

void ecs_insert_component()
{

}

#endif