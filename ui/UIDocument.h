#pragma once
#ifndef COMS_UI_DOCUMENT_H
#define COMS_UI_DOCUMENT_H

#include "../stdlib/Stdlib.h"
#include "../utils/StringUtils.h"

/**
 * Parsed ui text file (.layouttxt, .themetxt)
 *
 * Both formats use the same simple indentation based syntax:
 *
 *      version 2
 *
 *      // Comment (only full line comments)
 *      window test_window              <- block: "type name" (layout) or "name" (theme)
 *          pos: 512 256                <- property: "key: value" (belongs to the block above with less indentation)
 *          $title                      <- part: "$part [argument]" (fixed child of the block, e.g. the title of a window)
 *              background_color: #00FF00FF
 *          label status                <- child block (more indentation than the parent)
 *              text: Status: -
 *
 * Indentation: spaces, a tab counts as 4 spaces
 * Properties, parts and child blocks can be in any order
 *
 * The document only references the text (no copies), the values are interpreted by the user (@see UILayoutTxt.cpp)
 */

#define UI_DOCUMENT_TAB_WIDTH 4

enum UIDocumentNodeKind : byte {
    // "type [name]" in a layout, "name" in a theme
    UI_DOCUMENT_NODE_BLOCK,

    // "$part [argument]"
    UI_DOCUMENT_NODE_PART,
};

// Range in UIDocument::text
struct UIDocumentString {
    int32 offset;
    int32 length;
};

struct UIDocumentProperty {
    UIDocumentString key;
    UIDocumentString value;

    // Next property of the same node (-1 = none)
    int32 next;

    // Line number in the file (1-based) for error messages
    int32 line;
};

struct UIDocumentNode {
    // UIDocumentNodeKind
    byte kind;

    // Block: first word (element type or style name), part: part name without $
    UIDocumentString type;

    // Block: second word (element name), part: argument (e.g. index of a tab page)
    UIDocumentString name;

    // Line number in the file (1-based) for error messages
    int32 line;

    // -1 = none
    int32 first_property;
    int32 last_property;
    int32 first_child;
    int32 last_child;
    int32 next_sibling;
};

struct UIDocument {
    const char* text;
    int32 text_length;

    // "version N" in the first line, 0 = not defined
    int32 version;

    UIDocumentNode* nodes;
    int32 node_count;

    UIDocumentProperty* properties;
    int32 property_count;

    // First top level node (-1 = none), the others are linked through next_sibling
    int32 first_node;
};

/**
 * Upper bound for the amount of nodes/properties (= amount of lines)
 */
inline
int32 ui_document_line_count(const char* text, int32 length) NO_EXCEPT
{
    int32 lines = 1;
    for (int32 i = 0; i < length; ++i) {
        lines += text[i] == '\n';
    }

    return lines;
}

/**
 * Memory required for the node + property arrays
 */
FORCE_INLINE
int32 ui_document_memory_size(int32 line_count) NO_EXCEPT
{
    return (int32) ALIGN_UP(line_count * sizeof(UIDocumentNode), sizeof(size_t))
        + (int32) (line_count * sizeof(UIDocumentProperty));
}

FORCE_INLINE
bool ui_document_is_space(char c) NO_EXCEPT
{
    return c == ' ' || c == '\t' || c == '\r';
}

/**
 * Parses the text
 *
 * @param text      The text is NOT copied, it must stay valid as long as the document is used
 * @param memory    ui_document_memory_size(ui_document_line_count(text, length)) bytes, aligned to sizeof(size_t)
 *
 * @return Amount of errors (e.g. property without block), the lines with errors are ignored
 */
