/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_GPUAPI_OPENGL_UTILS_H
#define COMS_GPUAPI_OPENGL_UTILS_H

#include "../../stdlib/Stdlib.h"
#include "../../memory/RingMemory.cpp"
#include "../../utils/Assert.h"
#include "../../object/Texture.h"
#include "../../image/Image.cpp"
#include "../../utils/StringUtils.h"
#include "../../log/Log.h"
#include "../../log/Stats.h"
#include "../../log/PerformanceProfiler.h"
#include "../../system/FileUtils.cpp"
#include "../RenderUtils.h"
#include "Opengl.h"
#include "PersistentGpuBuffer.h"

#if DEBUG
    void gpuapi_error()
    {
        GLenum err;
        while ((err = glGetError()) != GL_NO_ERROR) {
            switch (err) {
                case GL_INVALID_ENUM: {
                    LOG_1("[ERROR] Opengl invalid enumeration parameter");
                    break;
                }
                case GL_INVALID_VALUE: {
                    LOG_1("[ERROR] Opengl invalid parameter");
                    break;
                }
                case GL_INVALID_OPERATION: {
                    LOG_1("[ERROR] Opengl invalid state and parameter combination");
                    break;
                }
                case GL_STACK_OVERFLOW: {
                    LOG_1("[ERROR] Opengl stack overflow");
                    break;
                }
                case GL_STACK_UNDERFLOW: {
                    LOG_1("[ERROR] Opengl stack underflow");
                    break;
                }
                case GL_OUT_OF_MEMORY: {
                    LOG_1("[ERROR] Opengl couldn't allocate memory");
                    break;
                }
                case GL_INVALID_FRAMEBUFFER_OPERATION: {
                    LOG_1("[ERROR] Opengl reading/writing from/to incomplete framebuffer");
                    break;
                }
                default:
                    LOG_1("[ERROR] Opengl %d", {DATA_TYPE_INT32, (int32 *) &err});
            }

            ASSERT_TRUE(err == GL_NO_ERROR);
        }
    }

    #define ASSERT_GPU_API() gpuapi_error()
#else
    #define ASSERT_GPU_API() ((void) 0)
#endif

void opengl_debug_callback(GLenum, GLenum, GLuint, GLenum severity, GLsizei, const GLchar* message, const void*)
{
    if (severity < GL_DEBUG_SEVERITY_LOW) {
        return;
    }

    LOG_1(message);
    ASSERT_TRUE(false);
}

FORCE_INLINE
GpuFence gpuapi_fence_create() NO_EXCEPT
{
    return (GpuFence) 0;
}

FORCE_INLINE
void gpuapi_fence_lock(GpuFence* const fence) NO_EXCEPT
{
    if (fence) {
        ASSERT_TRUE_CONST(false);
        glDeleteSync(*fence);
        *fence = 0;
    }

    *fence = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
}

// Checks if a fence is unlocked
inline
bool gpuapi_fence_is_unlocked(GpuFence* const fence) NO_EXCEPT
{
    if (!(*fence)) {
        return true;
    }

    const GLenum res = glClientWaitSync(*fence, GL_SYNC_FLUSH_COMMANDS_BIT, 0); /* timeout 0 -> poll */
    if (res == GL_ALREADY_SIGNALED || res == GL_CONDITION_SATISFIED) {
        // GPU is done; delete sync and mark slot free
        glDeleteSync(*fence);
        *fence = 0;

        return true;
    }

    ASSERT_TRUE_CONST(false);

    return false;
}

FORCE_INLINE
void gpuapi_fence_delete(GpuFence* const fence) NO_EXCEPT
{
    glDeleteSync(*fence);
    *fence = 0;
}

FORCE_INLINE
void change_viewport(f32 width, f32 height, int32 offset_x = 0, int32 offset_y = 0) NO_EXCEPT
{
    glViewport(offset_x, offset_y, (int32) width, (int32) height);
}

FORCE_INLINE
void vsync_set(int32 on) NO_EXCEPT
{
    wglSwapIntervalEXT((int32) on);
}

