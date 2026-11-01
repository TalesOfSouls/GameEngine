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

/**
 * TEXT FORMAT: .themetxt, version UI_THEME_VERSION
 *
 * The text format is the ui document syntax (@see UIDocument.h):
 *
 *      version 2
 *
 *      // Comment (only full line comments)
 *      @theme                          <- assets of the theme (hex asset ids, missing = not defined)
 *          font: 04000000              <- .fonttxt asset
 *          atlas: 04000002             <- .atlastxt asset
 *
 *      @button                         <- type default: applied to every button of the layout
 *          background_color: #444444FF
 *          background_color_hover: #2E7D32FF
 *
 *      window_default                  <- named style: "style: window_default" in the layout file
 *          background_color: #262626FF
 *          $title                      <- part: fixed child of the element ($part [argument])
 *              background_color: #00FF00FF
 *          $title_label
 *              text: @window_title     <- string of the theme language
 *
 *      @cursor                         <- mouse cursor style
 *          image: mouse_cursor         <- atlas element name
 *
 *      indentation: spaces, a tab counts as 4 (@see UI_DOCUMENT_TAB_WIDTH)
 *      style name:  max. HASH_MAP_MAX_KEY_LENGTH - 1 = 21 characters
 *      property:    "key: value", the keys are the element properties of the
 *                   layout format (@see UILayoutTxt.h / UILayoutTxt.cpp)
 *
 * Strings: "@key" is replaced with the value of the theme language
 * (theme file ./lang/<language_code>/<name>.langtxt, LANGUAGE_DEFAULT_CODE as
 * fallback). The language is stored inside the binary theme if it was loaded
 * before theme_to_data() (@see ui_theme_language_from_txt).
 *
 * BINARY FORMAT: .themebin, version UI_THEME_BINARY_VERSION
 * Also the payload of an ASSET_TYPE_THEME asset (@see AssetArchive.h).
 *
 *      offset      size        type        field
 *      0           4           int32       version (UI_THEME_BINARY_VERSION)
 *      4           12          -           hash map header
 *      16          S           byte[S]     hash map slots (+ free bitmap)
 *      16 + S      4           int32       document_size = D
 *      20 + S      D           byte[D]     document (all offsets relative to the theme data image)
 *
 *      S = capacity * 32 + ceil(capacity / 64) * 8 + 8
 *
 * Hash map header (chunk memory dump, @see hashmap_dump / chunk_dump)
 *
 *      offset  size    type    field
 *      0       4       int32   capacity        amount of slots (max(style_count * 2, 2))
 *      4       4       int32   last_pos        last reserved slot (-1 = none)
 *      8       4       uint32  free_offset     capacity * 32
 *      12      S       byte[]  slot memory (slots first, then the free bitmap)
 *
 * Slot (HashEntryStrT<int32>, 32 bytes)
 *
 *      0   22  char[22]    key     style name, NUL padded
 *      22  2   uint16      next    id + 1 of the next entry of the bucket (0 = end)
 *      24  2   uint16      head    id + 1 of the bucket head (belongs to bucket[slot], not to the entry)
 *      26  2   uint16      tag     16 bit of the key hash
 *      28  4   int32       value   node index in the document
 *
 * The free bitmap follows the slots: 8 bytes per 64 slots, bit set = slot in use.
 *
 * Document (D bytes, position independent, offsets relative to UITheme::data)
 *
 *      +0                      48                  UIThemeData
 *      +48                     text_length + 1     char[] text (NUL terminated)
 *      ...                     pad to sizeof(size_t)
 *      ...                     line_count * 44     UIDocumentNode[line_count] (upper bound, node_count used)
 *      ...                     property_count * 24 UIDocumentProperty[property_count]
 *      ...                     language_size       Language image, optional (@see Language.h)
 *
 * UIThemeData (48 bytes, 12 * int32)
 *
 *      0   text_offset         text in the data image
 *      4   text_length
 *      8   node_offset
 *      12  node_count
 *      16  property_offset
 *      20  property_count
 *      24  first_node          -1 = none
 *      28  version             text format version (UI_THEME_VERSION)
 *      32  language_offset     0 = no language
 *      36  language_size       0 = no language
 *      40  font_asset_id       UI_THEME_ASSET_NONE = not defined
 *      44  atlas_asset_id      UI_THEME_ASSET_NONE = not defined
 *
 * UIDocumentNode (44 bytes)
 *
 *      0   1   byte        kind                UI_DOCUMENT_NODE_BLOCK / UI_DOCUMENT_NODE_PART
 *      1   3   -           padding
 *      4   8   -           type                UIDocumentString {int32 offset; int32 length;}
 *      12  8   -           name                UIDocumentString
 *      20  4   int32       line
 *      24  4   int32       first_property
 *      28  4   int32       last_property
 *      32  4   int32       first_child
 *      36  4   int32       last_child
 *      40  4   int32       next_sibling
 *
 * UIDocumentProperty (24 bytes)
 *
 *      0   8   -       key     UIDocumentString
 *      8   8   -       value   UIDocumentString
 *      16  4   int32   next
 *      20  4   int32   line
 *
 * The node/property arrays never copy strings, they only reference the text.
 * Hash map, document and language form one position independent block, so
 * theme_from_data() only has to copy it and restore the pointers.
 *
 * WARNING: the document block is a memory image (only the version and the
 * document_size are converted to little endian). Themes are only portable
 * between builds with the same endianness and struct layout (incl. sizeof(size_t)).
 *
 * Memory layout of UITheme::data (identical to the binary format)
 *      Hashmap (style name -> node index in the document)
 *      UIThemeData
 *      text of the theme file
 *      document nodes
 *      document properties
 *      language (optional, @see ui_theme_language_from_txt)
 */
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
