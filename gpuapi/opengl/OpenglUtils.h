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

#include "../../stdlib/Types.h"
#include "../../memory/RingMemory.h"
#include "../../utils/TestUtils.h"
#include "../../object/Texture.h"
#include "../../image/Image.cpp"
#include "../../utils/StringUtils.h"
#include "../../log/Log.h"
#include "../../log/Stats.h"
#include "../../log/PerformanceProfiler.h"
#include "../../system/FileUtils.cpp"
#include "../RenderUtils.h"
#include "Opengl.h"

#if _WIN32
    #include "../../platform/win32/Window.h"
#elif __linux__
    #include "../../platform/linux/Window.h"
#endif

#if DEBUG
    void gpuapi_error()
    {
        GLenum err;
        while ((err = glGetError()) != GL_NO_ERROR) {
            LOG_1("Opengl error: %d", {{LOG_DATA_INT32, (int32 *) &err}});
            ASSERT_SIMPLE(err == GL_NO_ERROR);
        }
    }

    #define ASSERT_GPU_API() gpuapi_error()
#else
    #define ASSERT_GPU_API() ((void) 0)
#endif

struct OpenglFrameData {
    uint32 framebuffer;
    uint32 renderbuffer;
    Texture* texture;

    // msaa data
    uint32 framebuffer_msaa;
    uint32 colorbuffer_msaa;
    uint32 depthbuffer_msaa;
    Texture* texture_msaa;
};

void opengl_debug_callback(GLenum, GLenum, GLuint, GLenum severity, GLsizei, const GLchar* message, const void*)
{
    if (severity < GL_DEBUG_SEVERITY_LOW) {
        return;
    }

    LOG_1(message);
    ASSERT_SIMPLE(false);
}

inline
void change_viewport(int32 width, int32 height, int32 offset_x = 0, int32 offset_y = 0)
{
    glViewport(offset_x, offset_y, width, height);
}

inline
void vsync_set(int32 on)
{
    wglSwapIntervalEXT((int32) on);
}

inline
void wireframe_mode(bool on)
{
    glPolygonMode(GL_FRONT_AND_BACK, on ? GL_LINE : GL_FILL);
}

struct OpenglInfo {
    char* renderer;
    int32 major;
    int32 minor;
};

void opengl_info(OpenglInfo* info)
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
uint32 get_texture_data_type(uint32 texture_data_type)
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
// 4. load_texture_to_gpu
// 5. texture_use

// @todo this should have a gpuapi_ name
inline
void prepare_texture(Texture* texture)
{
    uint32 texture_data_type = get_texture_data_type(texture->texture_data_type);

    glGenTextures(1, (GLuint *) &texture->id);
    glActiveTexture(GL_TEXTURE0 + texture->sample_id);
    glBindTexture(texture_data_type, (GLuint) texture->id);
}

// @todo this should have a gpuapi_ name
inline
void load_texture_to_gpu(const Texture* texture, int32 mipmap_level = 0)
{
    // @todo also handle different texture formats (R, RG, RGB, 1 byte vs 4 byte per pixel)
    uint32 texture_data_type = get_texture_data_type(texture->texture_data_type);
    glTexImage2D(
        texture_data_type, mipmap_level, GL_RGBA,
        texture->image.width, texture->image.height,
        0, GL_RGBA, GL_UNSIGNED_BYTE,
        texture->image.pixels
    );

    if (mipmap_level > -1) {
        glGenerateMipmap(GL_TEXTURE_2D);
    }

    LOG_INCREMENT_BY(DEBUG_COUNTER_GPU_VERTEX_UPLOAD, texture->image.pixel_count * image_pixel_size_from_type(texture->image.image_settings));
}

// @todo this should have a gpuapi_ name
inline
void texture_use(const Texture* texture)
{
    uint32 texture_data_type = get_texture_data_type(texture->texture_data_type);

    glActiveTexture(GL_TEXTURE0 + texture->sample_id);
    glBindTexture(texture_data_type, (GLuint) texture->id);
}

// @todo this should have a gpuapi_ name
inline
void texture_delete(Texture* texture) {
    glDeleteTextures(1, &texture->id);
}

