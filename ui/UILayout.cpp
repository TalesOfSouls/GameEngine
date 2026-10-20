#pragma once
#ifndef COMS_UI_LAYOUT_C
#define COMS_UI_LAYOUT_C

#include "../stdlib/Stdlib.h"
#include "../stdlib/HashMapT.cpp"
#include "../asset/Asset.h"
#include "../camera/Camera.h"
#include "../system/FileUtils.cpp"

#include "UIUber.h"
#include "UICore.h"
#include "UILayout.h"
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
#include "attribute/UIAttribute.cpp"

#define UI_LAYOUT_MAX_CLASS_NAME_LENGTH 32

// Used if the hash map wasn't initialized by loading a layout file
#define UI_LAYOUT_DEFAULT_ELEMENT_COUNT 256

// Global token for element ID (must be globally unique)
#define LAYOUT_ELEMENT_ID_TOKEN '#'

// Global token for element skeleton definition (must be unique),
// e.g. define window skeleton once and reuse
#define LAYOUT_ELEMENT_SKELETON_TOKEN '@'

// Reference a child element (unique in that element)
// e.g. window_close_button (exists once in the element but multiple times in the scene)
#define LAYOUT_ELEMENT_CHILD_TOKEN '$'

FORCE_INLINE
UICore* ui_get_element(UILayout* const layout, int32 offset) NO_EXCEPT
{
    return (UICore *) (layout->ui_element_buffer.memory + offset);
}

FORCE_INLINE
UICore* ui_get_element(UILayout* const layout, const char* name) NO_EXCEPT
{
    if (!layout->hash_map.buf.memory) {
        return NULL;
    }

    const HashEntryStrT<int32>* entry = hashmap_entry_get(&layout->hash_map, name);
    if (!entry) {
        return NULL;
    }

    return (UICore *) (layout->ui_element_buffer.memory + entry->value);
}

FORCE_INLINE
UICore* ui_get_element(UILayout* const layout, SimpleString<const char> str) NO_EXCEPT
{
    char name[64];
    ASSERT_TRUE(ARRAY_COUNT(name) > str.length);

    memcpy(name, str.str, str.length);
    name[str.length] = '\0';

    return ui_get_element(layout, name);
}

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
    layout->dirty = true;
    layout->has_custom_render = false;
    layout->ui_element_changed.count = 0;
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
 * Makes an element findable by name (@see ui_get_element)
 */
inline
void ui_layout_name_element(UILayout* const layout, const char* name, const UICore* const element) NO_EXCEPT
{
    if (!layout->hash_map.buf.memory) {
        if (!layout->data) {
            return;
        }

        ui_layout_hashmap_init(layout, UI_LAYOUT_DEFAULT_ELEMENT_COUNT);
    }

    hashmap_insert(&layout->hash_map, name, ui_element_offset(layout, element));
}

/**
 * Finds the first element of a type in pre-order
 */
