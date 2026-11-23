#pragma once
#ifndef COMS_UI_LAYOUT_C
#define COMS_UI_LAYOUT_C

#include "../stdlib/Stdlib.h"
#include "../stdlib/HashMapT.cpp"
#include "../asset/Asset.h"
#include "../camera/Camera.h"
#include "../memory/BufferMemory.cpp"
#include "../memory/RingMemory.cpp"
#include "../hash/GeneralHash.h"
#include "../localization/Language.cpp"
#include "../system/FileUtils.cpp"

#include "UICore.h"
#include "UILayout.h"
#include "attribute/UIAttributeImage.cpp"
#include "attribute/UIAttributeBorder.cpp"
#include "UITheme.cpp"
#include "UIElementType.cpp"
#include "UICore.cpp"
#include "UIPanel.cpp"
#include "UILabel.cpp"
#include "UIButton.cpp"
#include "UIImage.cpp"
#include "UICursor.cpp"
#include "UIWindow.cpp"
#include "UICheckbox.cpp"
#include "UISlider.cpp"
#include "UIInput.cpp"
#include "UISelect.cpp"
#include "UITab.cpp"
#include "UITable.cpp"
#include "UIColorPicker.cpp"
#include "UIData.cpp"
#include "UITooltip.cpp"

// Max. amount of named elements (if the hash map wasn't initialized by loading a binary layout)
#define UI_LAYOUT_DEFAULT_ELEMENT_COUNT 512

/**
 * (Re-)initializes the hash map for element names in layout->data
 */
static inline
void ui_layout_hashmap_init(UILayout* const layout, int32 element_count) NO_EXCEPT
{
    const int32 alignment = ALIGN_UP((int32) sizeof(HashEntryStrT<int32>), 32);

    // hashmap_init expects zeroed memory
    const size_t size = chunk_size(sizeof(HashEntryStrT<int32>), element_count) + alignment;
    ASSERT_TRUE((int64) size <= layout->data_size);
    memset(layout->data, 0, size);

    hashmap_init(&layout->hash_map, element_count, layout->data, alignment);
}

/**
 * Removes all elements (incl. the root) and element names
 * The memory of the layout stays reserved
 */
inline
void ui_layout_reset(UILayout* const layout) NO_EXCEPT
{
    buffer_reset(&layout->ui_element_buffer);
    layout->ui_root = NULL;
    layout->cursor = NULL;
    memset(&layout->interaction, 0, sizeof(layout->interaction));
    layout->dirty |= UI_LAYOUT_DIRTY;
    layout->ui_vertex_cache.count = 0;
    layout->ui_index_cache.count = 0;

    if (layout->data) {
        ui_layout_hashmap_init(
            layout,
            layout->hash_map.buf.capacity ? layout->hash_map.buf.capacity : UI_LAYOUT_DEFAULT_ELEMENT_COUNT
        );
    }
}

/**
 * Makes an element findable by name
 * The name doesn't have to be \0 terminated
 */
inline
void ui_layout_name_element(UILayout* const layout, const char* name, int32 length, const UICore* const element) NO_EXCEPT
{
    if (!layout->hash_map.buf.memory) {
        if (!layout->data) {
            return;
        }

        ui_layout_hashmap_init(layout, UI_LAYOUT_DEFAULT_ELEMENT_COUNT);
    }

    hashmap_insert(&layout->hash_map, name, length, ui_element_offset(layout, element));
}

/**
 * Finds the first element of a type in pre-order
 */
static inline
UICore* ui_layout_find_first(const UILayout* const layout, UIElementType type) NO_EXCEPT
{
    ASSERT_TRUE(layout->ui_root);

    int32 offset = layout->ui_root->next_offset;
    while (offset) {
        UICore* const element = ui_element_get(layout, offset);
        if (element->type == type) {
            return element;
        }

        offset = element->next_offset;
    }

    return NULL;
}

/**
 * Binary format:
 *      version
 *      hash map
 *      element buffer size
 *      element buffer (position independent since it only uses offsets)
 */
int32 layout_to_data(
    const UILayout* const __restrict layout,
    byte* __restrict data
) {
    LOG_1("[INFO] UI save layout");
    byte* out = data;

    out = write_le(out, (int32) UI_LAYOUT_VERSION);

    // We don't save the used_data_size because that depends on the respective theme

    out += hashmap_dump(&layout->hash_map, out);

    const int32 buffer_size = (int32) (layout->ui_element_buffer.head - layout->ui_element_buffer.memory);
    out = write_le(out, buffer_size);
    memcpy(out, layout->ui_element_buffer.memory, buffer_size);
    out += buffer_size;

    LOG_1("[INFO] UI saved layout");

    return (int32) (out - data);
}

/**
 * The size of layout->data should be the hash map size + a bunch of additional data for theme dependent data
 *
 * @return int32 Bytes read or -1 if the data is not a compatible layout
 */
int32 layout_from_data(
    const byte* const __restrict data,
    UILayout* const __restrict layout
) {
    PROFILE_DEBUG(PROFILE_LAYOUT_FROM_DATA, (char *) NULL, PROFILE_FLAG_SHOULD_LOG);
    LOG_1("[INFO] UI load layout");

    const byte* in = data;

    int32 version;
    in = read_le(in, &version);

    if (version != UI_LAYOUT_VERSION) {
        LOG_1("[WARNING] Incompatible layout version %n", {DATA_TYPE_INT32, &version});

        return -1;
    }

    // Prepare hashmap (incl. reserve memory)
    // Of course we still need to populate the data using hashmap_load()
    ui_layout_hashmap_init(layout, (int32) SWAP_ENDIAN_LITTLE(*((uint32 *) in)));

    const int64 hashmap_data_size = hashmap_load(&layout->hash_map, in);
    layout->used_data_size = (int32) hashmap_size(&layout->hash_map);
    in += hashmap_data_size;

    int32 buffer_size;
    in = read_le(in, &buffer_size);
    ASSERT_TRUE((size_t) buffer_size <= layout->ui_element_buffer.size);

    memcpy(layout->ui_element_buffer.memory, in, buffer_size);
    layout->ui_element_buffer.head = layout->ui_element_buffer.memory + buffer_size;
    in += buffer_size;

    // Restore the helper pointers
    layout->ui_root = buffer_size ? (UICore *) layout->ui_element_buffer.memory : NULL;

    // The cached vertices of the elements don't exist anymore
    if (layout->ui_root) {
        int32 offset = 0;
        do {
            UICore* const element = ui_element_get(layout, offset);
            element->cache_frame = 0;
            offset = element->next_offset;
        } while (offset);
    }
    layout->cursor = (UICursor *) ui_layout_find_first(layout, UI_ELEMENT_TYPE_CURSOR);
    memset(&layout->interaction, 0, sizeof(layout->interaction));
    layout->dirty |= UI_LAYOUT_DIRTY;

    LOG_1("[INFO] UI loaded layout");

    return (int32) (in - data);
}

////////////////////////////////////////////////////////////////
// Text layout files (.layouttxt)
////////////////////////////////////////////////////////////////

/**
 * Layout file (.layouttxt)
 *
 * Defines the element tree
 *
 *      version 2
 *
 *      window test_window              <- "type [name]", the name can be used with ui_element_get()
 *          style: window_default       <- styles of the theme, applied before the own properties
 *          anchor: top left
 *          pos: 512 256
 *          size: 560 480
 *          title: Test window
 *          $close                      <- part = fixed child created by the element itself
 *              on_click: window_close
 *
 *          label status                <- child element
 *              text: Status: -
 *
 * Order in which the properties are applied:
 *      1. create arguments (e.g. columns of a table)
 *      2. type default styles of the themes ("@table")
 *      3. styles listed in "style:" (in this order)
 *      4. own properties and parts (in file order)
 *      5. child elements
 *
 * Values
 *      numbers:    12, -3.5, 50% (= relative to the parent for pos/size)
 *      size:       0 = content size (e.g. text), negative = parent size minus the value
 *      colors:     #RRGGBBAA or #RRGGBB
 *      lists:      Option A | Option B | Option C
 *      text:       \n = new line, \t = tab, \\ = backslash
 *      strings:    @key  = string of the language of the file (layout: ctx->language, theme: UITheme::language)
 *                  @@key = global string of the application (ctx->language_global)
 *                  The whole value is replaced (e.g. "options: @fruits" with "fruits: Apple | Banana" in the language file)
 *                  \@ at the beginning = literal @
 *      names:      functions (update, render, on_click, ...), images (atlas elements)
 *      anchor:     <parent point> [/ <element point>], points: left|center|right top|center|bottom
 *                  e.g. "top right" = inside the top right corner, "top right / top left" = right of the top right corner
 *      flags:      hitbox interactable movable resizable hoverable selectable ignore_hit always_on_top update_tick
 *                  +flag / -flag add/remove a flag, otherwise the flags are replaced
 *
 * Element types and their properties
 *      all:        style pos size anchor flags hidden update render
 *      label:      text pattern capacity font_size font_color
 *      button:     text image on_click background_color background_color_hover border_color border_color_hover border_width border_image, $label
 *      image:      image
 *      cursor:     image
 *      window:     title resizable background_color border_color border_width border_image, $title $title_label $close $resize_grip
 *      panel:      scroll(x|y|xy) popup(true) close_on_click background_color border_color border_width border_image, $content
 *      custom:     (update + render)
 *      checkbox:   text checked group on_change box_color border_color border_color_hover check_color, $label
 *      radio:      same as checkbox
 *      slider:     value step on_change track_color fill_color knob_color knob_color_hover
 *      input:      max_length text placeholder on_change on_submit background_color border_color border_color_focus selection_color font_size font_color
 *      textarea:   same as input
 *      select:     options search multi selected on_change background_color border_color border_color_hover option_hover_color check_color font_size font_color
 *      tab:        tabs orientation(horizontal|vertical) active on_change header_size tab_size header_color tab_color tab_color_hover tab_color_active page_color border_color font_size font_color, $page <index>
 *      table:      columns row_capacity cell_capacity widths row footer on_change row_height header_color row_color row_color_alt row_color_hover row_color_selected footer_color border_color font_size font_color
 *      colorpicker: type(gradient|palette) color on_change border_color border_color_hover
 *      tooltip:    font_size (only top level)
 *
 * Child elements of a scroll panel are added to its content panel, child elements of a tab have to be in a $page
 * Type default styles are only applied to elements of the layout file, not to parts
 */

// Max length of a property value
#define UI_TXT_VALUE_LENGTH 2048

// Max amount of list entries (options, columns, ...)
#define UI_TXT_LIST_MAX 128

// Max depth of styles that use other styles
#define UI_TXT_STYLE_DEPTH 8

////////////////////////////////////////////////////////////////
// Value parsing
////////////////////////////////////////////////////////////////

