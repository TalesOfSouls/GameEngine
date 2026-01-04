/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_GPUAPI_VULKAN_GPU_INFO_C
#define COMS_GPUAPI_VULKAN_GPU_INFO_C

#include "../../stdlib/Stdlib.h"
#include "../../utils/StringUtils.h"
#include "../GPUInfo.h"
#include <vulkan/vulkan.h>

uint64 gpu_info_features() {
    uint64 features = 0;

    VkPhysicalDeviceFeatures2 features2{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2 };
    VkPhysicalDeviceVulkan11Features vk11{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES };
    VkPhysicalDeviceVulkan12Features vk12{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES };
    VkPhysicalDeviceVulkan13Features vk13{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES };
    VkPhysicalDeviceRayTracingPipelineFeaturesKHR rtPipeline{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR };
    VkPhysicalDeviceAccelerationStructureFeaturesKHR accel{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR };
    VkPhysicalDeviceVariableRateShadingFeaturesKHR vrs{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VARIABLE_RATE_SHADING_FEATURES_KHR };
    VkPhysicalDeviceMeshShaderFeaturesEXT mesh{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_EXT };

    features2.pNext = &vk11;
    vk11.pNext = &vk12;
    vk12.pNext = &vk13;
    vk13.pNext = &rtPipeline;
    rtPipeline.pNext = &accel;
    accel.pNext = &vrs;
    vrs.pNext = &mesh;

    vkGetPhysicalDeviceFeatures2(device, &features2);

    if (features2.features.shaderStorageBufferArrayDynamicIndexing)
        features |= GPU_FEATURE_SHADER_STORAGE_BUFFER_OBJECTS;

    if (features2.features.sparseBinding)
        features |= GPU_FEATURE_SPARSE_TEXTURE;

    if (features2.features.shaderInt64)
        features |= GPU_FEATURE_SHADER_INT64;

    if (features2.features.shaderInt16)
        features |= GPU_FEATURE_SHADER_INT16;

    if (features2.features.shaderFloat64)
        features |= GPU_FEATURE_FP64;

    if (features2.features.shaderFloat16)
        features |= GPU_FEATURE_FP16;

    if (features2.features.shaderResourceResidency && features2.features.shaderResourceMinLod)
        features |= GPU_FEATURE_BINDLESS_TEXTURE;

    features |= GPU_FEATURE_PERSISTENT_BUFFERS;

    if (rtPipeline.rayTracingPipeline)
        features |= GPU_FEATURE_RAY_TRACING_VULKANRT;

    if (accel.accelerationStructure)
        features |= GPU_FEATURE_RAY_TRACING_ACCEL_BLAS | GPU_FEATURE_RAY_TRACING_ACCEL_TLAS;

    if (vrs.variableRateShading)
        features |= GPU_FEATURE_VARIABLE_RATE_SHADING;

    if (mesh.meshShader)
        features |= GPU_FEATURE_MASH_SHADERS;

    features |= GPU_FEATURE_COMPRESSION_BC;

    VkFormatProperties props;
    vkGetPhysicalDeviceFormatProperties(device, VK_FORMAT_ASTC_4x4_UNORM_BLOCK, &props);
    if (props.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT)
        features |= GPU_FEATURE_COMPRESSION_ASTC;

    vkGetPhysicalDeviceFormatProperties(device, VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK, &props);
    if (props.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT)
        features |= GPU_FEATURE_COMPRESSION_ETC;

    if (features2.features.samplerAnisotropy)
        features |= GPU_FEATURE_ANISOTROPIC_FILTERING;

    if (features2.features.textureCompressionBC)
        features |= GPU_FEATURE_COMPRESSION_BC;

    if (features2.features.shaderImageGatherExtended)
        features |= GPU_FEATURE_TEXTURE_GATHER;

    if (features2.features.textureCompressionASTC_LDR)
        features |= GPU_FEATURE_COMPRESSION_ASTC;

    if (features2.features.textureCompressionETC2)
        features |= GPU_FEATURE_COMPRESSION_ETC;

    if (features2.features.textureCubeArray || features2.features.imageCubeArray)
        features |= GPU_FEATURE_TEXTURE_ARRAYS;

    if (vk11.multiview)
        features |= GPU_FEATURE_INDIRECT_DRAW;

    if (features2.features.occlusionQueryPrecise)
        features |= GPU_FEATURE_OCCLUSION_RENDERING;

    if (vk11.shaderDrawParameters)
        features |= GPU_FEATURE_TRANSFORM_FEEDBACK;

    if (vk13.synchronization2)
        features |= GPU_FEATURE_TIMELINE_SEMAPHORES;

    if (vk11.deviceGeneratedCommands)
        features |= GPU_FEATURE_ASYNC_COMPUTE;

    if (vk12.shaderBitOperations)
        features |= GPU_FEATURE_SHADER_BITFIELD;

    if (vk12.shaderSubgroupExtendedTypes)
        features |= GPU_FEATURE_VARIABLE_PRECISION;

    return features;
}

void gpuapi_info_get(GpuInfo* info, VkPhysicalDevice device) {
    VkPhysicalDeviceProperties props;
    vkGetPhysicalDeviceProperties(device, &props);

    //str_copy(info->vendor, std::to_string(props.vendorID).c_str(), sizeof(info->vendor) - 1);
    str_copy(info->name, props.deviceName, sizeof(info->name) - 1);

    sprintf_fast(info->api_version, sizeof(info->api_version) - 1, "%d.%d.%d",
        VK_VERSION_MAJOR(props.apiVersion),
        VK_VERSION_MINOR(props.apiVersion),
        VK_VERSION_PATCH(props.apiVersion)
    );

    sprintf_fast(info->shader_version, sizeof(info->shader_version) - 1, "%d.%d",
        VK_VERSION_MAJOR(props.driverVersion),
        VK_VERSION_MINOR(props.driverVersion)
    );

    VkPhysicalDeviceMemoryProperties memProps;
    vkGetPhysicalDeviceMemoryProperties(device, &memProps);
    uint64 totalVRAM = 0;
    for (uint32 i = 0; i < memProps.memoryHeapCount; ++i) {
        if (memProps.memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) {
            totalVRAM += memProps.memoryHeaps[i].size;
        }
    }
    info->vram = (uint32) (totalVRAM / (1024 * 1024));

    info->alignment = (uint32) props.limits.minUniformBufferOffsetAlignment;

    info->features = gpu_info_features();
}

#endif