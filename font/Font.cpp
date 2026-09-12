#pragma once
#ifndef COMS_FONT_C
#define COMS_FONT_C

#include "../stdlib/Stdlib.h"
#include "../memory/RingMemory.cpp"
#include "../utils/Utils.h"
#include "../utils/BitUtils.h"
#include "../system/FileUtils.cpp"
#include "Font.h"

FORCE_INLINE
void font_init(Font* const font, byte* data, int count) NO_EXCEPT
{
    font->glyphs = (Glyph *) data;
    font->glyph_count = (uint16) count;
}

static inline
const Glyph* font_glyph_find_binary(const Font* const font, uint32 codepoint) NO_EXCEPT
{
    const uint32 perfect_glyph_pos = codepoint - font->glyphs[0].codepoint;
    const uint32 limit = OMS_MIN(perfect_glyph_pos, (uint32) (font->glyph_count - 1));

    // We try to jump to the correct glyph based on the glyph codepoint
    if (font->glyphs[limit].codepoint == codepoint) {
        return &font->glyphs[limit];
    }

    // If that doesn't work we iterate the glyph list BUT only until the last possible match.
    // Glyphs must be sorted ascending.
    int32 low = 0;
    int32 high = limit;
    while (low <= high) {
        const int32 mid = low + (high - low) / 2;
        if (font->glyphs[mid].codepoint == codepoint) {
            return &font->glyphs[mid];
        } else if (font->glyphs[mid].codepoint < codepoint) {
            low = mid + 1;
        } else {
            high = mid - 1;
        }
    }

    return NULL;
}

static inline
const Glyph* font_glyph_find_eytzinger(const Font* const font, uint32 codepoint) NO_EXCEPT
{
    uint32 k = 0;
    const uint32 n = font->glyph_count;

    while (k < n) {
        const uint32 gc = font->glyphs[k].codepoint;
        if (gc == codepoint) {
            return &font->glyphs[k];
        }

        k = gc < codepoint ? 2 * k + 2 : 2 * k + 1;
    }

    return NULL;
}

FORCE_INLINE
const Glyph* font_glyph_find(const Font* const font, uint32 codepoint) NO_EXCEPT
{
    return font->order_type == FONT_GLYPH_ORDER_TYPE_NORMAL
        ? font_glyph_find_binary(font, codepoint)
        : font_glyph_find_eytzinger(font, codepoint);
}

static inline
int16 font_glyph_index_find_binary(const Font* const font, uint32 codepoint) NO_EXCEPT
{
    const int16 perfect_glyph_pos = (int16) (codepoint - font->glyphs[0].codepoint);
    const int16 limit = OMS_MIN(perfect_glyph_pos, (int16) (font->glyph_count - 1));

    // We try to jump to the correct glyph based on the glyph codepoint
    if (font->glyphs[limit].codepoint == codepoint) {
        return limit;
    }

    // If that doesn't work we iterate the glyph list BUT only until the last possible match.
    // Glyphs must be sorted ascending.
    int16 low = 0;
    int16 high = limit;
    while (low <= high) {
        const int16 mid = low + (high - low) / 2;
        if (font->glyphs[mid].codepoint == codepoint) {
            return mid;
        } else if (font->glyphs[mid].codepoint < codepoint) {
            low = mid + 1;
        } else {
            high = mid - 1;
        }
    }

    return -1;
}

static inline
int16 font_glyph_index_find_eytzinger(const Font* const font, uint32 codepoint) NO_EXCEPT
{
    int16 k = 0;
    const int16 n = (int16) font->glyph_count;

    while (k < n) {
        const uint32 gc = font->glyphs[k].codepoint;
        if (gc == codepoint) {
            return k;
        }

        k = gc < codepoint ? (int16) (2 * k + 2) : (int16) (2 * k + 1);
    }

    return -1;
}

FORCE_INLINE
int16 font_glyph_index_find(const Font* const font, uint32 codepoint) NO_EXCEPT
{
    if (font->glyph_count == 0) {
        return -1;
    }

    return font->order_type == FONT_GLYPH_ORDER_TYPE_NORMAL
        ? font_glyph_index_find_binary(font, codepoint)
        : font_glyph_index_find_eytzinger(font, codepoint);
}

