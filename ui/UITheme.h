#ifndef COMS_UI_THEME_H
#define COMS_UI_THEME_H

#include "../stdlib/Stdlib.h"
#include "../stdlib/HashMap.h"
#include "../font/Font.h"

#define UI_THEME_VERSION 1

// @question Currently there is some data duplication in here and in the UIElement.
//      Not sure if this is how we want this to be or if we want to change this in the future
// Modified for every scene
// WARNING: Make sure the order of this struct and UITheme is the same for the first elements
//          This allows us to cast between both
struct UIThemeStyle {
    // A theme may have N named styles
    // The hashmap contains the offset where the respective style can be found
    // @performance Switch to perfect hash map
    // @question Both the layout and theme have their own hashmap
    //          Would it make sense to have one hashmap for both together?
    HashMap hash_map;

    // Total size of the theme incl. hash_map
    // Most likely the theme has some additional free data available
    // This is because we might want to dynamically grow the theme
    uint32 data_size;

    // This is how much we actually use in the theme
    uint32 used_data_size;

    // This buffer is used also by the hash_map
    byte* data;

    // @question It feels weird that this is here, especially considering we could have multiple fonts
    // Maybe we should have an array of fonts (e.g. allow up to 3 fonts per theme?)
    const Font* font;
};

#endif