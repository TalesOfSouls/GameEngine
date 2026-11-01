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

/**
 * TEXT FORMAT: .fonttxt (e.g. written by FontBuilder from an msdf-atlas-gen .csv)
 *
 *      texture: ./img/ui_font_atlas.png    <- atlas image (asset path; a number = existing asset id)
 *      font_size: 32                       <- size the font was rasterized at
 *      line_height: 32
 *      image_width: 512                    <- atlas size, used to normalize the uv
 *      image_height: 512
 *      glyph_count: 127
 *      32 1 1 6 0 6 1 1                    <- one line per glyph, ascending codepoint
 *      33 6 23 2 0 2 45 4
 *      ...
 *
 *      glyph line: codepoint width height offset_x offset_y advance_x uv_x uv_y
 *          width/height    glyph quad size in pixels (not the vertex)
 *          offset_x/y      offset from the baseline in pixels
 *          advance_x       horizontal advance in pixels
 *          uv_x/uv_y       top left corner in atlas pixels, the loader normalizes them
 *                          uv_start = (x / image_width, y / image_height)
 *                          uv_end   = uv_start + (width / image_width, height / image_height)
 *
 * BINARY FORMAT: ASSET_TYPE_FONT payload, written by font_to_data()
 *
 *      offset      size        type            field
 *      0           2           uint16          glyph_count
 *      2           32          char[32]        texture_name (NUL padded)
 *      34          4           f32             size
 *      38          4           f32             line_height
 *      42          40 * count  Glyph[count]    glyphs, ascending codepoint
 *      total       42 + 40 * count
 *
 * Glyph (40 bytes)
 *
 *      0   4   uint32          codepoint
 *      4   20  GlyphMetrics    width, height, offset_x, offset_y, advance_x (5 * f32)
 *      24  8   v2_f32          uv_start
 *      32  8   v2_f32          uv_end
 *
 * There is no version field and no order_type: the file always stores the glyphs
 * sorted by codepoint (FONT_GLYPH_ORDER_TYPE_NORMAL). The Eytzinger order is only
 * built at load time (font_optimize_order) and never written.
 *
 * The glyph_count is limited to 8191 by the loader, the texture is a separate
 * ASSET_TYPE_IMAGE asset that the archive builder adds as a dependency
 * (@see AssetArchive.h).
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