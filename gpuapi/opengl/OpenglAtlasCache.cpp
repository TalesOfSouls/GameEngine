/**
 * Example use case:
 *
 * 1. gpuapi_texture_cache_setup()
 * 2. gpuapi_texture_cache_create()
 *
 * 3. gpuapi_binds_cache()
 * 4. glEnable(GL_SCISSOR_TEST)
 * 5. gpuapi_texture_cache_begin()
 *      1. ... draw ...
 * 6. gpuapi_texture_cache_begin()
 *      1. ... draw ...
 * 7. glDisable(GL_SCISSOR_TEST)
 * 8. gpuapi_binds_restore()
 *
 * 9. gpuapi_binds_restore()
 * 10. gpuapi_texture_cache_delete()
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
#include "Opengl.h"
#include "OpenglAtlasCache.h"
#include "PersistentGpuBuffer.h"
#include "OpenglUtils.h"
#include "ShaderUtils.h"

void gpuapi_texture_cache_create(
    OpenglAtlasCache* const __restrict cache
    const char* const __restrict vertex_shader,
    const char* const __restrict fragment_shader,
) NO_EXCEPT
{
    // Setup program
    {
        GLuint vs_id = gpuapi_shader_make(..., );
        GLuint fs_id = gpuapi_shader_make(..., );
        cache->blit_pipeline.id = gpuapi_pipeline_make(vs_id, fs_id, -1);

        OpenglDescriptorSetLayoutBinding bindings[] = {
            {0, "uProj"},
            {0, "uPos"},
            {0, "uSize"},
            {0, "SamplerId"},
        };
        gpuapi_descriptor_set_layout_create(&cache->blit_pipeline, bindings, ARRAY_COUNT(bindings));
    }

    cache->blit_vao = gpuapi_vertex_buffer_create();
    cache->blit_vbo = gpuapi_buffer_generate_static(
        _GPUAPI_ATTRIBUTE_STRIDE[GPU_ATTRIBUTE_TYPE_VERTEX_2D_TEXTURE],
        NULL
    );

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

void gpuapi_texture_cache_delete(OpenglAtlasCache* const cache) NO_EXCEPT
{
    glDeleteFramebuffers(1, &cache->fbo);
    glDeleteTextures(1, &cache->texture_id);

    glDeleteProgram(cache->blit_pipeline);
    glDeleteVertexArrays(1, &cache->blit_vao);
    glDeleteBuffers(1, &cache->blit_vbo);
}

void gpuapi_texture_cache_setup(OpenglAtlasCache* const cache) NO_EXCEPT
{
    glCreateTextures(GL_TEXTURE_2D, 1, &cache->texture_id);
    glTextureStorage2D(cache->texture_id, 1, GL_RGBA8, cache->cache.dim.width, cache->cache.dim.height);
    glTextureParameteri(cache->texture_id, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTextureParameteri(cache->texture_id, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTextureParameteri(cache->texture_id, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameteri(cache->texture_id, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glCreateFramebuffers(1, &cache->fbo);
    glNamedFramebufferTexture(cache->fbo, GL_COLOR_ATTACHMENT0, cache->texture_id, 0);

    glNamedFramebufferDrawBuffer(cache->fbo, GL_COLOR_ATTACHMENT0);

    DEBUG_VAR_DEF_ASSIGN(const GLenum, status) glCheckNamedFramebufferStatus(cache->fbo, GL_FRAMEBUFFER);
    ASSERT_TRUE(status == GL_FRAMEBUFFER_COMPLETE);

    ASSERT_GPU_API();
}

// WARNING: You should probably call gpuapi_binds_cache() first
// WARNING: needs glEnable(GL_SCISSOR_TEST);
bool gpuapi_texture_cache_begin(
    OpenglAtlasCache* cache, uint32 cache_id, uint32 frame_id, CacheRect *outRect
) NO_EXCEPT
{
    const bool is_cached = gpuapi_cache_find(cache->cache, cache_id, frame_id);
    if (is_cached) {
        return false;
    }

    // We have to bind, because DSA doesn't support bindless rendering into a framebuffer
    // Draw commands render into whatever is bound

    glBindFramebuffer(GL_FRAMEBUFFER, cache->fbo[outRect->page]);
    glViewport(outRect->x, outRect->y, outRect->w, outRect->h);

    ASSERT_TRUE(glIsEnabled(GL_SCISSOR_TEST));

    glScissor(outRect->x, outRect->y, outRect->w, outRect->h);

    // Clear area before drawing
    static const f32 clear_color[] = {0.0f, 0.0f, 0.0f, 0.0f};
    glClearNamedFramebufferfv(cache->fbo, GL_COLOR, 0, clear_color);

    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

    return true;
}

// WARNING: This is slow, don't use it you should rather use the normal texture rendering
void gpuapi_texture_cache_blit(
    OpenglAtlasCache* cache, const CacheRect *rect, f32 x, f32 y, f32 w, f32 h,
    f32 r, f32 g, f32 b, f32 a, int screenW, int screenH
) NO_EXCEPT
{
    // WARNING: OpenGL's window/framebuffer *storage* is bottom-up
    f32 uLeft = (f32)rect->x / (f32)cache->pageW;
    f32 uRight = (f32)(rect->x + rect->w) / (f32)cache->pageW;
    f32 vTop = (f32)(rect->y + rect->h) / (f32)cache->pageH;
    f32 vBottom = (f32)rect->y / (f32)cache->pageH;

    const f32 verts[16] = {
        // pos      uv
        0.0f, 0.0f, uLeft, vTop,
        1.0f, 0.0f, uRight, vTop,
        1.0f, 1.0f, uRight, vBottom,
        0.0f, 1.0f, uLeft, vBottom,
    };

    // @question Is this really correct?
    const f32 proj[16] = {
        2.0f / screenW, 0.0f, 0.0f, 0.0f,
        0.0f, -2.0f / screenH, 0.0f, 0.0f,
        0.0f, 0.0f, -1.0f, 0.0f,
        -1.0f, 1.0f, 0.0f, 1.0f
    };

    glProgramUniformMatrix4fv(cache->blit_program, cache->uProj, 1, GL_FALSE, proj);
    glProgramUniform2f(cache->blit_program, cache->uPos, x, y);
    glProgramUniform2f(cache->blit_program, cache->uSize, w, h);
    glProgramUniform1i(cache->blit_program, cache->uTex, 0);

    // Binds to texture unit 0 by name,
    // which is the DSA replacement for glActiveTexture(GL_TEXTURE0) + glBindTexture(GL_TEXTURE_2D, ..)
    // @question is it really GL_TEXTURE0? or do we need a different unit?
    glBindTextureUnit(0, cache->texture_id);
    glNamedBufferSubData(cache->blit_vbo, 0, sizeof(verts), verts);

    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

    glUseProgram(cache->blit_program);
    glBindVertexArray(cache->blit_vao);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    glBindVertexArray(0);
}

#endif