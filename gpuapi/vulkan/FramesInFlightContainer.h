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

#include "../../stdlib/Types.h"
#include <vulkan/vulkan.h>

struct FramesInFlightContainer {
    uint32 count;
    uint32 index;
    VkSemaphore* image_available_semaphores;
    VkSemaphore* render_finished_semaphores;
    VkFence* fences;
};

#endif