FORCE_INLINE
void ui_txt_skip_space(const char** const pos) NO_EXCEPT
{
    while (**pos == ' ' || **pos == '\t') {
        ++(*pos);
    }
}

/**
 * Reads the next word (until whitespace or one of the stop characters)
 *
 * @return int32 Length of the word (0 = no word)
 */
static inline
int32 ui_txt_word(const char** const pos, char* const out, int32 out_size, const char* const stop = "") NO_EXCEPT
{
    ui_txt_skip_space(pos);

    int32 length = 0;
    while (**pos && **pos != ' ' && **pos != '\t' && !str_find(stop, **pos)) {
        if (length < out_size - 1) {
            out[length++] = **pos;
        }

        ++(*pos);
    }

    out[length] = '\0';

    return length;
}

/**
 * Number with an optional % (= relative, returned as fraction)
 *
 * @return bool false if there is no number
 */
static inline
bool ui_txt_number(const char** const pos, f32* const out, bool* const relative = NULL) NO_EXCEPT
{
    ui_txt_skip_space(pos);

    const char* p = *pos;
    f64 sign = 1.0;
    if (*p == '-' || *p == '+') {
        sign = *p == '-' ? -1.0 : 1.0;
        ++p;
    }

    if (!isdigit(*p) && !(*p == '.' && isdigit(p[1]))) {
        return false;
    }

    f64 value = 0.0;
    while (isdigit(*p)) {
        value = value * 10.0 + (*p - '0');
        ++p;
    }

    if (*p == '.') {
        ++p;

        f64 factor = 0.1;
        while (isdigit(*p)) {
            value += (*p - '0') * factor;
            factor *= 0.1;
            ++p;
        }
    }

    value *= sign;

    bool is_relative = false;
    if (*p == '%') {
        value /= 100.0;
        is_relative = true;
        ++p;
    }

    if (relative) {
        *relative = is_relative;
    }

    *out = (f32) value;
    *pos = p;

    return true;
}

static inline
bool ui_txt_color(const char* value, uint32* const out) NO_EXCEPT
{
    ui_txt_skip_space(&value);

    if (str_compare(value, "none", 4) == 0 || str_compare(value, "transparent", 11) == 0) {
        *out = 0;

        return true;
    }

    if (*value != '#') {
        return false;
    }

    ++value;

    int32 length = 0;
    uint32 color = 0;
    while (length < 8) {
        const char c = value[length];
        uint32 digit;
        if (c >= '0' && c <= '9') {
            digit = c - '0';
        } else if (c >= 'a' && c <= 'f') {
            digit = c - 'a' + 10;
        } else if (c >= 'A' && c <= 'F') {
            digit = c - 'A' + 10;
        } else {
            break;
        }

        color = (color << 4) | digit;
        ++length;
    }

    if (length == 6) {
        // No alpha -> opaque
        color = (color << 8) | 0xFF;
    } else if (length != 8) {
        return false;
    }

    *out = color;

    return true;
}

static inline
bool ui_txt_bool(const char* value) NO_EXCEPT
{
    ui_txt_skip_space(&value);

    return str_compare(value, "true", 4) == 0
        || str_compare(value, "yes", 3) == 0
        || str_compare(value, "on", 2) == 0
        || *value == '1';
}

/**
 * Splits a list "a | b | c" in place
 * The entries are \0 terminated and have their length (no strlen needed later on)
 *
 * @return int32 Amount of entries
 */
static inline
int32 ui_txt_list(char* value, SimpleString<const char>* const entries, int32 max_entries) NO_EXCEPT
{
    int32 count = 0;
    while (*value && count < max_entries) {
        while (*value == ' ' || *value == '\t') {
            ++value;
        }

        char* const start = value;
        while (*value && *value != '|') {
            ++value;
        }

        // Trim the end
        char* end = value;
        while (end > start && (end[-1] == ' ' || end[-1] == '\t')) {
            --end;
        }

        const bool more = *value == '|';
        *end = '\0';

        const int32 length = (int32) (end - start);
        entries[count].length = length;
        entries[count].max_length = length;
        entries[count].str = start;
        ++count;

        if (!more) {
            break;
        }

        ++value;
    }

    return count;
}

/**
 * @return int32 The id or -1 if the name is unknown ("none" = 0)
 */
static inline
int32 ui_txt_name_id(const UINameId* const names, int32 count, const char* const name) NO_EXCEPT
{
    if (str_compare(name, "none") == 0) {
        return 0;
    }

    for (int32 i = 0; i < count; ++i) {
        if (str_compare(names[i].name, name) == 0) {
            return names[i].id;
        }
    }

    return -1;
}

/**
 * Escape sequences in values: \n = new line, \t = tab, \\ = backslash
 */
static inline
int32 ui_txt_unescape(char* value) NO_EXCEPT
{
    char* const start = value;
    char* out = value;
    while (*value) {
        if (*value == '\\' && value[1]) {
            ++value;
            *out++ = *value == 'n' ? '\n' : (*value == 't' ? '\t' : *value);
            ++value;
        } else {
            *out++ = *value++;
        }
    }

    *out = '\0';

    return (int32) (out - start);
}

/**
 * Language of a document: the language of the theme for theme documents, otherwise the layout language
 */
static inline
const Language* ui_txt_document_language(const UILayoutTxtContext* const ctx, const UIDocument* const doc) NO_EXCEPT
{
    for (int32 i = 0; i < ctx->theme_count; ++i) {
        if (&ctx->themes[i]->document == doc) {
            return &ctx->themes[i]->language;
        }
    }

    return ctx->language;
}

/**
 * Final property value (escape sequences, language strings)
 *      "@key"  = string of the document language
 *      "@@key" = global string (ctx->language_global)
 * A missing string is reported and the value stays "@key" (visible in the ui)
 *
 * @param int32* length Length of the final value in bytes excl. \0 (optional output)
 */
static
char* ui_txt_value_resolve(
    const UILayoutTxtContext* const ctx,
    const UIDocument* const doc,
    UIDocumentString raw,
    MAYBE_UNUSED int32 line,
    char* const buffer, int32 buffer_size,
    int32* const length = NULL
) NO_EXCEPT
{
    ui_document_copy(doc, raw, buffer, buffer_size);

    if (buffer[0] != '@') {
        const int32 unescaped = ui_txt_unescape(buffer);
        if (length) {
            *length = unescaped;
        }

        return buffer;
    }

    const bool global = buffer[1] == '@';
    const char* const key = buffer + (global ? 2 : 1);

    const Language* const language = global ? ctx->language_global : ui_txt_document_language(ctx, doc);
    const SimpleString<const char> str = language_get_string(language, key);

    if (!str.str) {
        LOG_1(
            "[WARNING] UI layout: string %s doesn't exist (line %d)",
            {DATA_TYPE_CHAR_STR, (void *) buffer}, {DATA_TYPE_INT32, &line}
        );

        if (length) {
            *length = min_branched(raw.length, buffer_size - 1);
        }

        return buffer;
    }

    if (str.length >= buffer_size) {
        LOG_1("[WARNING] UI layout: string %s is too long (line %d)", {DATA_TYPE_CHAR_STR, (void *) buffer}, {DATA_TYPE_INT32, &line});
    }

    // The key is in the buffer -> copy after the lookup
    const int32 copy_length = min_branched(str.length, buffer_size - 1);
    memcpy(buffer, str.str, copy_length);
    buffer[copy_length] = '\0';

    if (length) {
        *length = copy_length;
    }

    return buffer;
}

// Final property value (NULL = the property doesn't exist)
static inline
char* ui_txt_value_get(
    const UILayoutTxtContext* const ctx,
    const UIDocument* const doc,
    const UIDocumentNode* const node,
    const char* const key,
    char* const buffer, int32 buffer_size
) NO_EXCEPT
{
    const int32 index = ui_document_property_find(doc, node, key);
    if (!index) {
        return NULL;
    }

    const UIDocumentProperty* const property = &doc->properties[index];

    return ui_txt_value_resolve(ctx, doc, property->value, property->line, buffer, buffer_size);
}

/**
 * Anchor/align points "left|center|right top|center|bottom"
 *
 * @param bool align true = element point (UI_ALIGN_*), false = parent point (UI_ANCHOR_*)
 */
static inline
byte ui_txt_anchor_point(const char** const pos, bool align) NO_EXCEPT
{
    // -1 = not defined, 0 = left/bottom, 1 = center, 2 = right/top
    int32 h = -1;
    int32 v = -1;
    bool center = false;

    char word[16];
    while (ui_txt_word(pos, word, sizeof(word), "/")) {
        if (str_compare(word, "left") == 0) {
            h = 0;
        } else if (str_compare(word, "right") == 0) {
            h = 2;
        } else if (str_compare(word, "top") == 0) {
            v = 2;
        } else if (str_compare(word, "bottom") == 0) {
            v = 0;
        } else if (str_compare(word, "center") == 0) {
            center = true;
        }
    }

    if (center) {
        h = h < 0 ? 1 : h;
        v = v < 0 ? 1 : v;
    }

    byte result = 0;
    if (h == 1) {
        result |= align ? UI_ALIGN_H_CENTER : UI_ANCHOR_H_CENTER;
    } else if (h == 2) {
        result |= align ? UI_ALIGN_H_RIGHT : UI_ANCHOR_H_RIGHT;
    }

    if (v == 1) {
        result |= align ? UI_ALIGN_V_CENTER : UI_ANCHOR_V_CENTER;
    } else if (v == 2) {
        result |= align ? UI_ALIGN_V_TOP : UI_ANCHOR_V_TOP;
    }

    return result;
}

/**
 * "top right" = anchor + align, "top right / top left" = anchor / align
 */
static inline
byte ui_txt_anchor(const char* value) NO_EXCEPT
{
    const char* pos = value;
    const byte anchor = ui_txt_anchor_point(&pos, false);

    ui_txt_skip_space(&pos);
    if (*pos == '/') {
        ++pos;

        return anchor | ui_txt_anchor_point(&pos, true);
    }

    // Same point for the element
    pos = value;

    return anchor | ui_txt_anchor_point(&pos, true);
}

