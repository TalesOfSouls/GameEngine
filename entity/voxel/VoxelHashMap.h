/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_ENTITY_VOXEL_HASH_MAP_H
#define COMS_ENTITY_VOXEL_HASH_MAP_H

#include "../../stdlib/Stdlib.h"
#include "Voxel.h"

static FORCE_INLINE
uint64 voxel_chunk_coord_pack(int32 x, int32 y, int32 z) NO_EXCEPT
{
    // We check if only the lowest 21 bits are set
    ASSERT_STRICT((((uint32) (x)) & ~((1u << 21) - 1u)) == 0);
    ASSERT_STRICT((((uint32) (y)) & ~((1u << 21) - 1u)) == 0);
    ASSERT_STRICT((((uint32) (z)) & ~((1u << 21) - 1u)) == 0);

    // 21 bits per axis signed (fits +/- 1,048,575)
    uint64 ux = ((uint64) (uint32) x) & 0x1FFFFF;
    uint64 uy = ((uint64) (uint32) y) & 0x1FFFFF;
    uint64 uz = ((uint64) (uint32) z) & 0x1FFFFF;

    return (ux) | (uy << 21) | (uz << 42);
}

FORCE_INLINE
HashEntryVoidPKeyInt64* voxel_hashmap_insert(HashMap* __restrict hm, int32 x, int32 y, int32 z, VoxelChunk* __restrict chunk) NO_EXCEPT
{
    uint64 key = voxel_chunk_coord_pack(x, y, z);
    return hashmap_insert(hm, key, (void *) chunk);
}

FORCE_INLINE
void voxel_hashmap_remove(HashMap* hm, int32 x, int32 y, int32 z) NO_EXCEPT
{
    uint64 key = voxel_chunk_coord_pack(x, y, z);
    hashmap_remove(hm, key);
}

FORCE_INLINE
HashEntryVoidPKeyInt64* voxel_hashmap_get_entry(HashMap* hm, int32 x, int32 y, int32 z) NO_EXCEPT
{
    uint64 key = voxel_chunk_coord_pack(x, y, z);
    return (HashEntryVoidPKeyInt64 *) hashmap_get_entry(hm, key);
}

FORCE_INLINE
VoxelChunk* voxel_hashmap_get_value(HashMap* hm, int32 x, int32 y, int32 z) NO_EXCEPT
{
    HashEntryVoidPKeyInt64* entry = (HashEntryVoidPKeyInt64 *) voxel_hashmap_get_entry(hm, x, y, z);

    return entry ? (VoxelChunk *) entry->value : NULL;
}

#endif