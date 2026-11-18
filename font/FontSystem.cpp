#pragma once
#ifndef COMS_FONT_SYSTEM_C
#define COMS_FONT_SYSTEM_C

#include "FontSystem.h"
#include "Font.cpp"
#include "../utils/StringUtils.h"

inline
void font_system_newline_cache(FontSystem* const font) NO_EXCEPT
{
    font->newline_id = font_glyph_index_find(&font->base, '\n');
    if (font->newline_id < 0) {
        font->newline_id = font_glyph_index_find(&font->extended, '\n');
    }
}

/**
 * Width of the text range [start, end) (line breaks restart the width -> width of the last line)
 * No temporary memory is needed (e.g. usable during input handling)
 */
inline
f32 font_system_text_range_width(
    const FontSystem* const font,
    const char* const text, int32 start, int32 end,
    f32 size
) NO_EXCEPT
{
    if (!font || !text || end <= start || size <= 0.0f) {
        return 0.0f;
    }

    // Global font scale (e.g. accessibility text size)
    size *= font->base.scale;

    const f32 scale = size / font->base.size;
    f32 width = 0.0f;

    int32 i = start;
    while (i < end) {
        uint32 codepoint;
        const int32 byte_len = utf8_decode(text + i, &codepoint);
        if (byte_len <= 0) {
            ++i;
            continue;
        }

        i += byte_len;

        if (codepoint == '\n') {
            width = 0.0f;
            continue;
        }

        const Glyph* glyph = font_glyph_find(&font->base, codepoint);
        if (!glyph && font->has_extended) {
            glyph = font_glyph_find(&font->extended, codepoint);
        }

        if (!glyph) {
            // The 0 index character is the special character for "not found"
            glyph = &font->base.glyphs[0];
        }

        width += (glyph->metrics.width + glyph->metrics.offset_x + glyph->metrics.advance_x) * scale;
    }

    return width;
}

#endif