static inline
uint16 ui_txt_flags(const char* value, uint16 flags) NO_EXCEPT
{
    static const UINameId FLAG_NAMES[] = {
        {"hitbox", UI_ELEMENT_FLAG_HITBOX},
        {"interactable", UI_ELEMENT_FLAG_INTERACTABLE},
        {"movable", UI_ELEMENT_FLAG_MOVABLE},
        {"resizable", UI_ELEMENT_FLAG_RESIZABLE},
        {"hoverable", UI_ELEMENT_FLAG_HOVERABLE},
        {"selectable", UI_ELEMENT_FLAG_SELECTABLE},
        {"ignore_hit", UI_ELEMENT_FLAG_IGNORE_HIT},
        {"always_on_top", UI_ELEMENT_FLAG_ALWAYS_ON_TOP},
        {"update_tick", UI_ELEMENT_FLAG_UPDATE_TICK},
    };

    // Without +/- the flags are replaced
    bool replace = false;
    byte add = 0;
    byte remove = 0;

    const char* pos = value;
    char word[32];
    while (ui_txt_word(&pos, word, sizeof(word))) {
        const char* name = word;
        bool is_remove = false;
        if (*name == '+' || *name == '-') {
            is_remove = *name == '-';
            ++name;
        } else {
            replace = true;
        }

        const int32 flag = ui_txt_name_id(FLAG_NAMES, (int32) ARRAY_COUNT(FLAG_NAMES), name);
        if (flag < 0) {
            LOG_1("[WARNING] UI layout: unknown flag %s", {DATA_TYPE_CHAR_STR, (void *) name});
            continue;
        }

        if (is_remove) {
            remove |= (byte) flag;
        } else {
            add |= (byte) flag;
        }
    }

    return (uint16) (((replace ? 0 : flags) | add) & ~remove);
}

////////////////////////////////////////////////////////////////
// Element properties
////////////////////////////////////////////////////////////////

enum UITxtFieldType : byte {
    UI_TXT_FIELD_COLOR,
    UI_TXT_FIELD_F32,
    UI_TXT_FIELD_UINT8,
    UI_TXT_FIELD_INT16,
    UI_TXT_FIELD_BOOL,

    // Function name -> int32 action id (on_click, on_change, ...)
    UI_TXT_FIELD_ACTION,

    // Atlas element name -> UIAttributeImage
    UI_TXT_FIELD_IMAGE,

    // 8 atlas element names (UI_BORDER_TL, T, TR, R, BR, B, BL, L) -> UIAttributeBorder
    UI_TXT_FIELD_BORDER_IMAGE,

    // Width in px of all sides -> UIAttributeBorder::size of the edges
    UI_TXT_FIELD_BORDER_WIDTH,
};

// Property that is stored directly in the element
struct UITxtField {
    const char* name;
    byte type;
    uint16 offset;
};

#define UI_TXT_FIELD(name, type, struct_type, member) { name, type, (uint16) offsetof(struct_type, member) }
#define UI_TXT_FIELD2(name, type, struct_type, member, sub_type, sub_member) \
    { name, type, (uint16) (offsetof(struct_type, member) + offsetof(sub_type, sub_member)) }

#define UI_TXT_FONT_FIELDS(struct_type, member) \
    UI_TXT_FIELD2("font_size", UI_TXT_FIELD_F32, struct_type, member, UIAttributeFont, size), \
    UI_TXT_FIELD2("font_color", UI_TXT_FIELD_COLOR, struct_type, member, UIAttributeFont, color)

#define UI_TXT_BORDER_FIELDS(struct_type, member) \
    UI_TXT_FIELD2("border_color", UI_TXT_FIELD_COLOR, struct_type, member, UIAttributeBorder, color), \
    UI_TXT_FIELD("border_width", UI_TXT_FIELD_BORDER_WIDTH, struct_type, member), \
    UI_TXT_FIELD("border_image", UI_TXT_FIELD_BORDER_IMAGE, struct_type, member)

static const UITxtField UI_TXT_FIELDS_LABEL[] = {
    UI_TXT_FONT_FIELDS(UILabel, font),
};

static const UITxtField UI_TXT_FIELDS_BUTTON[] = {
    UI_TXT_FIELD("background_color", UI_TXT_FIELD_COLOR, UIButton, background_color),
    UI_TXT_FIELD("background_color_hover", UI_TXT_FIELD_COLOR, UIButton, background_color_hover),
    UI_TXT_BORDER_FIELDS(UIButton, border),
    UI_TXT_FIELD("border_color_hover", UI_TXT_FIELD_COLOR, UIButton, border_color_hover),
    UI_TXT_FIELD("image", UI_TXT_FIELD_IMAGE, UIButton, image),
    UI_TXT_FIELD("on_click", UI_TXT_FIELD_ACTION, UIButton, on_click),
};

static const UITxtField UI_TXT_FIELDS_IMAGE[] = {
    UI_TXT_FIELD("image", UI_TXT_FIELD_IMAGE, UIImage, image),
};

static const UITxtField UI_TXT_FIELDS_CURSOR[] = {
    UI_TXT_FIELD("image", UI_TXT_FIELD_IMAGE, UICursor, image),
};

static const UITxtField UI_TXT_FIELDS_WINDOW[] = {
    UI_TXT_FIELD("background_color", UI_TXT_FIELD_COLOR, UIWindow, background_color),
    UI_TXT_BORDER_FIELDS(UIWindow, border),
};

static const UITxtField UI_TXT_FIELDS_PANEL[] = {
    UI_TXT_FIELD("background_color", UI_TXT_FIELD_COLOR, UIPanel, background_color),
    UI_TXT_BORDER_FIELDS(UIPanel, border),
};

static const UITxtField UI_TXT_FIELDS_CHECKBOX[] = {
    UI_TXT_FIELD("box_color", UI_TXT_FIELD_COLOR, UICheckbox, box_color),
    UI_TXT_FIELD("border_color", UI_TXT_FIELD_COLOR, UICheckbox, border_color),
    UI_TXT_FIELD("border_color_hover", UI_TXT_FIELD_COLOR, UICheckbox, border_color_hover),
    UI_TXT_FIELD("check_color", UI_TXT_FIELD_COLOR, UICheckbox, check_color),
    UI_TXT_FIELD("checked", UI_TXT_FIELD_BOOL, UICheckbox, checked),
    UI_TXT_FIELD("group", UI_TXT_FIELD_INT16, UICheckbox, group),
    UI_TXT_FIELD("on_change", UI_TXT_FIELD_ACTION, UICheckbox, on_change),
};

static const UITxtField UI_TXT_FIELDS_SLIDER[] = {
    UI_TXT_FIELD("track_color", UI_TXT_FIELD_COLOR, UISlider, track_color),
    UI_TXT_FIELD("fill_color", UI_TXT_FIELD_COLOR, UISlider, fill_color),
    UI_TXT_FIELD("knob_color", UI_TXT_FIELD_COLOR, UISlider, knob_color),
    UI_TXT_FIELD("knob_color_hover", UI_TXT_FIELD_COLOR, UISlider, knob_color_hover),
    UI_TXT_FIELD("value", UI_TXT_FIELD_F32, UISlider, value),
    UI_TXT_FIELD("step", UI_TXT_FIELD_F32, UISlider, step),
    UI_TXT_FIELD("on_change", UI_TXT_FIELD_ACTION, UISlider, on_change),
};

static const UITxtField UI_TXT_FIELDS_INPUT[] = {
    UI_TXT_FIELD("background_color", UI_TXT_FIELD_COLOR, UIInput, background_color),
    UI_TXT_FIELD("border_color", UI_TXT_FIELD_COLOR, UIInput, border_color),
    UI_TXT_FIELD("border_color_focus", UI_TXT_FIELD_COLOR, UIInput, border_color_focus),
    UI_TXT_FIELD("selection_color", UI_TXT_FIELD_COLOR, UIInput, selection_color),
    UI_TXT_FONT_FIELDS(UIInput, font),
    UI_TXT_FIELD("on_change", UI_TXT_FIELD_ACTION, UIInput, on_change),
    UI_TXT_FIELD("on_submit", UI_TXT_FIELD_ACTION, UIInput, on_submit),
};

static const UITxtField UI_TXT_FIELDS_SELECT[] = {
    UI_TXT_FIELD("background_color", UI_TXT_FIELD_COLOR, UISelect, background_color),
    UI_TXT_FIELD("border_color", UI_TXT_FIELD_COLOR, UISelect, border_color),
    UI_TXT_FIELD("border_color_hover", UI_TXT_FIELD_COLOR, UISelect, border_color_hover),
    UI_TXT_FIELD("option_hover_color", UI_TXT_FIELD_COLOR, UISelect, option_hover_color),
    UI_TXT_FIELD("check_color", UI_TXT_FIELD_COLOR, UISelect, check_color),
    UI_TXT_FONT_FIELDS(UISelect, font),
    UI_TXT_FIELD("on_change", UI_TXT_FIELD_ACTION, UISelect, on_change),
};

static const UITxtField UI_TXT_FIELDS_TAB[] = {
    UI_TXT_FIELD("header_color", UI_TXT_FIELD_COLOR, UITab, header_color),
    UI_TXT_FIELD("tab_color", UI_TXT_FIELD_COLOR, UITab, tab_color),
    UI_TXT_FIELD("tab_color_hover", UI_TXT_FIELD_COLOR, UITab, tab_color_hover),
    UI_TXT_FIELD("tab_color_active", UI_TXT_FIELD_COLOR, UITab, tab_color_active),
    UI_TXT_FIELD("page_color", UI_TXT_FIELD_COLOR, UITab, page_color),
    UI_TXT_FIELD("border_color", UI_TXT_FIELD_COLOR, UITab, border_color),
    UI_TXT_FONT_FIELDS(UITab, font),
    UI_TXT_FIELD("header_size", UI_TXT_FIELD_F32, UITab, header_size),
    UI_TXT_FIELD("tab_size", UI_TXT_FIELD_F32, UITab, tab_size),
    UI_TXT_FIELD("on_change", UI_TXT_FIELD_ACTION, UITab, on_change),
};

static const UITxtField UI_TXT_FIELDS_TABLE[] = {
    UI_TXT_FIELD("header_color", UI_TXT_FIELD_COLOR, UITable, header_color),
    UI_TXT_FIELD("row_color", UI_TXT_FIELD_COLOR, UITable, row_color),
    UI_TXT_FIELD("row_color_alt", UI_TXT_FIELD_COLOR, UITable, row_color_alt),
    UI_TXT_FIELD("row_color_hover", UI_TXT_FIELD_COLOR, UITable, row_color_hover),
    UI_TXT_FIELD("row_color_selected", UI_TXT_FIELD_COLOR, UITable, row_color_selected),
    UI_TXT_FIELD("footer_color", UI_TXT_FIELD_COLOR, UITable, footer_color),
    UI_TXT_FIELD("border_color", UI_TXT_FIELD_COLOR, UITable, border_color),
    UI_TXT_FONT_FIELDS(UITable, font),
    UI_TXT_FIELD("row_height", UI_TXT_FIELD_F32, UITable, row_height),
    UI_TXT_FIELD("on_change", UI_TXT_FIELD_ACTION, UITable, on_change),
};

static const UITxtField UI_TXT_FIELDS_COLOR_PICKER[] = {
    UI_TXT_FIELD("border_color", UI_TXT_FIELD_COLOR, UIColorPicker, border_color),
    UI_TXT_FIELD("border_color_hover", UI_TXT_FIELD_COLOR, UIColorPicker, border_color_hover),
    UI_TXT_FIELD("on_change", UI_TXT_FIELD_ACTION, UIColorPicker, on_change),
};

