/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_GPUAPI_OPENGL_FRAMES_IN_FLIGHT_CONTAINER_H
#define COMS_GPUAPI_OPENGL_FRAMES_IN_FLIGHT_CONTAINER_H

#include "../../stdlib/Stdlib.h"
#include "OpenglUtils.h"

struct FramesInFlightContainer {
    // @performance Can we make both uint16? I don't think because the other variables are 8 bytes values.
    uint32 count;
    uint32 index;
    OpenglFrameData* framebuffers;
    GpuFence framebuffer_sync;
};

#endif