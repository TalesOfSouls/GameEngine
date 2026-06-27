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
#include "UIInput.h"
#include "UILabel.cpp"
#include "UIWindow.cpp"
#include "attribute/UIAttribute.cpp"

#define UI_LAYOUT_MAX_CLASS_NAME_LENGTH 32

// Global token for element ID (must be globally unique)
#define LAYOUT_ELEMENT_ID_TOKEN '#'

// Global token for element skeleton definition (must be unique),
// e.g. define window skeleton once and reuse
#define LAYOUT_ELEMENT_SKELETON_TOKEN '@'

// Reference a child element (unique in that element)
// e.g. window_close_button (exists once in the element but multiple times in the scene)
#define LAYOUT_ELEMENT_CHILD_TOKEN '$'

FORCE_INLINE
UICore* ui_get_element(UILayout* const layout, int32 offset) NO_EXCEPT {
    return (UICore *) (layout->ui_element_buffer.memory + offset);
}

FORCE_INLINE
UIOffset* ui_get_offset(UILayout* const layout, int32 index) NO_EXCEPT {
    return (UIOffset *) (layout->ui_offset_buffer.memory + index);
}

FORCE_INLINE
UIOffset* ui_get_offset(UILayout* const layout, const char* name) NO_EXCEPT {
    const HashEntryStrT<int32>* entry = hashmap_get_entry(&layout->hash_map, name);
    if (!entry) {
        return NULL;
    }

    return (UIOffset *) (layout->ui_offset_buffer.memory + entry->value);
}

FORCE_INLINE
UIOffset* ui_get_offset(UILayout* const layout, SimpleString<const char> str) NO_EXCEPT {
    char name[64];
    ASSERT_TRUE(ARRAY_COUNT(name) > str.length);

    memcpy(name, str.str, str.length);
    name[str.length] = '\0';

    return ui_get_offset(layout, name);
}

static
UILabelOffset* ui_element_create(UILayout* const __restrict layout, const UIUber* const __restrict uber, UIElementType type) NO_EXCEPT
{
    switch (type) {
        case UI_ELEMENT_TYPE_BUTTON : {
            return NULL;
        };
        case UI_ELEMENT_TYPE_SELECT : {
            return NULL;
        };
        case UI_ELEMENT_TYPE_INPUT : {
            return NULL;
        };
        case UI_ELEMENT_TYPE_LABEL: {
                return ui_label_create(
                    layout,
                    uber->char_type,
                    uber->pattern_length,
                    uber->content_length
                );
            };
        case UI_ELEMENT_TYPE_TEXTAREA : {
            return NULL;
        };
        case UI_ELEMENT_TYPE_IMAGE : {
            return NULL;
        };
        case UI_ELEMENT_TYPE_TEXT : {
            return NULL;
        };
        case UI_ELEMENT_TYPE_LINK : {
            return NULL;
        };
        case UI_ELEMENT_TYPE_TABLE : {
            return NULL;
        };
        case UI_ELEMENT_TYPE_VIEW_WINDOW : {
            return NULL;
        };
        case UI_ELEMENT_TYPE_VIEW_PANEL : {
            return NULL;
        };
        case UI_ELEMENT_TYPE_VIEW_TAB : {
            return NULL;
        };
        case UI_ELEMENT_TYPE_CURSOR : {
            return NULL;
        };
        default: {
            UNREACHABLE();
        }
    }
}

/**
 * A label can never have children -> return NULL
 */
FORCE_INLINE
UIOffset* ui_child_offset_from_name(UIOffset*, SimpleString<const char>&) NO_EXCEPT
{
    return NULL;
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
    return *pos == LAYOUT_ELEMENT_ID_TOKEN
        || *pos == LAYOUT_ELEMENT_SKELETON_TOKEN
        || *pos == LAYOUT_ELEMENT_CHILD_TOKEN;
}

