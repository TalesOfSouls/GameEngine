#pragma once
#ifndef COMS_UI_CORE_H
#define COMS_UI_CORE_H

#include "../stdlib/Stdlib.h"
#include "../stdlib/ArrayVector.h"
#include "attribute/UIAttributeDimension.h"
#include "UIElementType.h"

enum UIElementFlag : byte {
    UI_ELEMENT_FLAG_HITBOX = 1 << 0, // e.g. can be clicked to change focus
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

    // @question Consider to pull out into this struct to reduce alignment paddings (no issue right now but maybe in the future?)
    UIAttributeDimension dimension;

    // Defines the style it uses
    int32 class_name;

    // Offset of the parent element (absolute position)
    int32 parent_offset;

    // @performance We could drop down to uint16 most likely but we wouldn't gain anything at the moment
    int32 vertex_count;
    int32 vertices;

    // @todo Probably needs offsets to different animations/states?
    //          But that would be bad since not every element needs it
};

#endif