/**
 * Binary layout files (.layoutbin)
 *
 * The parsed layout document (@see UIDocument) + the values of its data elements (@see UIData.h).
 * The elements are created from the document exactly like from the text file (ui_layout_from_document):
 * the styles of the theme + the strings of the language are applied when the elements are created,
 * several layout files can still be added to one UILayout.
 *
 * Used for
 *      1. The compiled layouts of a release (theme_builder: .layouttxt -> .layoutbin)
 *      2. The user copy of a layout (e.g. user/themes/<theme>/<name>.layoutbin): the elements of the original
 *         file + the values of its data elements changed by the user. Saving the user data only rewrites the
 *         data section (ui_layout_bin_data_update).
 *         The data elements are created zeroed, the data section is applied afterwards: by name, only if the
 *         size + version still match (a changed structure never receives old bytes, UI_DATA_FLAG_LOADED stays 0
 *         -> the application writes its defaults). Only data elements with user data are written.
 *         source_hash = hash of the original file -> a changed original (e.g. an update) is detected,
 *         the user copy is created again and the user data is carried over by name.
 *
 * All numbers are little endian, every field is written separately (no struct dumps -> independent of
 * the struct layout of a build).
 *
 *      offset  size    type        field
 *      0       4       uint32      magic UI_LAYOUT_BIN_MAGIC ("LDOC")
 *      4       4       int32       version UI_LAYOUT_BIN_VERSION
 *      8       8       uint64      source_hash (hash of the original file, 0 = unknown)
 *      16      4       int32       document version (version line of the text file)
 *      20      4       int32       node_count (incl. the reserved empty node 0)
 *      24      4       int32       property_count (incl. the reserved empty property 0)
 *      28      4       int32       pool_size (bytes of the string pool)
 *      32      4       int32       first_node (first top level node)
 *      36      4       int32       data_offset (byte offset of the data section)
 *      40      N * 41  -           nodes (node 0 included)
 *                                      byte kind, int32 type offset/length, int32 name offset/length,
 *                                      int32 line, first_property, last_property, first_child, last_child, next_sibling
 *              P * 24  -           properties (property 0 included)
 *                                      int32 key offset/length, int32 value offset/length, int32 next, int32 line
 *              S       char[S]     string pool (all strings of the nodes + properties, the offsets point into it)
 *      data_offset                 data section (the data elements of the file with user data)
 *              4       int32       element count
 *                                  per element: char[UI_LAYOUT_BIN_DATA_NAME_LENGTH] name, int32 version,
 *                                               int32 size, byte[size] content (as it is, the application defines it)
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_UI_LAYOUT_BIN_H
#define COMS_UI_LAYOUT_BIN_H

#include "../stdlib/Stdlib.h"
#include "../hash/GeneralHash.h"
#include "UIDocument.h"
#include "UIData.h"
#include "UILayout.h"

#define UI_LAYOUT_BIN_MAGIC 0x434F444C // "LDOC"
// 2: data elements with binary content (size + version) instead of typed values
#define UI_LAYOUT_BIN_VERSION 2

#define UI_LAYOUT_BIN_HEADER_SIZE 40
#define UI_LAYOUT_BIN_NODE_SIZE 41
#define UI_LAYOUT_BIN_PROPERTY_SIZE 24
// Fixed size of the names in the data section (>= UI_DATA_NAME_LENGTH)
#define UI_LAYOUT_BIN_DATA_NAME_LENGTH 32
#define UI_LAYOUT_BIN_DATA_ENTRY_HEADER_SIZE (UI_LAYOUT_BIN_DATA_NAME_LENGTH + 4 + 4)

static_assert(UI_LAYOUT_BIN_DATA_NAME_LENGTH >= UI_DATA_NAME_LENGTH, "The data names must fit into the data section");

struct UILayoutBinHeader {
    uint32 magic;
    int32 version;
    uint64 source_hash;
    int32 doc_version;
    int32 node_count;
    int32 property_count;
    int32 pool_size;
    int32 first_node;
    int32 data_offset;
};

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
