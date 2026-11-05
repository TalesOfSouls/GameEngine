/**
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_GPUAPI_OPENGL_APP_CMD_BUFFER_H
#define COMS_GPUAPI_OPENGL_APP_CMD_BUFFER_H

#include "../../stdlib/Stdlib.h"
#include "../../log/PerformanceProfiler.h"
#include "OpenglUtils.h"
#include "Pipeline.h"
#include "ShaderUtils.h"
#include "../ShaderType.h"
#include "../../asset/Asset.h"
#include "../../asset/AssetArchive.cpp"
#include "../../command/AppCmdBuffer.h"

// @todo implement
void* cmd_shader_load(AppCmdBuffer*, AppCommand*) NO_EXCEPT
{
    return NULL;
}

/**
 * Loads the shaders of a pipeline from the asset archives (temporary, not part of the asset management system)
 * and creates the pipeline
 *
 * @param const AssetArchive*               asset_archives  Archives
 * @param T*                                mem             Temporary memory
 * @param Pipeline*                         pipeline        Pipeline to create
 * @param const int32*                      shader_ids      Shader asset per ShaderType, -1 terminated
 * @param OpenglDescriptorSetLayoutBinding* bindings        Uniforms (explicit locations)
 * @param int32                             binding_count   Amount of uniforms
 * @param bool                              fatal           false = errors are not asserted (e.g. hot reload)
 *
 * @return bool false if a shader couldn't be loaded/compiled/linked (only if !fatal), pipeline->id is 0 then
 */
template <typename T>
bool cmd_pipeline_load_sync(
    const AssetArchive* const __restrict asset_archives,
    T* const __restrict mem,
    Pipeline* const __restrict pipeline,
    const int32* __restrict shader_ids,
    OpenglDescriptorSetLayoutBinding* __restrict bindings, int32 binding_count,
    bool fatal = true
) NO_EXCEPT
{
    PROFILE_DEBUG(PROFILE_CMD_SHADER_LOAD_SYNC, (char *) NULL, PROFILE_FLAG_SHOULD_LOG);
    char asset_id[9];

    int32 shader_assets_id[SHADER_TYPE_SIZE];
    for (int32 i = 0; i < SHADER_TYPE_SIZE; ++i) {
        shader_assets_id[i] = -1;
    }

    int shader_count = 0;
    const char* shader_str[SHADER_TYPE_SIZE];
    Asset* shader_assets[SHADER_TYPE_SIZE];
    bool success = true;

    for (int32 i = 0; i < SHADER_TYPE_SIZE; ++i) {
        if (!shader_ids[i]) {
            continue;
        } else if (shader_ids[i] < 0) {
            break;
        }

        // Load sub asset
        int_to_hex(shader_ids[i], asset_id);
        shader_assets[shader_count] = asset_archive_asset_load(
            &asset_archives[ARCHIVE_ID_FROM_ASSET_ID(shader_ids[i])],
            shader_ids[i],
            NULL,
            mem
        );

        if (!shader_assets[shader_count]) {
            ASSERT_TRUE(!fatal);
            success = false;
            shader_assets_id[i] = 0;

            continue;
        }

        // Make sub shader
        shader_assets_id[i] = gpuapi_shader_make(
            gpuapi_shader_type_index((ShaderType) (i + 1)),
            (char *) shader_assets[shader_count]->self,
            fatal
        );

        success &= shader_assets_id[i] != 0;

        shader_str[shader_count] = (const char *) shader_assets[shader_count]->self;

        shader_assets[shader_count]->state |= ASSET_MEMORY_STATE_RAM_GC;
        shader_assets[shader_count]->state |= ASSET_MEMORY_STATE_VRAM_GC;

        ++shader_count;
    }

    // Make shader/program (deletes the shaders)
    pipeline->id = gpuapi_pipeline_make(
        shader_assets_id[0], shader_assets_id[1], shader_assets_id[2],
        fatal
    );

    if (pipeline->id) {
        success &= gpuapi_descriptor_set_layout_create(
            pipeline,
            bindings, binding_count,
            shader_str, shader_count,
            fatal
        );
    } else {
        success = false;
    }

    // We don't need the shader any longer in memory
    for (int i = 0; i < shader_count; ++i) {
        chunk_free_elements(mem, (byte *) shader_assets[i], shader_assets[i]->chunk_count);
    }

    if (!success && pipeline->id) {
        glDeleteProgram(pipeline->id);
        pipeline->id = 0;
    }

    return success;
}

/**
 * Creates the pipeline again from the current shaders of the asset archives (hot reload)
 *
 * The new pipeline only replaces the current pipeline if all shaders compile + link and
 * all uniforms exist, otherwise the current pipeline stays as it is.
 * The uniform values are not taken over (the caller has to set the uniforms that are only set once)
 *
 * @return bool true if the pipeline was replaced
 */
template <typename T>
bool cmd_pipeline_reload_sync(
    const AssetArchive* const __restrict asset_archives,
    T* const __restrict mem,
    Pipeline* const __restrict pipeline,
    const int32* __restrict shader_ids,
    OpenglDescriptorSetLayoutBinding* __restrict bindings, int32 binding_count
) NO_EXCEPT
{
    Pipeline next = {};
    if (!cmd_pipeline_load_sync(asset_archives, mem, &next, shader_ids, bindings, binding_count, false)) {
        return false;
    }

    if (pipeline->id) {
        glDeleteProgram(pipeline->id);
    }

    *pipeline = next;

    return true;
}

#endif