static inline
uint32 font_glyph_eytzinger_build(Glyph* dst, const Glyph* src, uint32 n, uint32 i, uint32 k) NO_EXCEPT
{
    if (k < n) {
        i = font_glyph_eytzinger_build(dst, src, n, i, 2 * k + 1);
        dst[k] = src[i++];
        i = font_glyph_eytzinger_build(dst, src, n, i, 2 * k + 2);
    }

    return i;
}

inline
void font_glyphs_to_eytzinger(Glyph* glyphs, uint32 glyph_count, Glyph* tmp) NO_EXCEPT
{
    memcpy(tmp, glyphs, sizeof(Glyph) * glyph_count);
    font_glyph_eytzinger_build(glyphs, tmp, glyph_count, 0, 0);
}

// Optimizes the glyphs array order for faster access
inline
void font_optimize_order(Font* const font, byte* buf) NO_EXCEPT
{
    const char* to_validate = "AaZ? 09";

    int success = 0;
    uint32 codepoint;
    while (codepoint = *to_validate++) {
        const int16 perfect_glyph_pos = (int16) (codepoint - font->glyphs[0].codepoint);
        const int16 limit = OMS_MIN(perfect_glyph_pos, (int16) (font->glyph_count - 1));

        // We either need to perfectly match the codepoint = array index
        // Or the code point isn't in the array, then it doesn't count
        success += (int) (font->glyphs[limit].codepoint == codepoint
            || font_glyph_index_find(font, codepoint) < 0
        );
    }

    if (success >= (sizeof(to_validate) - 1) / 2) {
        font->order_type = FONT_GLYPH_ORDER_TYPE_NORMAL;
        return;
    }

    // The glyphs array is sparse/a bad match for direct array access
    // This means we often have to use binary search which means it is worth to optimize / change the order
    font_glyphs_to_eytzinger(font->glyphs, font->glyph_count, (Glyph *) buf);
    font->order_type = FONT_GLYPH_ORDER_TYPE_EYTZINGER;
}

/*
Example:
texture: ./img/ui_font_atlas.png
font_size: 32
line_height: 32
image_width: 512
image_height: 512
glyph_count: 95
32 1 1 6 0 6 1 1 //

per glyph:
1:codepoint 2:width 3:height 4:offset_x 5:offset_y 6:advance_x 7:uv_x 8:uv_y
*/

void font_from_file_txt(
    Font* const font,
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

    char* texture_pos = font->texture_name;

    int header_completed = 0;

    // Font header
    while (*pos != '\0') {
        // Parsing general data
        pos = str_skip_empty(pos);

        const char* block_name = pos;
        str_move_to(&pos, " :\r\n");

        if (*pos != ':') {
            break;
        }

        // Go to
        while (*pos == ' ' || *pos == '\t' || *pos == ':') {
            ++pos;
        }

        if (strncmp(block_name, "texture", sizeof("texture") - 1) == 0) {
            while (!is_eol(pos)) {
                *texture_pos++ = *pos++;
            }

            *texture_pos++ = '\0';
            ++header_completed;
        } else if (strncmp(block_name, "font_size", sizeof("font_size") - 1) == 0) {
            font->size = str_to_float(pos, &pos);
            ++header_completed;
        } else if (strncmp(block_name, "line_height", sizeof("line_height") - 1) == 0) {
            font->line_height = str_to_float(pos, &pos);
            ++header_completed;
        } else if (strncmp(block_name, "image_width", sizeof("image_width") - 1) == 0) {
            image_width = (int32) str_to_int(pos, &pos);
            ++header_completed;
        } else if (strncmp(block_name, "image_height", sizeof("image_height") - 1) == 0) {
            image_height = (int32) str_to_int(pos, &pos);
            ++header_completed;
        } else if (strncmp(block_name, "glyph_count", sizeof("glyph_count") - 1) == 0) {
            font->glyph_count = (uint16) str_to_int(pos, &pos);
            ++header_completed;
        }

        if (header_completed >= 6) {
            break;
        }

        // Go to next line
        pos = str_skip_line(pos);
    }

    int32 glyph_index = 0;
    ++pos;

    // Body
    while (*pos != '\0') {
        f32 glyph_width;
        f32 glyph_height;

        f32 uv_start_x;
        f32 uv_start_y;

        // Parsing glyphs
        // In the text file we don't have to define width and height of the character, we calculate that here
        font->glyphs[glyph_index] = {
            (uint32) str_to_int(pos, &pos), // codepoint
            { // metrics
                glyph_width = str_to_float(++pos, &pos), // width
                glyph_height = str_to_float(++pos, &pos), // height
                str_to_float(++pos, &pos), // offset_x
                str_to_float(++pos, &pos), // offset_y
                str_to_float(++pos, &pos) // advance_x
            },
            { // uv_start
                uv_start_x = str_to_float(++pos, &pos) / image_width, uv_start_y = str_to_float(++pos, &pos) / image_height
            },
            { // uv_end
                uv_start_x + glyph_width / image_width, uv_start_y + glyph_height / image_height
            },
        };

        ++glyph_index;

        pos = str_skip_line(pos);
    }
}

