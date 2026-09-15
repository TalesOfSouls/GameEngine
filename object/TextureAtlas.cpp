/**
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_MODELS_TEXTUREATLAS_C
#define COMS_MODELS_TEXTUREATLAS_C

#include "../stdlib/Stdlib.h"
#include "../utils/StringUtils.h"
#include "../image/Image.cpp"
#include "TextureAtlas.h"
#include "../memory/RingMemory.cpp"

void atlas_from_file_txt(
    TextureAtlas* const atlas,
    const char* path,
    RingMemory* const ring
) NO_EXCEPT
{
    FileBody file = {0};
    file_read(path, &file, ring);
    ASSERT_TRUE(file.size);

    const char* pos = (char *) file.content;

    int32 image_width = 0;
    int32 image_height = 0;

    atlas->element_count = 0;
    atlas->uv_count = 0;
    char* texture_pos = atlas->texture_name;

    int header_completed = 0;

    // Font header
    while (*pos != '\0') {
        // Parsing general data
        pos = str_skip_eol(pos);

        const char* block_name = pos;
        str_move_to(&pos, " :\r\n#");

        if (*pos != ':') {
            break;
        }

        // Go to value
        while (*pos == ' ' || *pos == '\t' || *pos == ':') {
            ++pos;
        }

        if (strncmp(block_name, "texture", sizeof("texture") - 1) == 0) {
            while (!is_eol(pos)) {
                *texture_pos++ = *pos++;
            }

            *texture_pos++ = '\0';
            ++header_completed;
        } else if (strncmp(block_name, "image_width", sizeof("image_width") - 1) == 0) {
            image_width = (int32) str_to_int(pos, &pos);
            ++header_completed;
        } else if (strncmp(block_name, "image_height", sizeof("image_height") - 1) == 0) {
            image_height = (int32) str_to_int(pos, &pos);
            ++header_completed;
        }

        if (header_completed >= 3) {
            break;
        }

        pos = str_skip_line(pos);
    }

    atlas->elements = (TextureAtlasElement*) memory_get(
        ring,
        10 * MEGABYTE,
        alignof(TextureAtlasElement)
    );

    // This is a global index since all uv are stored in a global array
    atlas->uv = (v2_f32*) memory_get(
        ring,
        1 * MEGABYTE,
        alignof(v2_f32)
    );

    // Parsing the actual data
    while (*pos != '\0') {
        // Go to next element
        while (*pos != '#' && *pos != '\0') {
            ++pos;
        }

        if (*pos != '#') {
            break;
        }

        atlas->elements[atlas->element_count].uv_count = 0;
        atlas->elements[atlas->element_count].uv_start = atlas->uv_count;

        str_move_to(&pos, '\n');
        ++pos;

        if (*pos == '\0') {
            break;
        }

        // Iterate through all of the coordinates
        while (*pos != '\0' && *pos != '\n' && *pos != '#' && isdigit(*pos)) {
            atlas->uv[atlas->uv_count++] = {
                str_to_float(pos, &pos) / image_width,
                str_to_float(++pos, &pos) / image_height
            };

            ++atlas->elements[atlas->element_count].uv_count;

            pos = str_skip_line(pos);
        }

        ++atlas->element_count;
    }
}

FORCE_INLINE
int32 atlas_data_size(const TextureAtlas* const atlas) NO_EXCEPT
{
    return (int32) (sizeof(atlas->element_count)
        + sizeof(atlas->uv_count)
        + sizeof(TextureAtlasElement) * atlas->element_count
        + sizeof(v2_f32) * atlas->uv_count
    );
}

/**
 * File structure
 *
 *      TextureAtlas (excl. pointers)
 *      TextureAtlasElement[]
 *          v2_f32[]
 */
// atlas->elements is often assigned a memory size equals to the binary file size
// this wastes some bytes due to header data but this way we can avoid pre-parsing the data to find the exact required data
// atlas->uv is then assigned in this function once we know how much space we need for elements
inline
int32 atlas_from_data(
    const byte* data,
    TextureAtlas* const atlas,
    MAYBE_UNUSED int32 steps = 8
) NO_EXCEPT
{
    // @question do we want to store and load the texture name? We do this for fonts

    data = read_le(data, &atlas->element_count);
    data = read_le(data, &atlas->uv_count);

    ASSERT_TRUE(atlas->element_count > 0);
    ASSERT_TRUE(atlas->uv_count > 0);

    memcpy(atlas->elements, data, sizeof(TextureAtlasElement) * atlas->element_count);
    data += sizeof(TextureAtlasElement) * atlas->element_count;

    SWAP_ENDIAN_LITTLE_SIMD(
        (int32 *) atlas->elements,
        (int32 *) atlas->elements,
        (atlas->element_count * sizeof(TextureAtlasElement)) / 4, // everything in here is 4 bytes -> easy to swap
        steps
    );
    PSEUDO_USE(steps);

    atlas->uv = (v2_f32 *) ALIGN_UP(
        (uintptr_t) atlas->elements + sizeof(TextureAtlasElement) * atlas->element_count,
        64 // 64 bytes so we can use AVX512 on uv data
    );
    memcpy(atlas->uv, data, sizeof(TextureAtlasElement) * atlas->uv_count);

    SWAP_ENDIAN_LITTLE_SIMD(
        (int32 *) atlas->uv,
        (int32 *) atlas->uv,
        (atlas->uv_count * sizeof(v2_f32)) / 4, // everything in here is 4 bytes -> easy to swap
        steps
    );
    PSEUDO_USE(steps);

    return atlas_data_size(atlas);
}

