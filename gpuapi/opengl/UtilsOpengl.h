/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef TOS_GPUAPI_OPENGL_UTILS_H
#define TOS_GPUAPI_OPENGL_UTILS_H

#include "../../stdlib/Types.h"
#include "../../memory/RingMemory.h"
#include "../../utils/TestUtils.h"
#include "../../models/Attrib.h"
#include "../../models/Texture.h"

#include "../RenderUtils.h"
#include "../../../EngineDependencies/opengl/glew/include/GL/glew.h"
#include "../../../EngineDependencies/opengl/glfw/include/glfw3.h"

#if GLFW_EXPOSE_NATIVE_WIN32
    #include "../../../EngineDependencies/opengl/glfw/include/glfw3native.h"
#endif

#ifdef _WIN32
    #include <windows.h>
    #include "../../platform/win32/UtilsWin32.h"
#endif

struct Window {
    bool is_fullscreen;
    int32 width;
    int32 height;
    char name[32];

    int32 x;
    int32 y;

    GLFWwindow* hwnd_lib;

    #ifdef _WIN32
        HWND hwnd;
    #endif
};

inline
void window_create(Window* window, void*)
{
    //GLFWmonitor *monitor = glfwGetPrimaryMonitor();
    window->hwnd_lib = glfwCreateWindow(
        window->width,
        window->height,
        window->name,
        NULL,
        NULL
    );

    ASSERT_SIMPLE(window->hwnd_lib);

    //glfwSetInputMode(window->hwnd_lib, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    glfwMakeContextCurrent(window->hwnd_lib);
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

    #if GLFW_EXPOSE_NATIVE_WIN32
        window->hwnd = glfwGetWin32Window(window->hwnd_lib);
    #endif
}

inline
void window_open(Window* window)
{
    glfwMakeContextCurrent(window->hwnd_lib);
    glViewport(window->x, window->y, window->width, window->height);
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
}

inline
void window_close(Window* window)
{
    glfwWindowShouldClose(window->hwnd_lib);
}

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

inline
void prepare_texture(TextureFile* texture, uint32 texture_unit)
{
    uint32 texture_data_type = get_texture_data_type(texture->texture_data_type);

    glGenTextures(1, (GLuint *) &texture->id);
    glActiveTexture(GL_TEXTURE0 + texture_unit);
    glBindTexture(texture_data_type, (GLuint) texture->id);
}

inline
void load_texture_to_gpu(const TextureFile* texture, int mipmap_level = 0)
{
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
}

inline
void texture_use(const TextureFile* texture, uint32 texture_unit)
{
    glActiveTexture(GL_TEXTURE0 + texture_unit);
    glBindTexture(GL_TEXTURE_2D, (GLuint) texture->id);
}

GLuint make_shader(GLenum type, const char *source, RingMemory* ring)
{
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);

    GLint status;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);

    if (status == GL_FALSE) {
        GLint length;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);

        GLchar *info = (GLchar *) ring_get_memory(ring, length * sizeof(GLchar));

        glGetShaderInfoLog(shader, length, NULL, info);

        // @todo log
    }

    return shader;
}

GLuint load_shader(GLenum type, const char *path, RingMemory* ring) {
    uint64 temp = ring->pos;

    // @bug potential bug for shaders > 4 mb
    FileBody file;
    file.content = ring_get_memory(ring, MEGABYTE * 4);

    // @todo consider to accept file as parameter and load file before
    file_read(path, &file);
    GLuint result = make_shader(type, (const char *) file.content, ring);

    // 4 mb of memory reservation is a lot and since we immediately can dispose of it
    // we can also reset our ring memory position
    ring->pos = temp;

    return result;
}

GLuint make_program(GLuint shader1, GLuint shader2, RingMemory* ring) {
    GLuint program = glCreateProgram();

    glAttachShader(program, shader1);
    glAttachShader(program, shader2);
    glLinkProgram(program);

    GLint status;
    glGetProgramiv(program, GL_LINK_STATUS, &status);

    if (status == GL_FALSE) {
        GLint length;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);

        GLchar *info = (GLchar *) ring_get_memory(ring, length * sizeof(GLchar));

        glGetProgramInfoLog(program, length, NULL, info);

        // @todo use global logger
        fprintf(stderr, "glLinkProgram failed: %s\n", info);
    }

    // @question really?
    glDetachShader(program, shader1);
    glDetachShader(program, shader2);

    // @question really?
    glDeleteShader(shader1);
    glDeleteShader(shader2);

    return program;
}

