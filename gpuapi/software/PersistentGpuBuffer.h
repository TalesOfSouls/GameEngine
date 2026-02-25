/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_GPUAPI_SOFTWARE_PERSISTENT_BUFFER_H
#define COMS_GPUAPI_SOFTWARE_PERSISTENT_BUFFER_H

#include "../../stdlib/Stdlib.h"

struct PersistentGpuBuffer {
    // The index of the frame (framecounter % 3 because of triple buffering)
    uint32 index;

    // Size of the buffer incl. additional alignment data
    int32 size;

    // either ubo or vbo
    uint32 bo;

    // The respective sub-ranges. We expect a triple buffer
    // Size of the range incl. padding
    int32 range_stride;

    // This size is the actual data size excl. alignment overhead
    int32 range_size;

    // actual memory range start
    byte* data;
};

#endif