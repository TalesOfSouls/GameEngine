#ifndef COMS_FONT_H
#define COMS_FONT_H

#include "../stdlib/Stdlib.h"

/**
 * @todo The font atlas should have a pixel perfect representation at the smallest possible font size. Currently it's not pixel perfect nor the smallest possible size
 * @todo The font atlas letters could be minimized by using one channel each. This allows us to reduce the font atlas size by a factor of 3
 * @todo Font atlas should be two fold:
 *      1. In memory contains all required font characters in a font atlas
 *      2. On gpu only contains the default pre-selected characters and extends that font atlas with new characters
 *          This means a font atlas has fixed characters and space for new characters which get uploaded when needed
 *          If space runs out, the least used characters are removed (not the pre-selected ones though)
 *          Ideally we have one pre-selected texture 1024x1024 and one that is empty to be filled like a cache
 */

struct GlyphMetrics {
    f32 width;     // Width of the glyph (not the vertex)
    f32 height;    // Height of the glyph (not the vertex)
    f32 offset_x;  // Horizontal offset from baseline
    f32 offset_y;  // Vertical offset from baseline
    f32 advance_x; // Horizontal advance after drawing the glyph
};

struct GlyphVertex {
    v2_f32 pos;
    v2_f32 uv;
};

struct Glyph {
    uint32 codepoint;
    GlyphMetrics metrics;
    int32 vertex_count;
    // We use either a quad or a triangle
    // Triangles are used for characters that can be efficiently represented by one
    // e.g. 7, A, V, v, L, T, r, <, >, /, \, ... (maybe even F, P, i, ?)
    GlyphVertex vertices[4];
};

struct Font {
    uint32 glyph_count;
    char texture_name[32];

    int32 sampler;

    // Default font size at which the font renders best
    f32 size;

    // How tall is a single line (mostly important for multiple lines)
    f32 line_height;

    // WARNING: Glyphs MUST be sorted ascending based on codepoint
    Glyph* glyphs;
};

#endif