/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_GPUAPI_DIRECT3D_FRAMES_IN_FLIGHT_CONTAINER_H
#define COMS_GPUAPI_DIRECT3D_FRAMES_IN_FLIGHT_CONTAINER_H

#include "../../stdlib/Stdlib.h"
#include <d3d12.h>

struct FrameInFlight {
    ID3D12Resource* framebuffer;
};

#endif