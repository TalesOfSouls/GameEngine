#ifndef COMS_UI_LAYOUT_C
#define COMS_UI_LAYOUT_C

#include <string.h>
#include "../stdlib/Types.h"
#include "../stdlib/HashMap.h"
#include "../asset/Asset.h"
#include "../camera/Camera.h"
#include "../system/FileUtils.cpp"
#include "../compiler/CompilerUtils.h"

#include "UILayout.h"
#include "UITheme.h"
#include "UIElement.h"
#include "UIElementType.h"
#include "UIInput.h"
#include "UILabel.h"
#include "UIWindow.h"

// @todo We should add some asserts that ensure that the respective structs at least start at a 4byte memory alignment
// @performance We are prefetching stuff but we are not yet ensuring data is cache line aligned. We should align each UIElements, element specific data

// Doesn't change the position of pos outside of the function, since lookahead
static
void ui_layout_count_direct_children(UIElement* __restrict element, const char* __restrict pos, int32 parent_level)
{
    // Find amount of child elements
    // We have to perform a lookahead since this determins the size of our children array
    uint16 direct_child_elements = 0;

    int32 level;
    while (*pos != '\0') {
        level = 0;
        while (is_whitespace(*pos)) {
            ++pos;
            ++level;
        }

        if (level > parent_level + 4) {
            // This element is a childrens child and not a direct child
            str_move_past(&pos, '\n');

            continue;
        } else if (level <= parent_level) {
            // We are no longer inside of element
            str_move_past(&pos, '\n');

            break;
        } else if (is_eol(pos)) {
            pos += is_eol(pos);
            continue;
        }

        str_move_past(&pos, '\n');

        ++direct_child_elements;
    }

    element->children_count = direct_child_elements;
}

static
void ui_layout_assign_children(
    UILayout* __restrict layout,
    UIElement* __restrict element,
    const char* __restrict pos,
    int32 parent_level
) {
    int32 current_child_pos = 0;

    char block_name[HASH_MAP_MAX_KEY_LENGTH];

    while (*pos != '\0') {
        int32 level = 0;
        while (is_whitespace(*pos)) {
            ++pos;
            ++level;
        }

        if (level > parent_level + 4) {
            // This element is a childrens child and not a direct child
            str_move_past(&pos, '\n');

            continue;
        } else if (level <= parent_level) {
            // We are no longer inside of element
            str_move_past(&pos, '\n');

            break;
        }

        str_copy_move_until(block_name, &pos, ":");
        str_move_past(&pos, '\n');

        // Children array (located after the UIElement)
        uint32* children = (uint32 *) (element + 1);

        // Set child offset
        HashEntryInt32* child_entry = (HashEntryInt32 *) hashmap_get_entry(&layout->hash_map, block_name);
        children[current_child_pos] = child_entry->value;

        // Create a reference to the parent element for the child element
        UIElement* child_element = (UIElement *) (layout->data + child_entry->value);
        child_element->parent = (uint32) ((uintptr_t) element - (uintptr_t) layout->data);

        ++current_child_pos;
    }
}

