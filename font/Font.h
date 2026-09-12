#pragma once
#ifndef COMS_FONT_H
#define COMS_FONT_H

#include "../stdlib/Stdlib.h"
#include "../object/Texture.h"

/**
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

struct Glyph {
    uint32 codepoint;
    GlyphMetrics metrics;
    v2_f32 uv_start;
    v2_f32 uv_end;
};

enum FontGlyphOrderType : uint16 {
    FONT_GLYPH_ORDER_TYPE_NORMAL,
    FONT_GLYPH_ORDER_TYPE_EYTZINGER
};

struct Font {
    // The data before the glyphs can be considered header data

    // @performance I hate that this is here. The only place that uses this is the archive_builder
    char texture_name[32];
    Texture* texture;

    uint16 glyph_count;
    FontGlyphOrderType order_type;

    // Default font size at which the font renders best
    f32 size;

    // How tall is a single line (mostly important for multiple lines)
    f32 line_height;

    // WARNING: Glyphs MUST be sorted ascending based on codepoint
    Glyph* glyphs;
};

#endif