static inline
const UITxtField* ui_txt_fields(UIElementType type, int32* const count) NO_EXCEPT
{
    #define UI_TXT_FIELDS_RETURN(fields) *count = (int32) ARRAY_COUNT(fields); return fields

    switch (type) {
        case UI_ELEMENT_TYPE_LABEL: { UI_TXT_FIELDS_RETURN(UI_TXT_FIELDS_LABEL); }
        case UI_ELEMENT_TYPE_BUTTON: { UI_TXT_FIELDS_RETURN(UI_TXT_FIELDS_BUTTON); }
        case UI_ELEMENT_TYPE_IMAGE: { UI_TXT_FIELDS_RETURN(UI_TXT_FIELDS_IMAGE); }
        case UI_ELEMENT_TYPE_CURSOR: { UI_TXT_FIELDS_RETURN(UI_TXT_FIELDS_CURSOR); }
        case UI_ELEMENT_TYPE_VIEW_WINDOW: { UI_TXT_FIELDS_RETURN(UI_TXT_FIELDS_WINDOW); }
        case UI_ELEMENT_TYPE_VIEW_PANEL: { UI_TXT_FIELDS_RETURN(UI_TXT_FIELDS_PANEL); }
        case UI_ELEMENT_TYPE_CHECKBOX:
        case UI_ELEMENT_TYPE_RADIO: { UI_TXT_FIELDS_RETURN(UI_TXT_FIELDS_CHECKBOX); }
        case UI_ELEMENT_TYPE_SLIDER: { UI_TXT_FIELDS_RETURN(UI_TXT_FIELDS_SLIDER); }
        case UI_ELEMENT_TYPE_INPUT:
        case UI_ELEMENT_TYPE_TEXTAREA: { UI_TXT_FIELDS_RETURN(UI_TXT_FIELDS_INPUT); }
        case UI_ELEMENT_TYPE_SELECT: { UI_TXT_FIELDS_RETURN(UI_TXT_FIELDS_SELECT); }
        case UI_ELEMENT_TYPE_VIEW_TAB: { UI_TXT_FIELDS_RETURN(UI_TXT_FIELDS_TAB); }
        case UI_ELEMENT_TYPE_TABLE: { UI_TXT_FIELDS_RETURN(UI_TXT_FIELDS_TABLE); }
        case UI_ELEMENT_TYPE_COLOR_PICKER: { UI_TXT_FIELDS_RETURN(UI_TXT_FIELDS_COLOR_PICKER); }
        default: {
            *count = 0;

            return NULL;
        }
    }

    #undef UI_TXT_FIELDS_RETURN
}

/**
 * @return bool false if the value is invalid
 */
static
bool ui_txt_field_set(
    const UILayoutTxtContext* const ctx,
    UICore* const element,
    const UITxtField* const field,
    const char* const value
) NO_EXCEPT
{
    byte* const member = (byte *) element + field->offset;

    switch (field->type) {
        case UI_TXT_FIELD_COLOR: {
            return ui_txt_color(value, (uint32 *) member);
        }
        case UI_TXT_FIELD_F32: {
            const char* pos = value;
            return ui_txt_number(&pos, (f32 *) member);
        }
        case UI_TXT_FIELD_UINT8: {
            const char* pos = value;
            f32 number = 0.0f;
            if (!ui_txt_number(&pos, &number)) {
                return false;
            }

            *member = (uint8) number;

            return true;
        }
        case UI_TXT_FIELD_INT16: {
            const char* pos = value;
            f32 number = 0.0f;
            if (!ui_txt_number(&pos, &number)) {
                return false;
            }

            *((int16 *) member) = (int16) number;

            return true;
        }
        case UI_TXT_FIELD_BOOL: {
            *((bool *) member) = ui_txt_bool(value);

            return true;
        }
        case UI_TXT_FIELD_ACTION: {
            const int32 id = ui_txt_name_id(ctx->actions, ctx->action_count, value);
            if (id < 0) {
                LOG_1("[WARNING] UI layout: unknown action %s", {DATA_TYPE_CHAR_STR, (void *) value});

                return false;
            }

            *((int32 *) member) = id;

            return true;
        }
        case UI_TXT_FIELD_IMAGE: {
            const int32 id = ui_txt_name_id(ctx->images, ctx->image_count, value);
            if (id < 0 || !ctx->atlas) {
                LOG_1("[WARNING] UI layout: unknown image %s", {DATA_TYPE_CHAR_STR, (void *) value});

                return false;
            }

            ui_image_attribute_from_atlas(ctx->atlas, id, (UIAttributeImage *) member);

            return true;
        }
        case UI_TXT_FIELD_BORDER_IMAGE: {
            int32 ids[UI_BORDER_SIZE];

            const char* pos = value;
            char name[64];
            for (int32 i = 0; i < UI_BORDER_SIZE; ++i) {
                if (!ui_txt_word(&pos, name, sizeof(name))) {
                    LOG_1("[WARNING] UI layout: border_image needs 8 images (tl t tr r br b bl l)");

                    return false;
                }

                ids[i] = ui_txt_name_id(ctx->images, ctx->image_count, name);
                if (ids[i] < 0 || !ctx->atlas) {
                    LOG_1("[WARNING] UI layout: unknown image %s", {DATA_TYPE_CHAR_STR, (void *) name});

                    return false;
                }
            }

            ui_border_attribute_from_atlas(ctx->atlas, ids, (UIAttributeBorder *) member);

            return true;
        }
        case UI_TXT_FIELD_BORDER_WIDTH: {
            const char* pos = value;
            f32 number = 0.0f;
            if (!ui_txt_number(&pos, &number)) {
                return false;
            }

            ui_border_width_set((UIAttributeBorder *) member, (uint8) number);

            return true;
        }
        default:
            UNREACHABLE();
    }
}

/**
 * Keys that are only used when the element is created
 */
static inline
bool ui_txt_is_create_key(const char* const type, const char* const key) NO_EXCEPT
{
    if (str_compare(key, "style") == 0) {
        return true;
    }

    if (str_compare(type, "panel") == 0) {
        return str_compare(key, "scroll") == 0 || str_compare(key, "popup") == 0;
    } else if (str_compare(type, "input") == 0 || str_compare(type, "textarea") == 0) {
        return str_compare(key, "max_length") == 0;
    } else if (str_compare(type, "select") == 0) {
        return str_compare(key, "options") == 0 || str_compare(key, "search") == 0 || str_compare(key, "multi") == 0;
    } else if (str_compare(type, "tab") == 0) {
        return str_compare(key, "tabs") == 0 || str_compare(key, "orientation") == 0;
    } else if (str_compare(type, "table") == 0) {
        return str_compare(key, "columns") == 0 || str_compare(key, "row_capacity") == 0 || str_compare(key, "cell_capacity") == 0;
    } else if (str_compare(type, "colorpicker") == 0) {
        return str_compare(key, "type") == 0;
    } else if (str_compare(type, "tooltip") == 0) {
        return str_compare(key, "font_size") == 0;
    } else if (str_compare(type, "button") == 0 || str_compare(type, "checkbox") == 0 || str_compare(type, "radio") == 0) {
        return str_compare(key, "text") == 0;
    } else if (str_compare(type, "data") == 0) {
        return str_compare(key, "size") == 0 || str_compare(key, "version") == 0;
    }

    return false;
}

// Makes sure the label can hold length characters (incl. \0) without re-allocating later on
static inline
void ui_txt_label_reserve(UILayout* const layout, UILabel* const label, int32 length) NO_EXCEPT
{
    if (length <= 0 || (label->content && ui_string_max_length(layout, label->content) >= length - 1)) {
        return;
    }

    const int32 offset = ui_string_reserve(layout, label->char_type, length);
    if (label->content) {
        // Characters incl. \0
        const int32 old_length = ui_string_length(layout, label->content);
        memcpy(
            layout->ui_element_buffer.memory + offset,
            layout->ui_element_buffer.memory + label->content,
            (size_t) (old_length + 1) * char_type_size(label->char_type)
        );
        ui_string_length_set(layout, offset, old_length);
    }

    label->content = offset;
}

/**
 * Applies one property to an element
 *
 * @return bool false if the key is unknown or the value invalid
 */