// WARNING: theme needs to have memory already reserved and assigned to data
void layout_from_file_txt(
    UILayout* __restrict layout,
    const char* __restrict path,
    RingMemory* ring
) {
    FileBody file = {};
    file_read(path, &file, ring);
    ASSERT_SIMPLE(file.size);

    const char* pos = (char *) file.content;

    // move past the "version" string
    pos += 8;

    // Use version for different handling
    [[maybe_unused]] int32 version = (int32) str_to_int(pos, &pos); ++pos;

    // 1. Iteration: We have to find how many elements are defined in the layout file.
    // Therefore we have to do an initial iteration
    // We start at 1 since we always have a root element
    int32 temp_element_count = 1;
    while (*pos != '\0') {
        // Skip all white spaces
        str_skip_empty(&pos);
        if (*pos == '\0') {
            break;
        }

        ++temp_element_count;

        // Go to the next line
        str_move_past(&pos, '\n');
    }

    // 2. Iteration: Fill HashMap
    // @performance This is probably horrible since we are not using a perfect hashing function (1 hash -> 1 index)
    //      I wouldn't be surprised if we have a 50% hash overlap (2 hashes -> 1 index)
    hashmap_create(&layout->hash_map, temp_element_count, sizeof(HashEntryInt32), layout->data);
    int64 hm_size = hashmap_size(&layout->hash_map);

    pos = (char *) file.content;

    // move past version string
    str_move_past(&pos, '\n');

    char block_name[HASH_MAP_MAX_KEY_LENGTH];
    char block_type[32];

    // We store the UIElement and associated data after the hashmap
    byte* element_data = layout->data + hm_size;

    // Create root element
    UIElement* root = (UIElement *) element_data;
    hashmap_insert(&layout->hash_map, "root", (int32) (element_data - layout->data));
    ui_layout_count_direct_children(root, pos, -4);

    // NOTE: The root element cannot have any animations or vertices
    element_data += sizeof(UIElement) + sizeof(uint32) * root->children_count;

    while (*pos != '\0') {
        while (is_eol(pos)) {
            pos += is_eol(pos);
        }

        int32 level = 0;
        while (is_whitespace(*pos))  {
            ++pos;
            ++level;
        }

        if (is_eol(pos) || *pos == '\0') {
            continue;
        }

        str_copy_move_until(block_name, &pos, ":"); ++pos;
        str_copy_move_until(block_type, &pos, " \r\n");
        str_move_past(&pos, '\n');

        // Insert new element
        UIElement* element = (UIElement *) element_data;
        hashmap_insert(&layout->hash_map, block_name, (int32) ((uintptr_t) element_data - (uintptr_t) layout->data));

        element->type = (UIElementType) ui_element_type_to_id(block_type);

        // The children array is dynamic in size and comes directly after the UIElement
        ui_layout_count_direct_children(element, pos, level);

        element_data += sizeof(UIElement)
            + sizeof(uint32) * element->children_count; // Children offsets come after the UIElement

        // We put the state data after this element
        element->state = (uint32) ((uintptr_t) element_data - (uintptr_t) layout->data);
        element_data += ui_element_state_size(element->type);

        // We put the active element data after this element
        element->style_types[UI_STYLE_TYPE_ACTIVE] = (uint32) ((uintptr_t) element_data - (uintptr_t) layout->data);
        // @performance We should probably make sure the data is nicely aligned here
        element_data += ui_element_type_size(element->type);

        // We put the default element data after this element
        // Depending on the theme we will have also additional styles (e.g. :active, :hidden, ...)
        element->style_types[UI_STYLE_TYPE_DEFAULT] = (uint32) ((uintptr_t) element_data - (uintptr_t) layout->data);
        // @performance We should probably make sure the data is nicely aligned here
        element_data += ui_element_type_size(element->type);
    }

    // 3. Iteration: Create child references
    pos = (char *) file.content;

    // move past version string
    str_move_past(&pos, '\n');

    while (*pos != '\0') {
        while (is_eol(pos)) {
            pos += is_eol(pos);
        }

        int32 level = 0;
        while (is_whitespace(*pos))  {
            ++pos;
            ++level;
        }

        if (is_eol(pos) || *pos == '\0') {
            continue;
        }

        str_copy_move_until(block_name, &pos, ":");
        str_move_past(&pos, '\n');

        UIElement* element = (UIElement *) (layout->data + ((HashEntryInt32 *) hashmap_get_entry(&layout->hash_map, block_name))->value);
        ui_layout_assign_children(layout, element, pos, level);

        // ui_layout_assign_children doesn't move the pos pointer
        str_move_past(&pos, '\n');
    }

    // 4. Iteration: Create root child references
    pos = (char *) file.content;

    // move past version string
    str_move_past(&pos, '\n');

    uint32* root_children = (uint32 *) (root + 1);

    int32 child = 0;
    while (*pos != '\0') {
        while (is_eol(pos)) {
            pos += is_eol(pos);
        }

        if (is_whitespace(*pos)) {
            str_move_past(&pos, '\n');
            continue;
        }

        if (is_eol(pos) || *pos == '\0') {
            continue;
        }

        str_copy_move_until(block_name, &pos, ":");
        str_move_past(&pos, '\n');
        root_children[child++] = ((HashEntryInt32 *) hashmap_get_entry(&layout->hash_map, block_name))->value;
    }
}