FORCE_INLINE
void wireframe_mode(bool on) NO_EXCEPT
{
    glPolygonMode(GL_FRONT_AND_BACK, on ? GL_LINE : GL_FILL);
}

struct OpenglInfo {
    char* renderer;
    int32 major;
    int32 minor;
};

inline
void opengl_info(OpenglInfo* const info) NO_EXCEPT
{
    info->renderer = (char *) glGetString(GL_RENDERER);
    info->major = 1;
    info->minor = 0;

    char* version = (char *) glGetString(GL_VERSION);

    for (char *at = version; *at; ++at) {
        if (*at == '.') {
            info->major = (int32) str_to_int(version);

            ++at;
            info->minor = (int32) str_to_int(at);
            break;
        }
    }
}

// @todo rename to gpuapi_*
inline
uint32 gpuapi_texture_data_type(uint32 texture_data_type) NO_EXCEPT
{
    switch (texture_data_type) {
        case TEXTURE_DATA_TYPE_2D: {
            return GL_TEXTURE_2D;
        }
        case TEXTURE_DATA_TYPE_1D: {
            return GL_TEXTURE_1D;
        }
        case TEXTURE_DATA_TYPE_3D: {
            return GL_TEXTURE_3D;
        }
        case TEXTURE_DATA_TYPE_1D_ARRAY: {
            return GL_TEXTURE_1D_ARRAY;
        }
        case TEXTURE_DATA_TYPE_2D_ARRAY: {
            return GL_TEXTURE_2D_ARRAY;
        }
        case TEXTURE_DATA_TYPE_2D_MULTISAMPLED: {
            return GL_TEXTURE_2D_MULTISAMPLE;
        }
        case TEXTURE_DATA_TYPE_2D_MULTISAMPLED_ARRAY: {
            return GL_TEXTURE_2D_MULTISAMPLE_ARRAY;
        }
        default: {
            return GL_TEXTURE_2D;
        }
    }
}

// 1. prepare_texture
// 2. define wrap
// 3. define filter
// 4. gpuapi_texture_to_gpu
// 5. texture_use

FORCE_INLINE
void gpuapi_prepare_texture(Texture* const texture) NO_EXCEPT
{
    const uint32 texture_data_type = gpuapi_texture_data_type(texture->texture_data_type);

    glGenTextures(1, (GLuint *) &texture->id);
    glActiveTexture(GL_TEXTURE0 + texture->sample_id);
    glBindTexture(texture_data_type, (GLuint) texture->id);
}

// @todo this should have a gpuapi_ name
inline
void gpuapi_texture_to_gpu(const Texture* const texture, int32 mipmap_level = 0) NO_EXCEPT
{
    PROFILE_START(PROFILE_GPU);
    // @todo also handle different texture formats (R, RG, RGB, 1 byte vs 4 byte per pixel)
    const uint32 texture_data_type = gpuapi_texture_data_type(texture->texture_data_type);
    glTexImage2D(
        texture_data_type, mipmap_level, GL_RGBA,
        texture->image.width, texture->image.height,
        0, GL_RGBA, GL_UNSIGNED_BYTE,
        texture->image.pixels
    );

    if (mipmap_level > -1) {
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    PROFILE_END(PROFILE_GPU);

    STATS_INCREMENT_BY(
        DEBUG_COUNTER_GPU_UPLOAD,
        texture->image.pixel_count * image_pixel_size_from_type(texture->image.image_settings)
    );
}

FORCE_INLINE
void gpuapi_texture_use(const Texture* const texture) NO_EXCEPT
{
    const uint32 texture_data_type = gpuapi_texture_data_type(texture->texture_data_type);

    glActiveTexture(GL_TEXTURE0 + texture->sample_id);
    glBindTexture(texture_data_type, (GLuint) texture->id);
}

FORCE_INLINE
void gpuapi_texture_delete(Texture* const texture) NO_EXCEPT
{
    glDeleteTextures(1, &texture->id);
}

inline
void draw_triangles_3d(VertexRef* const vertices, GLuint buffer, int32 count) NO_EXCEPT
{
    glBindBuffer(GL_ARRAY_BUFFER, buffer);

    // position attribute
    glVertexAttribPointer(vertices->data_id, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex3D), (void *) 0);
    glEnableVertexAttribArray(vertices->data_id);

    // normal attribute
    glVertexAttribPointer(vertices->normal_id, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex3D), (void *) (sizeof(f32) * 3));
    glEnableVertexAttribArray(vertices->normal_id);

    // texture coord attribute
    // vs glVertexAttribPointer
    glVertexAttribIPointer(vertices->tex_coord_id, 2, GL_UNSIGNED_INT, sizeof(Vertex3D), (void *) (sizeof(f32) * 6));
    glEnableVertexAttribArray(vertices->tex_coord_id);

    // color attribute
    glVertexAttribPointer(vertices->color_id, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex3D), (void *) (sizeof(f32) * 8));
    glEnableVertexAttribArray(vertices->color_id);

    glDrawArrays(GL_TRIANGLES, 0, count);

    glDisableVertexAttribArray(vertices->data_id);
    glDisableVertexAttribArray(vertices->normal_id);
    glDisableVertexAttribArray(vertices->tex_coord_id);
    glDisableVertexAttribArray(vertices->color_id);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

