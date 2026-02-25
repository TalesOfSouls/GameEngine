/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_GPUAPI_VULKAN_SHADER_UTILS_H
#define COMS_GPUAPI_VULKAN_SHADER_UTILS_H

#include <vulkan/vulkan.h>

#include "../../stdlib/Stdlib.h"
#include "../../memory/RingMemory.cpp"
#include "../GpuAttributeType.h"
#include "../../object/Vertex.h"
#include "../../log/Log.h"
#include "../../log/Stats.h"
#include "../../log/PerformanceProfiler.h"
#include "../../log/PerformanceProfiler.h"

inline
void shader_get_uniform_location(
    VkWriteDescriptorSet* descriptor,
    VkDescriptorSet descriptorSet, uint32 binding, VkDescriptorType descriptorType
) {
    descriptor->sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptor->dstSet = descriptorSet;
    descriptor->dstBinding = binding;
    descriptor->dstArrayElement = 0;
    descriptor->descriptorType = descriptorType;
    descriptor->descriptorCount = 1;
}

inline
void gpuapi_uniform_buffer_update(VkDevice device, VkDescriptorSet descriptorSet, uint32 binding, VkDescriptorType descriptorType, int32_t value)
{
    VkDescriptorBufferInfo bufferInfo = {
        {0}, // You should have a buffer holding the value, .buffer =
        0, // .offset =
        sizeof(value) // .range =
    };

    VkWriteDescriptorSet descriptorWrite = {
        VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, // .sType =
        NULL, // .pNext
        descriptorSet, // .dstSet =
        binding, // .dstBinding =
        0, // .dstArrayElement =
        1, // .descriptorCount =
        descriptorType, // .descriptorType =
        NULL, // .pImageInfo =
        &bufferInfo, // .pBufferInfo =
    };

    vkUpdateDescriptorSets(device, 1, &descriptorWrite, 0, NULL);
}

inline
VkShaderModule gpuapi_shader_make(VkDevice device, const char* source, int32 source_size)
{
    LOG_1("Create shader");
    // Create shader module create info
    VkShaderModuleCreateInfo create_info = {
        VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO, // .sType =
        NULL, // .pNext =
        0, // .flags =
        (size_t) source_size, // .codeSize =
        (uint32 *) source // .pCode =
    };

    // Create shader module
    VkShaderModule shader_module;
    VkResult result = vkCreateShaderModule(device, &create_info, NULL, &shader_module);

    if (result != VK_SUCCESS) {
        LOG_1("Vulkan vkCreateShaderModule: %d", {DATA_TYPE_INT32, (int32 *) &result});
        ASSERT_TRUE(false);

        return VK_NULL_HANDLE;
    }

    LOG_1("Created shader");

    return shader_module;
}

inline
void vulkan_vertex_binding_description(uint32 size, VkVertexInputBindingDescription* const binding) {
    binding->binding = 0;
    binding->stride = size;
    binding->inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
}

void gpuapi_attribute_info_create(GpuAttributeType type, VkVertexInputAttributeDescription* const attr)
{
    switch (type) {
        case GPU_ATTRIBUTE_TYPE_VERTEX_3D: {
            attr[0] = {
                0, // .location =
                0, // .binding =
                VK_FORMAT_R32G32B32_SFLOAT, // .format =
                offsetof(Vertex3D, position) // .offset =
            };

            attr[1] = {
                1, // .location =
                0, // .binding =
                VK_FORMAT_R32G32B32_SFLOAT, // .format =
                offsetof(Vertex3D, normal) // .offset =
            };

            attr[2] = {
                2, // .location =
                0, // .binding =
                VK_FORMAT_R32G32B32_SFLOAT, // .format =
                offsetof(Vertex3D, tex_coord) // .offset =
            };

            attr[3] = {
                3, // .location =
                0, // .binding =
                VK_FORMAT_R32_UINT, // .format =
                offsetof(Vertex3D, color) // .offset =
            };
        } return;
        case GPU_ATTRIBUTE_TYPE_VERTEX_3D_NORMAL: {
            attr[0] = {
                0, // .location =
                0, // .binding =
                VK_FORMAT_R32G32B32_SFLOAT, // .format =
                offsetof(Vertex3DNormal, position) // .offset =
            };

            attr[1] = {
                1, // .location =
                0, // .binding =
                VK_FORMAT_R32G32B32_SFLOAT, // .format =
                offsetof(Vertex3DNormal, normal) // .offset =
            };
        } return;
        case GPU_ATTRIBUTE_TYPE_VERTEX_3D_COLOR: {
            attr[0] = {
                0, // .location =
                0, // .binding =
                VK_FORMAT_R32G32B32_SFLOAT, // .format =
                offsetof(Vertex3DColor, position) // .offset =
            };

            attr[1] = {
                1, // .location =
                0, // .binding =
                VK_FORMAT_R32G32B32A32_SFLOAT, // .format =
                offsetof(Vertex3DColor, color) // .offset =
            };
        } return;
        case GPU_ATTRIBUTE_TYPE_VERTEX_3D_TEXTURE_COLOR: {
            attr[0] = {
                0, // .location =
                0, // .binding =
                VK_FORMAT_R32G32B32_SFLOAT, // .format =
                offsetof(Vertex3DTextureColor, position) // .offset =
            };

            attr[1] = {
                1, // .location =
                0, // .binding =
                VK_FORMAT_R32G32_SFLOAT, // .format =
                offsetof(Vertex3DTextureColor, texture_color) // .offset =
            };
        } return;
        case GPU_ATTRIBUTE_TYPE_VERTEX_3D_SAMPLER_TEXTURE_COLOR: {
            attr[0] = {
                0, // .location =
                0, // .binding =
                VK_FORMAT_R32G32B32_SFLOAT, // .format =
                offsetof(Vertex3DSamplerTextureColor, position) // .offset =
            };

            attr[1] = {
                1, // .location =
                0, // .binding =
                VK_FORMAT_R32_SINT, // .format =
                offsetof(Vertex3DSamplerTextureColor, sampler) // .offset =
            };

            attr[2] = {
                2, // .location =
                0, // .binding =
                VK_FORMAT_R32G32_SFLOAT, // .format =
                offsetof(Vertex3DSamplerTextureColor, texture_color) // .offset =
            };
        } return;
        default:
            UNREACHABLE();
    };
}

