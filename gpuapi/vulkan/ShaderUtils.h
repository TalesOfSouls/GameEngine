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

#include "../../stdlib/Types.h"
#include "../../memory/RingMemory.h"
#include "../GpuAttributeType.h"
#include "../../object/Vertex.h"
#include "../../log/Log.h"
#include "../../log/Stats.h"
#include "../../log/PerformanceProfiler.h"
#include "../../log/PerformanceProfiler.h"

inline
uint32_t shader_get_uniform_location(
    VkWriteDescriptorSet* descriptor,
    VkDescriptorSet descriptorSet, uint32_t binding, VkDescriptorType descriptorType
) {
    descriptor->sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptor->dstSet = descriptorSet;
    descriptor->dstBinding = binding;
    descriptor->dstArrayElement = 0;
    descriptor->descriptorType = descriptorType;
    descriptor->descriptorCount = 1;
}

inline
void gpuapi_uniform_buffer_update_value(VkDevice device, VkDescriptorSet descriptorSet, uint32_t binding, VkDescriptorType descriptorType, int32_t value)
{
    VkDescriptorBufferInfo bufferInfo = {};
    bufferInfo.buffer = {};  // You should have a buffer holding the value
    bufferInfo.offset = 0;
    bufferInfo.range = sizeof(value);

    VkWriteDescriptorSet descriptorWrite = {};
    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet = descriptorSet;
    descriptorWrite.dstBinding = binding;
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorType = descriptorType;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.pBufferInfo = &bufferInfo;

    vkUpdateDescriptorSets(device, 1, &descriptorWrite, 0, NULL);
}

inline
VkShaderModule gpuapi_shader_make(VkDevice device, const char* source, int32 source_size)
{
    LOG_1("Create shader");
    // Create shader module create info
    VkShaderModuleCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    create_info.codeSize = source_size;
    create_info.pCode = (uint32 *) source;

    // Create shader module
    VkShaderModule shader_module;
    VkResult result = vkCreateShaderModule(device, &create_info, NULL, &shader_module);

    if (result != VK_SUCCESS) {
        LOG_1("Vulkan vkCreateShaderModule: %d", {{LOG_DATA_INT32, (int32 *) &result}});
        ASSERT_SIMPLE(false);

        return VK_NULL_HANDLE;
    }

    LOG_1("Created shader");

    return shader_module;
}

inline
void vulkan_vertex_binding_description(uint32 size, VkVertexInputBindingDescription* binding) {
    binding->binding = 0;
    binding->stride = size;
    binding->inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
}

