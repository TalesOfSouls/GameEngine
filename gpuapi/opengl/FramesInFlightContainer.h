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

struct FrameInFlight {
    uint32 framebuffer;
    uint32 renderbuffer;
    Texture* texture;

    GpuFence fence;

    // msaa data
    uint32 framebuffer_msaa;
    uint32 colorbuffer_msaa;
    uint32 depthbuffer_msaa;
    Texture* texture_msaa;

};

#endif