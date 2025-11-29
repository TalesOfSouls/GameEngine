#ifndef COMS_FONT_H
#define COMS_FONT_H

#include "../stdlib/Types.h"
#include "../memory/BufferMemory.h"
#include "../utils/EndianUtils.h"
#include "../utils/Utils.h"
#include "../utils/BitUtils.h"
#include "../stdlib/Simd.h"
#include "../system/FileUtils.cpp"

struct GlyphMetrics {
    f32 width;     // Width of the glyph
    f32 height;    // Height of the glyph
    f32 offset_x;  // Horizontal offset from baseline
    f32 offset_y;  // Vertical offset from baseline
    f32 advance_x; // Horizontal advance after drawing the glyph
};

struct GlyphTextureCoords {
    v2_f32 start;
    v2_f32 end;
};

#define GLYPH_SIZE 40
struct Glyph {
    uint32 codepoint;
    GlyphMetrics metrics;
    GlyphTextureCoords coords;
};

struct Font {
    uint32 glyph_count;
    char texture_name[32]; // @question Do we even need this
    f32 size;              // Default font size at which the font renders best
    f32 line_height;       // How tall is a single line (mostly important for multiple lines)

    // WARNING: Glyphs MUST be sorted ascending based on codepoint
    Glyph* glyphs;
};

inline
void font_init(Font* font, byte* data, int count) NO_EXCEPT
{
    font->glyphs = (Glyph *) data;
    font->glyph_count = count;
}

