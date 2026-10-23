#pragma once
#ifndef COMS_UI_CORE_C
#define COMS_UI_CORE_C

#include "../stdlib/Stdlib.h"
#include "../memory/BufferMemory.cpp"
#include "../utils/BitUtils.h"
#include "UICore.h"
#include "UILayout.h"

////////////////////////////////////////////////////////////////
// Element tree helpers
////////////////////////////////////////////////////////////////
// The tree is stored in pre-order inside of ui_element_buffer (@see UICore)

/**
 * Marks the layout as changed -> it will be re-cached (@see ui_layout_needs_cache)
 */
FORCE_INLINE
void ui_layout_mark_dirty(UILayout* const layout) NO_EXCEPT
{
    layout->dirty = true;
}

FORCE_INLINE
UICore* ui_element_get(const UILayout* const layout, int32 offset) NO_EXCEPT
{
    return (UICore *) (layout->ui_element_buffer.memory + offset);
}

/**
 * The output of the element changed -> re-cache
 * Needed for custom elements (UI_ELEMENT_TYPE_CUSTOM) whose content comes from outside of the ui
 * (e.g. a chart of external data), normal elements do this in their setters (e.g. ui_label_set_text)
 */
FORCE_INLINE
void ui_element_mark_changed(UILayout* const layout, UICore* const element) NO_EXCEPT
{
    element->change_type |= UI_ELEMENT_CHANGE_CONTENT;
    layout->dirty = true;
}

FORCE_INLINE
int32 ui_element_offset(const UILayout* const layout, const void* const element) NO_EXCEPT
{
    return (int32) ((uintptr_t) element - (uintptr_t) layout->ui_element_buffer.memory);
}

// Returns NULL for the root element
FORCE_INLINE
UICore* ui_element_parent(const UILayout* const layout, const UICore* const element) NO_EXCEPT
{
    return element == layout->ui_root
        ? NULL
        : ui_element_get(layout, element->parent_offset);
}

inline
bool ui_element_is_self_or_child_of(const UILayout* const layout, const UICore* element, const UICore* const ancestor) NO_EXCEPT
{
    while (element) {
        if (element == ancestor) {
            return true;
        }

        element = ui_element_parent(layout, element);
    }

    return false;
}

FORCE_INLINE
bool ui_element_has_children(const UICore* const element) NO_EXCEPT
{
    return element->next_offset != element->skip_offset;
}

