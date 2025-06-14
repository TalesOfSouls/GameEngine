/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_GPUAPI_DIRECT3D_FRAMES_IN_FLIGHT_CONTAINER_H
#define COMS_GPUAPI_DIRECT3D_FRAMES_IN_FLIGHT_CONTAINER_H

#include "../../stdlib/Types.h"
#include <d3d12.h>

struct FramesInFlightContainer {
    uint32 count;
    uint32 index;
    ID3D12Fence* fence;
    UINT64 fence_value;
    HANDLE fence_event;
    ID3D12Resource** framebuffers;
};

#endif