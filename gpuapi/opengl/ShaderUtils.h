/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_GPUAPI_OPENGL_SHADER_UTILS_H
#define COMS_GPUAPI_OPENGL_SHADER_UTILS_H

#include "../../stdlib/Types.h"
#include "../../memory/RingMemory.h"
#include "../../log/Log.h"
#include "../../log/Stats.h"
#include "../../log/PerformanceProfiler.h"
#include "../../object/Vertex.h"
#include "../../utils/StringUtils.h"
#include "Shader.h"
#include "Opengl.h"
#include "../ShaderType.h"
#include "../GpuAttributeType.h"

struct OpenglVertexInputAttributeDescription {
    uint32 location;
    uint32 count;
    int32 format;
    int32 stride;
    void* offset;
};

inline
int32 ogl_shader_type_index(ShaderType type) NO_EXCEPT
{
    switch (type) {
        case SHADER_TYPE_VERTEX:
            return GL_VERTEX_SHADER;
        case SHADER_TYPE_FRAGMENT:
            return GL_FRAGMENT_SHADER;
        default:
            UNREACHABLE();
    }
}

// Set value based on uniform location
// @todo change naming to gpuapi_uniform_buffer_update (same as vulkan)
// @todo change from upload to uniform upload since it is a special form of upload
FORCE_INLINE
void gpuapi_uniform_buffer_update(uint32 location, bool value) NO_EXCEPT
{
    glUniform1i(location, (int32) value);
    STATS_INCREMENT_BY(DEBUG_COUNTER_GPU_UNIFORM_UPLOAD, sizeof(value));
}

FORCE_INLINE
void gpuapi_uniform_buffer_update(uint32 location, int32 value) NO_EXCEPT
{
    glUniform1i(location, value);
    STATS_INCREMENT_BY(DEBUG_COUNTER_GPU_UNIFORM_UPLOAD, sizeof(value));
}

FORCE_INLINE
void gpuapi_uniform_buffer_update(uint32 location, f32 value) NO_EXCEPT
{
    glUniform1f(location, value);
    STATS_INCREMENT_BY(DEBUG_COUNTER_GPU_UNIFORM_UPLOAD, sizeof(value));
}

FORCE_INLINE
void gpuapi_uniform_buffer_update_v2(uint32 location, const f32* value) NO_EXCEPT
{
    glUniform2fv(location, 1, value);
    STATS_INCREMENT_BY(DEBUG_COUNTER_GPU_UNIFORM_UPLOAD, sizeof(*value) * 2);
}

FORCE_INLINE
void gpuapi_uniform_buffer_update_v3(uint32 location, const f32* value) NO_EXCEPT
{
    glUniform3fv(location, 1, value);
    STATS_INCREMENT_BY(DEBUG_COUNTER_GPU_UNIFORM_UPLOAD, sizeof(*value) * 3);
}

FORCE_INLINE
void gpuapi_uniform_buffer_update(uint32 location, v3_f32 value) NO_EXCEPT
{
    glUniform3fv(location, 1, value.vec);
    STATS_INCREMENT_BY(DEBUG_COUNTER_GPU_UNIFORM_UPLOAD, sizeof(f32) * 3);
}

FORCE_INLINE
void gpuapi_uniform_buffer_update_v4(uint32 location, const f32* value) NO_EXCEPT
{
    glUniform4fv(location, 1, value);
    STATS_INCREMENT_BY(DEBUG_COUNTER_GPU_UNIFORM_UPLOAD, sizeof(*value) * 4);
}

FORCE_INLINE
void gpuapi_uniform_buffer_update(uint32 location, v4_f32 value) NO_EXCEPT
{
    glUniform4fv(location, 1, value.vec);
    STATS_INCREMENT_BY(DEBUG_COUNTER_GPU_UNIFORM_UPLOAD, sizeof(f32) * 4);
}

FORCE_INLINE
void gpuapi_uniform_buffer_update_m2(uint32 location, const f32* value) NO_EXCEPT
{
    glUniformMatrix2fv(location, 1, GL_FALSE, value);
    STATS_INCREMENT_BY(DEBUG_COUNTER_GPU_UNIFORM_UPLOAD, sizeof(*value) * 4);
}