static
void ui_layout_serialize_element_state(UIElementType type, const void* __restrict state, byte** __restrict pos) {
    switch (type) {
        case UI_ELEMENT_TYPE_INPUT: {
            ui_input_state_serialize((UIInputState *) state, pos);
        } break;
    }
}

static
void ui_layout_serialize_element_detail(UIElementType type, const void* __restrict details, byte** __restrict pos) {
    switch (type) {
        case UI_ELEMENT_TYPE_INPUT: {
            ui_input_element_serialize((UIInput *) details, pos);
        } break;
    }
}

static
void ui_layout_serialize_element(
    HashEntryInt32* entry,
    byte* data,
    byte** out
) {
    // @performance Are we sure the data is nicely aligned?
    // Probably depends on the from_txt function and the start of layout->data
    UIElement* element = (UIElement *) (data + entry->value);

    **out = element->state_flag;
    *out += sizeof(element->state_flag);

    **out = element->type;
    *out += sizeof(element->type);

    **out = element->style_old;
    *out += sizeof(element->style_old);

    **out = element->style_new;
    *out += sizeof(element->style_new);

    *((uint32 *) *out) = SWAP_ENDIAN_LITTLE(element->parent);
    *out += sizeof(element->parent);

    *((uint32 *) *out) = SWAP_ENDIAN_LITTLE(element->state);
    *out += sizeof(element->state);

    // Details
    for (int32 i = 0; i < UI_STYLE_TYPE_SIZE; ++i) {
        *((uint32 *) *out) = SWAP_ENDIAN_LITTLE(element->style_types[i]);
        *out += sizeof(element->style_types[i]);
    }

    *((uint16 *) *out) = SWAP_ENDIAN_LITTLE(element->children_count);
    *out += sizeof(element->children_count);

    /* We don't save the animation state since that is always 0 in the file
    memset(*out, 0, sizeof(element->animation_state));
    *out += sizeof(element->animation_state);
    */

    *((uint16 *) *out) = SWAP_ENDIAN_LITTLE(element->animation_count);
    *out += sizeof(element->animation_count);

    *((uint32 *) *out) = SWAP_ENDIAN_LITTLE(element->animations);
    *out += sizeof(element->animations);

    *((uint16 *) *out) = SWAP_ENDIAN_LITTLE(element->vertex_count_max);
    *out += sizeof(element->vertex_count_max);

    *((uint16 *) *out) = SWAP_ENDIAN_LITTLE(element->vertex_count_active);
    *out += sizeof(element->vertex_count_active);

    *((uint32 *) *out) = SWAP_ENDIAN_LITTLE(element->vertices_active_offset);
    *out += sizeof(element->vertices_active_offset);

    // Output dynamic length content directly after UIElement
    //
    // WARNING: The data ordering in our output data is not necessarily the same as in memory ESPECIALLY for animations
    //          However, we can simply reconstruct the memory order by reversing the logic
    //
    // @todo We could optimize the memory layout of our data 9e.g. ->style_types, children, ... to be more packed
    //  At this point we may have this data available now (if we save a cached version = layout+theme)
    //  Obviously, this won't have an effect on the current run-tim but would make the memory layout nicer on the next load
    //  It would be kind of a self-optimizing ui layout system :).
    //  Of course, updating the reference values (uint32) will be challenging since the file out will still not be the same as the offset due to alignment and padding
    //  We would probably need a helper_offset value that gets passed around also as parameter of this function
    //////////////////////////////////////

    // Children array
    uint32* children = (uint32 *) (element + 1);
    for (int32 i = 0; i < element->children_count; ++i) {
        *((uint32 *) *out) = SWAP_ENDIAN_LITTLE(children[i]);
        *out += sizeof(*children);
    }

    // State element data e.g. UIInputState
    ui_layout_serialize_element_state(element->type, data + element->state, out);

    // detailed element data/style_types e.g. UIInput
    // When you create a layout this is should only contain the default style type
    // BUT we also support layout caching where a fully parsed layout+theme can be saved and loaded
    // This is very fast since now we don't need to build the layout based on the theme as long as the theme and window dimensions didn't change
    for (int32 i = 0; i < UI_STYLE_TYPE_SIZE; ++i) {
        if (!element->style_types[i]) {
            continue;
        }

        ui_layout_serialize_element_detail(element->type, data + element->style_types[i], out);
    }

    UIAnimation* animations = (UIAnimation *) (data + element->animations);
    int32 element_style_type_size = ui_element_type_size(element->type);

    for (int32 i = 0; i < element->animation_count; ++i) {
        **out = animations[i].style_old;
        *out += sizeof(animations[i].style_old);

        **out = animations[i].style_new;
        *out += sizeof(animations[i].style_new);

        *((uint16 *) *out) = SWAP_ENDIAN_LITTLE(animations[i].duration);
        *out += sizeof(animations[i].duration);

        **out = animations[i].anim_type;
        *out += sizeof(animations[i].anim_type);

        **out = animations[i].keyframe_count;
        *out += sizeof(animations[i].keyframe_count);

        // The keyframes are the element detail information (e.g. UIInput) and they are located after the respective Animation definition
        byte* keyframes = (byte *) (&animations[i] + 1);
        for (int32 j = 0; j < animations[i].keyframe_count; ++j) {
            ui_layout_serialize_element_detail(element->type, keyframes + j * element_style_type_size, out);
        }
    }
}