FORCE_INLINE
void gpuapi_pipeline_use(VkCommandBuffer command_buffer, VkPipeline pipeline)
{
    vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
}

// @todo Instead of passing the shaders one by one, pass one array called ShaderStage* shader_stages
// This way we can handle this more dynamic
VkPipeline gpuapi_pipeline_make(
    VkDevice device, VkRenderPass render_pass, VkPipelineLayout* __restrict pipeline_layout, VkPipeline* __restrict pipeline,
    VkDescriptorSetLayout* descriptor_set_layouts,
    VkShaderModule vertex_shader, VkShaderModule fragment_shader,
    VkShaderModule
) {
    PROFILE(PROFILE_PIPELINE_MAKE, NULL, PROFILE_FLAG_SHOULD_LOG);
    LOG_1("Create pipeline");
    VkPipelineShaderStageCreateInfo vertex_shader_stage_info = {
        VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, // .sType =
        NULL, // .pNext =
        0, // .flags =
        VK_SHADER_STAGE_VERTEX_BIT, // .stage =
        vertex_shader, // .module =
        "main" // .pName =
    };

    VkPipelineShaderStageCreateInfo fragment_shader_stage_info = {
        VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, // .sType =
        NULL, // .pNext =
        0, // .flags =
        VK_SHADER_STAGE_FRAGMENT_BIT, // .stage =
        fragment_shader, // .module =
        "main" // .pName =
    };

    VkPipelineShaderStageCreateInfo shader_stages[] = {
        vertex_shader_stage_info,
        fragment_shader_stage_info
    };

    VkVertexInputBindingDescription binding_description;
    vulkan_vertex_binding_description(sizeof(Vertex3DSamplerTextureColor), &binding_description);

    VkVertexInputAttributeDescription input_attribute_description[gpuapi_attribute_count(GPU_ATTRIBUTE_TYPE_VERTEX_3D_SAMPLER_TEXTURE_COLOR)];
    gpuapi_attribute_info_create(GPU_ATTRIBUTE_TYPE_VERTEX_3D_SAMPLER_TEXTURE_COLOR, input_attribute_description);

    VkPipelineVertexInputStateCreateInfo vertex_input_info = {
        VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO, // .sType =
        NULL, // .pNext =
        0, // .flags =
        1, // .vertexBindingDescriptionCount =
        &binding_description, // .pVertexBindingDescriptions =
        (uint32) ARRAY_COUNT(input_attribute_description), // .vertexAttributeDescriptionCount =
        input_attribute_description, // .pVertexAttributeDescriptions =
    };

    VkPipelineInputAssemblyStateCreateInfo input_assembly = {
        VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO, // .sType =
        NULL, // .pNext =
        0, // .flags =
        VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, // .topology =
        VK_FALSE, // .primitiveRestartEnable =
    };

    VkPipelineViewportStateCreateInfo viewport_state = {0};
    viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_state.viewportCount = 1;
    viewport_state.scissorCount = 1;
    // @question if viewportcount and scissorcount are 1, why am I not passing a pointer of those then?

    const VkPipelineRasterizationStateCreateInfo rasterizer = {
        VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO, // .sType =
        NULL, // .pNext =
        0, // .flags =
        VK_FALSE, // .depthClampEnable =
        VK_FALSE, // .rasterizerDiscardEnable =
        VK_POLYGON_MODE_FILL, // .polygonMode =
        VK_CULL_MODE_BACK_BIT, // .cullMode =
        VK_FRONT_FACE_CLOCKWISE, // .frontFace =
        VK_FALSE, // .depthBiasEnable =
        0.0f, // .depthBiasConstantFactor =
        0.0f, // .depthBiasClamp =
        0.0f, // .depthBiasSlopeFactor =
        1.0f, // .lineWidth =
    };

    VkPipelineMultisampleStateCreateInfo multisampling = {0};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    // @todo This depends on the texture -> shouldn't be here
    const VkPipelineColorBlendAttachmentState color_blend_attachment = {
        VK_TRUE, // .blendEnable =
        VK_BLEND_FACTOR_SRC_ALPHA, // .srcColorBlendFactor =
        VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA, // .dstColorBlendFactor =
        VK_BLEND_OP_ADD, // .colorBlendOp =
        VK_BLEND_FACTOR_ONE, // .srcAlphaBlendFactor =
        VK_BLEND_FACTOR_ZERO, // .dstAlphaBlendFactor =
        VK_BLEND_OP_ADD, // .alphaBlendOp =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT // .colorWriteMask =
    };

    const VkPipelineColorBlendStateCreateInfo color_blending = {
        VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO, // .sType =
        NULL, // .pNext =
        0, // .flags =
        VK_FALSE, // .logicOpEnable =
        VK_LOGIC_OP_COPY, // .logicOp =
        1, // .attachmentCount =
        &color_blend_attachment, // .pAttachments =
        {0.0f, 0.0f, 0.0f, 0.0f}, // .blendConstants[0-3]
    };

    const VkDynamicState dynamic_states[] = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    const VkPipelineDynamicStateCreateInfo dynamic_state = {
        VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO, // .sType =
        NULL, // .pNext =
        0, // .flags =
        ARRAY_COUNT(dynamic_states), // .dynamicStateCount =
        dynamic_states, // .pDynamicStates =
    };

    const VkPipelineLayoutCreateInfo pipeline_info_layout = {
        VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO, // .sType =
        NULL, // .pNext =
        0, // .flags =
        1, // .setLayoutCount =
        descriptor_set_layouts, // .pSetLayouts =
        0 // .pushConstantRangeCount =
    };

    VkResult result;
    if ((result = vkCreatePipelineLayout(device, &pipeline_info_layout, NULL, pipeline_layout)) != VK_SUCCESS) {
        LOG_1("Vulkan vkCreatePipelineLayout: %d", {DATA_TYPE_INT32, (int32 *) &result});
        ASSERT_TRUE(false);

        return NULL;
    }

    const VkGraphicsPipelineCreateInfo pipeline_info = {
        VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO, // .sType =
        NULL, // .pNext =
        0, // .flags =
        2, // .stageCount =
        shader_stages, // .pStages =
        &vertex_input_info, // .pVertexInputState =
        &input_assembly, // .pInputAssemblyState =
        NULL,
        &viewport_state, // .pViewportState =
        &rasterizer, // .pRasterizationState =
        &multisampling, // .pMultisampleState =
        NULL, // .pDepthStencilState =
        &color_blending, // .pColorBlendState =
        &dynamic_state, // .pDynamicState =
        *pipeline_layout, // .layout =
        render_pass, // .renderPass =
        0, // .subpass =
        VK_NULL_HANDLE // .basePipelineHandle =
    };

    if ((result = vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipeline_info, NULL, pipeline)) != VK_SUCCESS) {
        LOG_1("Vulkan vkCreateGraphicsPipelines: %d", {DATA_TYPE_INT32, (int32 *) &result});
        ASSERT_TRUE(false);

        return NULL;
    }

    vkDestroyShaderModule(device, fragment_shader, NULL);
    vkDestroyShaderModule(device, vertex_shader, NULL);

    LOG_1("Created pipeline");

    // @question Do we want to return the value or the pointer?
    // I think the value is already a pointer?
    return *pipeline;
}

