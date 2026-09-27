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
 * 3. gpuapi_texture_cache_section_begin() depends on the situation
 * 4. glEnable(GL_SCISSOR_TEST)
 * 5. gpu_atlas_cache_exists() -> if false
 *      1. gpu_atlas_cache_reserve()
 *      3. ... create vertices ...
 * 7. gpu_atlas_cache_exists() -> if false
 *      1. gpu_atlas_cache_reserve()
 *      3. ... create vertices ...
 * 9. glDisable(GL_SCISSOR_TEST)
 * 10. or gpuapi_texture_cache_begin() instead of gpuapi_texture_cache_section_begin()
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

    gpu_cache->fbo = gpuapi_framebuffer_generate();
    glNamedFramebufferTexture(gpu_cache->fbo, GL_COLOR_ATTACHMENT0, gpu_cache->texture.texture_id, 0);
    glNamedFramebufferDrawBuffer(gpu_cache->fbo, GL_COLOR_ATTACHMENT0);

    LABEL_GPU_API(gpu_cache->fbo, GL_FRAMEBUFFER, "AtlasCacheFramebuffer");
    LABEL_GPU_API(gpu_cache->texture.texture_id, GL_TEXTURE, "AtlasCacheTexture");

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

// Use only to draw the entire cache, not just a subregion
void gpuapi_texture_cache_begin(OpenglAtlasCache* const __restrict cache) NO_EXCEPT
{
    glBindFramebuffer(GL_FRAMEBUFFER, cache->fbo);
    f32 clear_color[] = {0.0f, 0.0f, 0.0f, 0.0f};
    glClearNamedFramebufferfv(cache->fbo, GL_COLOR, 0, clear_color);
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
}

#endif