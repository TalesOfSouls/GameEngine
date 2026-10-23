#pragma once
#ifndef COMS_UI_LAYOUT_C
#define COMS_UI_LAYOUT_C

#include "../stdlib/Stdlib.h"
#include "../stdlib/HashMapT.cpp"
#include "../asset/Asset.h"
#include "../camera/Camera.h"
#include "../system/FileUtils.cpp"

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

// Used if the hash map wasn't initialized by loading a layout file
#define UI_LAYOUT_DEFAULT_ELEMENT_COUNT 256

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

    // The cached vertices of the elements don't exist anymore (@see ui_cache)
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
    layout->dirty = true;

    LOG_1("[INFO] UI loaded layout");

    return (int32) (in - data);
}

// Creating elements from layout text files (.layouttxt)
#include "UILayoutTxt.cpp"

#endif