inline
void draw_triangles_3d_textureless(VertexRef* const vertices, GLuint buffer, int32 count) NO_EXCEPT
{
    glBindBuffer(GL_ARRAY_BUFFER, buffer);

    // position attribute
    glVertexAttribPointer(vertices->data_id, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex3D), (void *) 0);
    glEnableVertexAttribArray(vertices->data_id);

    // normal attribute
    glVertexAttribPointer(vertices->normal_id, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex3D), (void *) (sizeof(f32) * 3));
    glEnableVertexAttribArray(vertices->normal_id);

    glDrawArrays(GL_TRIANGLES, 0, count);

    glDisableVertexAttribArray(vertices->data_id);
    glDisableVertexAttribArray(vertices->normal_id);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

inline
void draw_triangles_3d_colorless(VertexRef* const vertices, GLuint buffer, int32 count) NO_EXCEPT
{
    glBindBuffer(GL_ARRAY_BUFFER, buffer);

    // position attribute
    glVertexAttribPointer(vertices->data_id, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex3D), (void *) 0);
    glEnableVertexAttribArray(vertices->data_id);

    // normal attribute
    glVertexAttribPointer(vertices->normal_id, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex3D), (void *) (sizeof(f32) * 3));
    glEnableVertexAttribArray(vertices->normal_id);

    // texture coord attribute
    glVertexAttribIPointer(vertices->tex_coord_id, 2, GL_UNSIGNED_INT, sizeof(Vertex3D), (void *) (sizeof(f32) * 6));
    glEnableVertexAttribArray(vertices->tex_coord_id);

    glDrawArrays(GL_TRIANGLES, 0, count);

    glDisableVertexAttribArray(vertices->data_id);
    glDisableVertexAttribArray(vertices->normal_id);
    glDisableVertexAttribArray(vertices->tex_coord_id);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

inline
void draw_triangles_2d(VertexRef* const vertices, GLuint buffer, int32 count) NO_EXCEPT
{
    glBindBuffer(GL_ARRAY_BUFFER, buffer);

    // position attribute
    glVertexAttribPointer(vertices->position_id, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex2D), (void *) 0);
    glEnableVertexAttribArray(vertices->position_id);

    // texture coord attribute
    // vs glVertexAttribPointer
    glVertexAttribIPointer(vertices->tex_coord_id, 2, GL_UNSIGNED_INT, sizeof(Vertex2D), (void *) (sizeof(f32) * 2));
    glEnableVertexAttribArray(vertices->tex_coord_id);

    glDrawArrays(GL_TRIANGLES, 0, count);

    glDisableVertexAttribArray(vertices->position_id);
    glDisableVertexAttribArray(vertices->tex_coord_id);
    glDisableVertexAttribArray(vertices->color_id);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

