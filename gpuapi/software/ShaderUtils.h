/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_GPUAPI_SOFTWARE_SHADER_UTILS_H
#define COMS_GPUAPI_SOFTWARE_SHADER_UTILS_H

#include "../../stdlib/Types.h"
#include "../../memory/RingMemory.h"
#include "../../memory/BufferMemory.h"
#include "Shader.h"
#include "../ShaderType.h"
#include "../GpuAttributeType.h"

// NOTE: We don't make it const because we might want to support "compute shaders", which store data in the shader
// Of course this is kinda stupid anyways in a software renderer but for the sake of somewhat similar behavior between gpu and cpu we keep it
FORCE_INLINE
void gpuapi_pipeline_use(SoftwareRenderer* renderer, Shader* shader) NO_EXCEPT
{
    renderer->active_shader = shader;
}

// This sets a global descriptor set layout
// This can be used from every shader
FORCE_INLINE
void gpuapi_descriptor_set_layout_create(
    SoftwareRenderer* const __restrict renderer,
    const SoftwareDescriptorSetLayoutBinding* __restrict layout
) NO_EXCEPT
{
    for (int32 i = 0; i < ARRAY_COUNT(renderer->descriptor_set_layout); ++i) {
        if (!renderer->descriptor_set_layout[i].binding) {
            renderer->descriptor_set_layout[i].name = layout->name;
            renderer->descriptor_set_layout[i].size = ceil_div(layout->size, renderer->buf.chunk_size);
            renderer->descriptor_set_layout[i].binding = chunk_reserve(&renderer->buf, renderer->descriptor_set_layout[i].size) + 1;
            renderer->descriptor_set_layout[i].data = chunk_get_element(&renderer->buf, renderer->descriptor_set_layout[i].binding - 1);

            // @todo allow .data to be a reference to existing memory

            return;
        }
    }
}

FORCE_INLINE
void gpuapi_descriptor_set_layout_set(
    SoftwareRenderer* const __restrict renderer,
    const SoftwareDescriptorSetLayoutBinding* __restrict layout
) NO_EXCEPT
{
    for (int32 i = 0; i < ARRAY_COUNT(renderer->descriptor_set_layout); ++i) {
        if (!renderer->descriptor_set_layout[i].binding) {
            renderer->descriptor_set_layout[i].name = layout->name;
            renderer->descriptor_set_layout[i].size = ceil_div(layout->size, renderer->buf.chunk_size);
            renderer->descriptor_set_layout[i].binding = layout->binding;
            renderer->descriptor_set_layout[i].data = layout->data;

            // @todo allow .data to be a reference to existing memory

            return;
        }
    }
}

FORCE_INLINE
void gpuapi_descriptor_set_layout_set(
    SoftwareRenderer* const __restrict renderer,
    int32 binding,
    void* data
) NO_EXCEPT
{
    for (int32 i = 0; i < ARRAY_COUNT(renderer->descriptor_set_layout); ++i) {
        if (renderer->descriptor_set_layout[i].binding == binding) {
            renderer->descriptor_set_layout[i].data = data;

            return;
        }
    }
}

// This sets a shader descriptor set layout
// Only the respective shader can use these descriptors
FORCE_INLINE
void gpuapi_descriptor_set_layout_create(
    SoftwareRenderer* const __restrict renderer,
    Shader* const __restrict shader,
    const SoftwareDescriptorSetLayoutBinding* __restrict layouts,
    int32 layout_length
) NO_EXCEPT
{
    for (int32 i = 0; i < layout_length; ++i) {
        shader->descriptor_set_layout[i].name = layouts[i].name;
        shader->descriptor_set_layout[i].size = ceil_div(layouts[i].size, renderer->buf.chunk_size);
        shader->descriptor_set_layout[i].binding = chunk_reserve(&renderer->buf, shader->descriptor_set_layout[i].size) + 1;
        shader->descriptor_set_layout[i].data = chunk_get_element(&renderer->buf, shader->descriptor_set_layout[i].binding - 1);

        // @todo allow .data to be a reference to existing memory
    }
}

FORCE_INLINE
void gpuapi_buffer_update(void* location, bool value) NO_EXCEPT
{
    *((bool *) location) = value;
    STATS_INCREMENT_BY(DEBUG_COUNTER_GPU_UNIFORM_UPLOAD, sizeof(value));
}

FORCE_INLINE
void gpuapi_buffer_update(void* location, int32 value) NO_EXCEPT
{
    *((int32 *) location) = value;
    STATS_INCREMENT_BY(DEBUG_COUNTER_GPU_UNIFORM_UPLOAD, sizeof(value));
}

FORCE_INLINE
void gpuapi_buffer_update(void* location, f32 value) NO_EXCEPT
{
    *((f32 *) location) = value;
    STATS_INCREMENT_BY(DEBUG_COUNTER_GPU_UNIFORM_UPLOAD, sizeof(value));
}

FORCE_INLINE
void gpuapi_buffer_update(void* location, const void* value, size_t size) NO_EXCEPT
{
    memcpy(location, value, size);
    STATS_INCREMENT_BY(DEBUG_COUNTER_GPU_UNIFORM_UPLOAD, size);
}

FORCE_INLINE
void gpuapi_buffer_update(void* location, v3_f32 value) NO_EXCEPT
{
    memcpy(location, value.vec, sizeof(f32) * 3);
    STATS_INCREMENT_BY(DEBUG_COUNTER_GPU_UNIFORM_UPLOAD, sizeof(f32) * 3);
}

FORCE_INLINE
void gpuapi_buffer_update(void* location, v4_f32 value) NO_EXCEPT
{
    memcpy(location, value.vec, sizeof(f32) * 4);
    STATS_INCREMENT_BY(DEBUG_COUNTER_GPU_UNIFORM_UPLOAD, sizeof(f32) * 4);
}

FORCE_INLINE
void gpuapi_ref_update(void* location, void* value) NO_EXCEPT
{
    location = value;
    STATS_INCREMENT_BY(DEBUG_COUNTER_GPU_UNIFORM_UPLOAD, sizeof(void*));
}

FORCE_INLINE
void gpuapi_buffer_update_m4(void* location, const f32* value) NO_EXCEPT
{
    memcpy(location, value, sizeof(f32) * 16);
    STATS_INCREMENT_BY(DEBUG_COUNTER_GPU_UNIFORM_UPLOAD, sizeof(*value) * 16);
}

#endif