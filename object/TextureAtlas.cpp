/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_MODELS_TEXTUREATLAS_C
#define COMS_MODELS_TEXTUREATLAS_C

#include "../stdlib/Stdlib.h"
#include "../utils/StringUtils.h"
#include "../image/Image.cpp"
#include "TextureAtlas.h"

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

    char block_name[32];

    int32 image_width = 0;
    int32 image_height = 0;

    atlas->element_count = 0;
    atlas->uv_count = 0;
    char* texture_pos = atlas->texture_name;

    // Font header
    while (*pos != '\0') {
        // Parsing general data
        int32 i = 0;
        while (*pos == '\n') {
            ++pos;
        }

        while (*pos != '\0' && *pos != ' ' && *pos != ':' && *pos != '\n' && *pos != '#' && i < 31) {
            block_name[i] = *pos;
            ++pos;
            ++i;
        }

        block_name[i] = '\0';

        if (*pos != ':') {
            break;
        }

        // Go to value
        while (*pos == ' ' || *pos == '\t' || *pos == ':') {
            ++pos;
        }

        if (strcmp(block_name, "texture") == 0) {
            while (*pos != '\n') {
                *texture_pos++ = *pos++;
            }

            *texture_pos++ = '\0';
        } else if (strcmp(block_name, "image_width") == 0) {
            image_width = (int32) str_to_int(pos, &pos);
        } else if (strcmp(block_name, "image_height") == 0) {
            image_height = (int32) str_to_int(pos, &pos);

            // @bug it's a little bit of a bad design to force the order here
            //      this requires image_height to be the last element before the content starts
            break;
        }

        // Go to next line
        while (*pos != '\0' && *pos++ != '\n') {};
    }

    atlas->elements = (TextureAtlasElement*) ring_memory_get(
        ring,
        sizeof(TextureAtlasElement) * atlas->element_count,
        alignof(TextureAtlasElement)
    );

    // This is a global index since all uv are stored in a global array
    atlas->uv = (v2_f32*) ring_memory_get(
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
        atlas->elements[atlas->element_count].uv_start = atlas->element_count;

        str_move_to(&pos, '\n');
        ++pos;

        if (*pos == '\0') {
            break;
        }

        // Iterate through all of the coordinates
        while (*pos != '\0' && *pos != '\n' && *pos != '#' && isdigit(*pos)) {
            atlas->uv[
                atlas->elements[atlas->element_count].uv_start
                + atlas->elements[atlas->element_count].uv_count
            ] = {
                str_to_float(pos, &pos) / image_width,
                str_to_float(++pos, &pos) / image_height
            };

            ++atlas->elements[atlas->element_count].uv_count;
            ++atlas->uv_count;
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

    memcpy(atlas->elements, data, sizeof(TextureAtlasElement) * atlas->element_count);
    data += sizeof(TextureAtlasElement) * atlas->element_count;

    SWAP_ENDIAN_LITTLE_SIMD(
        (int32 *) atlas->elements,
        (int32 *) atlas->elements,
        (atlas->element_count * sizeof(TextureAtlasElement)) / 4, // everything in here is 4 bytes -> easy to swap
        steps
    );
    PSEUDO_USE(steps);

    atlas->uv = (v2_f32 *) align_up(
        (uintptr_t) atlas->elements + sizeof(TextureAtlasElement) * atlas->element_count,
        alignof(v2_f32)
    );

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
void atlas_invert_coordinates(TextureAtlas* const atlas) NO_EXCEPT
{
    for (int32 i = 0; i < atlas->uv_count; ++i) {
        atlas->uv[i].y = 1.0f - atlas->uv[i].y;
    }
}

#endif