static FORCE_INLINE
int32 font_data_size(const Font* const font) NO_EXCEPT
{
    return font->glyph_count * sizeof(Glyph)
        + sizeof(font->glyph_count)
        + sizeof(font->texture_name)
        + sizeof(font->size)
        + sizeof(font->line_height);
}

// font->glyphs is often assigned a memory size equals to the binary file size
// this wastes some bytes due to header data but this way we can avoid pre-parsing the data to find the exact required data
inline
int32 font_from_data(
    byte* __restrict data,
    Font* const __restrict font,
    MAYBE_UNUSED int32 steps = 8
) NO_EXCEPT
{
    if (!data) {
        LOG_1("[WARNING] No font data provided to load");
        return 0;
    }

    LOG_3("[INFO] Load font");

    const byte* pos = data;

    // Read count
    pos = read_le(pos, &font->glyph_count);
    ASSERT_TRUE(font->glyph_count > 0 && font->glyph_count < 8192);

    // Read texture name
    memcpy(font->texture_name, pos, ARRAY_COUNT(font->texture_name) * sizeof(char));
    pos += ARRAY_COUNT(font->texture_name) * sizeof(char);

    // Read font size
    pos = read_le(pos, &font->size);

    // Read line height
    pos = read_le(pos, &font->line_height);

    memcpy(font->glyphs, pos, font->glyph_count * sizeof(Glyph));

    SWAP_ENDIAN_LITTLE_SIMD(
        (int32 *) font->glyphs,
        (int32 *) font->glyphs,
        (font->glyph_count * sizeof(Glyph)) / 4, // everything in here is 4 bytes -> easy to swap
        steps
    );
    PSEUDO_USE(steps);

    // We are really evil here. We re-purpose the input data buffer as a temp buffer.
    // We can do this since the original buffer was a temp buffer (hopefully) to begin with and wasn't (hopefully)
    // used anywhere else.
    // We don't store the optimized order in file (yet) because we may change the format in the future
    font_optimize_order(font, data);

    return font_data_size(font);
}

int32 font_to_data(
    const Font* const font,
    byte* const data,
    MAYBE_UNUSED int32 steps = 8
) NO_EXCEPT
{
    byte* pos = data;

    // Glyph count
    pos = write_le(pos, font->glyph_count);

    // Texture name
    memcpy(pos, font->texture_name, ARRAY_COUNT(font->texture_name) * sizeof(char));
    pos += ARRAY_COUNT(font->texture_name) * sizeof(char);

    pos = write_le(pos, font->size);
    pos = write_le(pos, font->line_height);

    // The glyphs are naturally tightly packed -> we can just store the memory
    memcpy(pos, font->glyphs, font->glyph_count * sizeof(Glyph));
    //pos += font->glyph_count * sizeof(Glyph);

    const int32 size = font_data_size(font);

    SWAP_ENDIAN_LITTLE_SIMD(
        (int32 *) data,
        (int32 *) data,
        size / 4, // everything in here is 4 bytes -> easy to swap
        steps
    );
    PSEUDO_USE(steps);

    return size;
}

FORCE_INLINE
f32 font_line_height(const Font* const font, f32 size) NO_EXCEPT
{
    return font->line_height * size / font->size;
}

// Required depending on the 3D api.
// Some use top-down, some bottom-up coordinates
FORCE_INLINE
void font_invert_coordinates(Font* const font) NO_EXCEPT
{
    for (uint32 i = 0; i < font->glyph_count; ++i) {
        Glyph* const glyph = &font->glyphs[i];
        glyph->uv_start.y = 1.0f - glyph->uv_start.y;
        glyph->uv_end.y = 1.0f - glyph->uv_end.y;
    }
}

#endif