inline
int32 ui_document_parse(
    UIDocument* const __restrict doc,
    const char* __restrict text, int32 length,
    byte* __restrict memory
) NO_EXCEPT
{
    const int32 line_count = ui_document_line_count(text, length);

    doc->text = text;
    doc->text_length = length;
    doc->version = 0;
    doc->nodes = (UIDocumentNode *) memory;
    doc->node_count = 0;
    doc->properties = (UIDocumentProperty *) (memory + ALIGN_UP(line_count * sizeof(UIDocumentNode), sizeof(size_t)));
    doc->property_count = 0;
    doc->first_node = -1;

    // Open blocks/parts (indentation + node index)
    const int32 MAX_DEPTH = 64;
    int32 stack_indent[MAX_DEPTH];
    int32 stack_node[MAX_DEPTH];
    int32 stack_count = 0;

    int32 last_root = -1;
    int32 errors = 0;

    int32 pos = 0;
    int32 line_number = 0;
    while (pos < length) {
        ++line_number;

        // Line range [line_start, line_end) without the line break
        const int32 line_start = pos;
        while (pos < length && text[pos] != '\n') {
            ++pos;
        }

        int32 line_end = pos;
        ++pos;

        // Trailing whitespace (incl. \r)
        while (line_end > line_start && ui_document_is_space(text[line_end - 1])) {
            --line_end;
        }

        // Indentation
        int32 indent = 0;
        int32 i = line_start;
        while (i < line_end && (text[i] == ' ' || text[i] == '\t')) {
            indent += text[i] == '\t' ? UI_DOCUMENT_TAB_WIDTH : 1;
            ++i;
        }

        // Empty line or comment
        if (i >= line_end || (text[i] == '/' && i + 1 < line_end && text[i + 1] == '/')) {
            continue;
        }

        // First word (until whitespace or :)
        const int32 word_start = i;
        while (i < line_end && !ui_document_is_space(text[i]) && text[i] != ':') {
            ++i;
        }

        const int32 word_end = i;

        // Version line
        if (!doc->node_count && !doc->property_count && !doc->version
            && word_end - word_start == 7 && strncmp(text + word_start, "version", 7) == 0
            && i < line_end && text[i] != ':'
        ) {
            int32 version_start = word_end;
            while (version_start < line_end && ui_document_is_space(text[version_start])) {
                ++version_start;
            }

            doc->version = (int32) str_to_int(text + version_start);
            continue;
        }

        // Close all blocks with the same or a larger indentation
        while (stack_count > 0 && stack_indent[stack_count - 1] >= indent) {
            --stack_count;
        }

        const int32 parent = stack_count > 0 ? stack_node[stack_count - 1] : -1;

        if (i < line_end && text[i] == ':') {
            // Property "key: value"
            if (parent < 0) {
                LOG_1("[WARNING] UI document: property without block in line %d", {DATA_TYPE_INT32, &line_number});
                ++errors;

                continue;
            }

            int32 value_start = i + 1;
            while (value_start < line_end && ui_document_is_space(text[value_start])) {
                ++value_start;
            }

            const int32 property_index = doc->property_count++;
            UIDocumentProperty* const property = &doc->properties[property_index];
            property->key = {word_start, word_end - word_start};
            property->value = {value_start, line_end - value_start};
            property->next = -1;
            property->line = line_number;

            UIDocumentNode* const node = &doc->nodes[parent];
            if (node->last_property < 0) {
                node->first_property = property_index;
            } else {
                doc->properties[node->last_property].next = property_index;
            }

            node->last_property = property_index;

            continue;
        }

        // Block "type [name]" or part "$part [argument]"
        const bool is_part = text[word_start] == '$';
        if (is_part && parent < 0) {
            LOG_1("[WARNING] UI document: part without block in line %d", {DATA_TYPE_INT32, &line_number});
            ++errors;

            continue;
        }

        if (stack_count >= MAX_DEPTH) {
            LOG_1("[WARNING] UI document: nesting too deep in line %d", {DATA_TYPE_INT32, &line_number});
            ++errors;

            continue;
        }

        // Second word
        while (i < line_end && ui_document_is_space(text[i])) {
            ++i;
        }

        const int32 name_start = i;
        while (i < line_end && !ui_document_is_space(text[i])) {
            ++i;
        }

        const int32 node_index = doc->node_count++;
        UIDocumentNode* const node = &doc->nodes[node_index];
        node->kind = is_part ? UI_DOCUMENT_NODE_PART : UI_DOCUMENT_NODE_BLOCK;
        node->type = {word_start + (is_part ? 1 : 0), word_end - word_start - (is_part ? 1 : 0)};
        node->name = {name_start, i - name_start};
        node->line = line_number;
        node->first_property = -1;
        node->last_property = -1;
        node->first_child = -1;
        node->last_child = -1;
        node->next_sibling = -1;

        if (parent < 0) {
            if (last_root < 0) {
                doc->first_node = node_index;
            } else {
                doc->nodes[last_root].next_sibling = node_index;
            }

            last_root = node_index;
        } else {
            UIDocumentNode* const parent_node = &doc->nodes[parent];
            if (parent_node->last_child < 0) {
                parent_node->first_child = node_index;
            } else {
                doc->nodes[parent_node->last_child].next_sibling = node_index;
            }

            parent_node->last_child = node_index;
        }

        stack_indent[stack_count] = indent;
        stack_node[stack_count] = node_index;
        ++stack_count;
    }

    return errors;
}

FORCE_INLINE
bool ui_document_equals(const UIDocument* const doc, UIDocumentString str, const char* const literal) NO_EXCEPT
{
    const int32 length = (int32) strlen(literal);

    return str.length == length && strncmp(doc->text + str.offset, literal, length) == 0;
}

/**
 * Copies a string as null terminated string (truncated if the buffer is too small)
 *
 * @return The buffer
 */
FORCE_INLINE
char* ui_document_copy(const UIDocument* const doc, UIDocumentString str, char* const buffer, int32 buffer_size) NO_EXCEPT
{
    const int32 length = min_branched(str.length, buffer_size - 1);
    memcpy(buffer, doc->text + str.offset, length);
    buffer[length] = '\0';

    return buffer;
}

/**
 * First property with this key (-1 = none)
 */
inline
int32 ui_document_property_find(const UIDocument* const doc, const UIDocumentNode* const node, const char* const key) NO_EXCEPT
{
    for (int32 i = node->first_property; i >= 0; i = doc->properties[i].next) {
        if (ui_document_equals(doc, doc->properties[i].key, key)) {
            return i;
        }
    }

    return -1;
}

/**
 * Value of the first property with this key as null terminated string
 *
 * @return NULL if the property doesn't exist
 */
inline
const char* ui_document_property_get(
    const UIDocument* const doc,
    const UIDocumentNode* const node,
    const char* const key,
    char* const buffer, int32 buffer_size
) NO_EXCEPT
{
    const int32 index = ui_document_property_find(doc, node, key);
    if (index < 0) {
        return NULL;
    }

    return ui_document_copy(doc, doc->properties[index].value, buffer, buffer_size);
}

#endif