char* ui_layout_element_parse(
    UILayout* __restrict layout,
    char* __restrict pos,
    UIOffset* parent = NULL
) NO_EXCEPT
{
    const char* start = pos;
    str_skip_whitespace((const char**) &pos);
    const int32 self_indent = (int32) (pos - start);

    if (*pos != LAYOUT_ELEMENT_ID_TOKEN
        && *pos != LAYOUT_ELEMENT_SKELETON_TOKEN
        && *pos != LAYOUT_ELEMENT_CHILD_TOKEN
    ) {
        ASSERT_THROW();
        LOG_1("Couldn't find element");

        return str_skip_line(pos);
    }

    char element_name[HASH_MAP_MAX_KEY_LENGTH];
    int i = 0;
    while (isalnum(*pos) && i < ARRAY_COUNT(element_name) - 1) {
        element_name[i++] = *pos++;
    }
    element_name[i] = '\0';

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
        str_skip_empty((const char**) &pos);

        type = pos;
        str_move_to((const char**) &pos, " \r\n");
        str_skip_line((const char**) &pos);
    }

    UIElementType type_id = (UIElementType) ui_element_type_to_id(type);

    // Parse element values
    SimpleString<const char> class_name = {0};

    // Defines this elements child name in the parent (e.g. title_label in the UITitle)
    SimpleString<const char> child_name = {0};
    const UIOffset* inherit_offset = NULL;
    UIUber uber = {0};

    // We iterate as long as new element components show up or in other words until the next line would be another element
    while (*pos && !ui_layout_is_element(pos)) {
        str_skip_whitespace((const char**) &pos);

        const char* value_type = pos;
        str_move_to((const char**) &pos, ':');
        *pos++ = '\0';
        str_skip_whitespace((const char**) &pos);

        // We use a uber element that can hold all possible value types
        // We sacrifice some memory for simpler handling
        // Alternatively we would either have to:
        //      Write element type specific parsing which can repeat some parts between elements
        //      or, create some array which we then search e.g. struct {char[32], union { int32, f32, bool, ...}}[16];
        //          This is much smaller but probably slower since we would have to search that array every time and do a strcmp
        //          @performance Maybe test/profile that approach in the future, it shouldn't be that hard to test
        if (strncmp(value_type, "class", sizeof("class") - 1) == 0) {
            class_name.str = pos;
            str_move_to((const char**) &pos, " \r\n");
            class_name.length = (int32) (pos - class_name.str);
        } else if (strncmp(value_type, "inherit", sizeof("inherit") - 1) == 0) {
            SimpleString<const char> inherit_name;

            inherit_name.str = pos;
            str_move_to((const char**) &pos, " \r\n");
            inherit_name.length = (int32) (pos - inherit_name.str);

            inherit_offset = ui_get_offset(layout, inherit_name);
        } else if (strncmp(value_type, "child", sizeof("child") - 1) == 0) {
            child_name.str = pos;
            str_move_to((const char**) &pos, " \r\n");
            child_name.length = (int32) (pos - child_name.str);
        } else {
            ui_uber_from_txt(&uber, value_type, pos);
        }

        str_skip_line((const char**) &pos);
    }

    UIOffset* offset = NULL;
    if (!child_name.length || !parent) {
        // If it is a standalone element, we need to add a new offset and element to their respective arrays/vectors
        // (ui_offset_buffer and ui_element_buffer)
        // We also need to add the offset to the root vector ui_offset_root
        // We can be a standalone element with or without parent e.g.
        //      A window element is always a standalone
        //      A label could be standalone or not e.g.
        //          the title label is not standalone and a "fixed" part of the UIWindow element,
        //          but a label on a panel is not standalone, it is a dynamic child of UIPanel

        // @performance This is actually slow if we have inherit_offset because:
        //              First we create the element, then we potentially overwrite it again with inherit data
        //              It would be better to call a ui_label_reserve where we then copy over all the element data and some of the offset data
        offset = (UIOffset *) ui_element_create(layout, &uber, type_id);
    } else {
        ASSERT_TRUE(parent);

        // This element is a fixed/static child element defined in a parent (e.g. label in UITitle)
        offset = ui_child_offset_from_name(parent, child_name);
    }

    // General inheritance handling
    if (inherit_offset) {
        offset->children_count = inherit_offset->children_count;

        // @todo update children offset array
        //offset->children = ...
    }

    // Do element specific setup
    switch (type_id) {
        case UI_ELEMENT_TYPE_CURSOR: {

            } break;
        case UI_ELEMENT_TYPE_VIEW_WINDOW: {

            } break;
        case UI_ELEMENT_TYPE_LABEL: {
                UILabel* element = (UILabel*) ui_get_element(layout, offset->element);

                // Create the inherit data if we inherit from something
                if (inherit_offset) {
                    const UILabel* inherit_element = (UILabel *) ui_get_element(layout, inherit_offset->element);
                    memcpy(element, inherit_element, sizeof(UILabel));

                    // @todo Also copy over child offsets + elements
                }

                // Set element data directly defined for this element
                {
                    if (uber.pattern_length) {
                        memcpy(element->pattern, uber.pattern.str, uber.pattern.length);
                    }

                    if (uber.pattern_length) {
                        memcpy(element->content, uber.content.str, uber.content.length);
                    }
                }
            } break;
        default: {
            UNREACHABLE();
        }
    }

    // @todo We need to handle skeleton references e.g.
    //      #my_element
    //          inherit: my_skeleton
    //          @my_skeleton_title
    //              opacity: 0
    //      In this case we don't want to create a new element but modify the existing element from the skeleton

    // We need to load the style for every state change directly from the theme
    // For that we need the class name
    // Alternatively we could've created an element multiple times = once per state
    // This might have been fine for most situations since we don't have that many states.
    // However, animations completely break that approach
    // Therefore we decided to load it live from the theme which however is handled in a different state change function
    if (class_name.length) {
        ASSERT_TRUE(class_name.length < UI_LAYOUT_MAX_CLASS_NAME_LENGTH);
        UICore* const core = ui_get_element(layout, offset->element);
        core->class_name = (char*) memory_get(
            &layout->ui_element_buffer,
            sizeof(char) * UI_LAYOUT_MAX_CLASS_NAME_LENGTH,
            alignof(size_t)
        );
        str_copy(core->class_name, class_name);
    }

    // Create new element in hash map
    hashmap_insert(
        &layout->hash_map,
        element_name,
        (int32) MEMORY_OFFSET(offset, layout->ui_offset_buffer.memory)
    );

    // We skip useless empty lines
    str_skip_eol((const char**) &pos);

    // Handle all child elements
    if (ui_layout_is_element(pos)) {
        // Figure out if next element is a child by checking the indention
        const char* pos_temp = pos;
        str_skip_whitespace(&pos_temp);
        int32 child_indent = (int32) (pos_temp - pos);

        while (self_indent < child_indent) {
            pos = ui_layout_element_parse(layout, pos, offset);

            // Figure out if next element is a child by checking the indention
            pos_temp = pos;
            str_skip_whitespace(&pos_temp);
            child_indent = (int32) (pos_temp - pos);
        }
    }

    return pos;
}

