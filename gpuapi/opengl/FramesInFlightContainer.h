/**
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_GPUAPI_OPENGL_FRAMES_IN_FLIGHT_CONTAINER_H
#define COMS_GPUAPI_OPENGL_FRAMES_IN_FLIGHT_CONTAINER_H

#include "../../stdlib/Stdlib.h"
#include "OpenglUtils.h"

struct FrameInFlight {
    OpenGLFramebufferData framebuffer;
    Texture* texture;

    GpuFence fence;

    // msaa data
    OpenGLFramebufferData framebuffer_msaa;
    Texture* texture_msaa;
};

#endif