static
bool ui_txt_property_apply(
    const UILayoutTxtContext* const ctx,
    UILayout* const layout,
    UICore* const element,
    const char* const key,
    char* const value, int32 value_length
) NO_EXCEPT
{
    UIAttributeDimension* const dim = &element->dimension;

    // Core properties
    if (str_compare(key, "pos") == 0 || str_compare(key, "size") == 0) {
        const bool is_pos = key[0] == 'p';

        const char* pos = value;
        f32 x = 0.0f;
        f32 y = 0.0f;
        bool rel_x = false;
        bool rel_y = false;
        if (!ui_txt_number(&pos, &x, &rel_x) || !ui_txt_number(&pos, &y, &rel_y)) {
            return false;
        }

        const byte flag_x = is_pos ? UI_DIMENSION_POS_X_RELATIVE : UI_DIMENSION_DIM_X_RELATIVE;
        const byte flag_y = is_pos ? UI_DIMENSION_POS_Y_RELATIVE : UI_DIMENSION_DIM_Y_RELATIVE;
        dim->flag = (byte) ((dim->flag & ~(flag_x | flag_y)) | (rel_x ? flag_x : 0) | (rel_y ? flag_y : 0));

        if (is_pos) {
            dim->pos_raw = {x, y};
        } else {
            dim->dim_raw = {x, y};
        }

        return true;
    } else if (str_compare(key, "anchor") == 0) {
        dim->align_anchor = ui_txt_anchor(value);

        return true;
    } else if (str_compare(key, "flags") == 0) {
        element->flags = ui_txt_flags(value, element->flags);

        return true;
    } else if (str_compare(key, "hidden") == 0) {
        element->state = ui_txt_bool(value)
            ? (byte) (element->state | UI_STATE_HIDDEN)
            : (byte) (element->state & ~UI_STATE_HIDDEN);

        return true;
    } else if (str_compare(key, "update") == 0 || str_compare(key, "render") == 0) {
        const bool is_update = key[0] == 'u';
        const int32 id = is_update
            ? ui_txt_name_id(ctx->update_functions, ctx->update_function_count, value)
            : ui_txt_name_id(ctx->render_functions, ctx->render_function_count, value);

        if (id < 0) {
            LOG_1("[WARNING] UI layout: unknown function %s", {DATA_TYPE_CHAR_STR, (void *) value});

            return false;
        }

        if (is_update) {
            element->update_func = (int16) id;
        } else {
            element->render_func = (int16) id;
        }

        return true;
    }

    // Fields stored in the element
    int32 field_count;
    const UITxtField* const fields = ui_txt_fields(element->type, &field_count);
    for (int32 i = 0; i < field_count; ++i) {
        if (str_compare(fields[i].name, key) == 0) {
            return ui_txt_field_set(ctx, element, &fields[i], value);
        }
    }

    // Type specific properties
    switch (element->type) {
        case UI_ELEMENT_TYPE_LABEL: {
            UILabel* const label = (UILabel *) element;
            if (str_compare(key, "text") == 0) {
                ui_label_set_text(layout, label, value, value_length);

                return true;
            } else if (str_compare(key, "pattern") == 0) {
                ui_label_set_pattern(layout, label, value, value_length);

                return true;
            } else if (str_compare(key, "capacity") == 0) {
                ui_txt_label_reserve(layout, label, (int32) str_to_int(value));

                return true;
            }
        } break;
        case UI_ELEMENT_TYPE_BUTTON:
        case UI_ELEMENT_TYPE_CHECKBOX:
        case UI_ELEMENT_TYPE_RADIO: {
            // The text is the child label (created together with the element)
            if (str_compare(key, "text") == 0) {
                const int32 label = element->type == UI_ELEMENT_TYPE_BUTTON
                    ? ((UIButton *) element)->label
                    : ((UICheckbox *) element)->label;

                if (!label) {
                    return false;
                }

                ui_label_set_text(layout, (UILabel *) ui_element_get(layout, label), value, value_length);

                return true;
            }
        } break;
        case UI_ELEMENT_TYPE_VIEW_WINDOW: {
            UIWindow* const window = (UIWindow *) element;
            if (str_compare(key, "title") == 0) {
                ui_window_set_title(layout, window, value);

                return true;
            } else if (str_compare(key, "resizable") == 0) {
                const char* pos = value;
                v2_f32 min_dim = {160.0f, 100.0f};
                ui_txt_number(&pos, &min_dim.x);
                ui_txt_number(&pos, &min_dim.y);
                ui_window_set_resizable(layout, window, min_dim);

                return true;
            }
        } break;
        case UI_ELEMENT_TYPE_VIEW_PANEL: {
            UIPanel* const panel = (UIPanel *) element;
            if (str_compare(key, "close_on_click") == 0) {
                panel->panel_flags = ui_txt_bool(value)
                    ? (byte) (panel->panel_flags | UI_PANEL_FLAG_CLOSE_ON_CLICK)
                    : (byte) (panel->panel_flags & ~UI_PANEL_FLAG_CLOSE_ON_CLICK);

                return true;
            }
        } break;
        case UI_ELEMENT_TYPE_INPUT:
        case UI_ELEMENT_TYPE_TEXTAREA: {
            UIInput* const input = (UIInput *) element;
            if (str_compare(key, "text") == 0) {
                ui_input_set_text(layout, input, value, value_length);

                return true;
            } else if (str_compare(key, "placeholder") == 0) {
                ui_input_set_placeholder(layout, input, value, value_length);

                return true;
            }
        } break;
        case UI_ELEMENT_TYPE_SELECT: {
            if (str_compare(key, "selected") == 0) {
                // Single select: index, multi select: list of indices
                const char* pos = value;
                f32 index = 0.0f;
                while (ui_txt_number(&pos, &index)) {
                    ui_select_set(layout, (UISelect *) element, (int32) index);
                }

                return true;
            }
        } break;
        case UI_ELEMENT_TYPE_VIEW_TAB: {
            if (str_compare(key, "active") == 0) {
                ui_tab_set_active(layout, (UITab *) element, (int32) str_to_int(value));

                return true;
            }
        } break;
        case UI_ELEMENT_TYPE_TABLE: {
            UITable* const table = (UITable *) element;
            if (str_compare(key, "widths") == 0) {
                const char* pos = value;
                f32 width = 0.0f;
                for (int32 c = 0; c < table->column_count && ui_txt_number(&pos, &width); ++c) {
                    ui_table_set_column_width(layout, table, c, width);
                }

                return true;
            } else if (str_compare(key, "row") == 0 || str_compare(key, "footer") == 0) {
                SimpleString<const char> cells[UI_TXT_LIST_MAX] = {};
                const int32 count = ui_txt_list(value, cells, (int32) ARRAY_COUNT(cells));
                for (int32 c = count; c < table->column_count && c < UI_TXT_LIST_MAX; ++c) {
                    cells[c].str = "";
                }

                if (key[0] == 'r') {
                    ui_table_add_row(layout, table, cells);
                } else {
                    ui_table_set_footer(layout, table, cells);
                }

                return true;
            }
        } break;
        case UI_ELEMENT_TYPE_COLOR_PICKER: {
            if (str_compare(key, "color") == 0) {
                uint32 color = 0;
                if (!ui_txt_color(value, &color)) {
                    return false;
                }

                ui_color_picker_set(layout, (UIColorPicker *) element, color);

                return true;
            }
        } break;
        default: {}
    }

    return false;
}

////////////////////////////////////////////////////////////////
// Element creation
////////////////////////////////////////////////////////////////

static
void ui_txt_node_apply(
    const UILayoutTxtContext* const ctx,
    UILayout* const layout,
    UICore* const element,
    const UIDocument* const doc,
    const UIDocumentNode* const node,
    const char* const type,
    int32 depth
) NO_EXCEPT;

static
UICore* ui_txt_element_create(
    const UILayoutTxtContext* const ctx,
    UILayout* const layout,
    const UIDocument* const doc,
    const UIDocumentNode* const node,
    UICore* const parent
) NO_EXCEPT;

/**
 * Fixed child element ("$part") of an element
 */
static
UICore* ui_txt_part(UILayout* const layout, UICore* const element, const char* const part, int32 index) NO_EXCEPT
{
    switch (element->type) {
        case UI_ELEMENT_TYPE_VIEW_WINDOW: {
            const UIWindow* const window = (const UIWindow *) element;
            if (str_compare(part, "title") == 0) {
                return window->title ? ui_element_get(layout, window->title) : NULL;
            } else if (str_compare(part, "title_label") == 0) {
                return window->title_label ? ui_element_get(layout, window->title_label) : NULL;
            } else if (str_compare(part, "close") == 0) {
                return window->button_close ? ui_element_get(layout, window->button_close) : NULL;
            } else if (str_compare(part, "resize_grip") == 0) {
                return window->resize_grip ? ui_element_get(layout, window->resize_grip) : NULL;
            }
        } break;
        case UI_ELEMENT_TYPE_BUTTON: {
            const UIButton* const button = (const UIButton *) element;
            if (str_compare(part, "label") == 0) {
                return button->label ? ui_element_get(layout, button->label) : NULL;
            }
        } break;
        case UI_ELEMENT_TYPE_CHECKBOX:
        case UI_ELEMENT_TYPE_RADIO: {
            const UICheckbox* const checkbox = (const UICheckbox *) element;
            if (str_compare(part, "label") == 0) {
                return checkbox->label ? ui_element_get(layout, checkbox->label) : NULL;
            }
        } break;
        case UI_ELEMENT_TYPE_VIEW_PANEL: {
            UIPanel* const panel = (UIPanel *) element;
            if (str_compare(part, "content") == 0) {
                return ui_panel_content(layout, panel);
            }
        } break;
        case UI_ELEMENT_TYPE_VIEW_TAB: {
            const UITab* const tab = (const UITab *) element;
            if (str_compare(part, "page") == 0 && index >= 0 && index < tab->tab_count) {
                return &ui_tab_page(layout, tab, index)->core;
            }
        } break;
        default: {}
    }

    return NULL;
}

// Where the child elements of an element are added
static inline
UICore* ui_txt_children_parent(UILayout* const layout, UICore* const element) NO_EXCEPT
{
    if (element->type == UI_ELEMENT_TYPE_VIEW_PANEL
        && (((const UIPanel *) element)->panel_flags & (UI_PANEL_FLAG_SCROLL_X | UI_PANEL_FLAG_SCROLL_Y))
    ) {
        return ui_panel_content(layout, (UIPanel *) element);
    }

    return element;
}

/**
 * Applies a style of the themes (in theme order) incl. its parts
 *
 * @param const char* name Style name or "@type"
 */
static
void ui_txt_style_apply(
    const UILayoutTxtContext* const ctx,
    UILayout* const layout,
    UICore* const element,
    const char* const name,
    int32 depth
) NO_EXCEPT
{
    if (depth >= UI_TXT_STYLE_DEPTH) {
        LOG_1("[WARNING] UI layout: styles nested too deep (%s)", {DATA_TYPE_CHAR_STR, (void *) name});

        return;
    }

    bool found = false;
    for (int32 i = 0; i < ctx->theme_count; ++i) {
        const UITheme* const theme = ctx->themes[i];
        const int32 index = ui_theme_style(theme, name);
        if (!index) {
            continue;
        }

        found = true;
        ui_txt_node_apply(ctx, layout, element, &theme->document, &theme->document.nodes[index], "", depth + 1);
    }

    if (!found && name[0] != '@') {
        LOG_1("[WARNING] UI layout: unknown style %s", {DATA_TYPE_CHAR_STR, (void *) name});
    }
}

// Applies the styles listed in "style: a b c"
static inline
void ui_txt_styles_apply(
    const UILayoutTxtContext* const ctx,
    UILayout* const layout,
    UICore* const element,
    const UIDocument* const doc,
    const UIDocumentNode* const node,
    int32 depth
) NO_EXCEPT
{
    char value[256];
    if (!ui_txt_value_get(ctx, doc, node, "style", value, sizeof(value))) {
        return;
    }

    const char* pos = value;
    char name[HASH_MAP_MAX_KEY_LENGTH];
    while (ui_txt_word(&pos, name, sizeof(name))) {
        ui_txt_style_apply(ctx, layout, element, name, depth);
    }
}

/**
 * Applies the properties + parts of a node (layout element or style) to an element
 *
 * @param const char* type Element type name of the node in a layout file (create keys are skipped), "" for styles
 */
