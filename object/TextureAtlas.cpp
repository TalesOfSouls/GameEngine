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

    // Pre-parsing counts to figure out how many elements we have
    atlas->element_count = 0;
    const char* body_pos = pos;
    while (*body_pos != '\0') {
        if (*body_pos++ == '#') {
            ++atlas->element_count;
        }
    }

    if (!atlas->element_count) {
        ASSERT_THROW();
        return;
    }

    int32 element_idx = 0;
    TextureAtlasElement* element_memory = (TextureAtlasElement*) ring_memory_get(
        ring,
        sizeof(TextureAtlasElement) * atlas->element_count,
        alignof(TextureAtlasElement)
    );
    atlas->elements = element_memory;

    // This is a global index since all uv are stored in a global array
    int32 uv_coord_idx = 0;
    v2_f32* uv_coord = (v2_f32*) ring_memory_get(
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

        ASSERT_TRUE((uintptr_t) &uv_coord[uv_coord_idx] - (uintptr_t) uv_coord < 1 * MEGABYTE);

        atlas->elements[element_idx].uv_count = 0;
        atlas->elements[element_idx].uv = &uv_coord[uv_coord_idx];

        str_move_to(&pos, '\n');
        ++pos;

        if (*pos == '\0') {
            break;
        }

        // Iterate through all of the coordinates
        while (*pos != '\0' && *pos != '\n' && *pos != '#' && isdigit(*pos)) {
            atlas->elements[element_idx].uv[atlas->elements[element_idx].uv_count] = {
                str_to_float(pos, &pos) / image_width,
                str_to_float(++pos, &pos) / image_height
            };
            ++atlas->elements[element_idx].uv_count;
        }

        uv_coord_idx += atlas->elements[element_idx].uv_count;
        ++element_idx;
    }
}

FORCE_INLINE
int32 atlas_data_size(const TextureAtlas* const atlas) NO_EXCEPT
{
    size_t uv_size = 0;
    for (int32 i = 0; i < atlas->element_count; ++i) {
        uv_size += sizeof(v2_f32) * atlas->elements[i].uv_count;
    }

    return (int32) (atlas->element_count * sizeof(int32) // the data itself doesn't store a pointer
        + sizeof(atlas->element_count)
        + uv_size);
}

/**
 * File structure
 *
 *      TextureAtlas (excl. pointers)
 *      TextureAtlasElement[]
 *          v2_f32[]
 */
inline
int32 atlas_from_data(
    const byte* const data,
    TextureAtlas* const atlas,
    MAYBE_UNUSED int32 steps = 8
) NO_EXCEPT
{


    return 0;
}

int32 atlas_to_data(
    const TextureAtlas* const atlas,
    byte* const data,
    MAYBE_UNUSED int32 steps = 8
) NO_EXCEPT
{
    return 0;
}

// Required depending on the 3D api.
// Some use top-down, some bottom-up coordinates
FORCE_INLINE
void atlas_invert_coordinates(TextureAtlas* const atlas) NO_EXCEPT
{
    for (uint32 i = 0; i < atlas->element_count; ++i) {
        TextureAtlasElement* const element = &atlas->elements[i];
        for (int32 j = 0; j < element->uv_count; ++j) {
            element->uv[j].y = 1.0f - element->uv[j].y;
        }
    }
}

#endif