GLuint load_program(const char *path1, const char *path2, RingMemory* ring) {
    GLuint shader1 = load_shader(GL_VERTEX_SHADER, path1, ring);
    GLuint shader2 = load_shader(GL_FRAGMENT_SHADER, path2, ring);
    GLuint program = make_program(shader1, shader2, ring);

    return program;
}

inline
void shader_use(uint32 id)
{
    glUseProgram(id);
}

void draw_triangles_3d(Attrib *attrib, GLuint buffer, int count) {
    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    glEnableVertexAttribArray(attrib->position);
    glEnableVertexAttribArray(attrib->normal);
    glEnableVertexAttribArray(attrib->uv);
    glVertexAttribPointer(attrib->position, 3, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 8, 0);
    glVertexAttribPointer(attrib->normal, 3, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 8, (GLvoid *)(sizeof(GLfloat) * 3));
    glVertexAttribPointer(attrib->uv, 2, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 8, (GLvoid *)(sizeof(GLfloat) * 6));
    glDrawArrays(GL_TRIANGLES, 0, count);
    glDisableVertexAttribArray(attrib->position);
    glDisableVertexAttribArray(attrib->normal);
    glDisableVertexAttribArray(attrib->uv);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void draw_triangles_2d(Attrib *attrib, GLuint buffer, size_t count) {
    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    glEnableVertexAttribArray(attrib->position);
    glEnableVertexAttribArray(attrib->uv);
    glVertexAttribPointer(attrib->position, 2, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 4, 0);
    glVertexAttribPointer(attrib->uv, 2, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 4, (GLvoid *)(sizeof(GLfloat) * 2));
    glDrawArrays(GL_TRIANGLES, 0, (GLsizei) count);
    glDisableVertexAttribArray(attrib->position);
    glDisableVertexAttribArray(attrib->uv);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

inline
void draw_text(Attrib *attrib, GLuint buffer, size_t length)
{
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    draw_triangles_2d(attrib, buffer, length * 6);
    glDisable(GL_BLEND);
}

GLuint gen_text_buffer(float x, float y, float n, const char *text) {
    size_t length = strlen(text);
    GLfloat *data = NULL; //malloc_faces(4, length); // sizeof(GLfloat) * 6 * 4 * length

    for (int i = 0; i < length; i++) {
        make_character(data + i * 24, x, y, n / 2, n, text[i]);
        x += n;
    }

    return 0; //gen_faces(4, length, data);
}

inline
void render_text(Attrib *attrib, int justify, float x, float y, float n, const char *text)
{
    float matrix[16];
    //set_matrix_2d(matrix, g->width, g->height);

    glUseProgram(attrib->program);
    glUniformMatrix4fv(attrib->matrix, 1, GL_FALSE, matrix);
    glUniform1i(attrib->sampler, 1);
    glUniform1i(attrib->extra1, 0);

    size_t length = strlen(text);
    x -= n * justify * (length - 1) / 2;

    GLuint buffer = gen_text_buffer(x, y, n, text);
    draw_text(attrib, buffer, length);

    glDeleteBuffers(1, &buffer);
}

inline
int calculate_face_size(int components, int faces)
{
    return sizeof(GLfloat) * 6 * components * faces;
}

// generates faces
// data is no longer needed after this
inline
uint32 gpuapi_buffer_generate(int size, f32 *data)
{
    uint32 vbo;

    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, size, data, GL_STATIC_DRAW);

    return vbo;
}

inline
uint32 gpuapi_buffer_element_generate(int size, uint32 *data)
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
    int imgWidth, int imgHeight,
    int img_x1, int img_x2,
    int img_y1, int img_y2,
    int renderWidth, int renderHeight,
    int repeat
)
{

}
*/

#endif