// WARNING: theme needs to have memory already reserved and assigned to data
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

    // skip all empty lines
    str_skip_empty((const char**) &pos);

    // 1. Iteration: Find the element count
    ////////////////////////////////////////////////////////////
    int32 temp_element_count = 0;
    while (*pos != '\0') {
        str_skip_whitespace((const char**) &pos);
        if (*pos == '#') {
            ++temp_element_count;
        }

        str_skip_line((const char**) &pos);
    }

    // 2. Iteration: Fill HashMap
    ////////////////////////////////////////////////////////////
    // @performance we reserve * 2 memory to avoid too many hash collisions... urgh
    hashmap_create(
        &layout->hash_map,
        temp_element_count * 2,
        layout->data,
        align_up((int32) sizeof(HashEntryStrT<int32>), 32)
    );
    ASSERT_TRUE(layout->data_size >= hashmap_size(&layout->hash_map));

    pos = (char *) file.content;

    // move past version string
    str_move_past((const char**) &pos, '\n');

    // skip all empty lines
    str_skip_empty((const char**) &pos);

    while (*pos != '\0') {
        str_skip_eol((const char**) &pos);
        if (*pos == '#') {
            pos = ui_layout_element_parse(layout, pos, NULL);
        }
    }
}

int32 layout_to_data(
    const UILayout* const __restrict layout,
    byte* __restrict data
) {
    LOG_1("[INFO] UI save layout");
    byte* out = data;

    out = write_le(out, UI_LAYOUT_VERSION);

    // We don't save the used_data_size because that depends on the respective theme

    out += hashmap_dump(&layout->hash_map, out, MEMBER_SIZEOF(HashEntryInt32, value));

    out = write_le(out, layout->ui_offset_root.count);
    memcpy(out, layout->ui_offset_root.elements, layout->ui_offset_root.count);
    out += sizeof(int32) * layout->ui_offset_root.count;

    out = write_le(out, (int32) (layout->ui_offset_buffer.head - layout->ui_offset_buffer.memory));
    memcpy(out, layout->ui_offset_buffer.memory, layout->ui_offset_buffer.head - layout->ui_offset_buffer.memory);
    out += layout->ui_offset_buffer.head - layout->ui_offset_buffer.memory;

    out = write_le(out, (int32) (layout->ui_element_buffer.head - layout->ui_element_buffer.memory));
    memcpy(out, layout->ui_element_buffer.memory, layout->ui_element_buffer.head - layout->ui_element_buffer.memory);
    out += layout->ui_element_buffer.head - layout->ui_element_buffer.memory;

    LOG_1("[INFO] UI saved layout");

    return (int32) (out - data);
}

