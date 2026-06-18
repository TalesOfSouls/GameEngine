#pragma once
#ifndef COMS_FONT_SYSTEM_C
#define COMS_FONT_SYSTEM_C

#include "FontSystem.h"
#include "Font.cpp"

inline
void font_system_newline_cache(FontSystem* const font) NO_EXCEPT
{
    font->newline_id = font_glyph_index_find(&font->base, '\n');
    if (font->newline_id < 0) {
        font->newline_id = font_glyph_index_find(&font->extended, '\n');
    }
}

#endif