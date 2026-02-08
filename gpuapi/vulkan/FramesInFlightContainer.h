/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_GPUAPI_VULKAN_FRAMES_IN_FLIGHT_CONTAINER_H
#define COMS_GPUAPI_VULKAN_FRAMES_IN_FLIGHT_CONTAINER_H

#include "../../stdlib/Stdlib.h"
#include <vulkan/vulkan.h>

struct FramesInFlightContainer {
    // @performance Can we make both uint16? I don't think because the other variables are 8 bytes values.
    uint32 count;
    uint32 index;

    // @todo Instead of having individual arrays here maybe create a FramesInFlightElement
    //      That could contain the data below
    VkSemaphore* image_available_semaphores;
    VkSemaphore* render_finished_semaphores;
    VkFence* fences;
};

#endif