inline
void draw_triangles_3d(VertexRef* vertices, GLuint buffer, int32 count) {
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
void draw_triangles_3d_textureless(VertexRef* vertices, GLuint buffer, int32 count) {
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
void draw_triangles_3d_colorless(VertexRef* vertices, GLuint buffer, int32 count) {
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
void draw_triangles_2d(VertexRef* vertices, GLuint buffer, int32 count) {
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
void draw_triangles_2d_textureless(VertexRef* vertices, GLuint buffer, int32 count) {
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
void draw_triangles_2d_colorless(VertexRef* vertices, GLuint buffer, int32 count) {
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

inline
int calculate_face_size(int components, int32 faces)
{
    return sizeof(GLfloat) * 6 * components * faces;
}

// generates faces
// data is no longer needed after this
inline
uint32 gpuapi_buffer_generate(int32 size, const void* data)
{
    uint32 vbo;

    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, size, data, GL_STATIC_DRAW);

    LOG_INCREMENT_BY(DEBUG_COUNTER_GPU_VERTEX_UPLOAD, size);

    return vbo;
}

inline
uint32 gpuapi_buffer_generate_dynamic(int32 size, const void* data)
{
    uint32 vbo;

    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, size, data, GL_DYNAMIC_DRAW);

    LOG_INCREMENT_BY(DEBUG_COUNTER_GPU_VERTEX_UPLOAD, size);

    return vbo;
}

inline
uint32 gpuapi_framebuffer_generate()
{
    uint32 fbo;

    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    return fbo;
}

inline
uint32 gpuapi_renderbuffer_generate()
{
    uint32 rbo;

    glGenRenderbuffers(1, &rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, rbo);

    return rbo;
}

inline
void gpuapi_buffer_update_dynamic(uint32 vbo, int32 size, const void* data)
{
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, size, data, GL_DYNAMIC_DRAW);

    LOG_INCREMENT_BY(DEBUG_COUNTER_GPU_VERTEX_UPLOAD, size);
}

// @todo change name. vulkan and directx have different functions for vertex buffer updates
inline
void gpuapi_vertex_buffer_update(
    uint32 vbo,
    const void* data, int32 vertex_size, int32 vertex_count, int32 offset = 0
)
{
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    // @performance Does this if even make sense or is glBufferSubData always the better choice?
    if (offset) {
        glBufferSubData(GL_ARRAY_BUFFER, offset, vertex_size * vertex_count - offset, ((byte *) data) + offset);
    } else {
        glBufferData(GL_ARRAY_BUFFER, vertex_size * vertex_count, data, GL_DYNAMIC_DRAW);
    }
    ASSERT_GPU_API();

    LOG_INCREMENT_BY(DEBUG_COUNTER_GPU_VERTEX_UPLOAD, vertex_size * vertex_count - offset);
}

inline
uint32 gpuapi_shaderbuffer_generate(int32 size, const void* data)
{
    uint32 sbo;

    glGenBuffers(1, &sbo);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, sbo);
    glBufferData(GL_SHADER_STORAGE_BUFFER, size, data, GL_DYNAMIC_DRAW);

    return sbo;
}

inline
uint32 gpuapi_uniformbuffer_generate(int32 size, const void* data)
{
    uint32 ubo;

    glGenBuffers(1, &ubo);
    glBindBuffer(GL_UNIFORM_BUFFER, ubo);
    glBufferData(GL_UNIFORM_BUFFER, size, data, GL_STATIC_DRAW);

    return ubo;
}

inline
uint32 gpuapi_buffer_element_generate(int32 size, uint32 *data)
{
    uint32 ebo;

    glGenBuffers(1, &ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, size, data, GL_STATIC_DRAW);

    return ebo;
}

inline
uint32 gpuapi_vertex_array_generate()
{
    uint32 vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    return vao;
}

inline
void gpuapi_unbind_all()
{
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

inline
void gpuapi_buffer_delete(GLuint buffer)
{
    glDeleteBuffers(1, &buffer);
}

inline
void gpuapi_vertex_array_delete(GLuint buffer)
{
    glDeleteVertexArrays(1, &buffer);
}

int get_gpu_free_memory()
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