int32 layout_to_data(
    const UILayout* __restrict layout,
    byte* __restrict data
) {
    LOG_1("Save layout");
    byte* out = data;

    // version
    *((int32 *) out) = SWAP_ENDIAN_LITTLE(UI_LAYOUT_VERSION);
    out += sizeof(int32);

    // hashmap
    out += hashmap_dump(&layout->hash_map, out);

    // UIElement data
    uint32 chunk_id = 0;
    chunk_iterate_start(&layout->hash_map.buf, chunk_id) {
        HashEntryInt32* entry = (HashEntryInt32 *) chunk_get_element((ChunkMemory *) &layout->hash_map.buf, chunk_id);
        ui_layout_serialize_element(entry, layout->data, &out);
    } chunk_iterate_end;

    LOG_1("Saved layout");

    return (int32) (out - data);
}

static
void ui_layout_unserialize_element_state(UIElementType type, void* __restrict state, const byte** __restrict pos) {
    switch (type) {
        case UI_ELEMENT_TYPE_INPUT: {
            ui_input_state_unserialize((UIInputState *) state, pos);
        } break;
    }
}

static
void ui_layout_unserialize_element_detail(UIElementType type, void* __restrict details, const byte** __restrict pos) {
    switch (type) {
        case UI_ELEMENT_TYPE_INPUT: {
            ui_input_element_unserialize((UIInput *) details, pos);
        } break;
    }
}