FORCE_INLINE
void gpuapi_uniform_buffer_update_m3(uint32 location, const f32* value) NO_EXCEPT
{
    glUniformMatrix3fv(location, 1, GL_FALSE, value);
    STATS_INCREMENT_BY(DEBUG_COUNTER_GPU_UNIFORM_UPLOAD, sizeof(*value) * 9);
}

FORCE_INLINE
void gpuapi_uniform_buffer_update_m4(uint32 location, const f32* value) NO_EXCEPT
{
    glUniformMatrix4fv(location, 1, GL_FALSE, value);
    STATS_INCREMENT_BY(DEBUG_COUNTER_GPU_UNIFORM_UPLOAD, sizeof(*value) * 16);
}

FORCE_INLINE
uint32 opengl_get_attrib_location(uint32 id, const char* name) NO_EXCEPT
{
    // By using this you can retrieve the shader variable name at a point where and when you know it
    // BUT set values later on in generalized functions without knowing the shader variable name
    // Basically like pointers
    return glGetAttribLocation(id, name);
}

FORCE_INLINE
void opengl_check_link_errors(uint32 id, char* log) NO_EXCEPT
{
    GLint success;
    glGetProgramiv(id, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(id, 1024, NULL, log);
    }
}

FORCE_INLINE
void opengl_check_compile_errors(uint32 id, char* log) NO_EXCEPT
{
    GLint success;
    glGetShaderiv(id, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(id, 1024, NULL, log);
    }
}

// Only useful when creating shader files, not for optimizing shaders at runtime
// This only optimizes the file size of the shader on the hard drive
int32 opengl_program_optimize(const char* __restrict input, char* __restrict output) NO_EXCEPT
{
    const char* read_ptr = input;
    char* write_ptr = output;
    bool in_string = false;

    while (*read_ptr) {
        str_skip_empty(&read_ptr);

        if (write_ptr != output
            && *(write_ptr - 1) != '\n' && *(write_ptr - 1) != ';' && *(write_ptr - 1) != '{'
            && *(write_ptr - 1) != '('
            && *(write_ptr - 1) != ','
        ) {
            *write_ptr++ = '\n';
        }

        // Handle single-line comments (//)
        if (*read_ptr == '/' && *(read_ptr + 1) == '/' && !in_string) {
            str_move_to(&read_ptr, '\n');

            continue;
        }

        // Handle multi-line comments (/* */)
        if (*read_ptr == '/' && *(read_ptr + 1) == '*' && !in_string) {
            // Go to end of comment
            while (*read_ptr && (*read_ptr != '*' || *(read_ptr + 1) != '/')) {
                ++read_ptr;
            }

            if (*read_ptr == '*' && *(read_ptr + 1) == '/') {
                read_ptr += 2;
            }

            continue;
        }

        // Handle strings to avoid removing content within them
        if (*read_ptr == '"') {
            in_string = !in_string;
        }

        // Copy valid characters to write_ptr
        while (*read_ptr && !is_eol(read_ptr) && *read_ptr != '"'
            && !(*read_ptr == '/' && (*(read_ptr + 1) == '/' || *(read_ptr + 1) == '*'))
        ) {
            if (!in_string
                && (*read_ptr == '*' || *read_ptr == '/' || *read_ptr == '=' || *read_ptr == '+' || *read_ptr == '-' || *read_ptr == '%'
                    || *read_ptr == '(' || *read_ptr == ')'
                    || *read_ptr == '{' || *read_ptr == '}'
                    || *read_ptr == ',' || *read_ptr == '?' || *read_ptr == ':' || *read_ptr == ';'
                    || *read_ptr == '&' || *read_ptr == '|'
                    || *read_ptr == '>' || *read_ptr == '<'
                )
            ) {
                if (is_whitespace(*(write_ptr - 1)) || *(write_ptr - 1) == '\n') {
                    --write_ptr;
                }

                *write_ptr++ = *read_ptr++;

                if (*read_ptr && is_whitespace(*read_ptr)) {
                    ++read_ptr;
                }
            } else {
                *write_ptr++ = *read_ptr++;
            }
        }
    }

    *write_ptr = '\0';

    // -1 to remove \0 from length, same as strlen
    return (int32) (write_ptr - output);
}

