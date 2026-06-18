#pragma once
#ifndef COMS_UI_OFFSET_H
#define COMS_UI_OFFSET_H

#include "../stdlib/Stdlib.h"
#include "UIElementType.h"

struct UIOffset {
    // Represents the offset into ui_element_buffer
    int32 element;

    // UIElementType
    // @performance we could probably move some of the date into one of the element bytes
    UIElementType type;

    // @question what was this? normal, hover, active? If true, wouldn't state be better as name?
    byte style_type;

    // Is changed since last cache
    // @todo should probably be a bit-field representing different types of updates
    //      e.g. state changes, style changes = reload theme data, ???
    byte is_changed;

    // We sometimes need to reference parent elements because we need information from them
    // e.g. A window title text (label) is part of a title panel, which is in turn part of a window
    //      We need to recursively find the parent element which the anchor position
    //      to calculate the position of this (child) element:
    //      Example: window.x + title.x + label.x = 10 + 5 + 0
    //              Yes, in this example the title bar is offset to the window (unusual but possible)
    // @performance could be int16 if we make it relative to this id
    int32 parent_offset;

    int16 vertices_count;
    int16 children_count;

    // Where do the vertices for this element start in the vertex buffer
    int32 vertices;
    int32 children;
};

UIOffset* ui_parent_offset_by_type(UIOffset* base, int32 type) NO_EXCEPT
{
    while (base && base->type != type) {
        if (!base->parent_offset) {
            return NULL;
        }

        base = (UIOffset*) (((uintptr_t) base) - base->parent_offset);
    }

    return base;
}

#endif