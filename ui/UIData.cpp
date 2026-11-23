/**
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_UI_DATA_C
#define COMS_UI_DATA_C

#include "../stdlib/Stdlib.h"
#include "../utils/StringUtils.h"
#include "UIData.h"
#include "UICore.cpp"

/**
 * Stores the path of the file the data elements of a layout file are saved in
 *
 * @return int32 Offset of the path in ui_element_buffer (UIData::file)
 */
inline
int32 ui_data_file_create(UILayout* const __restrict layout, const wchar_t* const __restrict path) NO_EXCEPT
{
    const int32 length = (int32) str_length(path);
    wchar_t* const memory = (wchar_t *) memory_get(
        &layout->ui_element_buffer, (length + 1) * (int32) sizeof(wchar_t), (int32) alignof(wchar_t)
    );
    ASSERT_TRUE(memory);

    memcpy(memory, path, (length + 1) * sizeof(wchar_t));

    return (int32) ((byte *) memory - layout->ui_element_buffer.memory);
}

/**
 * Path of the file the bytes of a data element are saved in
 *
 * @return const wchar_t* NULL if the bytes are not saved
 */
FORCE_INLINE
const wchar_t* ui_data_file(const UILayout* const __restrict layout, const UIData* const __restrict data) NO_EXCEPT
{
    return data->file ? (const wchar_t *) (layout->ui_element_buffer.memory + data->file) : NULL;
}

FORCE_INLINE
byte* ui_data_values(const UILayout* const __restrict layout, const UIData* const __restrict data) NO_EXCEPT
{
    return layout->ui_element_buffer.memory + data->values;
}

/**
 * Creates a data element, the bytes are 0
 *
 * @param UILayout*     layout      Layout
 * @param const char*   name        Name (also needed for saving), the element is NOT named by this function
 * @param int32         name_length Name length
 * @param int32         size        Size of the content in bytes
 * @param int32         version     Version of the content
 * @param int32         file        File the bytes are saved in (ui_data_file_create), 0 = not saved
 * @param UICore*       parent      NULL = root
 *
 * @return UIData*
 */
inline
UIData* ui_data_create(
    UILayout* const __restrict layout,
    const char* const __restrict name, int32 name_length,
    int32 size, int32 version,
    int32 file,
    UICore* parent = NULL
) NO_EXCEPT
{
    ASSERT_TRUE(size >= 0);

    UIData* const data = UI_ELEMENT_ALLOC(layout, UI_ELEMENT_TYPE_DATA, UIData, parent);

    // Never drawn, laid out or hit tested
    data->core.state |= UI_STATE_HIDDEN;
    data->core.flags = UI_ELEMENT_FLAG_IGNORE_HIT;

    const int32 length = OMS_MIN(name_length, UI_DATA_NAME_LENGTH - 1);
    memcpy(data->name, name, length);
    data->name[length] = '\0';

    data->version = version;
    data->size = size;
    data->file = file;

    // The bytes follow the element
    byte* const values = memory_get(&layout->ui_element_buffer, OMS_MAX(size, 1), 8);
    ASSERT_TRUE(values);
    memset(values, 0, OMS_MAX(size, 1));
    data->values = (int32) (values - layout->ui_element_buffer.memory);

    return data;
}

/**
 * Data element by name
 * The size + version are the structure the application expects, a different layout definition is an error
 * (e.g. the struct changed but the version in the layout file didn't)
 *
 * @param UILayout*     layout  Layout
 * @param const char*   name    Element name
 * @param int32         size    Expected size of the content
 * @param int32         version Expected version of the content
 *
 * @return UIData* NULL if there is no data element with this name, size + version
 * 
 * @question Do I want to replace the version 
 */
inline
UIData* ui_data_get(
    const UILayout* const __restrict layout,
    const char* const __restrict name,
    int32 size, int32 version
) NO_EXCEPT
{
    UICore* const element = ui_element_get(layout, name);
    if (!element || element->type != UI_ELEMENT_TYPE_DATA) {
        return NULL;
    }

    UIData* const data = (UIData *) element;
    if (data->size != size || data->version != version) {
        LOG_1(
            "[WARNING] UI data %s: the layout defines size %d version %d, expected %d version %d",
            {DATA_TYPE_CHAR_STR, (void *) data->name},
            {DATA_TYPE_INT32, (void *) &data->size}, {DATA_TYPE_INT32, (void *) &data->version},
            {DATA_TYPE_INT32, (void *) &size}, {DATA_TYPE_INT32, (void *) &version}
        );

        return NULL;
    }

    return data;
}

/**
 * Marks the bytes as changed user data -> the application saves them (e.g. once per tick, several changes = one write)
 * The layout is NOT marked dirty (the element isn't drawn)
 */
FORCE_INLINE
void ui_data_changed(UIData* const data) NO_EXCEPT
{
    data->data_flags |= UI_DATA_FLAG_LOADED | UI_DATA_FLAG_DIRTY;
}

/**
 * Next data element in pre-order (all elements are walked, also hidden subtrees)
 *
 * @param const UILayout*   layout  Layout
 * @param int32*            offset  In: offset to start after (-1 = from the root), out: offset of the result
 *
 * @return UIData* NULL if there is none
 */
inline
UIData* ui_data_next(const UILayout* const __restrict layout, int32* const __restrict offset) NO_EXCEPT
{
    if (!layout->ui_root) {
        return NULL;
    }

    int32 next = *offset < 0 ? 0 : ui_element_get(layout, *offset)->next_offset;
    if (*offset >= 0 && !next) {
        return NULL;
    }

    do {
        UICore* const element = ui_element_get(layout, next);
        if (element->type == UI_ELEMENT_TYPE_DATA) {
            *offset = next;

            return (UIData *) element;
        }

        next = element->next_offset;
    } while (next);

    return NULL;
}

#endif