static
void ui_txt_node_apply(
    const UILayoutTxtContext* const ctx,
    UILayout* const layout,
    UICore* const element,
    const UIDocument* const doc,
    const UIDocumentNode* const node,
    const char* const type,
    int32 depth
) NO_EXCEPT
{
    // Styles of a style ("inheritance"), the styles of layout elements are applied by ui_txt_element_create
    if (!*type) {
        ui_txt_styles_apply(ctx, layout, element, doc, node, depth);
    }

    char key[64];
    char value[UI_TXT_VALUE_LENGTH];

    for (int32 i = node->first_property; i; i = doc->properties[i].next) {
        const UIDocumentProperty* const property = &doc->properties[i];
        ui_document_copy(doc, property->key, key, sizeof(key));

        if (str_compare(key, "style") == 0 || (*type && ui_txt_is_create_key(type, key))) {
            continue;
        }

        if (property->value.length >= UI_TXT_VALUE_LENGTH) {
            LOG_1("[WARNING] UI layout: value too long in line %d", {DATA_TYPE_INT32, (int32 *) &property->line});
        }

        int32 value_length;
        ui_txt_value_resolve(ctx, doc, property->value, property->line, value, UI_TXT_VALUE_LENGTH, &value_length);

        if (!ui_txt_property_apply(ctx, layout, element, key, value, value_length)) {
            LOG_1(
                "[WARNING] UI layout: invalid property %s in line %d",
                {DATA_TYPE_CHAR_STR, (void *) key}, {DATA_TYPE_INT32, (int32 *) &property->line}
            );
        }
    }

    // Parts + child elements of parts
    for (int32 i = node->first_child; i; i = doc->nodes[i].next_sibling) {
        const UIDocumentNode* const child = &doc->nodes[i];
        if (child->kind != UI_DOCUMENT_NODE_PART) {
            continue;
        }

        char part_name[32];
        ui_document_copy(doc, child->type, part_name, sizeof(part_name));

        char index_str[16];
        ui_document_copy(doc, child->name, index_str, sizeof(index_str));
        const int32 index = child->name.length ? (int32) str_to_int(index_str) : -1;

        UICore* const part = ui_txt_part(layout, element, part_name, index);
        if (!part) {
            LOG_1(
                "[WARNING] UI layout: unknown part %s in line %d",
                {DATA_TYPE_CHAR_STR, (void *) part_name}, {DATA_TYPE_INT32, (int32 *) &child->line}
            );

            continue;
        }

        // Styles of the part (in a style the styles are applied by ui_txt_node_apply)
        if (*type) {
            ui_txt_styles_apply(ctx, layout, part, doc, child, depth);
        }

        // Parts are never created from the file -> no create keys
        ui_txt_node_apply(ctx, layout, part, doc, child, *type ? "part" : "", depth + 1);

        // Child elements of the part (only in layout files)
        if (*type) {
            UICore* const children_parent = ui_txt_children_parent(layout, part);
            for (int32 j = child->first_child; j; j = doc->nodes[j].next_sibling) {
                if (doc->nodes[j].kind == UI_DOCUMENT_NODE_BLOCK) {
                    ui_txt_element_create(ctx, layout, doc, &doc->nodes[j], children_parent);
                }
            }
        }
    }
}

// @return -1 if the type is unknown
static inline
int32 ui_txt_element_type(const char* const type) NO_EXCEPT
{
    static const UINameId TYPE_NAMES[] = {
        {"label", UI_ELEMENT_TYPE_LABEL},
        {"button", UI_ELEMENT_TYPE_BUTTON},
        {"image", UI_ELEMENT_TYPE_IMAGE},
        {"cursor", UI_ELEMENT_TYPE_CURSOR},
        {"window", UI_ELEMENT_TYPE_VIEW_WINDOW},
        {"panel", UI_ELEMENT_TYPE_VIEW_PANEL},
        {"tooltip", UI_ELEMENT_TYPE_VIEW_PANEL},
        {"custom", UI_ELEMENT_TYPE_CUSTOM},
        {"checkbox", UI_ELEMENT_TYPE_CHECKBOX},
        {"radio", UI_ELEMENT_TYPE_RADIO},
        {"slider", UI_ELEMENT_TYPE_SLIDER},
        {"input", UI_ELEMENT_TYPE_INPUT},
        {"textarea", UI_ELEMENT_TYPE_TEXTAREA},
        {"select", UI_ELEMENT_TYPE_SELECT},
        {"tab", UI_ELEMENT_TYPE_VIEW_TAB},
        {"table", UI_ELEMENT_TYPE_TABLE},
        {"colorpicker", UI_ELEMENT_TYPE_COLOR_PICKER},
        {"data", UI_ELEMENT_TYPE_DATA},
    };

    // "none" would be 0 = UI_ELEMENT_TYPE_BUTTON
    return str_compare(type, "none") == 0
        ? -1
        : ui_txt_name_id(TYPE_NAMES, (int32) ARRAY_COUNT(TYPE_NAMES), type);
}

/**
 * Creates an element (incl. children) from a layout node
 *
 * @return UICore* NULL if the element type is unknown
 */
static
UICore* ui_txt_element_create(
    const UILayoutTxtContext* const ctx,
    UILayout* const layout,
    const UIDocument* const doc,
    const UIDocumentNode* const node,
    UICore* const parent
) NO_EXCEPT
{
    char type[32];
    ui_document_copy(doc, node->type, type, sizeof(type));

    const int32 element_type_id = ui_txt_element_type(type);
    if (element_type_id < 0) {
        LOG_1(
            "[WARNING] UI layout: unknown element type %s in line %d",
            {DATA_TYPE_CHAR_STR, (void *) type}, {DATA_TYPE_INT32, (int32 *) &node->line}
        );

        return NULL;
    }

    const UIElementType element_type = (UIElementType) element_type_id;

    char value[UI_TXT_VALUE_LENGTH];
    SimpleString<const char> list[UI_TXT_LIST_MAX] = {};
    UICore* element = NULL;

    // Create arguments
    switch (element_type) {
        case UI_ELEMENT_TYPE_LABEL: {
            // The text memory is reserved directly after the label
            int32 pattern_length = 0;
            if (ui_txt_value_get(ctx, doc, node, "pattern", value, UI_TXT_VALUE_LENGTH)) {
                pattern_length = (int32) str_length(value) + 1;
            }

            int32 content_length = 0;
            if (ui_txt_value_get(ctx, doc, node, "text", value, UI_TXT_VALUE_LENGTH)) {
                content_length = (int32) str_length(value) + 1;
            }

            if (ui_txt_value_get(ctx, doc, node, "capacity", value, UI_TXT_VALUE_LENGTH)) {
                content_length = max_branched(content_length, (int32) str_to_int(value));
            }

            element = &ui_label_create(layout, CHAR_TYPE_CHAR, pattern_length, content_length, parent)->core;
        } break;
        case UI_ELEMENT_TYPE_BUTTON: {
            const char* const text = ui_txt_value_get(ctx, doc, node, "text", value, UI_TXT_VALUE_LENGTH);
            element = &ui_button_create(layout, text, parent)->core;
        } break;
        case UI_ELEMENT_TYPE_IMAGE: {
            element = &ui_image_create(layout, parent)->core;
        } break;
        case UI_ELEMENT_TYPE_CURSOR: {
            element = &ui_cursor_create(layout, parent)->core;
        } break;
        case UI_ELEMENT_TYPE_VIEW_WINDOW: {
            element = &ui_window_create(layout, parent)->core;
        } break;
        case UI_ELEMENT_TYPE_VIEW_PANEL: {
            if (str_compare(type, "tooltip") == 0) {
                f32 font_size = UI_DEFAULT_FONT_SIZE;
                if (ui_txt_value_get(ctx, doc, node, "font_size", value, UI_TXT_VALUE_LENGTH)) {
                    const char* pos = value;
                    ui_txt_number(&pos, &font_size);
                }

                // Tooltips are always on the top level
                element = &ui_tooltip_create(layout, font_size)->core;
            } else if (ui_txt_value_get(ctx, doc, node, "popup", value, UI_TXT_VALUE_LENGTH) && ui_txt_bool(value)) {
                // Popups are always on the top level
                element = &ui_popup_create(layout)->core;
            } else if (ui_txt_value_get(ctx, doc, node, "scroll", value, UI_TXT_VALUE_LENGTH)) {
                byte flags = 0;
                flags |= str_find(value, 'x') ? UI_PANEL_FLAG_SCROLL_X : 0;
                flags |= str_find(value, 'y') ? UI_PANEL_FLAG_SCROLL_Y : 0;

                element = &ui_scroll_panel_create(layout, flags, parent)->core;
            } else {
                element = &ui_panel_create(layout, parent)->core;
            }
        } break;
        case UI_ELEMENT_TYPE_CUSTOM: {
            element = ui_custom_create(layout, 0, 0, parent);
        } break;
        case UI_ELEMENT_TYPE_CHECKBOX: {
            const char* const text = ui_txt_value_get(ctx, doc, node, "text", value, UI_TXT_VALUE_LENGTH);
            element = &ui_checkbox_create(layout, text, parent)->core;
        } break;
        case UI_ELEMENT_TYPE_RADIO: {
            char group[16];
            const int16 group_id = ui_txt_value_get(ctx, doc, node, "group", group, sizeof(group))
                ? (int16) str_to_int(group)
                : 0;

            const char* const text = ui_txt_value_get(ctx, doc, node, "text", value, UI_TXT_VALUE_LENGTH);
            element = &ui_radio_create(layout, text, group_id, parent)->core;
        } break;
        case UI_ELEMENT_TYPE_SLIDER: {
            f32 slider_value = 0.0f;
            if (ui_txt_value_get(ctx, doc, node, "value", value, UI_TXT_VALUE_LENGTH)) {
                const char* pos = value;
                ui_txt_number(&pos, &slider_value);
            }

            element = &ui_slider_create(layout, slider_value, parent)->core;
        } break;
        case UI_ELEMENT_TYPE_INPUT:
        case UI_ELEMENT_TYPE_TEXTAREA: {
            int32 max_length = 64;
            if (ui_txt_value_get(ctx, doc, node, "max_length", value, UI_TXT_VALUE_LENGTH)) {
                max_length = (int32) str_to_int(value);
            }

            element = element_type == UI_ELEMENT_TYPE_TEXTAREA
                ? &ui_textarea_create(layout, max_length, parent)->core
                : &ui_input_create(layout, max_length, parent)->core;
        } break;
        case UI_ELEMENT_TYPE_SELECT: {
            byte flags = 0;
            char flag_value[16];
            if (ui_txt_value_get(ctx, doc, node, "search", flag_value, sizeof(flag_value)) && ui_txt_bool(flag_value)) {
                flags |= UI_SELECT_FLAG_SEARCH;
            }

            if (ui_txt_value_get(ctx, doc, node, "multi", flag_value, sizeof(flag_value)) && ui_txt_bool(flag_value)) {
                flags |= UI_SELECT_FLAG_MULTI;
            }

            int32 count = 0;
            if (ui_txt_value_get(ctx, doc, node, "options", value, UI_TXT_VALUE_LENGTH)) {
                count = ui_txt_list(value, list, UI_TXT_LIST_MAX);
            }

            element = &ui_select_create(layout, list, count, parent, flags)->core;
        } break;
        case UI_ELEMENT_TYPE_VIEW_TAB: {
            char orientation[16];
            const UITabOrientation tab_orientation =
                ui_txt_value_get(ctx, doc, node, "orientation", orientation, sizeof(orientation))
                    && str_compare(orientation, "vertical") == 0
                ? UI_TAB_ORIENTATION_VERTICAL
                : UI_TAB_ORIENTATION_HORIZONTAL;

            int32 count = 0;
            if (ui_txt_value_get(ctx, doc, node, "tabs", value, UI_TXT_VALUE_LENGTH)) {
                count = ui_txt_list(value, list, UI_TXT_LIST_MAX);
            }

            element = &ui_tab_create(layout, list, count, tab_orientation, parent)->core;
        } break;
        case UI_ELEMENT_TYPE_TABLE: {
            char number[16];
            const int32 row_capacity = ui_txt_value_get(ctx, doc, node, "row_capacity", number, sizeof(number))
                ? (int32) str_to_int(number)
                : 32;

            const int32 cell_capacity = ui_txt_value_get(ctx, doc, node, "cell_capacity", number, sizeof(number))
                ? (int32) str_to_int(number)
                : 0;

            int32 count = 0;
            if (ui_txt_value_get(ctx, doc, node, "columns", value, UI_TXT_VALUE_LENGTH)) {
                count = ui_txt_list(value, list, UI_TXT_LIST_MAX);
            }

            element = &ui_table_create(layout, list, count, row_capacity, parent, cell_capacity)->core;
        } break;
        case UI_ELEMENT_TYPE_COLOR_PICKER: {
            char picker_type[16];
            const UIColorPickerType type_id =
                ui_txt_value_get(ctx, doc, node, "type", picker_type, sizeof(picker_type))
                    && str_compare(picker_type, "palette") == 0
                ? UI_COLOR_PICKER_TYPE_PALETTE
                : UI_COLOR_PICKER_TYPE_GRADIENT;

            element = &ui_color_picker_create(layout, type_id, parent)->core;
        } break;
        case UI_ELEMENT_TYPE_DATA: {
            // Only the memory is reserved, the content is defined by the application
            char number[16];
            const int32 size = ui_txt_value_get(ctx, doc, node, "size", number, sizeof(number))
                ? (int32) str_to_int(number)
                : 0;

            const int32 version = ui_txt_value_get(ctx, doc, node, "version", number, sizeof(number))
                ? (int32) str_to_int(number)
                : 0;

            if (!node->name.length) {
                LOG_1("[WARNING] UI layout: data without name in line %d (can't be saved)", {DATA_TYPE_INT32, (int32 *) &node->line});
            }

            element = &ui_data_create(
                layout, doc->text + node->name.offset, node->name.length,
                OMS_MAX(size, 0), version, ctx->data_file, parent
            )->core;
        } break;
        default:
            UNREACHABLE();
    }

    if (node->name.length) {
        if (node->name.length >= HASH_MAP_MAX_KEY_LENGTH) {
            LOG_1("[WARNING] UI layout: element name too long in line %d", {DATA_TYPE_INT32, (int32 *) &node->line});
        } else {
            ui_layout_name_element(layout, doc->text + node->name.offset, node->name.length, element);
        }
    }

    // Type default styles, element styles, own properties + parts
    char type_style[34];
    type_style[0] = '@';
    str_copy(type_style + 1, type);
    ui_txt_style_apply(ctx, layout, element, type_style, 0);
    ui_txt_styles_apply(ctx, layout, element, doc, node, 0);
    ui_txt_node_apply(ctx, layout, element, doc, node, type, 0);

    // Children
    UICore* const children_parent = ui_txt_children_parent(layout, element);
    for (int32 i = node->first_child; i; i = doc->nodes[i].next_sibling) {
        if (doc->nodes[i].kind == UI_DOCUMENT_NODE_BLOCK) {
            ui_txt_element_create(ctx, layout, doc, &doc->nodes[i], children_parent);
        }
    }

    return element;
}