GLuint gpuapi_shader_make(GLenum type, const char* source) NO_EXCEPT
{
    LOG_1("Create shader");
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, (GLchar **) &source, NULL);
    glCompileShader(shader);

    GLint status;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);

    #if DEBUG || INTERNAL
        if (status == GL_FALSE) {
            GLint length;
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);

            GLchar info[4096];
            length = OMS_MIN(length, (GLint) (ARRAY_COUNT(info) - 1));
            info[length] = '\0';
            glGetShaderInfoLog(shader, length, NULL, info);
            LOG_1(info);

            ASSERT_TRUE(false);
        }
    #endif

    LOG_1("Created shader");

    return shader;
}

FORCE_INLINE
int32 opengl_program_get_size(uint32 program) NO_EXCEPT
{
    int32 size;
    glGetProgramiv(program, GL_PROGRAM_BINARY_LENGTH, &size);

    return size;
}

// @todo Instead of passing the shaders one by one, pass one array called ShaderStage* shader_stages
// This way we can handle this more dynamic
GLuint gpuapi_pipeline_make(
    GLuint vertex_shader,
    GLuint fragment_shader,
    GLint geometry_shader
) NO_EXCEPT
{
    PROFILE(PROFILE_PIPELINE_MAKE, NULL, PROFILE_FLAG_SHOULD_LOG);
    LOG_1("Create pipeline");
    GLuint program = glCreateProgram();

    if (geometry_shader > -1) {
        glAttachShader(program, geometry_shader);
    }

    glAttachShader(program, vertex_shader);
    glAttachShader(program, fragment_shader);
    glProgramParameteri(program, GL_PROGRAM_BINARY_RETRIEVABLE_HINT, GL_TRUE);
    glLinkProgram(program);

    GLint status;
    glGetProgramiv(program, GL_LINK_STATUS, &status);

    #if DEBUG || INTERNAL
        if (status == GL_FALSE) {
            GLint length;
            glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);

            GLchar info[4096];
            length = OMS_MIN(length, (GLint) (ARRAY_COUNT(info) - 1));
            info[length] = '\0';
            glGetProgramInfoLog(program, length, NULL, info);
            LOG_1(info);

            ASSERT_TRUE(false);
        }
    #endif

    // @question really?
    if (geometry_shader > -1) {
        glDetachShader(program, geometry_shader);
    }

    glDetachShader(program, vertex_shader);
    glDetachShader(program, fragment_shader);

    // @question really?
    if (geometry_shader > -1) {
        glDeleteShader(geometry_shader);
    }

    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);

    LOG_1("Created pipeline");

    return program;
}

FORCE_INLINE
void gpuapi_pipeline_use(uint32 id) NO_EXCEPT
{
    glUseProgram(id);
}

inline
void gpuapi_attribute_setup(GpuAttributeType type, const OpenglVertexInputAttributeDescription* attr) NO_EXCEPT
{
    int32 length = gpuapi_attribute_count(type);
    for (int32 i = 0; i < length; ++i) {
        if (attr[i].format == GL_INT) {
            glVertexAttribIPointer(attr[i].location, attr[i].count, attr[i].format, attr[i].stride, attr[i].offset);
        } else {
            glVertexAttribPointer(attr[i].location, attr[i].count, attr[i].format, false, attr[i].stride, attr[i].offset);
        }
        glEnableVertexAttribArray(attr[i].location);
    }
}