int32 atlas_to_data(
    const TextureAtlas* const atlas,
    byte* data,
    MAYBE_UNUSED int32 steps = 8
) NO_EXCEPT
{
    const byte* start = data;

    data = write_le(data, atlas->element_count);
    data = write_le(data, atlas->uv_count);

    memcpy(data, atlas->elements, sizeof(TextureAtlasElement) * atlas->element_count);
    SWAP_ENDIAN_LITTLE_SIMD(
        (int32 *) data,
        (int32 *) data,
        (sizeof(TextureAtlasElement) * atlas->element_count) / 4, // everything in here is 4 bytes -> easy to swap
        steps
    );
    PSEUDO_USE(steps);
    data += sizeof(TextureAtlasElement) * atlas->element_count;

    // @bug we are storing floats into the data and to the file system
    //      depending on the compiler floats are not consistent across platforms
    memcpy(data, atlas->uv, sizeof(v2_f32) * atlas->uv_count);
    SWAP_ENDIAN_LITTLE_SIMD(
        (int32 *) data,
        (int32 *) data,
        (sizeof(v2_f32) * atlas->uv_count) / 4, // everything in here is 4 bytes -> easy to swap
        steps
    );
    PSEUDO_USE(steps);
    data += sizeof(v2_f32) * atlas->uv_count;

    return (int32) ((uintptr_t) data - (uintptr_t) start);
}

// Required depending on the 3D api.
// Some use top-down, some bottom-up coordinates
FORCE_INLINE
void atlas_invert_coordinates(TextureAtlas* const atlas, int32 steps = 8) NO_EXCEPT
{
    f32* const data = (f32*) atlas->uv;
    const int32 float_count = atlas->uv_count * 2;

    int32 i = 0;
    #if defined(__ARM_FEATURE_SVE)
        if (steps >= 4) {
            const svbool_t all = svptrue_b32();
            const svuint32_t idx   = svindex_u32(0, 1);
            const svbool_t is_y    = svcmpeq_n_u32(all, svand_n_u32_x(all, idx, 1), 1);
            const svfloat32_t mul_p = svsel_f32(is_y, svdup_n_f32(-1.0f), svdup_n_f32(1.0f));
            const svfloat32_t add_p = svsel_f32(is_y, svdup_n_f32(1.0f),  svdup_n_f32(0.0f));

            for (; i < float_count; i += steps) {
                svbool_t pg = svwhilelt_b32(i, float_count);
                svfloat32_t v = svld1_f32(pg, data + i);
                svfloat32_t r = svmad_f32_x(pg, v, mul_p, add_p); // v*mul + add
                svst1_f32(pg, data + i, r);
            }
        }
    #elif defined(__ARM_NEON)
        if (steps >= 4) {
            const float32x4_t mul4 = {1.0f, -1.0f, 1.0f, -1.0f};
            const float32x4_t add4 = {0.0f,  1.0f, 0.0f,  1.0f};
            for (; i + 4 <= float_count; i += 4) {
                float32x4_t v = vld1q_f32(data + i);
                float32x4_t r = vfmaq_f32(add4, v, mul4); // add4 + v*mul4
                vst1q_f32(data + i, r);
            }
        }
    #else
        #if defined(__AVX512F__)
            if (steps >= 16) {
                const __m512 mul16 = _mm512_set_ps(-1,1,-1,1,-1,1,-1,1,-1,1,-1,1,-1,1,-1,1);
                const __m512 add16 = _mm512_set_ps( 1,0, 1,0, 1,0, 1,0, 1,0, 1,0, 1,0, 1,0);
                for (; i + 16 <= float_count; i += 16) {
                    __m512 v = _mm512_load_ps(data + i);
                    __m512 r = _mm512_fmadd_ps(v, mul16, add16); // AVX-512F implies FMA
                    _mm512_store_ps(data + i, r);
                }

                steps = 4;
            }
        #endif

        #if defined(__AVX2__)
            if (steps >= 8) {
                const __m256 mul8 = _mm256_set_ps(-1,1,-1,1,-1,1,-1,1);
                const __m256 add8 = _mm256_set_ps( 1,0, 1,0, 1,0, 1,0);
                for (; i + 8 <= float_count; i += 8) {
                    __m256 v = _mm256_load_ps(data + i);

                    #if defined(__FMA__)
                        __m256 r = _mm256_fmadd_ps(v, mul8, add8);
                    #else
                        __m256 r = _mm256_add_ps(_mm256_mul_ps(v, mul8), add8);
                    #endif

                    _mm256_store_ps(data + i, r);
                }
            }
        #endif

        #if defined(__SSE4_2__)
            if (steps >= 4) {
                const __m128 mul4 = _mm_set_ps(-1,1,-1,1);
                const __m128 add4 = _mm_set_ps( 1,0, 1,0);
                for (; i + 4 <= float_count; i += 4) {
                    __m128 v = _mm_load_ps(data + i);

                    #if defined(__FMA__)
                        __m128 r = _mm_fmadd_ps(v, mul4, add4);
                    #else
                        __m128 r = _mm_add_ps(_mm_mul_ps(v, mul4), add4);
                    #endif

                    _mm_store_ps(data + i, r);
                }
            }
        #endif
    #endif

    for (; i < float_count; i += 2) {
        data[i + 1] = 1.0f - data[i + 1];
    }
}

#endif