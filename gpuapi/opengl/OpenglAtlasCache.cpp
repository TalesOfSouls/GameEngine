/**
 * Example use case for updating individual sections
 *
 * 1. gpuapi_texture_cache_setup()
 * 2. gpuapi_texture_blit_create()
 *
 * 3. gpuapi_binds_cache()
 * 4. glEnable(GL_SCISSOR_TEST)
 * 5. gpu_atlas_cache_exists() -> if false
 * 6. gpuapi_texture_cache_section_begin()
 *      1. gpu_atlas_cache_reserve()
 *      2. ... draw ...
 *      3. gpu_atlas_cache_completed()
 * 7. gpuapi_texture_cache_section_begin()
 *      1. gpu_atlas_cache_reserve()
 *      2. ... draw ...
 *      3. gpu_atlas_cache_completed()
 * 8. glDisable(GL_SCISSOR_TEST)
 * 9. gpuapi_binds_restore()
 *
 * 10. gpuapi_binds_restore()
 * 11. gpuapi_texture_cache_delete()
 *
 * ========================================================================================
 *
 * Alternatively to rendering individual sections which uses 1 draw call each
 * you may want to call gpuapi_texture_cache_begin() and render all updated elements
 *
 * 1. gpuapi_texture_cache_setup()
 * 2. gpuapi_texture_blit_create()
 *
 * 3. gpuapi_binds_cache()
 * 4. glEnable(GL_SCISSOR_TEST)
 * 5. gpu_atlas_cache_exists() -> if false
 * 6. gpuapi_texture_cache_begin()
 *      1. gpu_atlas_cache_reserve()
 *      2. gpu_atlas_cache_incompleted()
 *      3. ... create vertices ...
 * 7. gpu_atlas_cache_exists() -> if false
 *      1. gpu_atlas_cache_reserve()
 *      2. gpu_atlas_cache_incompleted()
 *      3. ... create vertices ...
 * 9. glDisable(GL_SCISSOR_TEST)
 * 10. ... draw ... <- THIS IS THE REAL DIFFERENCE TO EXAMPLE 1, ONLY ONE DRAW
 * 11. gpu_atlas_cache_completed() on drawn elements
 * 11. gpuapi_binds_restore()
 *
 * 12. gpuapi_binds_restore()
 * 13. gpuapi_texture_cache_delete()
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_GPUAPI_OPENGL_ATLAS_CACHE_C
#define COMS_GPUAPI_OPENGL_ATLAS_CACHE_C

#include "../../stdlib/Stdlib.h"
#include "../../object/Texture.h"
#include "../../image/Image.cpp"
#include "../../utils/StringUtils.h"
#include "../../log/Log.h"
#include "../../log/Stats.h"
#include "../../log/PerformanceProfiler.h"
#include "../../system/FileUtils.cpp"
#include "../RenderUtils.h"
#include "../GpuAtlasCache.h"
#include "Opengl.h"
#include "OpenglAtlasCache.h"
#include "PersistentGpuBuffer.h"
#include "OpenglUtils.h"
#include "ShaderUtils.h"

void gpuapi_texture_blit_create(
    OpenglAtlasBlitter* const __restrict cache,
    const char* const __restrict vertex_shader,
    const char* const __restrict fragment_shader
) NO_EXCEPT
{
    // Setup program
    {
        const GLuint vs_id = gpuapi_shader_make(
            gpuapi_shader_type_index(SHADER_TYPE_VERTEX),
            vertex_shader
        );
        const GLuint fs_id = gpuapi_shader_make(
            gpuapi_shader_type_index(SHADER_TYPE_FRAGMENT),
            fragment_shader
        );
        cache->blit_pipeline.id = gpuapi_pipeline_make(vs_id, fs_id, -1);
        LABEL_GPU_API(cache->blit_pipeline.id, GL_PROGRAM, "AtlasCachePipeline");

        const char* const shader_table[] = { vertex_shader, fragment_shader };
        OpenglDescriptorSetLayoutBinding bindings[] = {
            {0, "uProj"},
            {0, "uPos"},
            {0, "uSize"},
            {0, "SamplerId"},
        };
        gpuapi_descriptor_set_layout_create(
            &cache->blit_pipeline,
            bindings, ARRAY_COUNT(bindings),
            shader_table, ARRAY_COUNT(shader_table)
        );
    }

    cache->blit_vao = gpuapi_vertex_buffer_create();
    cache->blit_vbo = gpuapi_buffer_generate_static(
        _GPUAPI_ATTRIBUTE_STRIDE[GPU_ATTRIBUTE_TYPE_VERTEX_2D_TEXTURE],
        NULL
    );
    LABEL_GPU_API(cache->blit_vao, GL_VERTEX_ARRAY, "AtlasCacheVAO");
    LABEL_GPU_API(cache->blit_vbo, GL_BUFFER, "AtlasCacheBuffer");

    glVertexArrayVertexBuffer(
        cache->blit_vao, 0,
        cache->blit_vbo, 0,
        _GPUAPI_ATTRIBUTE_STRIDE[GPU_ATTRIBUTE_TYPE_VERTEX_2D_TEXTURE]
    );

    // aPos and texture coordinate/uv
    CONSTEXPR auto attr = gpuapi_attribute_info_create<GPU_ATTRIBUTE_TYPE_VERTEX_2D_TEXTURE>();
    gpuapi_attribute_setup_static(
        cache->blit_vao,
        0,
        GPU_ATTRIBUTE_TYPE_VERTEX_2D_TEXTURE,
        attr.data
    );

    ASSERT_GPU_API();
}

void gpuapi_texture_blit_delete(OpenglAtlasBlitter* const cache) NO_EXCEPT
{
    glDeleteProgram(cache->blit_pipeline.id);
    glDeleteVertexArrays(1, &cache->blit_vao);
    glDeleteBuffers(1, &cache->blit_vbo);
}

void gpuapi_texture_cache_delete(OpenglAtlasCache* const cache) NO_EXCEPT
{
    glDeleteFramebuffers(1, &cache->fbo);
    glDeleteTextures(1, &cache->texture.texture_id);
}

void gpuapi_texture_cache_setup(
    OpenglAtlasCache* const __restrict gpu_cache,
    const GpuAtlasCache* const __restrict cpu_cache
) NO_EXCEPT
{
    ASSERT_TRUE(cpu_cache->dim > 0);

    glCreateTextures(GL_TEXTURE_2D, 1, &gpu_cache->texture.texture_id);
    glTextureStorage2D(gpu_cache->texture.texture_id, 1, GL_RGBA8, cpu_cache->dim, cpu_cache->dim);
    glTextureParameteri(gpu_cache->texture.texture_id, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTextureParameteri(gpu_cache->texture.texture_id, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTextureParameteri(gpu_cache->texture.texture_id, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameteri(gpu_cache->texture.texture_id, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glCreateFramebuffers(1, &gpu_cache->fbo);
    glNamedFramebufferTexture(gpu_cache->fbo, GL_COLOR_ATTACHMENT0, gpu_cache->texture.texture_id, 0);

    LABEL_GPU_API(gpu_cache->fbo, GL_FRAMEBUFFER, "AtlasCacheFramebuffer");
    LABEL_GPU_API(gpu_cache->texture.texture_id, GL_TEXTURE, "AtlasCacheTexture");

    glNamedFramebufferDrawBuffer(gpu_cache->fbo, GL_COLOR_ATTACHMENT0);

    DEBUG_VAR_DEF_ASSIGN(const GLenum, status) glCheckNamedFramebufferStatus(gpu_cache->fbo, GL_FRAMEBUFFER);
    ASSERT_TRUE(status == GL_FRAMEBUFFER_COMPLETE);

    ASSERT_GPU_API();
}

// WARNING: You should probably call gpuapi_binds_cache() first
// WARNING: needs glEnable(GL_SCISSOR_TEST);
/**
 * This function needs to be called if you ONLY want to update/cache a small subsection of the cache
 * For multiple or even a complete cache update use gpuapi_texture_cache_begin
 */
