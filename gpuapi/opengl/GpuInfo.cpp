/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_GPUAPI_OPENGL_GPU_INFO_C
#define COMS_GPUAPI_OPENGL_GPU_INFO_C

#include "../../stdlib/Stdlib.h"
#include "../../utils/StringUtils.h"
#include "../GPUInfo.h"

uint64 gpu_info_features() {
    uint64 features = 0;

    if (gl_has_extension("GL_ARB_bindless_texture")) {
        features |= GPU_FEATURE_BINDLESS_TEXTURE;
    }

    if (gl_has_extension("GL_ARB_sparse_texture")) {
        features |= GPU_FEATURE_SPARSE_TEXTURE;
    }

    if (gl_has_extension("GL_ARB_buffer_storage")) {
        features |= GPU_FEATURE_PERSISTENT_BUFFERS;
    }

    if (gl_has_extension("GL_ARB_shader_storage_buffer_object")) {
        features |= GPU_FEATURE_SHADER_STORAGE_BUFFER_OBJECTS;
    }

    if (gl_has_extension("GL_EXT_texture_compression_s3tc")
        || gl_has_extension("GL_EXT_texture_compression_dxt1")
    ) {
        features |= GPU_FEATURE_COMPRESSION_BC;
    }

    if (gl_has_extension("GL_KHR_texture_compression_astc_ldr")) {
        features |= GPU_FEATURE_COMPRESSION_ASTC;
    }

    if (gl_has_extension("GL_OES_compressed_ETC1_RGB8_texture")) {
        features |= GPU_FEATURE_COMPRESSION_ETC;
    }

    if (gl_has_extension("GL_EXT_texture_filter_anisotropic")) {
        features |= GPU_FEATURE_ANISOTROPIC_FILTERING;
    }

    if (gl_has_extension("GL_ARB_texture_gather")) {
        features |= GPU_FEATURE_TEXTURE_GATHER;
    }

    if (gl_has_extension("GL_EXT_texture_array")
        || gl_has_extension("GL_ARB_texture_cube_map_array")
    ) {
        features |= GPU_FEATURE_TEXTURE_ARRAYS;
    }

    if (gl_has_extension("GL_ARB_draw_indirect")) {
        features |= GPU_FEATURE_INDIRECT_DRAW;
    }

    if (gl_has_extension("GL_ARB_occlusion_query")
        || gl_has_extension("GL_ARB_occlusion_query2")
    ) {
        features |= GPU_FEATURE_OCCLUSION_RENDERING;
    }

    if (gl_has_extension("GL_EXT_transform_feedback")) {
        features |= GPU_FEATURE_TRANSFORM_FEEDBACK;
    }

    if (gl_has_extension("GL_ARB_gpu_shader_fp64")) {
        features |= GPU_FEATURE_FP64;
    }

    if (gl_has_extension("GL_AMD_gpu_shader_half_float")
        || gl_has_extension("GL_NV_gpu_shader5")
    ) {
        features |= GPU_FEATURE_FP16;
    }

    if (gl_has_extension("GL_ARB_shader_bit_encoding")) {
        features |= GPU_FEATURE_SHADER_BITFIELD;
    }

    if (gl_has_extension("GL_ARB_gpu_shader_int64")) {
        features |= GPU_FEATURE_SHADER_INT64;
    }

    if (gl_has_extension("GL_EXT_shader_integer_mix")) {
        features |= GPU_FEATURE_SHADER_INT16;
    }

    return features;
}

void gpuapi_info_get(GpuInfo* info) {
    const char* vendor  = (const char*) glGetString(GL_VENDOR);
    const char* renderer = (const char*) glGetString(GL_RENDERER);
    const char* version = (const char*) glGetString(GL_VERSION);
    const char* shader_version = (const char*) glGetString(GL_SHADING_LANGUAGE_VERSION);

    if (vendor) {
        str_copy(info->vendor, vendor, sizeof(info->vendor) - 1);
    }

    if (renderer) {
        str_copy(info->name, renderer, sizeof(info->name) - 1);
    }

    if (version) {
        str_copy(info->api_version, version, sizeof(info->api_version) - 1);
    }

    if (shader_version) {
        str_copy(info->shader_version, shader_version, sizeof(info->shader_version) - 1);
    }

    GLint vramKB = 0;
    glGetIntegerv(0x9048 /*GL_GPU_MEMORY_INFO_TOTAL_AVAILABLE_MEMORY_NVX*/, &vramKB);
    if (vramKB == 0) {
        glGetIntegerv(0x9167 /*GL_TEXTURE_FREE_MEMORY_ATI*/, &vramKB);
    }

    info->vram = (uint32_t) (vramKB / 1024);

    GLint align = 0;
    glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &align);
    info->alignment = (uint32_t) align;

    info->features = gpu_info_features();
}

#endif