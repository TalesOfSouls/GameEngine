/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_GPUAPI_VULKAN_APP_CMD_BUFFER_H
#define COMS_GPUAPI_VULKAN_APP_CMD_BUFFER_H

#include "../../stdlib/Types.h"
#include "../../log/PerformanceProfiler.h"
#include "Shader.h"
#include "ShaderUtils.h"
#include "../ShaderType.h"
#include "../../asset/Asset.h"
#include "../../command/AppCmdBuffer.h"

void* cmd_shader_load(AppCmdBuffer*, Command*) {
    return NULL;
}

void* cmd_shader_load_sync(
    AppCmdBuffer* __restrict cb, Shader* __restrict shader, const int32* __restrict shader_ids,
    VkDevice device, VkRenderPass render_pass, VkPipelineLayout* __restrict pipeline_layout, VkPipeline* __restrict pipeline,
    VkDescriptorSetLayout* __restrict descriptor_set_layouts
) {
    PROFILE(PROFILE_CMD_SHADER_LOAD_SYNC, NULL, false, true);
    char asset_id[9];

    VkShaderModule shader_assets[SHADER_TYPE_SIZE];
    for (int32 i = 0; i < SHADER_TYPE_SIZE; ++i) {
        shader_assets[i] = NULL;
    }

    for (int32 i = 0; i < SHADER_TYPE_SIZE; ++i) {
        if (!shader_ids[i]) {
            continue;
        } else if (shader_ids[i] < 0) {
            break;
        }

        // Load sub asset
        int_to_hex(shader_ids[i], asset_id);
        Asset* shader_asset = thrd_ams_get_asset_wait(cb->ams, asset_id);
        if (!shader_asset) {
            int32 archive_id = (shader_ids[i] >> 24) & 0xFF;
            shader_asset = asset_archive_asset_load(&cb->asset_archives[archive_id], shader_ids[i], cb->ams, cb->mem_vol);
        }

        // Make sub shader
        shader_assets[i] = gpuapi_shader_make(
            device,
            (char *) shader_asset->self,
            shader_asset->ram_size
        );

        shader_asset->state |= ASSET_STATE_RAM_GC;
        shader_asset->state |= ASSET_STATE_VRAM_GC;
    }

    // Make shader/program
    shader->id = gpuapi_pipeline_make(
        device, render_pass, pipeline_layout, pipeline,
        descriptor_set_layouts,
        shader_assets[0], shader_assets[1], shader_assets[2]
    );

    return NULL;
}

#endif