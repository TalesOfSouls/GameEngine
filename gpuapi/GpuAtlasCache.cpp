/**
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_GPUAPI_GPU_CACHE_C
#define COMS_GPUAPI_GPU_CACHE_C

#include "GpuAtlasCache.h"
#include "../stdlib/HashMapT.cpp"

inline CONSTEXPR
size_t gpu_atlas_cache_size(uint16 dim, uint16 min_dim) NO_EXCEPT
{
    const uint16 element_count = dim / min_dim;

    // @todo eventually we cannot assume squares
    return element_count * 2 * sizeof(HashEntryT<uint32, uint16>) + ASSUMED_CACHE_LINE_SIZE
        + element_count * sizeof(GpuAtlasRect) + ASSUMED_CACHE_LINE_SIZE
        + element_count * sizeof(GpuAtlasElement) + ASSUMED_CACHE_LINE_SIZE
        + element_count * 4 * sizeof(v2_f32) + ASSUMED_CACHE_LINE_SIZE
        + ceil_div_pow2<(int32) (sizeof(size_t) * 8)>(element_count) + ASSUMED_CACHE_LINE_SIZE; // changed bit array
}

void gpu_atlas_cache_init(GpuAtlasCache* cache, uint16 dim, uint16 min_dim, byte* data) NO_EXCEPT
{
    cache->element_capacity = dim / min_dim;
    cache->dim = dim;
    cache->min_dim = min_dim;

    hashmap_init(&cache->hm, cache->element_capacity, data);
    data += hashmap_size(&cache->hm);

    cache->changed = (uint64 *) ALIGN_UP((uintptr_t) data, alignof(uint64));
    data += ceil_div_pow2<(int32) (sizeof(size_t) * 8)>(cache->element_capacity);

    cache->free_rects = (GpuAtlasRect *) ALIGN_UP((uintptr_t) data, alignof(GpuAtlasRect));
    data += cache->element_capacity * sizeof(GpuAtlasRect);

    cache->elements = (GpuAtlasElement *) ALIGN_UP((uintptr_t) data, alignof(GpuAtlasElement));
    data += cache->element_capacity * sizeof(GpuAtlasElement);

    cache->uv = (v2_f32 *) ALIGN_UP((uintptr_t) data, alignof(v2_f32));

    // The default free rect is always the original box
    cache->free_rects[0] = {dim, 0};
    cache->free_count = 1;

    spinlock_init(&cache->lock);
}

inline
GpuAtlasElement* gpu_atlas_cache_find(GpuAtlasCache* cache, uint32 cache_id) NO_EXCEPT
{
    SpinlockGuard _guard = SpinlockGuard(&cache->lock);

    HashEntryT<uint32, uint16>* entry = hashmap_entry_get(&cache->hm, cache_id);
    if (!entry) {
        return NULL;
    }

    return (GpuAtlasElement *) &cache->elements[entry->value];
}

FORCE_INLINE
bool gpu_atlas_cache_exists(GpuAtlasCache* cache, uint32 cache_id) NO_EXCEPT
{
    SpinlockGuard _guard = SpinlockGuard(&cache->lock);

    return (bool) hashmap_entry_get(&cache->hm, cache_id);
}

GpuAtlasElement* gpu_atlas_cache_reserve(
    GpuAtlasCache* cache,
    uint32 cache_id,
    v2_uint16 rect
) NO_EXCEPT
{
    // Adjust rect to match minimum dimensions
    rect.width = ALIGN_UP(rect.width, cache->min_dim);
    rect.height = ALIGN_UP(rect.height, cache->min_dim);

    int best_match_id = -1;
    GpuAtlasRect* best_match = NULL;

    SpinlockGuard _guard = SpinlockGuard(&cache->lock);

    for (int i = cache->free_count - 1; i >= 0; --i) {
        GpuAtlasRect* const free_rect = &cache->free_rects[i];
        // Finding free rect that can contain rect with as little waste as possible
        // @performance instead of defining the best match by as little waste as possible
        //      it could make sense to give a perfect width or height match a higher priority
        //      and only afterwards focus on the wasted space and if no perfect match exists
        //      then focus on the overall wasted area
        //      The reason for that is that a perfect width/height match reduces the fragmentation of the remaining space
        if ((rect.width <= free_rect->dim.width
            && rect.height <= free_rect->dim.height) // rect fits?
            && (!best_match || free_rect->dim.width * free_rect->dim.height < best_match->dim.width * best_match->dim.height) // wastes less space
        ) {
            best_match_id = i;
            best_match = free_rect;
        }
    }

    if (!best_match) {
        return NULL;
    }

    GpuAtlasElement* const element = &cache->elements[best_match->element_id];
    element->dim = best_match->dim;

    // IF best match 100% fits (width and height) we have to move the free_rects array around
    // If we didn't have a perfect match we have to split our existing free rect
    // This is either a split in 2 or 3 parts
    //  A: 1 used 1 free (if one axis is a perfect match)
    //  B: 1 used, 2 free if no axis is a perfect match

    const int height_count = cache->dim / cache->min_dim;

    if (best_match->dim.width == rect.width && best_match->dim.height == rect.height) {
        if (best_match_id != cache->free_count - 1) {
            memmove(
                cache->free_rects + best_match_id,
                cache->free_rects + (best_match_id + 1),
                (cache->free_count - 1 - best_match_id) * sizeof(GpuAtlasRect)
            );
        }

        --cache->free_count;
    } else if (rect.width == best_match->dim.width) {
        // We can just replace the free_rects entry with the split rect
        best_match->dim.y += rect.height;
        best_match->dim.height -= rect.height;

        best_match->element_id = height_count * ((best_match->dim.height - cache->min_dim) / cache->min_dim)
            + best_match->dim.x / cache->min_dim;
    } else if (rect.height == best_match->dim.height) {
        // We can just replace the free_rects entry with the split rect
        best_match->dim.x += rect.width;
        best_match->dim.width -= rect.width;

        best_match->element_id = height_count * ((best_match->dim.height - cache->min_dim) / cache->min_dim)
            + best_match->dim.x / cache->min_dim;
    } else {
        // We need to add one additional free rect due to the split
        GpuAtlasRect* new_rect = &cache->free_rects[cache->free_count];
        new_rect->dim = {
            best_match->dim.x,
            (uint16) (best_match->dim.y + rect.height),
            best_match->dim.width,
            (uint16) (best_match->dim.height - rect.height)
        };
        new_rect->element_id = 0;

        new_rect->element_id = height_count * ((new_rect->dim.height - cache->min_dim) / cache->min_dim)
            + new_rect->dim.x / cache->min_dim;

        ++cache->free_count;

        // We can just replace the free_rects entry with the other split rect
        best_match->dim.x += rect.width;
        best_match->dim.width -= rect.width;
        best_match->dim.height = rect.height;

        best_match->element_id = height_count * ((best_match->dim.height - cache->min_dim) / cache->min_dim)
            + best_match->dim.x / cache->min_dim;
    }

    HashEntryT<uint32, uint16>* entry = hashmap_reserve(&cache->hm, cache_id);

    // This is REALLY bad and shouldn't happen.
    ASSERT_TRUE(entry);

    entry->value = (uint16) best_match_id;

    return element;
}

void gpu_atlas_cache_completed(
    GpuAtlasCache* cache,
    uint32 cache_id
) NO_EXCEPT
{
    HashEntryT<uint32, uint16>* entry = hashmap_entry_get(&cache->hm, cache_id);
    if (!entry) {
        return;
    }

    cache->elements[entry->value].completed.store(true);
}

void gpu_atlas_cache_incompleted(
    GpuAtlasCache* cache,
    uint32 cache_id
) NO_EXCEPT
{
    HashEntryT<uint32, uint16>* entry = hashmap_entry_get(&cache->hm, cache_id);
    if (!entry) {
        return;
    }

    cache->elements[entry->value].completed.store(false);
}

// @todo we need to implement a defragment function that
//      1. merges neighboring rects
//      2. re-orders elements so we can create larger free rects

#endif