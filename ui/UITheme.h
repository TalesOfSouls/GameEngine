#pragma once
#ifndef COMS_UI_THEME_H
#define COMS_UI_THEME_H

#include "../stdlib/Stdlib.h"
#include "../stdlib/HashMapT.h"
#include "../font/FontSystem.h"
#include "../font/Font.h"

#define UI_THEME_VERSION 1

// Memory layout (data)
// Hashmap
// [alignment - UIAttributeGroup]
// UIAttributeGroup - This is where the pointers point to (or what the offset represents)
//      [alignment - Attributes]
//      Attributes ...
//      Attributes ...
//      Attributes ...
// [alignment - UIAttributeGroup]
// UIAttributeGroup
//      [alignment - Attributes]
//      Attributes ...
//      Attributes ...
//      Attributes ...
struct UITheme {
    // A theme may have N named styles
    // The hashmap contains the offset where the respective style can be found
    // @performance Consider to switch to perfect hash map
    HashMapT<HashEntryStrT<int32>> hash_map;

    // Total size of the theme incl. hash_map
    // Most likely the theme has some additional free data available
    // This is because we might want to dynamically grow the theme/allow modification that increases the theme size
    //      e.g. theme didn't define an outline, user wants to have an outline -> additional attribute needed
    int32 data_size;

    // This is how much we actually use in the theme right now
    int32 used_data_size;

    // This data is the owner of the theme data
    // This also holds the hashmap data in the very beginning
    byte* data;

    // @question It feels weird that this is here, especially considering we could have multiple fonts
    // Maybe we should have an array of fonts (e.g. allow up to 3 fonts per theme?)
    FontSystem* font;
};

#endif