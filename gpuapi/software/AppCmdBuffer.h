/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_GPUAPI_SOFTWARE_APP_CMD_BUFFER_H
#define COMS_GPUAPI_SOFTWARE_APP_CMD_BUFFER_H

#include "../../stdlib/Stdlib.h"
#include "../../log/PerformanceProfiler.h"
#include "Shader.h"
#include "SoftwareRenderer.h"
#include "../ShaderType.h"
#include "../../asset/Asset.h"
#include "../../command/AppCmdBuffer.h"

void* cmd_shader_load(AppCmdBuffer*, Command*) {
    return NULL;
}

void* cmd_shader_load_sync(AppCmdBuffer*, Shader* shader, const SoftShaderFunc* shader_func) {
    // @question Currently the shader id has no purpose in software rendering
    // MAybe we want to bind all "loaded" shaders to the SoftwareRenderer then you could use them via ids like in opengl
    // Currently we use the shader simply by setting the currently active pointer to the shader
    // Which of course is also nice because we cut out all the other bullshit like implementing a shader hash map for all the ids
    shader->id = 1;

    int32 shader_count = 0;
    while (*shader_func) {
        shader->shader_functions[shader_count] = *shader_func;

        ++shader_func;
        ++shader_count;
    }

    shader->shader_count = shader_count;

    return NULL;
}

#endif