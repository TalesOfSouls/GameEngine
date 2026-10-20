#pragma once
#ifndef COMS_UI_CORE_H
#define COMS_UI_CORE_H

#include "../stdlib/Stdlib.h"
#include "attribute/UIAttributeDimension.h"
#include "UIElementType.h"

enum UIElementFlag : byte {
    UI_ELEMENT_FLAG_HITBOX = 1 << 0, // e.g. can be clicked to change focus
    UI_ELEMENT_FLAG_INTERACTABLE = 1 << 1, // Used for interaction (e.g. drop down, button, ...)
    UI_ELEMENT_FLAG_MOVABLE = 1 << 2, // Used for draggable areas (e.g. window title)
    UI_ELEMENT_FLAG_RESIZABLE = 1 << 3, // Used for resizable elements (e.g. window border)
    UI_ELEMENT_FLAG_HOVERABLE = 1 << 4, // Used for elements with hovering (e.g. links, buttons)
    UI_ELEMENT_FLAG_SELECTABLE = 1 << 5, // Used for selectable content (e.g. text)

    // The element is ignored by hit testing (e.g. mouse cursor, decorative elements)
    // Its children are still tested
    UI_ELEMENT_FLAG_IGNORE_HIT = 1 << 6,

    // Other elements are never moved in front of this element (e.g. mouse cursor)
    // @see ui_element_move_to_front()
    UI_ELEMENT_FLAG_ALWAYS_ON_TOP = 1 << 7,
};

// Elements with any of these flags "have an action bound"
// Used by the hit testing to bubble up to the closest element that can handle the input
#define UI_ELEMENT_FLAG_ACTION_MASK (UI_ELEMENT_FLAG_HITBOX \
    | UI_ELEMENT_FLAG_INTERACTABLE \
    | UI_ELEMENT_FLAG_MOVABLE \
    | UI_ELEMENT_FLAG_RESIZABLE \
    | UI_ELEMENT_FLAG_HOVERABLE \
    | UI_ELEMENT_FLAG_SELECTABLE)

enum UIState : byte {
    UI_STATE_NONE = 0,
    UI_STATE_ACTIVE = 1 << 0, // e.g. during drag and drop
    UI_STATE_FOCUS = 1 << 1, // e.g. active window or active input field etc.
    UI_STATE_HOVER = 1 << 2,
    UI_STATE_DISABLED = 1 << 3,
    UI_STATE_ANIMATION = 1 << 4,

    // The element AND all its children are neither rendered nor hit tested
    UI_STATE_HIDDEN = 1 << 5,
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

/**
 * Hot data of every UI element
 *
 * All elements live in UILayout::ui_element_buffer and form a tree that is stored in pre-order (depth-first):
 *      parent, child 1, grandchild 1.1, child 2, ...
 * This order is also the render order (painter's algorithm), later elements are drawn on top.
 *
 * Elements are allocated in pre-order whenever possible (e.g. a window allocates its title, label, buttons directly after itself)
 * This means that iterating the tree is basically a linear walk through memory.
 * Elements added later (e.g. a label added to an existing panel) are linked into the correct position
 * through next_offset/skip_offset without having to move any memory.
 *
 * All links are byte offsets into ui_element_buffer (no pointers) -> the buffer can be saved/loaded/copied as is
 * Offset 0 is ALWAYS the root element. Since nothing can link to the root, 0 is also used as "no element"
 *
 * 60 bytes
 */
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

    UIAttributeDimension dimension;

    // Offset of the parent element (0 = root, the root itself also has 0)
    int32 parent_offset;

    // Next element in pre-order (0 = end of the tree)
    // This is the first child if the element has children
    int32 next_offset;

    // Next element in pre-order that is NOT part of this element's subtree (0 = end of the tree)
    // Used to skip entire subtrees (e.g. hidden windows)
    // If next_offset == skip_offset the element has no children
    int32 skip_offset;

    // Offset to the class name string (0 = no class)
    // Only needed when applying a theme
    int32 class_name;
};

#endif