// The size of layout->data should be the file size + a bunch of additional data for additional theme dependent "UIElements->style_types".
// Yes, this means we have a little too much data but not by a lot
int32 layout_from_data(
    const byte* const __restrict data,
    UILayout* const __restrict layout
) {
    PROFILE_DEBUG(PROFILE_LAYOUT_FROM_DATA, NULL, PROFILE_FLAG_SHOULD_LOG);
    LOG_1("[INFO] UI load layout");

    const byte* in = data;

    int32 version;
    in = read_le(in, &version);

    // Prepare hashmap (incl. reserve memory) by initializing it the same way we originally did
    // Of course we still need to populate the data using hashmap_load()
    hashmap_create(
        &layout->hash_map,
        (int32) SWAP_ENDIAN_LITTLE(*((uint32 *) in)),
        layout->data,
        align_up((int32) sizeof(HashEntryStrT<int32>), 32)
    );

    layout->used_data_size = (int32) hashmap_load(&layout->hash_map, in, MEMBER_SIZEOF(HashEntryInt32, value));
    in += layout->used_data_size;

    in = read_le(in, &layout->ui_offset_root.count);
    memcpy(layout->ui_offset_root.elements, in, sizeof(int32) * layout->ui_offset_root.count);
    in += sizeof(int32) * layout->ui_offset_root.count;

    int32 offset;
    in = read_le(in, &offset);
    layout->ui_offset_buffer.head = layout->ui_offset_buffer.memory + offset;
    memcpy(layout->ui_offset_buffer.memory, in, offset);
    in += offset;

    in = read_le(in, &offset);
    layout->ui_element_buffer.head = layout->ui_element_buffer.memory + offset;
    memcpy(layout->ui_element_buffer.memory, in, offset);
    //in += offset;

    LOG_1("[INFO] UI loaded layout");

    return (int32) layout->data_size;
}

static inline
void layout_update_element_by_type(
    UIAttributeFont* const __restrict font,
    const UIAttribute* const __restrict attr
) NO_EXCEPT
{
    switch (attr->attribute_id) {
        case UI_ATTRIBUTE_TYPE_FONT_COLOR: {
                font->color = attr->value_uint;
            } break;
        case UI_ATTRIBUTE_TYPE_FONT_SIZE: {
                font->size = attr->value_float;
            } break;
        default: {
            UNREACHABLE();
        }
    }
}

static FORCE_INLINE
void layout_update_element_by_type(
    UICore* const __restrict core,
    const UIAttribute* const __restrict attr,
    UIElementType type
) NO_EXCEPT
{
    switch (type) {
        case UI_ELEMENT_TYPE_BUTTON : {
        } break;
        case UI_ELEMENT_TYPE_SELECT : {
        } break;
        case UI_ELEMENT_TYPE_INPUT : {
        } break;
        case UI_ELEMENT_TYPE_LABEL: {
            UILabel* element = (UILabel *) core;
            if (attr->attribute_id >= UI_ATTRIBUTE_TYPE_FONT_NAME
                && attr->attribute_id <= UI_ATTRIBUTE_TYPE_FONT_LINE_HEIGHT
            ) {
                layout_update_element_by_type(&element->font, attr);
                return;
            }

            // switch (attr->attribute_id) {
            //     default: {
            //         UNREACHABLE();
            //     }
            // }
        } break;
        case UI_ELEMENT_TYPE_TEXTAREA : {
            UITextarea* element = (UITextarea *) core;
            if (attr->attribute_id >= UI_ATTRIBUTE_TYPE_FONT_NAME
                && attr->attribute_id <= UI_ATTRIBUTE_TYPE_FONT_LINE_HEIGHT
            ) {
                layout_update_element_by_type(&element->font, attr);
                return;
            }

            // switch (attr->attribute_id) {
            //     default: {
            //         UNREACHABLE();
            //     }
            // }
        } break;
        case UI_ELEMENT_TYPE_IMAGE : {
        } break;
        case UI_ELEMENT_TYPE_TEXT : {
            UIText* element = (UIText *) core;
            if (attr->attribute_id >= UI_ATTRIBUTE_TYPE_FONT_NAME
                && attr->attribute_id <= UI_ATTRIBUTE_TYPE_FONT_LINE_HEIGHT
            ) {
                layout_update_element_by_type(&element->font, attr);
                return;
            }

            // switch (attr->attribute_id) {
            //     default: {
            //         UNREACHABLE();
            //     }
            // }
        } break;
        case UI_ELEMENT_TYPE_LINK : {
        } break;
        case UI_ELEMENT_TYPE_TABLE : {
        } break;
        case UI_ELEMENT_TYPE_VIEW_WINDOW: {
        } break;
        case UI_ELEMENT_TYPE_VIEW_PANEL : {
        } break;
        case UI_ELEMENT_TYPE_VIEW_TAB : {
        } break;
        case UI_ELEMENT_TYPE_CURSOR : {
        } break;
        default: {
            UNREACHABLE();
        }
    }
}