// Iterates the DIRECT children of an element (in render order)
// Usage:
//      UICore* child;
//      ui_element_children_iterate_start(layout, parent, child) {
//          ...
//      } ui_element_children_iterate_end;
#define ui_element_children_iterate_start(layout, parent, child) {        \
    for (int32 _child_off = (parent)->next_offset;                         \
        _child_off != (parent)->skip_offset;                               \
        _child_off = (child)->skip_offset                                  \
    ) {                                                                    \
        child = ui_element_get((layout), _child_off);

#define ui_element_children_iterate_end }}

// Last element of the element's subtree in pre-order (the element itself if it has no children)
static inline
UICore* ui_element_subtree_last(const UILayout* const layout, UICore* const element) NO_EXCEPT
{
    UICore* last = element;
    while (last->next_offset != element->skip_offset) {
        last = ui_element_get(layout, last->next_offset);
    }

    return last;
}

/**
 * Removes the element and its subtree from the pre-order links
 * The memory isn't touched, the subtree stays intact and can be re-inserted (@see ui_element_subtree_insert)
 *
 * Complexity is O(element count), this only happens on rare structural changes (e.g. bring window to front)
 */
static
void ui_element_subtree_unlink(UILayout* const layout, UICore* const element) NO_EXCEPT
{
    ASSERT_TRUE(element != layout->ui_root);

    const int32 element_offset = ui_element_offset(layout, element);
    const int32 subtree_end = element->skip_offset;

    // Everything that pointed to the element now points to what comes after the subtree
    // The subtree itself is skipped automatically since the predecessor link gets updated before we follow it
    int32 offset = 0;
    do {
        UICore* const node = ui_element_get(layout, offset);
        if (node->skip_offset == element_offset) {
            node->skip_offset = subtree_end;
        }

        if (node->next_offset == element_offset) {
            node->next_offset = subtree_end;
        }

        offset = node->next_offset;
    } while (offset);
}

/**
 * Inserts an unlinked subtree (@see ui_element_subtree_unlink) as child of parent
 *
 * @param before Offset of the child of parent the subtree is inserted before (0 = insert as last child)
 */
static
void ui_element_subtree_insert(
    UILayout* const __restrict layout,
    UICore* const __restrict element,
    UICore* const __restrict parent,
    int32 before = 0
) NO_EXCEPT
{
    const int32 element_offset = ui_element_offset(layout, element);
    const int32 insert_at = before ? before : parent->skip_offset;
    const int32 old_end = element->skip_offset;

    // Inside of the subtree: everything that pointed past the subtree now points to the insert position
    UICore* last = element;
    while (true) {
        if (last->skip_offset == old_end) {
            last->skip_offset = insert_at;
        }

        if (last->next_offset == old_end) {
            last->next_offset = insert_at;
            break;
        }

        last = ui_element_get(layout, last->next_offset);
    }

    // Inside of the parent's subtree (before the insert position):
    // everything that pointed to the insert position now points to the element
    UICore* prev = parent;
    int32 next = parent->next_offset;
    while (next != insert_at) {
        UICore* const node = ui_element_get(layout, next);
        if (node->skip_offset == insert_at) {
            node->skip_offset = element_offset;
        }

        prev = node;
        next = node->next_offset;
    }

    prev->next_offset = element_offset;
    element->parent_offset = ui_element_offset(layout, parent);
}

/**
 * Offset of the first child that must stay on top (UI_ELEMENT_FLAG_ALWAYS_ON_TOP), 0 = none
 * New/moved elements are inserted before it
 */
static inline
int32 ui_element_on_top_child(const UILayout* const layout, const UICore* const parent, const UICore* const ignore = NULL) NO_EXCEPT
{
    UICore* child;
    ui_element_children_iterate_start(layout, parent, child) {
        if (child != ignore && (child->flags & UI_ELEMENT_FLAG_ALWAYS_ON_TOP)) {
            return ui_element_offset(layout, child);
        }
    } ui_element_children_iterate_end;

    return 0;
}

/**
 * Links a new element (without children) as the last child of parent
 * but before children that must stay on top (e.g. mouse cursor)
 *
 * We only have to update links, the element memory itself doesn't move
 * Complexity is O(size of parent subtree), this only happens on element creation
 */
static
void ui_element_link(
    UILayout* const __restrict layout,
    UICore* const __restrict element,
    UICore* const __restrict parent
) NO_EXCEPT
{
    // An "unlinked" element without children (@see ui_element_subtree_insert)
    element->next_offset = -1;
    element->skip_offset = -1;

    ui_element_subtree_insert(
        layout, element, parent,
        (element->flags & UI_ELEMENT_FLAG_ALWAYS_ON_TOP) ? 0 : ui_element_on_top_child(layout, parent)
    );
}

/**
 * Allocates a zeroed element and links it as the last child of parent
 *
 * @param parent NULL = root
 */
static inline
UICore* ui_element_alloc(
    UILayout* const __restrict layout,
    UIElementType type,
    int32 size,
    int32 alignment,
    UICore* parent = NULL
) NO_EXCEPT
{
    // The root MUST be created first, everything else is a child of the root
    ASSERT_TRUE(layout->ui_root);

    byte* const memory = memory_get(&layout->ui_element_buffer, size, alignment);
    ASSERT_TRUE(memory);
    memset(memory, 0, size);

    UICore* const element = (UICore *) memory;
    element->type = type;
    element->change_type = UI_ELEMENT_CHANGE_CONTENT;

    ui_element_link(layout, element, parent ? parent : layout->ui_root);
    ui_layout_mark_dirty(layout);

    return element;
}

#define UI_ELEMENT_ALLOC(layout, type_enum, struct_type, parent) \
    ((struct_type *) ui_element_alloc((layout), (type_enum), (int32) sizeof(struct_type), (int32) alignof(struct_type), (UICore *) (parent)))

/**
 * Moves the element (incl. its subtree) to the end of its siblings -> it is drawn on top of them
 * Siblings with UI_ELEMENT_FLAG_ALWAYS_ON_TOP stay in front (e.g. the mouse cursor)
 *
 * @return true if the order changed
 */
bool ui_element_move_to_front(UILayout* const layout, UICore* const element) NO_EXCEPT
{
    UICore* const parent = ui_element_parent(layout, element);
    if (!parent) {
        return false;
    }

    // Find the insert position = first sibling that must stay on top
    const int32 before = (element->flags & UI_ELEMENT_FLAG_ALWAYS_ON_TOP)
        ? 0
        : ui_element_on_top_child(layout, parent, element);

    // Already directly in front of the insert position?
    if (element->skip_offset == (before ? before : parent->skip_offset)) {
        return false;
    }

    ui_element_subtree_unlink(layout, element);
    ui_element_subtree_insert(layout, element, parent, before);
    element->change_type |= UI_ELEMENT_CHANGE_ORDER;
    ui_layout_mark_dirty(layout);

    return true;
}

/**
 * Finds the closest parent of a specific type
 */
UICore* ui_parent_element_by_type(const UILayout* const layout, const UICore* element, UIElementType type) NO_EXCEPT
{
    if (!element) {
        return NULL;
    }

    UICore* parent = ui_element_parent(layout, element);
    while (parent) {
        if (parent->type == type) {
            return parent;
        }

        parent = ui_element_parent(layout, parent);
    }

    return NULL;
}

/**
 * Finds the closest parent that has one of the types (1 bit per UIElementType)
 */
UICore* ui_parent_element_by_type(const UILayout* const layout, const UICore* element, uint32 type_flags) NO_EXCEPT
{
    if (!element) {
        return NULL;
    }

    UICore* parent = ui_element_parent(layout, element);
    while (parent) {
        if (OMS_BIT_SET(type_flags, parent->type)) {
            return parent;
        }

        parent = ui_element_parent(layout, parent);
    }

    return NULL;
}

/**
 * Element that is rendered by a callback (layout->render[render_func - 1])
 * and/or updated by a callback (layout->update[update_func - 1])
 *
 * @param parent NULL = root
 */
UICore* ui_custom_create(
    UILayout* const layout,
    int16 update_func,
    int16 render_func,
    UICore* parent = NULL
) NO_EXCEPT
{
    UICore* const element = ui_element_alloc(
        layout, UI_ELEMENT_TYPE_CUSTOM,
        (int32) sizeof(UICore), (int32) alignof(UICore),
        parent
    );

    element->update_func = update_func;
    element->render_func = render_func;

    // Custom elements handle their own interaction
    element->flags = UI_ELEMENT_FLAG_IGNORE_HIT;

    return element;
}

#endif
