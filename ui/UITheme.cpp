#pragma once
#ifndef COMS_UI_THEME_C
#define COMS_UI_THEME_C

#include "../stdlib/Stdlib.h"
#include "../memory/RingMemory.cpp"
#include "../utils/StringUtils.h"
#include "../stdlib/HashMapT.cpp"
#include "../font/Font.cpp"
#include "../system/FileUtils.cpp"
#include "UITheme.h"
#include "UIDocument.h"
#include "../localization/Language.cpp"

/**
 * Theme file (.themetxt), version 2
 *
 * A theme is a list of named styles. A style is a set of element properties (@see UILayoutTxt.cpp)
 * that is applied to elements, it can also style the parts of an element ($title of a window, ...).
 *
 *      version 2
 *
 *      // Type default: applied to every element of this type (before the element styles)
 *      @button
 *          background_color: #444444FF
 *          background_color_hover: #2E7D32FF
 *
 *      // Named style: applied to elements with "style: window_default" in the layout file
 *      window_default
 *          background_color: #262626FF
 *          $title
 *              background_color: #00FF00FF
 *          $title_label
 *              text: @window_title         <- string of the theme language (@see ui_theme_language_from_txt)
 *
 * Style names are limited to HASH_MAP_MAX_KEY_LENGTH - 1 characters
 *
 * Language: themes/<theme>/lang/<language>/<name>.theme.langtxt (@see ui_theme_language_load, Language.cpp)
 *
 * Assets: the "@theme" block defines the assets of the theme as hex asset ids (@see the generated asset id headers)
 *      @theme
 *          font: 04000000      <- font (.fonttxt asset), e.g. AA_ID_UI_FONT_FONTTXT = 0 | (4 << 24)
 *          atlas: 04000002     <- ui texture atlas (.atlastxt asset) for the images of the layouts
 * The archive is part of the id (high byte) -> a theme can use its own assets or the assets of the application
 * Not defined = the font/atlas of another theme is used (e.g. a scene theme uses the general theme font)
 * The assets are loaded or re-used (if already loaded) by the theme loader (@see cmd_theme_load_sync)
 */

#define UI_THEME_HASHMAP_ALIGNMENT ALIGN_UP((int32) sizeof(HashEntryStrT<int32>), 32)

// Runtime data at the end of theme->data (not part of the binary theme)
#define UI_THEME_RUNTIME_SIZE ((int32) ALIGN_UP(sizeof(FontSystem), 64) + 64)

// Usable size for the theme data (without the runtime data)
FORCE_INLINE
int32 ui_theme_capacity(const UITheme* const theme) NO_EXCEPT
{
    return theme->data_size - UI_THEME_RUNTIME_SIZE;
}

/**
 * Asset id of the "@theme" block: hex, optional 0x (e.g. 04000002)
 *
 * @return UI_THEME_ASSET_NONE if the value is not a valid hex id
 */
static inline
int32 ui_theme_asset_id_parse(const char* value) NO_EXCEPT
{
    if (value[0] == '0' && (value[1] == 'x' || value[1] == 'X')) {
        value += 2;
    }

    uint32 id = 0;
    int32 digits = 0;
    for (; *value && *value != ' ' && *value != '\t'; ++value, ++digits) {
        const char c = *value;
        uint32 digit;
        if (c >= '0' && c <= '9') {
            digit = (uint32) (c - '0');
        } else if (c >= 'a' && c <= 'f') {
            digit = (uint32) (c - 'a' + 10);
        } else if (c >= 'A' && c <= 'F') {
            digit = (uint32) (c - 'A' + 10);
        } else {
            return UI_THEME_ASSET_NONE;
        }

        id = (id << 4) | digit;
    }

    // Asset ids are positive int32 values (archive id in the high byte)
    return digits > 0 && digits <= 8 && id <= 0x7FFFFFFFU ? (int32) id : UI_THEME_ASSET_NONE;
}

/**
 * Asset ids of the "@theme" block (@see the format description above)
 */
