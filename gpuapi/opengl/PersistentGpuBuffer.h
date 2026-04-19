/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_GPUAPI_OPENGL_PERSISTENT_BUFFER_H
#define COMS_GPUAPI_OPENGL_PERSISTENT_BUFFER_H

#include "../../stdlib/Stdlib.h"

struct PersistentGpuBuffer {
    // The index of the frame (framecounter % 3 because of triple buffering)
    uint32 index;

    // Size of the buffer incl. additional alignment data
    // This just informs us about the total memory allocated on the GPU
    int32 size;

    // either ubo or vbo
    uint32 bo;

    // The respective sub-ranges. We expect a triple buffer
    // Size of the range incl. padding
    // We need to know this information to jump to the correct sub-buffer when flushing to the GPU
    int32 range_stride;

    // This size is the actual data size excl. alignment overhead
    // We need this information to know how much data we have to flush to the GPU
    int32 range_size;

    // actual memory range start
    byte* data;
};

#endif