static
void ui_layout_parse_element(HashEntryInt32* entry, byte* data, const byte** in)
{
    // @performance Are we sure the data is nicely aligned?
    // Probably depends on the from_txt function and the start of layout->data
    UIElement* element = (UIElement *) (data + entry->value);

    element->state_flag = **in;
    *in += sizeof(element->state_flag);

    element->type = (UIElementType) **in;
    *in += sizeof(element->type);

    element->style_old = (UIStyleType) **in;
    *in += sizeof(element->style_old);

    element->style_new = (UIStyleType) **in;
    *in += sizeof(element->style_new);

    element->parent = SWAP_ENDIAN_LITTLE(*((uint32 *) *in));
    *in += sizeof(element->parent);

    element->state = SWAP_ENDIAN_LITTLE(*((uint32 *) *in));
    *in += sizeof(element->state);

    // Details
    for (int32 i = 0; i < UI_STYLE_TYPE_SIZE; ++i) {
        element->style_types[i] = SWAP_ENDIAN_LITTLE(*((uint32 *) *in));
        *in += sizeof(element->style_types[i]);
    }

    element->children_count = SWAP_ENDIAN_LITTLE(*((uint16 *) *in));
    *in += sizeof(element->children_count);

    // @question Do we really have to do that? Shouldn't the animation_state data be 0 anyways or could there be garbage values?
    memset(&element->animation_state, 0, sizeof(element->animation_state));

    element->animation_count = SWAP_ENDIAN_LITTLE(*((uint16 *) *in));
    *in += sizeof(element->animation_count);

    element->animations = SWAP_ENDIAN_LITTLE(*((uint32 *) *in));
    *in += sizeof(element->animations);

    element->vertex_count_max = SWAP_ENDIAN_LITTLE(*((uint16 *) *in));
    *in += sizeof(element->vertex_count_max);

    element->vertex_count_active = SWAP_ENDIAN_LITTLE(*((uint16 *) *in));
    *in += sizeof(element->vertex_count_active);

    element->vertices_active_offset = SWAP_ENDIAN_LITTLE(*((uint32 *) *in));
    *in += sizeof(element->vertices_active_offset);

    // Load dynamic length content
    // Some of the content belongs directly after the element but some of it belongs at very specific offsets
    // The reason for that is that the offsets are stored e.g. in element->state
    // The memory is fragmented since a lot of the information is split up in different files (layout file and theme file)
    // Therefor, we cannot create a nice memory layout when loading a layout+theme
    //
    // @question Can we optimize the memory layout to a less fragmented version?
    //      One solution could be to combine layout file and theme file. In that case we always know the correct element count
    //      Or see the serialization function for more comments
    //////////////////////////////////////

    // Children array
    uint32* children = (uint32 *) (element + 1);
    for (int32 i = 0; i < element->children_count; ++i) {
        children[i] = SWAP_ENDIAN_LITTLE(*((uint32 *) *in));
        *in += sizeof(*children);
    }

    // State element data e.g. UIInputState
    ui_layout_unserialize_element_state(element->type, data + element->state, in);

    // detailed element data/style_types e.g. UIInput
    for (int32 i = 0; i < UI_STYLE_TYPE_SIZE; ++i) {
        if (!element->style_types[i]) {
            continue;
        }

        ui_layout_unserialize_element_detail(element->type, data + element->style_types[i], in);
    }

    UIAnimation* animations = (UIAnimation *) (data + element->animations);
    int32 element_style_type_size = ui_element_type_size(element->type);

    for (int32 i = 0; i < element->animation_count; ++i) {
        animations[i].style_old = (UIStyleType) **in;
        *in += sizeof(animations[i].style_old);

        animations[i].style_new = (UIStyleType) **in;
        *in += sizeof(animations[i].style_new);

        animations[i].duration = SWAP_ENDIAN_LITTLE(*((uint16 *) *in));
        *in += sizeof(animations[i].duration);

        animations[i].anim_type = (AnimationEaseType) **in;
        *in += sizeof(animations[i].anim_type);

        animations[i].keyframe_count = **in;
        *in += sizeof(animations[i].keyframe_count);

        // The keyframes are the element detail information (e.g. UIInput) and they are located after the respective Animation definition
        byte* keyframes = (byte *) (&animations[i] + 1);
        for (int32 j = 0; j < animations[i].keyframe_count; ++j) {
            ui_layout_unserialize_element_detail(element->type, keyframes + j * element_style_type_size, in);
        }
    }
}

// The size of layout->data should be the file size + a bunch of additional data for additional theme dependent "UIElements->style_types".
// Yes, this means we have a little too much data but not by a lot
int32 layout_from_data(
    const byte* __restrict data,
    UILayout* __restrict layout
) {
    PROFILE(PROFILE_LAYOUT_FROM_DATA, NULL, false, true);
    LOG_1("Load layout");

    const byte* in = data;

    int32 version = SWAP_ENDIAN_LITTLE(*((int32 *) in));
    in += sizeof(version);

    // Prepare hashmap (incl. reserve memory) by initializing it the same way we originally did
    // Of course we still need to populate the data using hashmap_load()
    hashmap_create(&layout->hash_map, (int32) SWAP_ENDIAN_LITTLE(*((uint32 *) in)), sizeof(HashEntryInt32), layout->data);

    in += hashmap_load(&layout->hash_map, in);

    // layout data
    // @performance We are iterating the hashmap twice (hashmap_load and here)
    uint32 chunk_id = 0;
    chunk_iterate_start(&layout->hash_map.buf, chunk_id) {
        HashEntryInt32* entry = (HashEntryInt32 *) chunk_get_element((ChunkMemory *) &layout->hash_map.buf, chunk_id);
        ui_layout_parse_element(entry, layout->data, &in);
    } chunk_iterate_end;

    layout->layout_size = (uint32) (in - data);

    LOG_1("Loaded layout");

    return (int32) layout->layout_size;
}