/**
 * Creates the elements of a parsed layout (text file or binary layout)
 *
 * @param UICore* parent The elements are added to this element (NULL = root), the root must already exist
 *
 * @return int32 Amount of created top level elements, -1 on error
 */
int32 ui_layout_from_document(
    UILayout* const __restrict layout,
    const UIDocument* const __restrict doc,
    const UILayoutTxtContext* const __restrict ctx,
    UICore* parent = NULL
) NO_EXCEPT
{
    if (!layout->ui_root) {
        LOG_1("[ERROR] UI layout: the root has to be created first");

        return -1;
    }

    if (doc->version != UI_LAYOUT_TXT_VERSION) {
        LOG_1("[WARNING] UI layout: file version %d is not supported", {DATA_TYPE_INT32, (void *) &doc->version});
    }

    int32 count = 0;
    for (int32 i = doc->first_node; i; i = doc->nodes[i].next_sibling) {
        const UIDocumentNode* const node = &doc->nodes[i];
        if (node->kind != UI_DOCUMENT_NODE_BLOCK) {
            continue;
        }

        if (ui_txt_element_create(ctx, layout, doc, node, parent)) {
            ++count;
        }
    }

    layout->dirty |= UI_LAYOUT_DIRTY;

    return count;
}

/**
 * Creates the elements of a layout file
 *
 * @param UICore*       parent  The elements are added to this element (NULL = root), the root must already exist
 * @param BufferMemory* mem     Temporary memory for the parsed document
 *
 * @return int32 Amount of created top level elements, -1 on error
 */
int32 ui_layout_from_txt(
    UILayout* const __restrict layout,
    const char* __restrict text, int32 length,
    const UILayoutTxtContext* const __restrict ctx,
    BufferMemory* const __restrict mem,
    UICore* parent = NULL
) NO_EXCEPT
{
    PROFILE_DEBUG(PROFILE_UI_LAYOUT_FROM_TXT, (char *) NULL, PROFILE_FLAG_SHOULD_LOG);

    LOG_1("[INFO] UI create layout from text");

    const int32 line_count = ui_document_line_count(text, length);
    byte* const memory = memory_get(mem, ui_document_memory_size(line_count), sizeof(size_t));

    UIDocument doc;
    ui_document_parse(&doc, text, length, memory);

    return ui_layout_from_document(layout, &doc, ctx, parent);
}

/**
 * Creates the layout from a text file (e.g. used by the theme builder)
 * The elements are created as children of a newly created root (the root size is defined at runtime)
 * Without a context no styles, images and functions are resolved
 *
 * WARNING: layout->data needs to have memory already reserved
 */
void layout_from_file_txt(
    UILayout* const __restrict layout,
    const char* __restrict path,
    RingMemory* const ring,
    const UILayoutTxtContext* ctx = NULL
) NO_EXCEPT
{
    FileBody file = {};
    file_read(path, &file, ring);
    ASSERT_TRUE(file.size);

    if (!layout->ui_element_buffer.memory) {
        const int32 size = 2 * MEGABYTE;
        buffer_init(&layout->ui_element_buffer, memory_get(ring, size + 64, 64), size);
    }

    ui_layout_reset(layout);
    ui_root_create(layout, 0.0f, 0.0f);

    // Temporary memory for the document
    const int32 line_count = ui_document_line_count((const char *) file.content, (int32) file.size);
    const int32 doc_size = ui_document_memory_size(line_count) + 64;
    BufferMemory doc_memory = {};
    buffer_init(&doc_memory, memory_get(ring, doc_size + 64, 64), doc_size);

    const UILayoutTxtContext empty_ctx = {};
    ui_layout_from_txt(layout, (const char *) file.content, (int32) file.size, ctx ? ctx : &empty_ctx, &doc_memory);

    layout->cursor = (UICursor *) ui_layout_find_first(layout, UI_ELEMENT_TYPE_CURSOR);
}

////////////////////////////////////////////////////////////////
// Binary layout files (.layoutbin)
////////////////////////////////////////////////////////////////

/**
 * Hash of an original layout file (stored in the user copy to detect changes of the original)
 */
FORCE_INLINE
uint64 ui_layout_bin_source_hash(const void* const data, size_t size) NO_EXCEPT
{
    return hash_murmur3_64(data, size);
}

/**
 * Reads + validates the header
 *
 * @return bool false if the data isn't a compatible binary layout
 */
inline
bool ui_layout_bin_header_read(const byte* const __restrict data, int64 size, UILayoutBinHeader* const __restrict header) NO_EXCEPT
{
    if (!data || size < UI_LAYOUT_BIN_HEADER_SIZE) {
        return false;
    }

    const byte* in = data;
    in = read_le(in, &header->magic);
    in = read_le(in, &header->version);
    in = read_le(in, &header->source_hash);
    in = read_le(in, &header->doc_version);
    in = read_le(in, &header->node_count);
    in = read_le(in, &header->property_count);
    in = read_le(in, &header->pool_size);
    in = read_le(in, &header->first_node);
    in = read_le(in, &header->data_offset);

    if (header->magic != UI_LAYOUT_BIN_MAGIC || header->version != UI_LAYOUT_BIN_VERSION) {
        return false;
    }

    // The sections must be inside of the data
    const int64 document_end = UI_LAYOUT_BIN_HEADER_SIZE
        + (int64) header->node_count * UI_LAYOUT_BIN_NODE_SIZE
        + (int64) header->property_count * UI_LAYOUT_BIN_PROPERTY_SIZE
        + header->pool_size;

    return header->node_count >= 1 && header->property_count >= 1 && header->pool_size >= 0
        && header->first_node >= 0 && header->first_node < header->node_count
        && header->data_offset == document_end
        && header->data_offset + 4 <= size;
}

/**
 * Size of the data section of one file
 *
 * @param const UILayout*   layout  Layout with the data elements (NULL = empty data section)
 * @param int32             file    File of the data elements (UIData::file)
 */
inline
int32 ui_layout_bin_data_size(const UILayout* const layout, int32 file) NO_EXCEPT
{
    int32 size = 4;
    if (!layout || !file) {
        return size;
    }

    int32 offset = -1;
    for (const UIData* data = ui_data_next(layout, &offset); data; data = ui_data_next(layout, &offset)) {
        if (data->file == file && (data->data_flags & UI_DATA_FLAG_LOADED)) {
            size += UI_LAYOUT_BIN_DATA_ENTRY_HEADER_SIZE + data->size;
        }
    }

    return size;
}

/**
 * Max. size of a binary layout
 */
inline
int32 ui_layout_bin_size(const UIDocument* const doc, const UILayout* const layout, int32 file) NO_EXCEPT
{
    // The string pool is at most the text
    return UI_LAYOUT_BIN_HEADER_SIZE
        + doc->node_count * UI_LAYOUT_BIN_NODE_SIZE
        + doc->property_count * UI_LAYOUT_BIN_PROPERTY_SIZE
        + doc->text_length
        + ui_layout_bin_data_size(layout, file);
}

/**
 * Writes the data section (the values of the data elements of one file)
 *
 * @return byte* End of the written data, NULL if out_end is reached
 */
