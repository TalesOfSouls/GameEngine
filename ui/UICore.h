#pragma once
#ifndef COMS_UI_CORE_H
#define COMS_UI_CORE_H

#include "../stdlib/Stdlib.h"
#include "../stdlib/ArrayVector.h"
#include "attribute/UIAttributeDimension.h"
#include "UIElementType.h"

enum UIElementFlag : byte {
    UI_ELEMENT_FLAG_HITBOX = 1 << 0, // Used to to create the chroma code (e.g. can be clicked to change focus)
    UI_ELEMENT_FLAG_INTERACTABLE = 1 << 1, // Used for interaction (e.g. drop down, button, ...)
    UI_ELEMENT_FLAG_MOVABLE = 1 << 2, // Used for draggable areas (e.g. window title)
    UI_ELEMENT_FLAG_RESIZABLE = 1 << 3, // Used for resizable elements (e.g. window border)
    UI_ELEMENT_FLAG_HOVERABLE = 1 << 4, // Used for elements with hovering (e.g. links, buttons)
    UI_ELEMENT_FLAG_SELECTABLE = 1 << 5, // Used for selectable content (e.g. text)
};

enum UIState : byte {
    UI_STATE_NONE = 0,
    UI_STATE_ACTIVE = 1 << 0, // e.g. during drag and drop
    UI_STATE_FOCUS = 1 << 1, // e.g. active window or active input field etc.
    UI_STATE_HOVER = 1 << 2,
    UI_STATE_DISABLED = 1 << 3,
    UI_STATE_ANIMATION = 1 << 4,
};

enum UIElementChangeType : byte {
    // If an element got larger we can update it VERY efficiently
    UI_ELEMENT_CHANGE_DIM_LARGER = 1 << 0,

    // dimensions got smaller or one axis got smaller and only one got bigger
    UI_ELEMENT_CHANGE_DIM_OTHER = 1 << 1,

    UI_ELEMENT_CHANGE_POS = 1 << 2,

    // Z-axis changed
    UI_ELEMENT_CHANGE_ORDER = 1 << 3,

    // This is the most complex change since it also results in different vertex counts
    // We now have to change the entire vertex cache/index cache after this element as well
    UI_ELEMENT_CHANGE_CONTENT = 1 << 4,

    // We use this to check if the chroma code needs to get recreated
    UI_ELEMENT_CHANGE_CHROMA_CODE = UI_ELEMENT_CHANGE_DIM_LARGER
        | UI_ELEMENT_CHANGE_DIM_OTHER
        | UI_ELEMENT_CHANGE_POS
        | UI_ELEMENT_CHANGE_ORDER,
};

struct UICore {
    UIElementType type;

    // UIElementFlag
    byte flags;

    // UIState
    byte state;

    // UIElementChangeType
    byte change_type;

    // 1-indexed, 0 = no function defined
    int16 update_func;
    int16 render_func;

    // @question Consider to pull out into this struct to reduce alignment paddings
    //          We are currently wasting at least 3 bytes after opacity due to alignment
    UIAttributeDimension dimension;

    // Defines the style it uses
    int32 class_name;

    // Offset of the parent element (absolute position)
    int32 parent_offset;

    int32 vertex_count;
    int32 vertices;

    // @todo Probably needs offsets to different animations/states?
    //          But that would be bad since not every element needs it
};

#endif