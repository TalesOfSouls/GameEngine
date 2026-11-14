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

    // Stencil of framebuffer.depth_stencil_texture as its own texture (a depth stencil texture is sampled either as
    // depth or as stencil), 0 if unused
    uint32 stencil_view;

    GpuFence fence;

    // msaa data
    OpenGLFramebufferData framebuffer_msaa;
    Texture* texture_msaa;
};

#endif