static inline
void ui_theme_assets_parse(const UIDocument* const doc, UIThemeData* const header) NO_EXCEPT
{
    header->font_asset_id = UI_THEME_ASSET_NONE;
    header->atlas_asset_id = UI_THEME_ASSET_NONE;

    for (int32 i = doc->first_node; i >= 0; i = doc->nodes[i].next_sibling) {
        const UIDocumentNode* const node = &doc->nodes[i];
        if (node->kind != UI_DOCUMENT_NODE_BLOCK || !ui_document_equals(doc, node->type, "@theme")) {
            continue;
        }

        for (int32 p = node->first_property; p >= 0; p = doc->properties[p].next) {
            const UIDocumentProperty* const property = &doc->properties[p];

            char value[32];
            ui_document_copy(doc, property->value, value, sizeof(value));
            const int32 id = ui_theme_asset_id_parse(value);

            int32* target = NULL;
            if (ui_document_equals(doc, property->key, "font")) {
                target = &header->font_asset_id;
            } else if (ui_document_equals(doc, property->key, "atlas")) {
                target = &header->atlas_asset_id;
            } else {
                LOG_1("[WARNING] Theme: unknown asset in line %d", {DATA_TYPE_INT32, (int32 *) &property->line});
                continue;
            }

            if (id == UI_THEME_ASSET_NONE) {
                LOG_1("[WARNING] Theme: invalid asset id in line %d", {DATA_TYPE_INT32, (int32 *) &property->line});
                continue;
            }

            *target = id;
        }
    }
}

// The document data starts after the hash map
static inline
UIThemeData* ui_theme_data_header(const UITheme* const theme) NO_EXCEPT
{
    const size_t hashmap_bytes = chunk_size(sizeof(HashEntryStrT<int32>), theme->hash_map.buf.capacity)
        + UI_THEME_HASHMAP_ALIGNMENT;

    return (UIThemeData *) (theme->data + ALIGN_UP(hashmap_bytes, sizeof(size_t)));
}

// Restores the document pointers after loading/copying the theme data
static inline
void ui_theme_document_restore(UITheme* const theme) NO_EXCEPT
{
    const UIThemeData* const header = ui_theme_data_header(theme);

    theme->document.text = (const char *) (theme->data + header->text_offset);
    theme->document.text_length = header->text_length;
    theme->document.version = header->version;
    theme->document.nodes = (UIDocumentNode *) (theme->data + header->node_offset);
    theme->document.node_count = header->node_count;
    theme->document.properties = (UIDocumentProperty *) (theme->data + header->property_offset);
    theme->document.property_count = header->property_count;
    theme->document.first_node = header->first_node;

    theme->font_asset_id = header->font_asset_id;
    theme->atlas_asset_id = header->atlas_asset_id;

    theme->language.data = header->language_size ? theme->data + header->language_offset : NULL;
    theme->language.size = header->language_size;
    theme->language.count = header->language_size
        ? ((const LanguageHeader *) theme->language.data)->count
        : 0;
}

/**
 * Parses a theme from text
 * The text is copied into theme->data
 *
 * WARNING: theme->data and theme->data_size need to be set
 *
 * @return false if theme->data is too small
 */