void gpuapi_attribute_info_create(GpuAttributeType type, VkVertexInputAttributeDescription* attr)
{
    switch (type) {
        case GPU_ATTRIBUTE_TYPE_VERTEX_3D: {
            attr[0] = {
                .location = 0,
                .binding = 0,
                .format = VK_FORMAT_R32G32B32_SFLOAT,
                .offset = offsetof(Vertex3D, position)
            };

            attr[1] = {
                .location = 1,
                .binding = 0,
                .format = VK_FORMAT_R32G32B32_SFLOAT,
                .offset = offsetof(Vertex3D, normal)
            };

            attr[2] = {
                .location = 2,
                .binding = 0,
                .format = VK_FORMAT_R32G32B32_SFLOAT,
                .offset = offsetof(Vertex3D, tex_coord)
            };

            attr[3] = {
                .location = 3,
                .binding = 0,
                .format = VK_FORMAT_R32_UINT,
                .offset = offsetof(Vertex3D, color)
            };
        } return;
        case GPU_ATTRIBUTE_TYPE_VERTEX_3D_NORMAL: {
            attr[0] = {
                .location = 0,
                .binding = 0,
                .format = VK_FORMAT_R32G32B32_SFLOAT,
                .offset = offsetof(Vertex3DNormal, position)
            };

            attr[1] = {
                .location = 1,
                .binding = 0,
                .format = VK_FORMAT_R32G32B32_SFLOAT,
                .offset = offsetof(Vertex3DNormal, normal)
            };
        } return;
        case GPU_ATTRIBUTE_TYPE_VERTEX_3D_COLOR: {
            attr[0] = {
                .location = 0,
                .binding = 0,
                .format = VK_FORMAT_R32G32B32_SFLOAT,
                .offset = offsetof(Vertex3DColor, position)
            };

            attr[1] = {
                .location = 1,
                .binding = 0,
                .format = VK_FORMAT_R32G32B32A32_SFLOAT,
                .offset = offsetof(Vertex3DColor, color)
            };
        } return;
        case GPU_ATTRIBUTE_TYPE_VERTEX_3D_TEXTURE_COLOR: {
            attr[0] = {
                .location = 0,
                .binding = 0,
                .format = VK_FORMAT_R32G32B32_SFLOAT,
                .offset = offsetof(Vertex3DTextureColor, position)
            };

            attr[1] = {
                .location = 1,
                .binding = 0,
                .format = VK_FORMAT_R32G32_SFLOAT,
                .offset = offsetof(Vertex3DTextureColor, texture_color)
            };
        } return;
        case GPU_ATTRIBUTE_TYPE_VERTEX_3D_SAMPLER_TEXTURE_COLOR: {
            attr[0] = {
                .location = 0,
                .binding = 0,
                .format = VK_FORMAT_R32G32B32_SFLOAT,
                .offset = offsetof(Vertex3DSamplerTextureColor, position)
            };

            attr[1] = {
                .location = 1,
                .binding = 0,
                .format = VK_FORMAT_R32_SINT,
                .offset = offsetof(Vertex3DSamplerTextureColor, sampler)
            };

            attr[2] = {
                .location = 2,
                .binding = 0,
                .format = VK_FORMAT_R32G32_SFLOAT,
                .offset = offsetof(Vertex3DSamplerTextureColor, texture_color)
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
    PROFILE(PROFILE_PIPELINE_MAKE, NULL, false, true);
    LOG_1("Create pipeline");
    VkPipelineShaderStageCreateInfo vertex_shader_stage_info = {};
    vertex_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertex_shader_stage_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertex_shader_stage_info.module = vertex_shader;
    vertex_shader_stage_info.pName = "main";

    VkPipelineShaderStageCreateInfo fragment_shader_stage_info = {};
    fragment_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragment_shader_stage_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragment_shader_stage_info.module = fragment_shader;
    fragment_shader_stage_info.pName = "main";

    VkPipelineShaderStageCreateInfo shader_stages[] = {
        vertex_shader_stage_info,
        fragment_shader_stage_info
    };

    VkVertexInputBindingDescription binding_description;
    vulkan_vertex_binding_description(sizeof(Vertex3DSamplerTextureColor), &binding_description);

    VkVertexInputAttributeDescription input_attribute_description[gpuapi_attribute_count(GPU_ATTRIBUTE_TYPE_VERTEX_3D_SAMPLER_TEXTURE_COLOR)];
    gpuapi_attribute_info_create(GPU_ATTRIBUTE_TYPE_VERTEX_3D_SAMPLER_TEXTURE_COLOR, input_attribute_description);

    VkPipelineVertexInputStateCreateInfo vertex_input_info = {};
    vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex_input_info.vertexBindingDescriptionCount = 1;
    vertex_input_info.pVertexBindingDescriptions = &binding_description;
    vertex_input_info.vertexAttributeDescriptionCount = ARRAY_COUNT(input_attribute_description);
    vertex_input_info.pVertexAttributeDescriptions = input_attribute_description;

    VkPipelineInputAssemblyStateCreateInfo input_assembly = {};
    input_assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    input_assembly.primitiveRestartEnable = VK_FALSE;

    VkPipelineViewportStateCreateInfo viewport_state = {};
    viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_state.viewportCount = 1;
    viewport_state.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo rasterizer = {};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisampling = {};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    // @todo This depends on the texture -> shouldn't be here
    VkPipelineColorBlendAttachmentState color_blend_attachment = {};
    color_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    color_blend_attachment.blendEnable = VK_TRUE;
    color_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    color_blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    color_blend_attachment.colorBlendOp = VK_BLEND_OP_ADD;
    color_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    color_blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    color_blend_attachment.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo color_blending = {};
    color_blending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    color_blending.logicOpEnable = VK_FALSE;
    color_blending.logicOp = VK_LOGIC_OP_COPY;
    color_blending.attachmentCount = 1;
    color_blending.pAttachments = &color_blend_attachment;
    color_blending.blendConstants[0] = 0.0f;
    color_blending.blendConstants[1] = 0.0f;
    color_blending.blendConstants[2] = 0.0f;
    color_blending.blendConstants[3] = 0.0f;

    const VkDynamicState dynamic_states[] = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineDynamicStateCreateInfo dynamic_state = {};
    dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic_state.dynamicStateCount = ARRAY_COUNT(dynamic_states);
    dynamic_state.pDynamicStates = dynamic_states;

    VkPipelineLayoutCreateInfo pipeline_info_layout = {};
    pipeline_info_layout.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_info_layout.setLayoutCount = 1;
    pipeline_info_layout.pSetLayouts = descriptor_set_layouts;
    pipeline_info_layout.pushConstantRangeCount = 0;

    VkResult result;
    if ((result = vkCreatePipelineLayout(device, &pipeline_info_layout, NULL, pipeline_layout)) != VK_SUCCESS) {
        LOG_1("Vulkan vkCreatePipelineLayout: %d", {{LOG_DATA_INT32, (int32 *) &result}});
        ASSERT_SIMPLE(false);

        return NULL;
    }

    VkGraphicsPipelineCreateInfo pipeline_info = {};
    pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline_info.stageCount = 2;
    pipeline_info.pStages = shader_stages;
    pipeline_info.pVertexInputState = &vertex_input_info;
    pipeline_info.pInputAssemblyState = &input_assembly;
    pipeline_info.pViewportState = &viewport_state;
    pipeline_info.pRasterizationState = &rasterizer;
    pipeline_info.pMultisampleState = &multisampling;
    pipeline_info.pColorBlendState = &color_blending;
    pipeline_info.pDynamicState = &dynamic_state;
    pipeline_info.layout = *pipeline_layout;
    pipeline_info.renderPass = render_pass;
    pipeline_info.subpass = 0;
    pipeline_info.basePipelineHandle = VK_NULL_HANDLE;

    if ((result = vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipeline_info, NULL, pipeline)) != VK_SUCCESS) {
        LOG_1("Vulkan vkCreateGraphicsPipelines: %d", {{LOG_DATA_INT32, (int32 *) &result}});
        ASSERT_SIMPLE(false);

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
    VkDescriptorSetLayoutCreateInfo layout_info = {};
    layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layout_info.bindingCount = binding_length;
    layout_info.pBindings = bindings;

    VkResult result;
    if ((result = vkCreateDescriptorSetLayout(device, &layout_info, NULL, descriptor_set_layout)) != VK_SUCCESS) {
        LOG_1("Vulkan vkCreateDescriptorSetLayout: %d", {{LOG_DATA_INT32, (int32 *) &result}});
        ASSERT_SIMPLE(false);
    }
}

inline
void vulkan_descriptor_pool_create(
    VkDevice device, VkDescriptorPool* descriptor_pool,
    uint32 frames_in_flight
)
{
    // @question Why is the pool size 2?
    // @todo Isn't this shader specific?
    VkDescriptorPoolSize poolSizes[2] = {
        {
            .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = frames_in_flight,
        },
        {
            .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = frames_in_flight,
        }
    };

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = ARRAY_COUNT(poolSizes);
    poolInfo.pPoolSizes = poolSizes;
    poolInfo.maxSets = frames_in_flight;

    VkResult result;
    if ((result = vkCreateDescriptorPool(device, &poolInfo, NULL, descriptor_pool)) != VK_SUCCESS) {
        LOG_1("Vulkan vkCreateDescriptorPool: %d", {{LOG_DATA_INT32, (int32 *) &result}});
        ASSERT_SIMPLE(false);
    }
}

void vulkan_descriptor_sets_create(
    VkDevice device, VkDescriptorPool descriptor_pool,
    VkDescriptorSetLayout descriptor_set_layout, VkDescriptorSet* descriptor_sets,
    VkImageView texture_image_view, VkSampler& texture_sampler,
    VkBuffer* __restrict uniform_buffers,
    size_t uniform_buffer_object_size,
    uint32 frames_in_flight, RingMemory* ring
)
{
    VkDescriptorSetLayout* layouts = (VkDescriptorSetLayout *) ring_get_memory(ring, sizeof(VkDescriptorSetLayout) * frames_in_flight, 64);
    for (uint32 i = 0; i < frames_in_flight; ++i) {
        layouts[i] = descriptor_set_layout;
    }

    VkDescriptorSetAllocateInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    alloc_info.descriptorPool = descriptor_pool;
    alloc_info.descriptorSetCount = frames_in_flight;
    alloc_info.pSetLayouts = layouts;

    VkResult result;
    if ((result = vkAllocateDescriptorSets(device, &alloc_info, descriptor_sets)) != VK_SUCCESS) {
        LOG_1("Vulkan vkAllocateDescriptorSets: %d", {{LOG_DATA_INT32, (int32 *) &result}});
        ASSERT_SIMPLE(false);

        return;
    }

    // @todo this is shader specific, it shouldn't be here
    for (uint32 i = 0; i < frames_in_flight; ++i) {
        VkDescriptorBufferInfo buffer_info = {};
        buffer_info.buffer = uniform_buffers[i];
        buffer_info.offset = 0;
        buffer_info.range = uniform_buffer_object_size;

        VkDescriptorImageInfo image_info[] = {
            {
                .sampler = texture_sampler,
                .imageView = texture_image_view,
                .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            },
            { // @bug this needs to be the ui sampler
                .sampler = texture_sampler,
                .imageView = texture_image_view,
                .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            }
        };

        VkWriteDescriptorSet descriptor_writes[] = {
            {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = descriptor_sets[i],
                .dstBinding = 0,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .pBufferInfo = &buffer_info,
            },
            {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = descriptor_sets[i],
                .dstBinding = 1,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .pImageInfo = &image_info[0],
            },
            { // @bug this needs to be the ui sampler
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = descriptor_sets[i],
                .dstBinding = 2,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .pImageInfo = &image_info[1],
            }
        };

        vkUpdateDescriptorSets(device, ARRAY_COUNT(descriptor_writes), descriptor_writes, 0, NULL);
    }
}

#endif