inline
void draw_triangles_2d_textureless(VertexRef* const vertices, GLuint buffer, int32 count) NO_EXCEPT
{
    glBindBuffer(GL_ARRAY_BUFFER, buffer);

    // position attribute
    glVertexAttribPointer(vertices->data_id, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex2D), (void *) 0);
    glEnableVertexAttribArray(vertices->data_id);

    // color attribute
    glVertexAttribPointer(vertices->color_id, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex2D), (void *) (sizeof(f32) * 4));
    glEnableVertexAttribArray(vertices->color_id);

    glDrawArrays(GL_TRIANGLES, 0, count);

    glDisableVertexAttribArray(vertices->data_id);
    glDisableVertexAttribArray(vertices->color_id);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

inline
void draw_triangles_2d_colorless(VertexRef* const vertices, GLuint buffer, int32 count) NO_EXCEPT
{
    glBindBuffer(GL_ARRAY_BUFFER, buffer);

    // position attribute
    glVertexAttribPointer(vertices->data_id, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex2D), (void *) 0);
    glEnableVertexAttribArray(vertices->data_id);

    // texture coord attribute
    glVertexAttribIPointer(vertices->tex_coord_id, 2, GL_UNSIGNED_INT, sizeof(Vertex2D), (void *) (sizeof(f32) * 2));
    glEnableVertexAttribArray(vertices->tex_coord_id);

    glDrawArrays(GL_TRIANGLES, 0, count);

    glDisableVertexAttribArray(vertices->data_id);
    glDisableVertexAttribArray(vertices->tex_coord_id);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

FORCE_INLINE
int32 calculate_face_size(int components, int32 faces) NO_EXCEPT
{
    return sizeof(GLfloat) * 6 * components * faces;
}

// generates faces
// data is no longer needed after this
inline
uint32 gpuapi_buffer_generate(int32 type, int32 size, const void* data) NO_EXCEPT
{
    uint32 bo;

    glGenBuffers(1, &bo);
    glBindBuffer(type, bo);
    glBufferData(type, size, data, GL_STATIC_DRAW);

    STATS_INCREMENT_BY(DEBUG_COUNTER_GPU_UPLOAD, size);

    return bo;
}

inline
uint32 gpuapi_buffer_generate_dynamic(int32 type, int32 size, const void* data) NO_EXCEPT
{
    uint32 bo;

    glGenBuffers(1, &bo);
    glBindBuffer(type, bo);
    glBufferData(type, size, data, GL_DYNAMIC_DRAW);

    STATS_INCREMENT_BY(DEBUG_COUNTER_GPU_UPLOAD, size);

    return bo;
}

// type is GL_UNIFORM_BUFFER or GL_ARRAY_BUFFER
inline
void gpuapi_buffer_persistent_generate(int32 type, PersistentGpuBuffer* const buffer) NO_EXCEPT
{
    // @todo we need to dynamically get the alignment and pass it in
    //      For that we need to get it once and then store it in the system info struct?
    // glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &alignment);
    ASSERT_TRUE(MODULO_2(buffer->size, 256) == 0);
    ASSERT_TRUE(MODULO_2(buffer->range_stride, 256) == 0);

    glGenBuffers(1, &buffer->bo);
    glBindBuffer(type, buffer->bo);
    glBufferStorage(
        type, buffer->size, NULL,
        GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_DYNAMIC_STORAGE_BIT
    );

    buffer->data = (byte *) glMapBufferRange(
        type, 0, buffer->size,
        GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_FLUSH_EXPLICIT_BIT
    );

    ASSERT_GPU_API();
    ASSERT_TRUE(buffer->data);

    STATS_INCREMENT_BY(DEBUG_COUNTER_GPU_UPLOAD, buffer->size);
}

FORCE_INLINE
void gpuapi_buffer_persistent_delete(GLuint vbo) NO_EXCEPT
{
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glUnmapBuffer(GL_ARRAY_BUFFER);
}