inline
void pipeline_cleanup(VkDevice device, VkPipeline pipeline, VkPipelineLayout pipeline_layout) {
    vkDestroyPipeline(device, pipeline, NULL);
    vkDestroyPipelineLayout(device, pipeline_layout, NULL);
}

inline
void gpuapi_descriptor_set_layout_create(
    VkDevice device,
    VkDescriptorSetLayout* descriptor_set_layout, VkDescriptorSetLayoutBinding* bindings, int32 binding_length
) {
    VkDescriptorSetLayoutCreateInfo layout_info = {0};
    layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layout_info.bindingCount = binding_length;
    layout_info.pBindings = bindings;

    VkResult result;
    if ((result = vkCreateDescriptorSetLayout(device, &layout_info, NULL, descriptor_set_layout)) != VK_SUCCESS) {
        LOG_1("Vulkan vkCreateDescriptorSetLayout: %d", {DATA_TYPE_INT32, (int32 *) &result});
        ASSERT_TRUE(false);
    }
}

inline
void vulkan_descriptor_pool_create(
    VkDevice device, VkDescriptorPool* descriptor_pool,
    uint32 frames_in_flight
)
{
    // @todo Isn't this shader specific?
    const VkDescriptorPoolSize poolSizes[2] = {
        {
            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, // .type =
            frames_in_flight, // .descriptorCount =
        },
        {
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, // .type =
            frames_in_flight, // .descriptorCount =
        }
    };

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = ARRAY_COUNT(poolSizes);
    poolInfo.pPoolSizes = poolSizes;
    poolInfo.maxSets = frames_in_flight;

    VkResult result;
    if ((result = vkCreateDescriptorPool(device, &poolInfo, NULL, descriptor_pool)) != VK_SUCCESS) {
        LOG_1("Vulkan vkCreateDescriptorPool: %d", {DATA_TYPE_INT32, (int32 *) &result});
        ASSERT_TRUE(false);
    }
}

