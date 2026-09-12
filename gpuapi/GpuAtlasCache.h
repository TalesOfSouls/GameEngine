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

struct GpuAtlasRect {
    v2_uint16 dim;

    // References the atlas element
    uint16 element;
};

struct GpuAtlasElement {
    uint16 uv_count;

    // Offset into "uv" in the TextureAtlas struct
    uint16 uv_start;

    uint16 last_frame_used;
};

struct GpuAtlasCache {
    ThrdHashMapT<ThrdHashEntryT<uint64, uint16>> hm;

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
    uint16 smallest_dim;
    v2_uint16 dim;

    // The element count is defined by the size of the smallest_dim
    GpuAtlasRect* free_frects;

    int32 element_capacity;

    GpuAtlasElement* elements;
    v2_f32* uv;
};

#endif