// @performance Implement a way to only load a specific element and all its children
// This way we can re-load specific elements on change and we could also greatly reduce the setup time by ignoring ui elements that are rarely visible

void layout_from_theme(
    UILayout* __restrict layout,
    const UIThemeStyle* __restrict theme
) {
    PROFILE(PROFILE_LAYOUT_FROM_THEME, NULL, false, true);
    LOG_1("Load theme for layout");

    // @todo Handle animations
    // @todo Handle vertices_active offset
    if (theme->font) {
        layout->font = theme->font;
    }

    // Current position where we can the different sub elements (e.g. :hover, :active, ...)
    // We make sure that the offset is a multiple of 8 bytes for better alignment
    uint32 dynamic_pos = ROUND_TO_NEAREST(layout->layout_size, 8);

    // We first need to handle the default element -> iterate all elements but only handle the default style
    // The reason for this is, later on in the specialized style we use the base style and copy it over as foundation
    uint32 chunk_id = 0;
    chunk_iterate_start(&theme->hash_map.buf, chunk_id) {
        HashEntryInt32* style_entry = (HashEntryInt32 *) chunk_get_element((ChunkMemory *) &theme->hash_map.buf, chunk_id);

        // We don't handle special styles here, only the default one
        if (str_find(style_entry->key, ':')) {
            continue;
        }

        // +1 to skip '#' or '.'
        HashEntryInt32* entry = (HashEntryInt32 *) hashmap_get_entry(&layout->hash_map, style_entry->key + 1);
        if (!entry) {
            // Couldn't find the base element
            continue;
        }

        // Populate default element
        UIElement* element = (UIElement *) (layout->data + entry->value);
        UIAttributeGroup* group = (UIAttributeGroup *) (theme->data + style_entry->value);

        // @todo Continue implementation
        switch (element->type) {
            case UI_ELEMENT_TYPE_LABEL: {
                ui_label_state_populate(group, (UILabelState *) (layout->data + element->state));
                ui_label_element_populate(
                    layout,
                    element,
                    group,
                    (UILabel *) (layout->data + element->style_types[UI_STYLE_TYPE_DEFAULT])
                );
            } break;
            case UI_ELEMENT_TYPE_INPUT: {
                ui_input_state_populate(group, (UIInputState *) (layout->data + element->state));
                ui_input_element_populate(
                    layout,
                    element,
                    group,
                    (UIInput *) (layout->data + element->style_types[UI_STYLE_TYPE_DEFAULT])
                );
            } break;
            case UI_ELEMENT_TYPE_VIEW_WINDOW: {
                ui_window_state_populate(group, (UIWindowState *) (layout->data + element->state));
                ui_window_element_populate(
                    layout,
                    element,
                    group,
                    (UIWindow *) (layout->data + element->style_types[UI_STYLE_TYPE_DEFAULT])
                );
            } break;
        }
    } chunk_iterate_end;

    // We iterate every style
    //      1. Fill default element if it is default style
    //      2. Create and fill new element if it isn't default style (e.g. :hover)
    // @performance It is dumb that we iterate here again (see iteration above). It would be nice to combine both iterations
    // If we could see if the default element is already populated we could easily combine this
    // We could use a helper array to keep track of initialized chunk_id but we also don't have access to malloc/ring memory here
    chunk_id = 0;
    chunk_iterate_start(&theme->hash_map.buf, chunk_id) {
        HashEntryInt32* style_entry = (HashEntryInt32 *) chunk_get_element((ChunkMemory *) &theme->hash_map.buf, chunk_id);

        // We only handle special styles here, not the default one
        const char* special = str_find(style_entry->key, ':');
        if (!special) {
            // The default element was already handled outside this loop
            continue;
        }

        char pure_name[HASH_MAP_MAX_KEY_LENGTH];
        // +1 to skip '#' or '.'
        str_copy_until(style_entry->key + 1, pure_name, ':');

        HashEntryInt32* entry = (HashEntryInt32 *) hashmap_get_entry(&layout->hash_map, pure_name);
        if (!entry) {
            // Couldn't find the base element
            continue;
        }

        UIElement* element = (UIElement *) (layout->data + entry->value);
        UIStyleType style_type = (UIStyleType) ui_style_type_to_id(special);

        // Doesn't exist (usually the first load, but exists when we resize our window)
        if (!element->style_types[style_type]) {
            element->style_types[style_type] = dynamic_pos;
            dynamic_pos += ui_element_type_size(element->type);
        }

        // The style inherits from the default style/element
        memcpy(
            layout->data + element->style_types[style_type],
            layout->data + element->style_types[UI_STYLE_TYPE_DEFAULT],
            ui_element_type_size(element->type)
        );

        // Populate element style_types
        UIAttributeGroup* group = (UIAttributeGroup *) (theme->data + style_entry->value);

        // @todo Continue implementation
        switch (element->type) {
            case UI_ELEMENT_TYPE_LABEL: {
                ui_label_element_populate(
                    layout,
                    element,
                    group,
                    (UILabel *) (layout->data + element->style_types[style_type])
                );
            } break;
            case UI_ELEMENT_TYPE_INPUT: {
                ui_input_element_populate(
                    layout,
                    element,
                    group,
                    (UIInput *) (layout->data + element->style_types[style_type])
                );
            } break;
            case UI_ELEMENT_TYPE_VIEW_WINDOW: {
                ui_window_element_populate(
                    layout,
                    element,
                    group,
                    (UIWindow *) (layout->data + element->style_types[style_type])
                );
            } break;
        }
    } chunk_iterate_end;
}

