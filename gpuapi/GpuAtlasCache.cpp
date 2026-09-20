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
        + sizeof(size_t) * ceil_div_pow2<(int32) (sizeof(size_t) * 8)>(element_count) // changed bit array
        + ASSUMED_CACHE_LINE_SIZE;
}

void gpu_atlas_cache_init(GpuAtlasCache* cache, uint16 dim, uint16 min_dim, BufferMemory* const buf) NO_EXCEPT
{
    cache->element_capacity = dim / min_dim;
    cache->dim = dim;
    cache->min_dim = min_dim;

    hashmap_init(&cache->hm, cache->element_capacity, buf);

    cache->changed = (uint64 *) memory_get(
        buf,
        sizeof(size_t) * ceil_div_pow2<(int32) (sizeof(size_t) * 8)>(cache->element_capacity),
        alignof(uint64)
    );

    cache->free_rects = (GpuAtlasRect *) memory_get(
        buf,
        cache->element_capacity * sizeof(GpuAtlasRect),
        alignof(GpuAtlasRect)
    );

    cache->elements = (GpuAtlasElement *) memory_get(
        buf,
        cache->element_capacity * sizeof(GpuAtlasElement),
        alignof(GpuAtlasElement)
    );

    cache->uv = (v2_f32 *) memory_get(
        buf,
        cache->element_capacity * 4 * sizeof(v2_f32),
        alignof(v2_f32)
    );

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
    bool best_match_is_perfect = false;
    int64 best_wasted_area = 0;

    SpinlockGuard _guard = SpinlockGuard(&cache->lock);

    for (int i = cache->free_count - 1; i >= 0; --i) {
        GpuAtlasRect* const free_rect = &cache->free_rects[i];
        if (rect.width > free_rect->dim.width || rect.height > free_rect->dim.height) {
            // Doesn't fit inside
            continue;
        }

        // Is perfect for at least one axis
        const bool is_perfect_match = (rect.width == free_rect->dim.width)
            || (rect.height == free_rect->dim.height);

        const int64 wasted_area = (int64)free_rect->dim.width * free_rect->dim.height
            - (int64)rect.width * rect.height;

        if (!best_match
            || (is_perfect_match && !best_match_is_perfect)
            || (is_perfect_match == best_match_is_perfect && wasted_area < best_wasted_area)
        ) {
            best_match_id = i;
            best_match = free_rect;
            best_match_is_perfect = is_perfect_match;
            best_wasted_area = wasted_area;
        }
    }

    if (!best_match) {
        return NULL;
    }

    GpuAtlasElement* const element = &cache->elements[best_match->element_id];
    element->dim = best_match->dim;

    const uint32 changed_index = best_match->element_id >> div_pow2(sizeof(size_t) * 8);
    const uint32 bit_index = MODULO_2(best_match->element_id, (sizeof(size_t) * 8));
    cache->changed[changed_index] |= (OMS_UINT_ONE << bit_index);

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

inline
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

inline
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

inline
void gpu_atlas_cache_all_unchanged(
    GpuAtlasCache* cache
) NO_EXCEPT
{
    const uint32 changed_count = ceil_div_pow2<(int32) (sizeof(size_t) * 8)>(cache->element_capacity);
    memset(cache->changed, 0, changed_count * sizeof(uint64));
}

// @todo we need to implement a defragment function that
//      1. merges neighboring rects
//      2. re-orders elements so we can create larger free rects

#endif