static inline
UICore* ui_layout_find_first(const UILayout* const layout, UIElementType type) NO_EXCEPT
{
    if (!layout->ui_root) {
        return NULL;
    }

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

static inline
int32 ui_layout_text_copy(UILayout* const layout, int32 offset, SimpleString<const char> text) NO_EXCEPT
{
    if (!offset || !text.length) {
        return offset;
    }

    char* const dest = (char *) (layout->ui_element_buffer.memory + offset);
    str_copy(dest, text.str, text.length);

    return offset;
}

/**
 * Creates an element from the parsed layout file data
 *
 * @param parent NULL = root
 */
static
UICore* ui_element_create_from_uber(
    UILayout* const __restrict layout,
    const UIUber* const __restrict uber,
    UICore* parent
) NO_EXCEPT
{
    UICore* element = NULL;

    switch (uber->core.type) {
        case UI_ELEMENT_TYPE_LABEL: {
            // Make sure the text fits
            const int32 pattern_length = max_branched(uber->pattern_length, uber->pattern.length ? uber->pattern.length + 1 : 0);
            const int32 content_length = max_branched(uber->content_length, uber->content.length ? uber->content.length + 1 : 0);

            UILabel* const label = ui_label_create(layout, CHAR_TYPE_CHAR, pattern_length, content_length, parent);
            ui_layout_text_copy(layout, label->pattern, uber->pattern);
            ui_layout_text_copy(layout, label->content, uber->content);

            if (uber->font.size > 0.0f) {
                label->font.size = uber->font.size;
            }

            if (uber->font.color) {
                label->font.color = uber->font.color;
            }

            element = &label->core;
        } break;
        case UI_ELEMENT_TYPE_BUTTON: {
            UIButton* const button = ui_button_create(layout, NULL, parent);
            button->background_color = uber->background_color;

            if (uber->content.length) {
                UILabel* const label = ui_label_create(layout, CHAR_TYPE_CHAR, 0, uber->content.length + 1, &button->core);
                label->core.dimension.align_anchor = UI_ANCHOR_ALIGN_CENTER;
                ui_layout_text_copy(layout, label->content, uber->content);

                button->label = ui_element_offset(layout, label);
            }

            element = &button->core;
        } break;
        case UI_ELEMENT_TYPE_IMAGE: {
            element = &ui_image_create(layout, parent)->core;
        } break;
        case UI_ELEMENT_TYPE_VIEW_WINDOW: {
            UIWindow* const window = ui_window_create(layout, parent);
            window->background_color = uber->background_color;

            element = &window->core;
        } break;
        case UI_ELEMENT_TYPE_VIEW_PANEL: {
            element = &ui_panel_create(layout, parent, uber->background_color)->core;
        } break;
        case UI_ELEMENT_TYPE_CURSOR: {
            element = &ui_cursor_create(layout, parent)->core;
        } break;
        case UI_ELEMENT_TYPE_CUSTOM: {
            element = ui_custom_create(layout, 0, 0, parent);
        } break;
        case UI_ELEMENT_TYPE_CHECKBOX:
        case UI_ELEMENT_TYPE_RADIO: {
            // content = label text
            char text[128] = {};
            if (uber->content.length) {
                str_copy(text, uber->content.str, min_branched(uber->content.length, (int32) sizeof(text) - 1));
            }

            element = &ui_checkbox_create(layout, uber->content.length ? text : NULL, parent, uber->core.type)->core;
        } break;
        case UI_ELEMENT_TYPE_SLIDER: {
            element = &ui_slider_create(layout, 0.0f, parent)->core;
        } break;
        case UI_ELEMENT_TYPE_INPUT:
        case UI_ELEMENT_TYPE_TEXTAREA: {
            // content_length = max length
            const int32 max_length = max_branched(uber->content_length, uber->content.length) > 0
                ? max_branched(uber->content_length, uber->content.length)
                : 64;

            UIInput* const input = ui_input_create(layout, max_length, parent, uber->core.type);

            if (uber->content.length) {
                ui_layout_text_copy(layout, input->content, uber->content);
                input->caret = uber->content.length;
            }

            if (uber->font.size > 0.0f) {
                input->font.size = uber->font.size;
            }

            if (uber->background_color) {
                input->background_color = uber->background_color;
            }

            element = &input->core;
        } break;
        case UI_ELEMENT_TYPE_SELECT: {
            // @todo options are not defined in the layout file yet
            element = &ui_select_create(layout, NULL, 0, parent)->core;
        } break;
        case UI_ELEMENT_TYPE_TABLE: {
            // @todo columns/rows are not defined in the layout file yet
            element = &ui_table_create(layout, NULL, 0, 0, parent)->core;
        } break;
        case UI_ELEMENT_TYPE_VIEW_TAB: {
            // @todo tabs are not defined in the layout file yet
            element = &ui_tab_create(layout, NULL, 0, UI_TAB_ORIENTATION_HORIZONTAL, parent)->core;
        } break;
        case UI_ELEMENT_TYPE_COLOR_PICKER: {
            element = &ui_color_picker_create(layout, UI_COLOR_PICKER_TYPE_GRADIENT, parent)->core;
        } break;
        case UI_ELEMENT_TYPE_OPTION_LIST:
        case UI_ELEMENT_TYPE_COLOR_PANEL:
        case UI_ELEMENT_TYPE_TEXT:
        case UI_ELEMENT_TYPE_LINK:
        case UI_ELEMENT_TYPE_MANUAL: {
            // @todo Not implemented yet, but we still need them in the tree (e.g. for their children)
            element = ui_element_alloc(
                layout, uber->core.type,
                ui_element_type_size(uber->core.type), (int32) sizeof(size_t),
                parent
            );
        } break;
        default: {
            UNREACHABLE();
        }
    }

    // Core values defined in the layout file
    if (uber->core.dimension.align_anchor) {
        element->dimension.align_anchor = uber->core.dimension.align_anchor;
    }

    if (uber->core.dimension.flag) {
        element->dimension.flag = uber->core.dimension.flag;
    }

    if (uber->core.dimension.pos_raw.x != 0.0f || uber->core.dimension.pos_raw.y != 0.0f) {
        element->dimension.pos_raw = uber->core.dimension.pos_raw;
    }

    if (uber->core.dimension.dim_raw.x != 0.0f || uber->core.dimension.dim_raw.y != 0.0f) {
        element->dimension.dim_raw = uber->core.dimension.dim_raw;
    }

    if (uber->core.update_func) {
        element->update_func = uber->core.update_func;
    }

    return element;
}

/**
 * Check if this line is a UI element
 */
static FORCE_INLINE
bool ui_layout_is_element(const char* pos) NO_EXCEPT
{
    str_skip_whitespace(&pos);
    // # = element id
    // @ = general element definition
    // $ = child element
    return *pos == LAYOUT_ELEMENT_ID_TOKEN
        || *pos == LAYOUT_ELEMENT_SKELETON_TOKEN
        || *pos == LAYOUT_ELEMENT_CHILD_TOKEN;
}

static FORCE_INLINE
int32 ui_layout_line_indent(const char* pos) NO_EXCEPT
{
    const char* const start = pos;
    str_skip_whitespace(&pos);

    return (int32) (pos - start);
}

/**
 * Parses an element and all of its children (= lines with a larger indentation)
 *
 * @param pos MUST point to the beginning of the line
 */
static
char* ui_layout_element_parse(
    UILayout* __restrict layout,
    char* __restrict pos,
    UICore* parent = NULL
) NO_EXCEPT
{
    const int32 self_indent = ui_layout_line_indent(pos);
    str_skip_whitespace((const char**) &pos);

    // check if this is even an element
    if (!ui_layout_is_element(pos)) {
        ASSERT_THROW();
        LOG_1("Couldn't find element");

        return str_skip_line(pos);
    }

    // Skip the element token (#, @, $)
    ++pos;

    // find element name
    char element_name[HASH_MAP_MAX_KEY_LENGTH];
    {
        int32 i = 0;
        while ((isalnum(*pos) || *pos == '_' || *pos == '-')
            && i < (int32) ARRAY_COUNT(element_name) - 1
        ) {
            element_name[i++] = *pos++;
        }
        element_name[i] = '\0';
    }

    str_skip_line((const char**) &pos);

    // find element type
    // WARNING: The type MUST be specified right after the element id
    const char* type;
    {
        str_skip_whitespace((const char**) &pos);
        if (strncmp(pos, "type:", sizeof("type:") - 1) != 0) {
            ASSERT_THROW();
            LOG_1("Couldn't find element type");

            return str_skip_line(pos);
        }

        pos += sizeof("type:") - 1;
        str_skip_whitespace((const char**) &pos);

        type = pos;
        str_skip_line((const char**) &pos);
    }

    // Parse element values
    SimpleString<const char> class_name = {};
    UIUber uber = {};
    uber.core.type = (UIElementType) ui_element_type_to_id(type);

    // We iterate as long as new element components show up
    // or in other words until the next line would be another element or even a child element
    while (*pos && !ui_layout_is_element(pos)) {
        str_skip_whitespace((const char**) &pos);

        // Empty line
        if (*pos == '\n' || *pos == '\r') {
            str_skip_line((const char**) &pos);
            continue;
        }

        const char* value_type = pos;
        str_move_to((const char**) &pos, ":\n");
        if (*pos != ':') {
            // Invalid line
            str_skip_line((const char**) &pos);
            continue;
        }

        *pos++ = '\0';
        str_skip_whitespace((const char**) &pos);

        // We use a uber element that can hold all possible value types
        // We sacrifice some memory for simpler handling
        if (strcmp(value_type, "class") == 0) {
            class_name.str = pos;
            str_move_to((const char**) &pos, " \r\n");
            class_name.length = (int32) (pos - class_name.str);
        } else if (strcmp(value_type, "inherit") == 0) {
            // @todo copy inherit_element + child data
            //      Skeletons are currently created as normal elements
        } else {
            ui_uber_from_txt(&uber, value_type, pos);
        }

        str_skip_line((const char**) &pos);
    }

    UICore* const element = ui_element_create_from_uber(layout, &uber, parent);

    // @todo We need to handle skeleton references e.g.
    //      #my_element
    //          inherit: my_skeleton
    //          @my_skeleton_title
    //              opacity: 0
    //      In this case we don't want to create a new element but modify the existing element from the skeleton

    // We need to load the style for every state change directly from the theme
    // For that we need the class name
    if (class_name.length) {
        ASSERT_TRUE(class_name.length < UI_LAYOUT_MAX_CLASS_NAME_LENGTH);
        const int32 length = min_branched(class_name.length, UI_LAYOUT_MAX_CLASS_NAME_LENGTH - 1);

        char* inserted_class_name = (char *) memory_get(
            &layout->ui_element_buffer,
            sizeof(char) * (length + 1),
            alignof(char)
        );
        str_copy(inserted_class_name, class_name.str, length);

        element->class_name = ui_element_offset(layout, inserted_class_name);
    }

    if (*element_name) {
        hashmap_insert(&layout->hash_map, element_name, ui_element_offset(layout, element));
    }

    // We skip useless empty lines
    str_skip_eol((const char**) &pos);

    // Handle all child elements = elements with a larger indentation
    while (*pos && ui_layout_is_element(pos) && ui_layout_line_indent(pos) > self_indent) {
        pos = ui_layout_element_parse(layout, pos, element);
        str_skip_eol((const char**) &pos);
    }

    return pos;
}

/**
 * Creates the layout from a text file
 * The elements are created as children of a newly created root (the root size is defined at runtime @see ui_root_resize)
 *
 * WARNING: layout->data and layout->ui_element_buffer need to have memory already reserved
 */
void layout_from_file_txt(
    UILayout* const __restrict layout,
    const char* __restrict path,
    RingMemory* const ring
) {
    FileBody file = {0};
    file_read(path, &file, ring);
    ASSERT_TRUE(file.size);

    char* pos = (char *) file.content;

    // skip version
    str_skip_line((const char**) &pos);

    // 1. Iteration: Find the element count
    ////////////////////////////////////////////////////////////
    int32 temp_element_count = 0;
    while (*pos != '\0') {
        str_skip_whitespace((const char**) &pos);
        if (ui_layout_is_element(pos)) {
            ++temp_element_count;
        }

        str_skip_line((const char**) &pos);
    }

    // 2. Iteration: Fill HashMap
    ////////////////////////////////////////////////////////////
    // @performance we reserve * 2 memory to avoid too many hash collisions... urgh
    ui_layout_hashmap_init(layout, max_branched(temp_element_count * 2, (int32) 2));

    buffer_reset(&layout->ui_element_buffer);
    layout->ui_root = NULL;
    ui_root_create(layout, 0.0f, 0.0f);

    pos = (char *) file.content;

    // move past version string
    str_skip_line((const char**) &pos);

    while (*pos != '\0') {
        str_skip_eol((const char**) &pos);

        const char* line = pos;
        str_skip_whitespace(&line);

        if (*line == LAYOUT_ELEMENT_ID_TOKEN
            || *line == LAYOUT_ELEMENT_SKELETON_TOKEN
            || *line == LAYOUT_ELEMENT_CHILD_TOKEN
        ) {
            pos = ui_layout_element_parse(layout, pos);
        } else {
            str_skip_line((const char**) &pos);
        }
    }

    layout->cursor = (UICursor *) ui_layout_find_first(layout, UI_ELEMENT_TYPE_CURSOR);
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
 * @return Bytes read or -1 if the data is not a compatible layout
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

    // Prepare hashmap (incl. reserve memory) by initializing it the same way we originally did
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
    layout->cursor = (UICursor *) ui_layout_find_first(layout, UI_ELEMENT_TYPE_CURSOR);
    memset(&layout->interaction, 0, sizeof(layout->interaction));
    layout->dirty = true;

    LOG_1("[INFO] UI loaded layout");

    return (int32) (in - data);
}

static inline
void layout_update_element_font(
    UIAttributeFont* const __restrict font,
    const UIAttribute* const __restrict attr
) NO_EXCEPT
{
    ui_theme_assign_font(font, attr);
}

/**
 * Element specific theme attributes
 * The theme values are written directly into the (inline) element data,
 * this way rendering never has to access the theme
 */
static
void layout_update_element_by_type(
    UICore* const __restrict core,
    const UIAttribute* const __restrict attr
) NO_EXCEPT
{
    const bool is_font_attr = attr->attribute_id >= UI_ATTRIBUTE_TYPE_FONT_NAME
        && attr->attribute_id <= UI_ATTRIBUTE_TYPE_FONT_LINE_HEIGHT;

    switch (core->type) {
        case UI_ELEMENT_TYPE_LABEL: {
            if (is_font_attr) {
                layout_update_element_font(&((UILabel *) core)->font, attr);
            }
        } break;
        case UI_ELEMENT_TYPE_TEXTAREA: {
            if (is_font_attr) {
                layout_update_element_font(&((UITextarea *) core)->font, attr);
            }
        } break;
        case UI_ELEMENT_TYPE_TEXT: {
            if (is_font_attr) {
                layout_update_element_font(&((UIText *) core)->font, attr);
            }
        } break;
        case UI_ELEMENT_TYPE_LINK: {
            if (is_font_attr) {
                layout_update_element_font(&((UILink *) core)->font, attr);
            }
        } break;
        case UI_ELEMENT_TYPE_BUTTON: {
            if (attr->attribute_id == UI_ATTRIBUTE_TYPE_BACKGROUND_COLOR) {
                ((UIButton *) core)->background_color = ui_theme_color(attr);
            }
        } break;
        case UI_ELEMENT_TYPE_INPUT: {
            if (attr->attribute_id == UI_ATTRIBUTE_TYPE_BACKGROUND_COLOR) {
                ((UIInput *) core)->background_color = ui_theme_color(attr);
            }
        } break;
        case UI_ELEMENT_TYPE_VIEW_WINDOW: {
            if (attr->attribute_id == UI_ATTRIBUTE_TYPE_BACKGROUND_COLOR) {
                ((UIWindow *) core)->background_color = ui_theme_color(attr);
            } else if (attr->attribute_id == UI_ATTRIBUTE_TYPE_BORDER_COLOR) {
                ((UIWindow *) core)->border.color = ui_theme_color(attr);
            } else if (attr->attribute_id == UI_ATTRIBUTE_TYPE_BORDER_WIDTH) {
                f32 width = 0.0f;
                ui_theme_assign_f32(&width, attr);
                ((UIWindow *) core)->border.width = (uint8) width;
            }
        } break;
        case UI_ELEMENT_TYPE_VIEW_PANEL: {
            if (attr->attribute_id == UI_ATTRIBUTE_TYPE_BACKGROUND_COLOR) {
                ((UIPanel *) core)->background_color = ui_theme_color(attr);
            } else if (attr->attribute_id == UI_ATTRIBUTE_TYPE_BORDER_COLOR) {
                ((UIPanel *) core)->border.color = ui_theme_color(attr);
            } else if (attr->attribute_id == UI_ATTRIBUTE_TYPE_BORDER_WIDTH) {
                f32 width = 0.0f;
                ui_theme_assign_f32(&width, attr);
                ((UIPanel *) core)->border.width = (uint8) width;
            }
        } break;
        default: {
            // @todo Other elements/attributes are not supported yet
        }
    }
}

static
void layout_update_element(
    UILayout* const __restrict layout,
    const UITheme* const __restrict theme,
    UICore* const __restrict core
) NO_EXCEPT
{
    if (!core->class_name) {
        return;
    }

    const HashEntryStrT<int32>* entry = (HashEntryStrT<int32> *) hashmap_entry_get(
        &theme->hash_map,
        (const char*) (layout->ui_element_buffer.memory + core->class_name)
    );

    if (!entry) {
        // The theme doesn't define this class
        return;
    }

    const UIAttributeGroup* attr_group = (UIAttributeGroup *) (theme->data + entry->value);
    const UIAttribute* attributes = (UIAttribute*) ALIGN_UP((uintptr_t) (attr_group + 1), alignof(UIAttribute));

    // @todo We should first update the skeleton
    //      then inherit the skeleton style
    //      then update the style directly assigned

    for (int32 i = 0; i < attr_group->attribute_count; ++i) {
        const UIAttribute* const attr = &attributes[i];

        switch (attr->attribute_id) {
            // First handle core attributes
            case UI_ATTRIBUTE_TYPE_POSITION_X:
            case UI_ATTRIBUTE_TYPE_POSITION_Y:
            case UI_ATTRIBUTE_TYPE_DIMENSION_WIDTH:
            case UI_ATTRIBUTE_TYPE_DIMENSION_HEIGHT: {
                    ui_theme_assign_dimension(&core->dimension, attr);
                } break;
            case UI_ATTRIBUTE_TYPE_ANCHOR: {
                    if (attr->datatype == UI_ATTRIBUTE_DATA_TYPE_STR) {
                        core->dimension.align_anchor = ui_anchor_align_from_str(attr->value_str);
                    }
                } break;
            default: {
                // Attribute type was not part of core, handle element specific
                layout_update_element_by_type(core, attr);
            }
        }
    }

    core->change_type |= UI_ELEMENT_CHANGE_CONTENT;
}

// @question What about general theme?
// force_update allows us to force a update of all elements even if they didn't change
void layout_from_theme(
    UILayout* const __restrict layout,
    const UITheme* __restrict theme,
    bool force_update = false
) {
    PROFILE_DEBUG(PROFILE_LAYOUT_FROM_THEME, (char *) NULL, PROFILE_FLAG_SHOULD_LOG);
    LOG_1("[INFO] UI load theme for layout");

    // @todo Handle animations
    if (theme->font) {
        layout->font = theme->font;
    }

    ui_layout_mark_dirty(layout);

    if (!layout->ui_root) {
        return;
    }

    if (force_update) {
        // Linear walk through all elements
        int32 offset = layout->ui_root->next_offset;
        while (offset) {
            UICore* const core = ui_element_get(layout, offset);
            layout_update_element(layout, theme, core);

            offset = core->next_offset;
        }

        return;
    }

    // Only the changed elements
    for (int32 i = 0; i < layout->ui_element_changed.count; ++i) {
        UICore* const core = ui_element_get(layout, layout->ui_element_changed.elements[i].element);
        layout_update_element(layout, theme, core);
    }
}

#endif