void vulkan_descriptor_sets_create(
    VkDevice device, VkDescriptorPool descriptor_pool,
    VkDescriptorSetLayout descriptor_set_layout, VkDescriptorSet* descriptor_sets,
    VkImageView texture_image_view, VkSampler& texture_sampler,
    VkBuffer* __restrict uniform_buffers,
    size_t uniform_buffer_object_size,
    int32 frames_in_flight, RingMemory* const ring
)
{
    VkDescriptorSetLayout* layouts = (VkDescriptorSetLayout *) ring_get_memory(ring, sizeof(VkDescriptorSetLayout) * frames_in_flight, ASSUMED_CACHE_LINE_SIZE);
    for (int32 i = 0; i < frames_in_flight; ++i) {
        layouts[i] = descriptor_set_layout;
    }

    const VkDescriptorSetAllocateInfo alloc_info = {
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO, // .sType =
        NULL, // .pNext =
        descriptor_pool, // .descriptorPool =
        (uint32) frames_in_flight, // .descriptorSetCount =
        layouts, // .pSetLayouts =
    };

    VkResult result;
    if ((result = vkAllocateDescriptorSets(device, &alloc_info, descriptor_sets)) != VK_SUCCESS) {
        LOG_1("Vulkan vkAllocateDescriptorSets: %d", {DATA_TYPE_INT32, (int32 *) &result});
        ASSERT_TRUE(false);

        return;
    }

    // @todo this is shader specific, it shouldn't be here
    for (int32 i = 0; i < frames_in_flight; ++i) {
        const VkDescriptorBufferInfo buffer_info = {
            uniform_buffers[i], // .buffer =
            0, // .offset =
            uniform_buffer_object_size // .range =
        };

        const VkDescriptorImageInfo image_info[] = {
            {
                texture_sampler, // .sampler =
                texture_image_view, // .imageView =
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, // .imageLayout =
            },
            { // @bug this needs to be the ui sampler
                texture_sampler, // .sampler =
                texture_image_view, // .imageView =
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, // .imageLayout =
            }
        };

        const VkWriteDescriptorSet descriptor_writes[] = {
            {
                VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, // .sType =
                NULL, // .pNext =
                descriptor_sets[i], // .dstSet =
                0, // .dstBinding =
                0, // .dstArrayElement =
                1, // .descriptorCount =
                VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, // .descriptorType =
                NULL, // .pImageInfo =
                &buffer_info, // .pBufferInfo =
            },
            {
                VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, // .sType =
                NULL, // .pNext =
                descriptor_sets[i], // .dstSet =
                1, // .dstBinding =
                0, // .dstArrayElement =
                1, // .descriptorCount =
                VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, // .descriptorType =
                &image_info[0], // .pImageInfo =
            },
            { // @bug this needs to be the ui sampler
                VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, // .sType =
                NULL, // .pNext =
                descriptor_sets[i], // .dstSet =
                2, // .dstBinding =
                0, // .dstArrayElement =
                1, // .descriptorCount =
                VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, // .descriptorType =
                &image_info[1], // .pImageInfo =
            }
        };

        vkUpdateDescriptorSets(device, ARRAY_COUNT(descriptor_writes), descriptor_writes, 0, NULL);
    }
}

#endif