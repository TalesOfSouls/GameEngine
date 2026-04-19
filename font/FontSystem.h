#pragma once
#ifndef COMS_FONT_SYSTEM_H
#define COMS_FONT_SYSTEM_H

#include "../stdlib/Stdlib.h"
#include "Font.h"
#include "../stdlib/HashMapT.h"
#include "../image/Image.h"

struct FontSystem {
    // Usually a program has one standard character set which is fast to load
    // and widely used. If you only need these characters you can access them really fast
    // both on the CPU and GPU since they are static and well positioned.
    // That is also why the glyphs in here are not added to the font_map.
    // Searching them without hashin is most likely faster
    Font base;

    // This is where all the glyphs are stored, that are not part of the base font
    // Usually this includes chinese characters etc.
    Font extended;

    // Stores all glyphs that are currently used in our temp font atlas
    HashMapT<HashEntryT<int32, Glyph>> font_map;

    // This is where we load the extended font image data from
    // Contrary to the base font, we need to keep this in memory because
    // our temp_font_atlas needs to be updated on a per use basis
    Image extended_font_atlas;

    // This is where we put the temporary glyphs currently used in the font_map
    // This image is then uploaded to the GPU where it can be used
    Image temp_font_atlas;

    // Has the font_map changed, which is important for GPU,
    // since we need to upload the new temp font atlas
    bool has_changes;

    // Do we even want to allow extended font characters?
    // Maybe a user doesn't even need/want that.
    bool has_extended;
};

#endif