void ui_layout_update(UILayout* layout, UIElement* element) {
    if (element->style_new != element->style_old
        && (element->state_flag & UI_ELEMENT_STATE_CHANGED)
        && (element->state_flag & UI_ELEMENT_STATE_ANIMATION)
    ) {
        // @todo Even if an animation is ongoing we might not want to update if the last step is < n ms ago
        switch (element->type) {
            case UI_ELEMENT_TYPE_BUTTON: {

                } break;
            case UI_ELEMENT_TYPE_SELECT: {

                } break;
            case UI_ELEMENT_TYPE_INPUT: {
                    ui_input_element_update(layout, element);
                } break;
            case UI_ELEMENT_TYPE_LABEL: {
                    ui_label_element_update(layout, element);
                } break;
            case UI_ELEMENT_TYPE_TEXT: {

                } break;
            case UI_ELEMENT_TYPE_TEXTAREA: {

                } break;
            case UI_ELEMENT_TYPE_IMAGE: {

                } break;
            case UI_ELEMENT_TYPE_LINK: {

                } break;
            case UI_ELEMENT_TYPE_TABLE: {

                } break;
            case UI_ELEMENT_TYPE_VIEW_WINDOW: {

                } break;
            case UI_ELEMENT_TYPE_VIEW_PANEL: {

                } break;
            case UI_ELEMENT_TYPE_VIEW_TAB: {

                } break;
            case UI_ELEMENT_TYPE_CURSOR: {

                } break;
            default:
                UNREACHABLE();
        }
    }

    LOG_1("Loaded theme for layout");
}

// @question We might want to change the names of update/render
// In some cases it's more like cache/render
// @question We might want to allow rendering without caching (currently we always rely on the cache)
// This increases our RAM requirements (every vertex is in cache AND in the asset AND in VRAM)
// However, this also has the benefit of allowing us to ONLY re-render individual elements

// @performance Profile our prefetching, no sure if it is actually helpful or harmful

// @performance In our immediate mode solution we decided the update/render based on a bitfield
// That is very efficient, the code below isn't doing that maybe there is a way to implement that here as well?
// I don't think so but it would be nice
// This function caches the vertices
void ui_layout_update_dfs(UILayout* layout, UIElement* element, byte category = 0) {
    if (element->type == UI_ELEMENT_TYPE_MANUAL
        || !(element->state_flag & UI_ELEMENT_STATE_VISIBLE)
        || !(element->state_flag & UI_ELEMENT_STATE_CHANGED)
    ) {
        return;
    }

    if (element->category == category) {
        ui_layout_update(layout, element);
    }

    if (element->children_count) {
        uint32* children = (uint32 *) (element + 1);
        for (int32 i = 0; i < element->children_count - 1; ++i) {
            intrin_prefetch_l2(layout->data + children[i + 1]);
            ui_layout_update(layout, (UIElement *) (layout->data + children[i]));
        }

        ui_layout_update(layout, (UIElement *) (layout->data + children[element->children_count - 1]));
    }
}