inline
byte* ui_layout_bin_data_write(
    const UILayout* const __restrict layout, int32 file,
    byte* __restrict out, const byte* const __restrict out_end
) NO_EXCEPT
{
    if (out + ui_layout_bin_data_size(layout, file) > out_end) {
        return NULL;
    }

    byte* const count_pos = out;
    out += 4;

    int32 count = 0;
    int32 offset = -1;
    for (const UIData* data = layout && file ? ui_data_next(layout, &offset) : NULL; data; data = ui_data_next(layout, &offset)) {
        // Only user data (the zeroed defaults are not saved, the application writes them)
        if (data->file != file || !(data->data_flags & UI_DATA_FLAG_LOADED)) {
            continue;
        }

        ++count;

        memset(out, 0, UI_LAYOUT_BIN_DATA_NAME_LENGTH);
        memcpy(out, data->name, str_length(data->name));
        out += UI_LAYOUT_BIN_DATA_NAME_LENGTH;

        out = write_le(out, data->version);
        out = write_le(out, data->size);

        memcpy(out, ui_data_values(layout, data), data->size);
        out += data->size;
    }

    write_le(count_pos, count);

    return out;
}

FORCE_INLINE
byte* ui_layout_bin_string_write(
    const UIDocument* const __restrict doc, UIDocumentString str,
    byte* __restrict out, char* __restrict pool, int32* __restrict pool_size
) NO_EXCEPT
{
    // Empty strings don't need pool memory
    const int32 offset = str.length ? *pool_size : 0;
    if (str.length) {
        memcpy(pool + *pool_size, doc->text + str.offset, str.length);
        *pool_size += str.length;
    }

    out = write_le(out, offset);
    out = write_le(out, str.length);

    return out;
}

/**
 * Writes a binary layout
 *
 * @param const UIDocument*   doc         Parsed layout
 * @param uint64              source_hash Hash of the original file (ui_layout_bin_source_hash, 0 = unknown)
 * @param const UILayout*     layout      Layout with the data elements of this file (NULL = empty data section)
 * @param int32               file        File of the data elements (UIData::file)
 * @param byte*               out         Output (ui_layout_bin_size bytes)
 * @param int32               out_size    Output size
 *
 * @return int32 Size, -1 if out is too small
 */
inline
int32 ui_layout_bin_write(
    const UIDocument* const __restrict doc,
    uint64 source_hash,
    const UILayout* const __restrict layout, int32 file,
    byte* __restrict out, int32 out_size
) NO_EXCEPT
{
    if (out_size < ui_layout_bin_size(doc, layout, file)) {
        return -1;
    }

    byte* const start = out;

    // The string pool is built while the nodes + properties are written, it is moved behind them afterwards
    // (it is written into the end of the output first, the output is large enough for the whole text)
    char* const pool = (char *) start + out_size - doc->text_length;
    int32 pool_size = 0;

    byte* const header = out;
    out += UI_LAYOUT_BIN_HEADER_SIZE;

    for (int32 i = 0; i < doc->node_count; ++i) {
        const UIDocumentNode* const node = &doc->nodes[i];
        *out++ = node->kind;
        out = ui_layout_bin_string_write(doc, node->type, out, pool, &pool_size);
        out = ui_layout_bin_string_write(doc, node->name, out, pool, &pool_size);
        out = write_le(out, node->line);
        out = write_le(out, node->first_property);
        out = write_le(out, node->last_property);
        out = write_le(out, node->first_child);
        out = write_le(out, node->last_child);
        out = write_le(out, node->next_sibling);
    }

    for (int32 i = 0; i < doc->property_count; ++i) {
        const UIDocumentProperty* const property = &doc->properties[i];
        out = ui_layout_bin_string_write(doc, property->key, out, pool, &pool_size);
        out = ui_layout_bin_string_write(doc, property->value, out, pool, &pool_size);
        out = write_le(out, property->next);
        out = write_le(out, property->line);
    }

    // memmove: the pool is behind the nodes/properties but the regions can overlap
    memmove(out, pool, pool_size);
    out += pool_size;

    const int32 data_offset = (int32) (out - start);
    out = ui_layout_bin_data_write(layout, file, out, start + out_size);
    if (!out) {
        return -1;
    }

    byte* h = header;
    h = write_le(h, (uint32) UI_LAYOUT_BIN_MAGIC);
    h = write_le(h, (int32) UI_LAYOUT_BIN_VERSION);
    h = write_le(h, source_hash);
    h = write_le(h, doc->version);
    h = write_le(h, doc->node_count);
    h = write_le(h, doc->property_count);
    h = write_le(h, pool_size);
    h = write_le(h, doc->first_node);
    h = write_le(h, data_offset);

    return (int32) (out - start);
}

/**
 * Memory for the nodes + properties of the document of a binary layout
 */
FORCE_INLINE
int32 ui_layout_bin_document_memory_size(const UILayoutBinHeader* const header) NO_EXCEPT
{
    return (int32) ALIGN_UP(header->node_count * sizeof(UIDocumentNode), sizeof(size_t))
        + (int32) (header->property_count * sizeof(UIDocumentProperty));
}

FORCE_INLINE
const byte* ui_layout_bin_string_read(const byte* in, UIDocumentString* const str) NO_EXCEPT
{
    in = read_le(in, &str->offset);
    in = read_le(in, &str->length);

    return in;
}

FORCE_INLINE
bool ui_layout_bin_string_valid(UIDocumentString str, int32 pool_size) NO_EXCEPT
{
    return str.offset >= 0 && str.length >= 0 && str.offset + str.length <= pool_size;
}

/**
 * Reads the document of a binary layout
 * The strings of the document point into data -> data must stay valid while the document is used
 *
 * @param const byte*               data    Binary layout
 * @param const UILayoutBinHeader*  header  Header (ui_layout_bin_header_read)
 * @param UIDocument*               doc     Output
 * @param byte*                     memory  ui_layout_bin_document_memory_size bytes, aligned to sizeof(size_t)
 *
 * @return bool false if the document is corrupt
 */
inline
bool ui_layout_bin_document_read(
    const byte* const __restrict data,
    const UILayoutBinHeader* const __restrict header,
    UIDocument* const __restrict doc,
    byte* __restrict memory
) NO_EXCEPT
{
    doc->nodes = (UIDocumentNode *) memory;
    doc->properties = (UIDocumentProperty *) (memory + ALIGN_UP(header->node_count * sizeof(UIDocumentNode), sizeof(size_t)));
    doc->node_count = header->node_count;
    doc->property_count = header->property_count;
    doc->first_node = header->first_node;
    doc->version = header->doc_version;
    doc->text = (const char *) data + header->data_offset - header->pool_size;
    doc->text_length = header->pool_size;

    const byte* in = data + UI_LAYOUT_BIN_HEADER_SIZE;

    for (int32 i = 0; i < header->node_count; ++i) {
        UIDocumentNode* const node = &doc->nodes[i];
        node->kind = *in++;
        in = ui_layout_bin_string_read(in, &node->type);
        in = ui_layout_bin_string_read(in, &node->name);
        in = read_le(in, &node->line);
        in = read_le(in, &node->first_property);
        in = read_le(in, &node->last_property);
        in = read_le(in, &node->first_child);
        in = read_le(in, &node->last_child);
        in = read_le(in, &node->next_sibling);

        if (!ui_layout_bin_string_valid(node->type, header->pool_size)
            || !ui_layout_bin_string_valid(node->name, header->pool_size)
            || (uint32) node->first_property >= (uint32) header->property_count
            || (uint32) node->last_property >= (uint32) header->property_count
            || (uint32) node->first_child >= (uint32) header->node_count
            || (uint32) node->last_child >= (uint32) header->node_count
            || (uint32) node->next_sibling >= (uint32) header->node_count
        ) {
            return false;
        }
    }

    for (int32 i = 0; i < header->property_count; ++i) {
        UIDocumentProperty* const property = &doc->properties[i];
        in = ui_layout_bin_string_read(in, &property->key);
        in = ui_layout_bin_string_read(in, &property->value);
        in = read_le(in, &property->next);
        in = read_le(in, &property->line);

        if (!ui_layout_bin_string_valid(property->key, header->pool_size)
            || !ui_layout_bin_string_valid(property->value, header->pool_size)
            || (uint32) property->next >= (uint32) header->property_count
        ) {
            return false;
        }
    }

    return true;
}

/**
 * Applies the data section to the data elements of the layout (after the elements were created)
 * Only data elements of the file with the same name, size + version (a changed structure keeps its zeroed bytes,
 * UI_DATA_FLAG_LOADED stays 0 -> the application writes its defaults or migrates the data)
 *
 * @param const byte*               data    Binary layout
 * @param int64                     size    Size of data
 * @param const UILayoutBinHeader*  header  Header
 * @param UILayout*                 layout  Layout with the created data elements
 * @param int32                     file    File of the data elements (UIData::file)
 *
 * @return int32 Amount of applied data elements, -1 if the data section is corrupt
 */
inline
int32 ui_layout_bin_data_read(
    const byte* const __restrict data, int64 size,
    const UILayoutBinHeader* const __restrict header,
    UILayout* const __restrict layout, int32 file
) NO_EXCEPT
{
    const byte* in = data + header->data_offset;
    const byte* const end = data + size;

    int32 count;
    in = read_le(in, &count);

    int32 applied = 0;
    for (int32 i = 0; i < count; ++i) {
        if (in + UI_LAYOUT_BIN_DATA_ENTRY_HEADER_SIZE > end) {
            return -1;
        }

        char name[UI_LAYOUT_BIN_DATA_NAME_LENGTH];
        memcpy(name, in, UI_LAYOUT_BIN_DATA_NAME_LENGTH);
        name[UI_LAYOUT_BIN_DATA_NAME_LENGTH - 1] = '\0';
        in += UI_LAYOUT_BIN_DATA_NAME_LENGTH;

        int32 version;
        int32 content_size;
        in = read_le(in, &version);
        in = read_le(in, &content_size);

        if (content_size < 0 || in + content_size > end) {
            return -1;
        }

        UICore* const element = ui_element_get(layout, name);
        UIData* const target = element && element->type == UI_ELEMENT_TYPE_DATA ? (UIData *) element : NULL;

        if (target && target->file == file && target->size == content_size && target->version == version) {
            memcpy(ui_data_values(layout, target), in, content_size);
            target->data_flags |= UI_DATA_FLAG_LOADED;
            ++applied;
        }

        in += content_size;
    }

    return applied;
}

/**
 * Replaces the data section of a binary layout with the current values of the data elements (in place)
 *
 * @param byte*             data        Binary layout (the document part stays unchanged)
 * @param int32             capacity    Size of the data memory
 * @param const UILayout*   layout      Layout with the data elements
 * @param int32             file        File of the data elements (UIData::file)
 *
 * @return int32 New size, -1 if the data isn't a binary layout or the capacity is too small
 */
inline
int32 ui_layout_bin_data_update(
    byte* const __restrict data, int32 size, int32 capacity,
    const UILayout* const __restrict layout, int32 file
) NO_EXCEPT
{
    UILayoutBinHeader header;
    if (!ui_layout_bin_header_read(data, size, &header)) {
        return -1;
    }

    byte* const end = ui_layout_bin_data_write(layout, file, data + header.data_offset, data + capacity);

    return end ? (int32) (end - data) : -1;
}

#endif