// region_start is the start of that specific persistent buffer region
//      what i mean by this we may have one large persistent buffer where one part contains the camera matrix, another ui data etc.
//      and every region may have double/triple buffering
// frame_index usually 0, 1, 2 in triple buffer
// region_size size per sub-buffer (not the complete triple buffer but one of the triple buffers)
//      e.g. for a camera it would be 4x4 float matrix = 16*sizeof(float)
//      -> total subregion if we use triple buffering = 16*sizeof(float)*3
//      -> careful we are aligning the region_size to 16 bytes since some work better with that alignment or is even required in some cases
// WARNING: This doesn't check for locks in fences
//          You have to create your own fences and check them before calling this function
//          The reason for this is only you know which fence to use and how many you need for the entire region
//          Each subregion * count_of_buffers needs its own fence
/*
inline
void gpuapi_draw_buffer_subregion(void* region_start, int32 frame_index, size_t region_size) NO_EXCEPT
{
    region_size = align_up(region_size, 16);
    size_t offset = frame_index * region_size;

    glDrawArrays(GL_TRIANGLES, 0, vertex_count);
}
*/

FORCE_INLINE
uint32 gpuapi_framebuffer_generate() NO_EXCEPT
{
    uint32 fbo;

    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    return fbo;
}

FORCE_INLINE
uint32 gpuapi_renderbuffer_generate() NO_EXCEPT
{
    uint32 rbo;

    glGenRenderbuffers(1, &rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, rbo);

    return rbo;
}

FORCE_INLINE
void gpuapi_buffer_update_dynamic(uint32 vbo, int32 size, const void* data) NO_EXCEPT
{
    PROFILE_START(PROFILE_GPU);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, size, data, GL_DYNAMIC_DRAW);
    PROFILE_END(PROFILE_GPU);

    STATS_INCREMENT_BY(DEBUG_COUNTER_GPU_UPLOAD, size);
}

// @todo change name. vulkan and directx have different functions for vertex buffer updates
// @question vertex_count is a count where offset is bytes, this seems inconsistent
// WARNING: if the offset is 0 you MUST provide the max. number of vertices for rendering,
//  otherwise a subsequent call to the same vbo may fail if it has more vertices
inline
void gpuapi_vertex_buffer_update(
    uint32 vbo,
    const void* data, int32 vertex_size, int32 vertex_count, int32 offset = 0
) NO_EXCEPT
{
    PROFILE_START(PROFILE_GPU);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    // @performance Does this if even make sense or is glBufferSubData always the better choice?
    if (offset) {
        // @bug press Ctrl+1 = hide and show debug info multiple times and this will trigger an exception
        //      First I thought I overflow the data but after a couple of tests I even got a bug with very small vertex_count numbers
        //      Maybe something happens to the void* data content, or the vbo gets unbound?
        glBufferSubData(GL_ARRAY_BUFFER, offset, vertex_size * vertex_count - offset, ((byte *) data) + offset);
    } else {
        glBufferData(GL_ARRAY_BUFFER, vertex_size * vertex_count, data, GL_DYNAMIC_DRAW);
    }
    PROFILE_END(PROFILE_GPU);
    ASSERT_GPU_API();

    STATS_INCREMENT_BY(DEBUG_COUNTER_GPU_UPLOAD, vertex_size * vertex_count - offset);
}

FORCE_INLINE
uint32 gpuapi_vertex_buffer_create() NO_EXCEPT
{
    uint32 vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    return vao;
}

FORCE_INLINE
void gpuapi_unbind_all() NO_EXCEPT
{
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

FORCE_INLINE
void gpuapi_buffer_delete(GLuint buffer) NO_EXCEPT
{
    glDeleteBuffers(1, &buffer);
}

FORCE_INLINE
void gpuapi_vertex_array_delete(GLuint buffer) NO_EXCEPT
{
    glDeleteVertexArrays(1, &buffer);
}

inline
int32 get_gpu_free_memory() NO_EXCEPT
{
    GLint available = 0;
    glGetIntegerv(GL_GPU_MEMORY_INFO_CURRENT_AVAILABLE_VIDMEM_NVX, &available);

    if (available != 0) {
        return available;
    }

    glGetIntegerv(GL_TEXTURE_FREE_MEMORY_ATI, &available);

    return available;
}

/*
void render_9_patch(GLuint texture,
    int32 imgWidth, int32 imgHeight,
    int32 img_x1, int32 img_x2,
    int32 img_y1, int32 img_y2,
    int32 renderWidth, int32 renderHeight,
    int32 repeat
)
{

}
*/

#endif