#ifndef COMS_FONT_H
#define COMS_FONT_H

#include "../stdlib/Stdlib.h"

/**
 * @todo The font atlas should have a pixel perfect representation at the smallest possible font size. Currently it's not pixel perfect nor the smallest possible size
 * @todo The font atlas letters could be minimized by using one channel each. This allows us to reduce the font atlas size by a factor of 3
 */

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

#endif