// @performance replace with Eytzinger (obviously we would also have to change the order in the font font file itself)
inline
const Glyph* font_glyph_find(const Font* font, uint32 codepoint) NO_EXCEPT
{
    const uint32 perfect_glyph_pos = codepoint - font->glyphs[0].codepoint;
    const uint32 limit = OMS_MIN(perfect_glyph_pos, font->glyph_count - 1);

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

void font_from_file_txt(
    Font* font,
    const char* path,
    RingMemory* ring
) NO_EXCEPT
{
    FileBody file = {};
    file_read(path, &file, ring);
    ASSERT_TRUE(file.size);

    const char* pos = (char *) file.content;

    char block_name[32];

    int32 image_width = 0;
    int32 image_height = 0;

    char* texture_pos = font->texture_name;

    // Font header
    while (*pos != '\0') {
        // Parsing general data
        int32 i = 0;
        while (*pos != '\0' && *pos != ' ' && *pos != ':' && *pos != '\n' && i < 31) {
            block_name[i] = *pos;
            ++pos;
            ++i;
        }

        block_name[i] = '\0';

        // Go to value
        while (*pos == ' ' || *pos == '\t' || *pos == ':') {
            ++pos;
        }

        if (str_compare(block_name, "texture") == 0) {
            while (*pos != '\n') {
                *texture_pos++ = *pos++;
            }

            *texture_pos++ = '\0';
        } else if (str_compare(block_name, "font_size") == 0) {
            font->size = str_to_float(pos, &pos);
        } else if (str_compare(block_name, "line_height") == 0) {
            font->line_height = str_to_float(pos, &pos);
        } else if (str_compare(block_name, "image_width") == 0) {
            image_width = (int32) str_to_int(pos, &pos);
        } else if (str_compare(block_name, "image_height") == 0) {
            image_height = (int32) str_to_int(pos, &pos);
        } else if (str_compare(block_name, "glyph_count") == 0) {
            // glyph_count has to be the last general element
            font->glyph_count = (uint32) str_to_int(pos, &pos);
            break;
        }

        // Go to next line
        while (*pos != '\0' && *pos++ != '\n') {};
    }

    int32 glyph_index = 0;
    ++pos;

    // Body
    while (*pos != '\0') {
        // Parsing glyphs
        // In the text file we don't have to define width and height of the character, we calculate that here
        font->glyphs[glyph_index] = {
            (uint32) str_to_int(pos, &pos), // codepoint
            {0.0f, 0.0f, str_to_float(++pos, &pos), str_to_float(++pos, &pos), str_to_float(++pos, &pos)},
            {str_to_float(++pos, &pos), str_to_float(++pos, &pos), str_to_float(++pos, &pos), str_to_float(++pos, &pos)}
        };

        font->glyphs[glyph_index].metrics.width = font->glyphs[glyph_index].coords.end.x - font->glyphs[glyph_index].coords.start.x;
        font->glyphs[glyph_index].metrics.height = font->glyphs[glyph_index].coords.end.y - font->glyphs[glyph_index].coords.start.y;

        font->glyphs[glyph_index].coords.start.x /= image_width;
        font->glyphs[glyph_index].coords.end.x /= image_width;

        font->glyphs[glyph_index].coords.start.y /= image_height;
        font->glyphs[glyph_index].coords.end.y /= image_height;

        ++glyph_index;

        // Go to next line
        while (*pos != '\n' && *pos != '\0') { ++pos; };
        ++pos;
    }
}

FORCE_INLINE
int32 font_data_size(const Font* font) NO_EXCEPT
{
    ASSERT_TRUE_CONST(sizeof(Glyph) == GLYPH_SIZE);
    return font->glyph_count * sizeof(Glyph)
        + sizeof(font->glyph_count)
        + sizeof(font->texture_name)
        + sizeof(font->size)
        + sizeof(font->line_height);
}

int32 font_from_data(
    const byte* data,
    Font* font,
    MAYBE_UNUSED int32 steps = 8
) NO_EXCEPT
{
    const byte* pos = data;

    // Read count
    font->glyph_count = SWAP_ENDIAN_LITTLE(*((uint32 *) pos));
    pos += sizeof(font->glyph_count);

    // Read texture name
    memcpy(font->texture_name, pos, ARRAY_COUNT(font->texture_name) * sizeof(char));
    pos += ARRAY_COUNT(font->texture_name) * sizeof(char);

    // Read font size
    font->size = SWAP_ENDIAN_LITTLE(*((f32 *) pos));
    pos += sizeof(font->size);

    // Read line height
    font->line_height = SWAP_ENDIAN_LITTLE(*((f32 *) pos));
    pos += sizeof(font->line_height);

    memcpy(font->glyphs, pos, font->glyph_count * sizeof(Glyph));

    SWAP_ENDIAN_LITTLE_SIMD(
        (int32 *) font->glyphs,
        (int32 *) font->glyphs,
        font->glyph_count * sizeof(Glyph) / 4, // everything in here is 4 bytes -> easy to swap
        steps
    );
    PSEUDO_USE(steps);

    return font_data_size(font);
}

int32 font_to_data(
    const Font* font,
    byte* data,
    MAYBE_UNUSED int32 steps = 8
) NO_EXCEPT
{
    byte* pos = data;

    // Glyph count
    *((uint32 *) pos) = font->glyph_count;
    pos += sizeof(font->glyph_count);

    // Texture name
    memcpy(pos, font->texture_name, ARRAY_COUNT(font->texture_name) * sizeof(char));
    pos += ARRAY_COUNT(font->texture_name) * sizeof(char);

    // Font size
    *((f32 *) pos) = font->size;
    pos += sizeof(font->size);

    // Line height
    *((f32 *) pos) = font->line_height;
    pos += sizeof(font->line_height);

    // The glyphs are naturally tightly packed -> we can just store the memory
    memcpy(pos, font->glyphs, font->glyph_count * sizeof(Glyph));
    pos += font->glyph_count * sizeof(Glyph);

    int32 size = font_data_size(font);

    SWAP_ENDIAN_LITTLE_SIMD(
        (int32 *) file.content,
        (int32 *) file.content,
        compiler_div_pow2(size, 4), // everything in here is 4 bytes -> easy to swap
        steps
    );
    PSEUDO_USE(steps);

    return size;
}

FORCE_INLINE
f32 font_line_height(Font* font, f32 size) NO_EXCEPT
{
    return font->line_height * size / font->size;
}

FORCE_INLINE
void font_invert_coordinates(Font* font) NO_EXCEPT
{
    // @todo Implement y-offset correction
    for (uint32 i = 0; i < font->glyph_count; ++i) {
        const f32 temp = font->glyphs[i].coords.start.y;
        font->glyphs[i].coords.start.y = 1.0f - font->glyphs[i].coords.end.y;
        font->glyphs[i].coords.end.y = 1.0f - temp;
    }
}

#endif