inline
bool ui_theme_from_txt(UITheme* const __restrict theme, const char* __restrict text, int32 length) NO_EXCEPT
{
    PROFILE_DEBUG(PROFILE_UI_THEME_FROM_TXT, (char *) NULL, PROFILE_FLAG_SHOULD_LOG);

    LOG_1("[INFO] Parse theme");

    // Upper bound for the amount of styles = lines without indentation
    int32 style_count = 0;
    for (int32 i = 0; i < length; ++i) {
        if ((i == 0 || text[i - 1] == '\n') && text[i] > ' ' && text[i] != '/') {
            ++style_count;
        }
    }

    const int32 line_count = ui_document_line_count(text, length);

    // Hash map (zeroed memory required)
    // * 2 to avoid too many hash collisions
    const int32 capacity = max_branched(style_count * 2, (int32) 2);
    const size_t hashmap_bytes = chunk_size(sizeof(HashEntryStrT<int32>), capacity) + UI_THEME_HASHMAP_ALIGNMENT;

    const size_t header_offset = ALIGN_UP(hashmap_bytes, sizeof(size_t));
    const size_t text_offset = header_offset + sizeof(UIThemeData);
    const size_t node_offset = ALIGN_UP(text_offset + length + 1, sizeof(size_t));
    const size_t total = node_offset + ui_document_memory_size(line_count);

    if ((int64) total > ui_theme_capacity(theme)) {
        LOG_1("[ERROR] Theme data too small: %n B needed", {DATA_TYPE_INT64, &total});
        ASSERT_TRUE(false);

        return false;
    }

    memset(theme->data, 0, hashmap_bytes);
    hashmap_init(&theme->hash_map, capacity, theme->data, UI_THEME_HASHMAP_ALIGNMENT);

    char* const text_copy = (char *) (theme->data + text_offset);
    memcpy(text_copy, text, length);
    text_copy[length] = '\0';

    UIDocument document;
    ui_document_parse(&document, text_copy, length, theme->data + node_offset);

    UIThemeData* const header = ui_theme_data_header(theme);
    header->text_offset = (int32) text_offset;
    header->text_length = length;
    header->node_offset = (int32) node_offset;
    header->node_count = document.node_count;
    header->property_offset = (int32) ((byte *) document.properties - theme->data);
    header->property_count = document.property_count;
    header->first_node = document.first_node;
    header->version = document.version;
    header->language_offset = 0;
    header->language_size = 0;
    ui_theme_assets_parse(&document, header);

    theme->used_data_size = (int32) ((byte *) (document.properties + document.property_count) - theme->data);
    ui_theme_document_restore(theme);

    // The assets are loaded by the theme loader (@see cmd_theme_load_sync)
    theme->font = NULL;
    theme->atlas = NULL;

    if (document.version != UI_THEME_VERSION) {
        LOG_1("[WARNING] Theme file version %d is not supported", {DATA_TYPE_INT32, &document.version});
    }

    // Style name -> node
    for (int32 i = theme->document.first_node; i >= 0; i = theme->document.nodes[i].next_sibling) {
        const UIDocumentNode* const node = &theme->document.nodes[i];

        if (node->type.length >= HASH_MAP_MAX_KEY_LENGTH) {
            LOG_1("[WARNING] Theme style name too long in line %d", {DATA_TYPE_INT32, (int32 *) &node->line});
            continue;
        }

        char name[HASH_MAP_MAX_KEY_LENGTH];
        ui_document_copy(&theme->document, node->type, name, sizeof(name));
        hashmap_insert(&theme->hash_map, name, i);
    }

    return true;
}

/**
 * Adds the language of the theme (strings referenced with "@key" in the theme file)
 * The language is stored in theme->data after the document -> it is part of the binary theme (@see theme_to_data)
 *
 * WARNING: Must be called after ui_theme_from_txt(), replaces an existing language
 *
 * @return false if theme->data is too small
 */
inline
bool ui_theme_language_from_txt(UITheme* const __restrict theme, const char* __restrict text, int32 length) NO_EXCEPT
{
    UIThemeData* const header = ui_theme_data_header(theme);

    // Replace an existing language
    if (header->language_size) {
        theme->used_data_size = header->language_offset;
    }

    const int32 offset = ALIGN_UP(theme->used_data_size, (int32) sizeof(size_t));

    Language language = {};
    language.data = theme->data + offset;

    if (language_from_txt(&language, text, length, ui_theme_capacity(theme) - offset) < 0) {
        header->language_offset = 0;
        header->language_size = 0;
        ui_theme_document_restore(theme);

        return false;
    }

    header->language_offset = offset;
    header->language_size = language.size;
    theme->used_data_size = offset + language.size;
    ui_theme_document_restore(theme);

    return true;
}

/**
 * Loads the language file of a theme file (falls back to LANGUAGE_DEFAULT_CODE)
 *      ./themes/default/general.themetxt -> ./themes/default/lang/<language_code>/general.theme.langtxt
 *
 * @param mem Temporary memory for the file content
 *
 * @return false if the theme has no language file
 */
template <typename M>
bool ui_theme_language_load(
    UITheme* const __restrict theme,
    const wchar_t* __restrict theme_path,
    const wchar_t* __restrict language_code,
    M* const __restrict mem
) NO_EXCEPT
{
    wchar_t path[PATH_MAX_LENGTH];
    // build path and check if file exists
    // @performance this can be optimized heavily
    {
        wcscpy(path, theme_path);
        wchar_t* pos = str_right(path, L'.');
        if (pos) {
            *pos = L'\0';
        }
        str_insert(path, wcslen(path), L".langtxt");
        pos = str_right(path, L'/');
        const size_t dir_end = pos ? (size_t) (pos - path) + 1 : 0;
        str_insert(path, dir_end, L"/");
        str_insert(path, dir_end, language_code);
        str_insert(path, dir_end, L"lang/");

        if (!file_exists(path)) {
            return false;
        }
    }

    FileBody file = {0};
    file_read(path, &file, mem);
    if (!file.content) {
        return false;
    }

    return ui_theme_language_from_txt(theme, (const char *) file.content, (int32) file.size);
}