bool gpuapi_texture_cache_section_begin(
    OpenglAtlasCache* const __restrict cache, v4_uint16& rect
) NO_EXCEPT
{
    // This needs to be enabled
    ASSERT_TRUE(glIsEnabled(GL_SCISSOR_TEST));

    // We have to bind, because DSA doesn't support bindless rendering into a framebuffer
    // Draw commands render into whatever is bound

    glBindFramebuffer(GL_FRAMEBUFFER, cache->fbo);
    glViewport(rect.x, rect.y, rect.width, rect.height);
    glScissor(rect.x, rect.y, rect.width, rect.height);

    // Clear area before drawing
    f32 clear_color[] = {0.0f, 0.0f, 0.0f, 0.0f};
    glClearNamedFramebufferfv(cache->fbo, GL_COLOR, 0, clear_color);

    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

    return true;
}

void gpuapi_texture_cache_begin(OpenglAtlasCache* const __restrict cache) NO_EXCEPT
{
    glBindFramebuffer(GL_FRAMEBUFFER, cache->fbo);
    f32 clear_color[] = {0.0f, 0.0f, 0.0f, 0.0f};
    glClearNamedFramebufferfv(cache->fbo, GL_COLOR, 0, clear_color);
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
}

// WARNING: This is slow, don't use it you should rather use the normal texture rendering
void gpuapi_texture_cache_blit(
    OpenglAtlasCache* gpu_cache,
    GpuAtlasCache* cpu_cache,
    OpenglAtlasBlitter* blit,
    const GpuAtlasRect* rect, v4_f32 dim,
    v2_f32 screen_dim
) NO_EXCEPT
{
    // WARNING: OpenGL's window/framebuffer *storage* is bottom-up
    const f32 uLeft = (f32)rect->dim.x / (f32)cpu_cache->dim;
    const f32 uRight = (f32)(rect->dim.x + rect->dim.width) / (f32)cpu_cache->dim;
    const f32 vTop = (f32)(rect->dim.y + rect->dim.height) / (f32)cpu_cache->dim;
    const f32 vBottom = (f32)rect->dim.y / (f32)cpu_cache->dim;

    const f32 verts[16] = {
        // pos      uv
        0.0f, 0.0f, uLeft, vTop,
        1.0f, 0.0f, uRight, vTop,
        1.0f, 1.0f, uRight, vBottom,
        0.0f, 1.0f, uLeft, vBottom,
    };

    // @question Is this really correct?
    const f32 proj[16] = {
        2.0f / screen_dim.width, 0.0f, 0.0f, 0.0f,
        0.0f, -2.0f / screen_dim.height, 0.0f, 0.0f,
        0.0f, 0.0f, -1.0f, 0.0f,
        -1.0f, 1.0f, 0.0f, 1.0f
    };

    gpuapi_uniform_buffer_update_m4( // uProj
        blit->blit_pipeline.id,
        blit->blit_pipeline.descriptor_set_layout[0].binding,
        proj
    );
    gpuapi_uniform_buffer_update_v2( // uPos
        blit->blit_pipeline.id,
        blit->blit_pipeline.descriptor_set_layout[1].binding,
        &dim.x
    );
    gpuapi_uniform_buffer_update_v2( // uSize
        blit->blit_pipeline.id,
        blit->blit_pipeline.descriptor_set_layout[2].binding,
        &dim.width
    );
    gpuapi_uniform_buffer_update( // SamplerId
        blit->blit_pipeline.id,
        blit->blit_pipeline.descriptor_set_layout[3].binding,
        (int32) 0 // @question why is this 0?
    );

    // Binds to texture unit 0 by name,
    // which is the DSA replacement for glActiveTexture(GL_TEXTURE0) + glBindTexture(GL_TEXTURE_2D, ..)
    // @question is it really GL_TEXTURE0? or do we need a different unit?
    glBindTextureUnit(0, gpu_cache->texture.texture_id);
    glNamedBufferSubData(blit->blit_vbo, 0, sizeof(verts), verts);

    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

    glUseProgram(blit->blit_pipeline.id);
    glBindVertexArray(blit->blit_vao);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    glBindVertexArray(0);
}

#endif