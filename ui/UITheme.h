#pragma once
#ifndef COMS_UI_THEME_H
#define COMS_UI_THEME_H

#include "../stdlib/Stdlib.h"
#include "../stdlib/HashMapT.h"
#include "../font/FontSystem.h"
#include "../font/Font.h"
#include "UIDocument.h"
#include "../localization/Language.h"
#include "../object/TextureAtlas.h"

// Version 2: text format (@see UITheme.cpp)
#define UI_THEME_VERSION 2

// Version of the binary format (@see theme_to_data)
// 3: + language
// 4: + font/atlas asset ids
#define UI_THEME_BINARY_VERSION 5

// Asset id of a theme asset that is not defined (0 is a valid asset id)
#define UI_THEME_ASSET_NONE (-1)

// Memory layout (data)
//      Hashmap (style name -> node index in the document)
//      UIThemeData
//      text of the theme file
//      document nodes
//      document properties
//      language (optional, @see ui_theme_language_from_txt)
struct UITheme {
    // Style name -> index of the style node in document.nodes
    // Type defaults are stored as "@type" (e.g. "@button")
    // @performance Consider to switch to perfect hash map
    HashMapT<HashEntryStrT<int32>> hash_map;

    // Total size of the theme incl. hash_map
    int32 data_size;

    // This is how much we actually use in the theme right now
    int32 used_data_size;

    // This data is the owner of the theme data
    // This also holds the hashmap data in the very beginning
    byte* data;

    // Parsed theme file, all pointers point into data
    UIDocument document;

    // Strings of the theme file ("@key" values, @see UILayoutTxt.cpp), data points into data
    // Empty if the theme has no language file
    Language language;

    // Assets of the theme ("@theme" block in the theme file), UI_THEME_ASSET_NONE = not defined
    // The theme only stores the ids, the assets are loaded/re-used by the theme loader (@see cmd_theme_load_sync)
    int32 font_asset_id;
    int32 atlas_asset_id;

    // Loaded assets (NULL = not loaded or not defined -> the ui uses the ones of another theme, e.g. the general theme)
    // @question It feels weird that this is here, especially considering we could have multiple fonts
    // Maybe we should have an array of fonts (e.g. allow up to 3 fonts per theme?)
    FontSystem* font;
    const TextureAtlas* atlas;
};

// Offsets of the document data in UITheme::data (stored directly after the hash map)
struct UIThemeData {
    int32 text_offset;
    int32 text_length;
    int32 node_offset;
    int32 node_count;
    int32 property_offset;
    int32 property_count;
    int32 first_node;
    int32 version;

    // Language memory image (@see Language.h), 0 = no language
    int32 language_offset;
    int32 language_size;

    // @see UITheme::font_asset_id
    int32 font_asset_id;
    int32 atlas_asset_id;
};

#endif