/**
 * Index of a style node in theme->document.nodes
 *
 * @param name Style name or "@type" for the type default
 *
 * @return -1 if the theme doesn't define the style
 */
inline
int32 ui_theme_style(const UITheme* const theme, const char* const name) NO_EXCEPT
{
    if (!theme || !theme->hash_map.buf.memory) {
        return -1;
    }

    const HashEntryStrT<int32>* const entry = hashmap_entry_get(&theme->hash_map, name);

    return entry ? entry->value : -1;
}

/**
 * Loads a theme text file (e.g. used by the theme builder)
 *
 * WARNING: theme->data and theme->data_size need to be set
 */
inline
void theme_from_file_txt(
    UITheme* const theme,
    const char* path,
    RingMemory* const ring
) NO_EXCEPT
{
    FileBody file = {0};
    file_read(path, &file, ring);
    ASSERT_TRUE(file.size);

    ui_theme_from_txt(theme, (const char *) file.content, (int32) file.size);
}

/**
 * Binary representation:
 *
 * 00 01 02 03 = version (UI_THEME_BINARY_VERSION)
 * .. .. .. .. = hash map
 * .. .. .. .. = size of the document data
 * .. .. .. .. = document data (UIThemeData, text, nodes, properties, language)
 */
int32 theme_to_data(
    const UITheme* __restrict theme,
    byte* __restrict data
) NO_EXCEPT
{
    byte* out = data;

    out = write_le(out, (int32) UI_THEME_BINARY_VERSION);
    out += hashmap_dump(&theme->hash_map, out);

    // The document data (incl. the language) is position independent (offsets relative to theme->data)
    const UIThemeData* const header = ui_theme_data_header(theme);
    const int32 document_offset = (int32) ((const byte *) header - theme->data);
    const int32 document_size = theme->used_data_size - document_offset;

    out = write_le(out, document_size);
    memcpy(out, header, document_size);
    out += document_size;

    return (int32) (out - data);
}

/**
 * WARNING: theme->data and theme->data_size need to be set
 *
 * @return Bytes read or -1 if the data is not a compatible theme
 */
int32 theme_from_data(
    const byte* __restrict data,
    UITheme* __restrict theme
) NO_EXCEPT
{
    PROFILE_DEBUG(PROFILE_THEME_FROM_THEME, (char *) NULL, PROFILE_FLAG_SHOULD_LOG);
    LOG_1("[INFO] Load theme");

    const byte* in = data;

    int32 version;
    in = read_le(in, &version);

    if (version != UI_THEME_BINARY_VERSION) {
        LOG_1("[WARNING] Incompatible theme version %d", {DATA_TYPE_INT32, &version});

        return -1;
    }

    // The capacity is the first value of the hash map data (@see hashmap_dump)
    int32 capacity;
    read_le(in, &capacity);
    SWAP_ENDIAN_LITTLE_SELF(capacity);

    const size_t hashmap_bytes = chunk_size(sizeof(HashEntryStrT<int32>), capacity) + UI_THEME_HASHMAP_ALIGNMENT;
    memset(theme->data, 0, hashmap_bytes);
    hashmap_init(&theme->hash_map, capacity, theme->data, UI_THEME_HASHMAP_ALIGNMENT);
    in += hashmap_load(&theme->hash_map, in);

    int32 document_size;
    in = read_le(in, &document_size);

    UIThemeData* const header = ui_theme_data_header(theme);
    if ((byte *) header + document_size > theme->data + ui_theme_capacity(theme)) {
        LOG_1("[ERROR] Theme data too small: %d B needed", {DATA_TYPE_INT32, &document_size});
        ASSERT_TRUE(false);

        return -1;
    }

    memcpy(header, in, document_size);
    in += document_size;

    theme->used_data_size = (int32) ((byte *) header - theme->data) + document_size;
    ui_theme_document_restore(theme);

    // The assets are loaded by the theme loader (@see cmd_theme_load_sync)
    theme->font = NULL;
    theme->atlas = NULL;

    LOG_1("[INFO] Loaded theme");

    return (int32) (in - data);
}

#endif