uint32 ui_layout_render_dfs(
    UILayout* layout,
    UIElement* element, Vertex3DSamplerTextureColor* __restrict vertices,
    byte category = 0
) {
    if (element->type == UI_ELEMENT_TYPE_MANUAL
        || !(element->state_flag & UI_ELEMENT_STATE_VISIBLE)
    ) {
        return 0;
    }

    uint32 vertex_count = 0;

    if (element->vertex_count_active && element->category == category) {
        memcpy(vertices, layout->vertices_active + element->vertices_active_offset, sizeof(*vertices) * element->vertex_count_active);
        vertices += element->vertex_count_active;
        vertex_count += element->vertex_count_active;
    }

    if (element->children_count) {
        uint32* children = (uint32 *) (element + 1);
        for (int32 i = 0; i < element->children_count - 1; ++i) {
            intrin_prefetch_l2(layout->data + children[i + 1]);

            uint32 child_vertex_count = ui_layout_render_dfs(layout, (UIElement *) (layout->data + children[i]), vertices, category);
            vertices += child_vertex_count;
            vertex_count += child_vertex_count;
        }

        uint32 child_vertex_count = ui_layout_render_dfs(layout, (UIElement *) (layout->data + children[element->children_count - 1]), vertices, category);
        vertices += child_vertex_count;
        vertex_count += child_vertex_count;
    }

    return vertex_count;
}

uint32 ui_layout_update_render_dfs(
    UILayout* layout,
    UIElement* __restrict element, Vertex3DSamplerTextureColor* __restrict vertices,
    byte category = 0
) {
    if (element->type == UI_ELEMENT_TYPE_MANUAL
        || !(element->state_flag & UI_ELEMENT_STATE_VISIBLE)
    ) {
        return 0;
    }

    uint32 vertex_count = 0;

    if (element->category == category) {
        ui_layout_update(layout, element);

        memcpy(vertices, layout->vertices_active + element->vertices_active_offset, sizeof(*vertices) * element->vertex_count_active);
        vertices += element->vertex_count_active;
        vertex_count += element->vertex_count_active;
    }

    if (element->children_count) {
        uint32* children = (uint32 *) (element + 1);
        for (int32 i = 0; i < element->children_count - 1; ++i) {
            intrin_prefetch_l2(layout->data + children[i + 1]);

            uint32 child_vertex_count = ui_layout_update_render_dfs(layout, (UIElement *) (layout->data + children[i]), vertices, category);
            vertices += child_vertex_count;
            vertex_count += child_vertex_count;
        }

        uint32 child_vertex_count = ui_layout_update_render_dfs(layout, (UIElement *) (layout->data + children[element->children_count - 1]), vertices, category);
        vertices += child_vertex_count;
        vertex_count += child_vertex_count;
    }

    return vertex_count;
}

inline
uint32 layout_element_from_location(UILayout* layout, uint16 x, uint16 y) noexcept
{
    return layout->ui_chroma_codes[layout->width * y / 4 + x / 4];
}

inline
UIElement* layout_get_element(const UILayout* __restrict layout, const char* __restrict element) noexcept
{
    HashEntryInt32* entry = (HashEntryInt32 *) hashmap_get_entry((HashMap *) &layout->hash_map, element);
    if (!entry) {
        return NULL;
    }

    return (UIElement *) (layout->data + entry->value);
}

inline
void* layout_get_element_state(const UILayout* layout, UIElement* element) noexcept
{
    return layout->data + element->state;
}

inline
void* layout_get_element_style(const UILayout* layout, UIElement* element, UIStyleType style_type) noexcept
{
    if (!element) {
        return NULL;
    }

    return layout->data + element->style_types[style_type];
}

inline
UIElement* layout_get_element_parent(const UILayout* layout, UIElement* element) noexcept
{
    if (!element) {
        return NULL;
    }

    return (UIElement *) (layout->data + element->parent);
}

inline
UIElement* layout_get_element_child(const UILayout* layout, UIElement* element, uint16 child) noexcept
{
    if (!element) {
        return NULL;
    }

    uint16* children = (uint16 *) (element + 1);
    return (UIElement *) (layout->data + children[child]);
}

#endif