static
void layout_update_element(
    UILayout* const __restrict layout,
    const UITheme* const __restrict theme,
    const UIOffset* const __restrict offset
) NO_EXCEPT {
    UICore* core = ui_get_element(layout, offset->element);
    if (!core->class_name) {
        return;
    }

    const HashEntryStrT<int32>* entry = (HashEntryStrT<int32> *) hashmap_get_entry(&theme->hash_map, core->class_name);
    const UIAttributeGroup* attr_group = (UIAttributeGroup *) (theme->data + entry->value);
    const UIAttribute* attributes = (UIAttribute*) align_up((uintptr_t) (attr_group + 1), alignof(UIAttribute));

    // @todo We should first update the skeleton
    //      then inherit the skeleton style
    //      then update the style directly assigned

    for (int i = 0; i < attr_group->attribute_count; ++i) {
        const UIAttribute* const attr = &attributes[i];

        switch (attr->attribute_id) {
            // First handle core attributes
            case UI_ATTRIBUTE_TYPE_POSITION_X: {
                    core->dimension.pos.x = attr->value_float;
                } break;
            case UI_ATTRIBUTE_TYPE_POSITION_Y: {
                    core->dimension.pos.y = attr->value_float;
                } break;
            case UI_ATTRIBUTE_TYPE_DIMENSION_WIDTH: {
                    core->dimension.dimension.width = attr->value_float;
                } break;
            case UI_ATTRIBUTE_TYPE_DIMENSION_HEIGHT: {
                    core->dimension.dimension.height = attr->value_float;
                } break;
            default: {
                // Attribute type was not part of core, handle element specific
                layout_update_element_by_type(core, attr, offset->type);
            }
        }
    }
}

// @question What about general theme?
// force_update allows us to force a updateof all elements even if they didn't change
void layout_from_theme(
    UILayout* const __restrict layout,
    const UITheme* __restrict theme,
    bool force_update = false
) {
    PROFILE_DEBUG(PROFILE_LAYOUT_FROM_THEME, NULL, PROFILE_FLAG_SHOULD_LOG);
    LOG_1("[INFO] UI load theme for layout");

    // @todo Handle animations
    if (theme->font) {
        layout->font = theme->font;
    }

    int32 chunk_id = 0;
    chunk_iterate_start(&layout->hash_map.buf, chunk_id) {
        const HashEntryStrT<int32>* entry = (HashEntryStrT<int32> *) chunk_get_element((ChunkMemory *) &layout->hash_map.buf, chunk_id);

        const UIOffset* offset = ui_get_offset(layout, entry->value);
        if (!force_update && !offset->is_changed) {
            chunk_iterate_continue;
        }

        // @todo Don't update skeletons?!
        layout_update_element(layout, theme, offset);
    } chunk_iterate_end;
}

FORCE_INLINE
uint32 layout_element_from_location(const UILayout* layout, uint16 x, uint16 y) NO_EXCEPT
{
    // UI elements have a precision of 4 pixels
    return layout->chroma_codes.codes[layout->chroma_codes.width * y / 4 + x / 4];
}

#endif