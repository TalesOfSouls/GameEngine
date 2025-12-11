/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_GPUAPI_SOFTWARE_UTILS_H
#define COMS_GPUAPI_SOFTWARE_UTILS_H

#include "../../stdlib/Types.h"
#include "../../memory/ChunkMemory.h"
#include "PersistentGpuBuffer.h"
#include "SoftwareRenderer.h"

// Since we don't upload textures in software rendering this is the same as texture_use
FORCE_INLINE
void gpuapi_prepare_texture(SoftwareRenderer* renderer, const Texture* texture) NO_EXCEPT
{
    renderer->textures[texture->sample_id] = texture;
}

FORCE_INLINE
void gpuapi_texture_use(SoftwareRenderer* renderer, const Texture* texture) NO_EXCEPT
{
    renderer->textures[texture->sample_id] = texture;
}

// @question Why is this here instead of in the shaderutils? same goes for opengl
FORCE_INLINE
void gpuapi_buffer_persistent_generate(SoftwareRenderer* renderer, const char* name, PersistentGpuBuffer* buffer) NO_EXCEPT
{
    buffer->bo = chunk_reserve(&renderer->buf, ceil_div(buffer->size, renderer->buf.chunk_size)) + 1;
    buffer->data = chunk_get_element(&renderer->buf, buffer->bo - 1);

    for (int32 i = 0; i < ARRAY_COUNT(renderer->descriptor_set_layout); ++i) {
        if (renderer->descriptor_set_layout[i].binding) {
            continue;
        }

        renderer->descriptor_set_layout[i].binding = buffer->bo;
        renderer->descriptor_set_layout[i].name = name;
        renderer->descriptor_set_layout[i].size = ceil_div(buffer->size, renderer->buf.chunk_size);
        renderer->descriptor_set_layout[i].data = buffer->data;

        break;
    }

    // @bug This is not a vertex upload it's just a generic data upload
    STATS_INCREMENT_BY(DEBUG_COUNTER_GPU_VERTEX_UPLOAD, buffer->size);
}

inline
int32 gpuapi_buffer_generate(SoftwareRenderer* renderer, void* data, int32 size) NO_EXCEPT
{
    int32 id = chunk_reserve(&renderer->buf, ceil_div(size, renderer->buf.chunk_size)) + 1;
    byte* mem = chunk_get_element(&renderer->buf, id - 1);

    if (data) {
        memcpy(mem, data, size);
    }

    STATS_INCREMENT_BY(DEBUG_COUNTER_GPU_VERTEX_UPLOAD, size);

    return id;
}

#endif