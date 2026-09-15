/**
 * A cache for gpu renders
 *
 * In some cases we need to or can cache renderings on the gpu. Examples are:
 *
 *      Rendered text (not individual glyphs, full text)
 *      Completely rendered UI components or sub-components (e.g. button with text)
 *      Player skins that a player can modify (the asset is a base skin but the modified version updates this base skin)
 *
 * This is also different from persistent memory on the GPU where you maybe want to store vertex information.
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_GPUAPI_GPU_CACHE_H
#define COMS_GPUAPI_GPU_CACHE_H

#include "../stdlib/Stdlib.h"
#include "../stdlib/ThrdHashMapT.h"
#include "../thread/Atomic.h"
#include "../thread/Spinlock.h"

struct GpuAtlasRect {
    v4_uint16 dim;

    // References the atlas element
    // In reality we don't even need this since this is can be calculated by
    // dim.x and min_dim as its element id is the top left corner of the rect
    uint32 element_id;
};

struct GpuAtlasElement {
    v4_uint16 dim;
    uint16 uv_count;

    // Offset into "uv" in the TextureAtlas struct
    uint16 uv_start;

    uint16 last_frame_used;
    atomic<bool> completed;
};

struct GpuAtlasCache {
    HashMapT<HashEntryT<uint32, uint16>> hm;

    /**
     * The strategy is to divide a large rect into smaller rects whenever we add content.
     *
     * In the beginning there is 1 free rect. after adding a texture it most likely becomes:
     *      1 used rect
     *      2 free rects
     *
     * Most likely first iteration (not guaranteed if used is full width)
     * +------------|-------+
     * |  used      | free  |
     * |            |       |
     * |------------|-------|
     * | free               |
     * |                    |
     * |                    |
     * +--------------------+
     */
    // The smallest dim defines the smallest chunk which is also used for the box incremental
    // This means the box MUST be a multiple of smallest_dim
    uint16 min_dim;
    uint16 dim;

    // element_capacity also represents the length of free_rects
    uint16 element_capacity;

    // Currently available free rects
    uint16 free_count;

    uint64* changed;

    // The element count is indirectly defined by the size of the smallest_dim together with the dim
    // element count = dim.x / smallest_dim
    GpuAtlasRect* free_rects;
    GpuAtlasElement* elements;
    v2_f32* uv;

    spinlock32 lock;

    // @todo we need a way to track changed elements so we can update the cpu cache only for changed elements
};

#endif