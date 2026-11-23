/**
 * Data element: a named buffer of binary user data in the layout (e.g. the slots of a quick access bar)
 *
 * A normal element of the layout tree (usually a child of the element the data belongs to) that is never
 * drawn, laid out or hit tested (always hidden). Changing its bytes doesn't mark the layout dirty.
 *
 *      panel editor_hotbar
 *          data editor_hotbar_slots        <- "data <name>", the name is needed to save the bytes
 *              size: 12                    <- bytes
 *              version: 1                  <- version of the content (raise it when the structure changes)
 *
 * The application defines the structure (e.g. a struct, its own byte order)
 * and writes the defaults (the buffer starts zeroed). The layout file only reserves the memory.
 *
 * Saved user data (the binary user copy of the layout) is only applied if the size + version
 * still match -> a changed structure never receives old bytes, the application writes its defaults again
 * (or migrates the data itself). UI_DATA_FLAG_LOADED tells the application whether user data exists.
 *
 * Memory: the bytes + the path of the file they are saved in are stored in ui_element_buffer (like the text of
 * a label), the element references them by offset -> they disappear with the layout (ui_layout_reset)
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_UI_DATA_H
#define COMS_UI_DATA_H

#include "../stdlib/Stdlib.h"
#include "../stdlib/HashMapT.h"
#include "UICore.h"

// Same limit as the element names
#define UI_DATA_NAME_LENGTH HASH_MAP_MAX_KEY_LENGTH

enum UIDataFlag : byte {
    // The bytes are user data (applied from the user copy or changed by the application), not the zeroed defaults
    UI_DATA_FLAG_LOADED = 1 << 0,

    // Changed since they were saved
    UI_DATA_FLAG_DIRTY = 1 << 1,
};

struct UIData {
    UICore core;

    // Element name (the elements don't store their names, the bytes are saved by name)
    char name[UI_DATA_NAME_LENGTH];

    // @type UIDataFlag
    byte data_flags;

    // Version of the content (defined by the layout file)
    int32 version;

    // Size of the content in bytes (defined by the layout file)
    int32 size;

    // Offset of the bytes in ui_element_buffer
    int32 values;

    // Offset of the path of the file the bytes are saved in, 0 = not saved
    int32 file;
};

#endif