CONSTEXPR
void gpuapi_attribute_info_create(GpuAttributeType type, OpenglVertexInputAttributeDescription* attr) NO_EXCEPT
{
    switch (type) {
        case GPU_ATTRIBUTE_TYPE_VERTEX_3D: {
            attr[0] = {
                0, // .location =
                3, // .count =
                GL_FLOAT, // .format =
                sizeof(Vertex3D), // .stride =
                (void *) offsetof(Vertex3D, position) // .offset =
            };

            attr[1] = {
                1, // .location =
                3, // .count =
                GL_FLOAT, // .format =
                sizeof(Vertex3D), // .stride =
                (void *) offsetof(Vertex3D, normal) // .offset =
            };

            attr[2] = {
                2, // .location =
                2, // .count =
                GL_FLOAT, // .format =
                sizeof(Vertex3D), // .stride =
                (void *) offsetof(Vertex3D, tex_coord) // .offset =
            };

            attr[3] = {
                3, // .location =
                4, // .count =
                GL_FLOAT, // .format =
                sizeof(Vertex3D), // .stride =
                (void *) offsetof(Vertex3D, color) // .offset =
            };
        } return;
        case GPU_ATTRIBUTE_TYPE_VERTEX_3D_NORMAL: {
            attr[0] = {
                0, // .location =
                3, // .count =
                GL_FLOAT, // .format =
                sizeof(Vertex3DNormal), // .stride =
                (void *) offsetof(Vertex3DNormal, position) // .offset =
            };

            attr[1] = {
                1, // .location =
                3, // .count =
                GL_FLOAT, // .format =
                sizeof(Vertex3DNormal), // .stride =
                (void *) offsetof(Vertex3DNormal, normal) // .offset =
            };
        } return;
        case GPU_ATTRIBUTE_TYPE_VERTEX_3D_COLOR: {
            attr[0] = {
                0, // .location =
                3, // .count =
                GL_FLOAT, // .format =
                sizeof(Vertex3DColor), // .stride =
                (void *) offsetof(Vertex3DColor, position) // .offset =
            };

            attr[1] = {
                1, // .location =
                4, // .count =
                GL_FLOAT, // .format =
                sizeof(Vertex3DColor), // .stride =
                (void *) offsetof(Vertex3DColor, color) // .offset =
            };
        } return;
        case GPU_ATTRIBUTE_TYPE_VERTEX_3D_TEXTURE_COLOR: {
            attr[0] = {
                0, // .location =
                3, // .count =
                GL_FLOAT, // .format =
                sizeof(Vertex3DTextureColor), // .stride =
                (void *) offsetof(Vertex3DTextureColor, position) // .offset =
            };

            attr[1] = {
                1, // .location =
                2, // .count =
                GL_FLOAT, // .format =
                sizeof(Vertex3DTextureColor), // .stride =
                (void *) offsetof(Vertex3DTextureColor, texture_color) // .offset =
            };
        } return;
        case GPU_ATTRIBUTE_TYPE_VERTEX_3D_SAMPLER_TEXTURE_COLOR: {
            attr[0] = {
                0, // .location =
                3, // .count =
                GL_FLOAT, // .format =
                sizeof(Vertex3DSamplerTextureColor), // .stride =
                (void *) offsetof(Vertex3DSamplerTextureColor, position) // .offset =
            };

            attr[1] = {
                1, // .location =
                1, // .count =
                GL_INT, // .format =
                sizeof(Vertex3DSamplerTextureColor), // .stride =
                (void *) offsetof(Vertex3DSamplerTextureColor, sampler) // .offset =
            };

            attr[2] = {
                2, // .location =
                2, // .count =
                GL_FLOAT, // .format =
                sizeof(Vertex3DSamplerTextureColor), // .stride =
                (void *) offsetof(Vertex3DSamplerTextureColor, texture_color) // .offset =
            };
        } return;
        case GPU_ATTRIBUTE_TYPE_VERTEX_2D_TEXTURE: {
            attr[0] = {
                0, // .location =
                2, // .count =
                GL_FLOAT, // .format =
                sizeof(Vertex2DTexture), // .stride =
                (void *) offsetof(Vertex2DTexture, position) // .offset =
            };

            attr[1] = {
                1, // .location =
                2, // .count =
                GL_FLOAT, // .format =
                sizeof(Vertex2DTexture), // .stride =
                (void *) offsetof(Vertex2DTexture, tex_coord) // .offset =
            };
        } return;
        default:
            UNREACHABLE();
    };
}

FORCE_INLINE
void gpuapi_descriptor_set_layout_create(Shader* const __restrict shader, const OpenglDescriptorSetLayoutBinding* __restrict bindings, int32 binding_length) NO_EXCEPT
{
    for (int32 i = 0; i < binding_length; ++i) {
        shader->descriptor_set_layout[i].binding = glGetUniformLocation(shader->id, bindings[i].name);
        shader->descriptor_set_layout[i].name = bindings[i].name;
    }
}

#endif