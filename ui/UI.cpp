/**
 * How does UI rendering work?
 *
 * 1. Load UI components from layout file or create programmatically
 * 2. Style elements by loading theme from file
 *
 * 3. Update state of UI components
 * 4. (Re-)Create vertices cache of changed UI components (1. stage)
 *
 * 5. (Re-)Create and add (2. stage) cache for *some* components to CPU cache (GpuAtlasCache)
 * 6. Render *some* UI components and elements to a on gpu texture cache
 * 7. Copy vertices to compact/continuous vertex array on CPU (3. stage)
 *
 * 8. Draw vertices/send cache (3. stage) to GPU
 */
#pragma once
#ifndef COMS_UI_C
#define COMS_UI_C

#include "UILayout.cpp"
#include "UIInput.h"
#include "UILabel.cpp"
#include "UIWindow.cpp"
#include "UICursor.cpp"

void ui_cache(
    void* app,
    GpuApiType gpu_api_type,
    UILayout* const layout,
    BufferMemory* const __restrict mem
) NO_EXCEPT
{
    PROFILE_DEBUG(PROFILE_UI_CACHE);

    layout->ui_vertex_cache.count = 0;
    layout->ui_index_cache.count = 0;
    array_vector_insert(&layout->ui_vertex_cache, {{0.0f, 0.0f, 10.0f}, 0, {0}});

    int32* iter;
    array_vector_iterate_start(layout->ui_element_root, iter) {
        UICore* const element = (UICore *) (layout->ui_element_buffer.memory + *iter);

        // This assert isn't really working since we don't know how large vertices_count will be
        // We would have to simulate/guess the max vertex count and check against this
        // This just assumes that this update is smaller or as large as the previous one
        // Of course for the first update this doesn't work at all
        ASSERT_TRUE(
            element->vertex_count + layout->ui_vertex_cache.count
                <= layout->ui_vertex_cache.capacity
        );

        element->vertices = layout->ui_vertex_cache.count;

        // @question This means ui_vertex_cache MUST be tightly packed
        //          AND it mustn't have unused data (not the case for pre-calculated hover styles)
        //          For that reason we might need a vertex_array with all the possible data and one with tightly packed data
        //          In an ideal scenario the tightly packed data is just replaced by equally long data without memmoves required

        switch (element->type) {
            case UI_ELEMENT_TYPE_BUTTON : {
            } break;
            case UI_ELEMENT_TYPE_SELECT : {
            } break;
            case UI_ELEMENT_TYPE_INPUT : {
            } break;
            case UI_ELEMENT_TYPE_LABEL: {
                UILabel* test_label_element = (UILabel*) element;
                ui_vertices_cache(
                    app,
                    test_label_element,
                    layout, 10.0f, // @todo fix actual value
                    mem
                );
            } break;
            case UI_ELEMENT_TYPE_TEXTAREA : {
            } break;
            case UI_ELEMENT_TYPE_IMAGE : {
            } break;
            case UI_ELEMENT_TYPE_TEXT : {
            } break;
            case UI_ELEMENT_TYPE_LINK : {
            } break;
            case UI_ELEMENT_TYPE_TABLE : {
            } break;
            case UI_ELEMENT_TYPE_VIEW_WINDOW: {
                UIWindow* window_element = (UIWindow*) element;
                ui_vertices_cache(
                    app,
                    window_element, gpu_api_type,
                    layout, 10.0f, // @todo fix actual value
                    mem
                );
            } break;
            case UI_ELEMENT_TYPE_VIEW_PANEL : {
            } break;
            case UI_ELEMENT_TYPE_VIEW_TAB : {
            } break;
            case UI_ELEMENT_TYPE_CURSOR : {
                UICursor* cursor_element = (UICursor*) element;
                ui_vertices_cache(
                    app,
                    cursor_element,
                    layout
                );
            } break;
            case UI_ELEMENT_TYPE_CUSTOM : {
                layout->render[element->render_func - 1](
                    app,
                    element,
                    gpu_api_type,
                    layout,
                    10.0f,
                    mem
                );
            } break;
            default:
                UNREACHABLE();
        }

        element->vertex_count = (int16) (layout->ui_vertex_cache.count - element->vertices);
    } array_vector_iterate_end;

    layout->ui_element_changed.count = 0;
}

void ui_update(
    void* app,
    GpuApiType gpu_api_type,
    UILayout* const layout,
    BufferMemory* const __restrict mem
) NO_EXCEPT
{
    PROFILE_DEBUG(PROFILE_UI_UPDATE);

    int32* iter;
    array_vector_iterate_start(layout->ui_element_root, iter) {
        UICore* const element = (UICore *) (layout->ui_element_buffer.memory + *iter);

        // This assert isn't really working since we don't know how large vertices_count will be
        // We would have to simulate/guess the max vertex count and check against this
        // This just assumes that this update is smaller or as large as the previous one
        // Of course for the first update this doesn't work at all
        ASSERT_TRUE(
            element->vertex_count + layout->ui_vertex_cache.count
                <= layout->ui_vertex_cache.capacity
        );

        element->vertices = layout->ui_vertex_cache.count;

        // @question This means ui_vertex_cache MUST be tightly packed
        //          AND it mustn't have unused data (not the case for pre-calculated hover styles)
        //          For that reason we might need a vertex_array with all the possible data and one with tightly packed data
        //          In an ideal scenario the tightly packed data is just replaced by equally long data without memmoves required

        switch (element->type) {
            case UI_ELEMENT_TYPE_BUTTON : {
            } break;
            case UI_ELEMENT_TYPE_SELECT : {
            } break;
            case UI_ELEMENT_TYPE_INPUT : {
            } break;
            case UI_ELEMENT_TYPE_LABEL: {
                UILabel* test_label_element = (UILabel*) element;
                // @todo replace with a update function
                ui_vertices_cache(
                    app,
                    test_label_element,
                    layout, 10.0f, // @todo fix actual value
                    mem
                );
            } break;
            case UI_ELEMENT_TYPE_TEXTAREA : {
            } break;
            case UI_ELEMENT_TYPE_IMAGE : {
            } break;
            case UI_ELEMENT_TYPE_TEXT : {
            } break;
            case UI_ELEMENT_TYPE_LINK : {
            } break;
            case UI_ELEMENT_TYPE_TABLE : {
            } break;
            case UI_ELEMENT_TYPE_VIEW_WINDOW: {
                UIWindow* test_window_element = (UIWindow*) element;
                ui_vertices_cache(
                    app,
                    test_window_element, gpu_api_type,
                    layout, 10.0f, // @todo fix actual value
                    mem
                );
            } break;
            case UI_ELEMENT_TYPE_VIEW_PANEL : {
            } break;
            case UI_ELEMENT_TYPE_VIEW_TAB : {
            } break;
            case UI_ELEMENT_TYPE_CURSOR : {
            } break;
            default:
                UNREACHABLE();
        }

        element->vertex_count = (int16) (layout->ui_vertex_cache.count - element->vertices);
    } array_vector_iterate_end;
}

////////////////////////////////////////////////////////////////
// Hit testing
////////////////////////////////////////////////////////////////
//
// Finds the UI element at a given position.
//
// Coordinates are in the same space as UICore::dimension (y-up, origin bottom left).
// dimension.pos is the TOP-LEFT corner, the element extends to the right and downwards
// (see ui_root_create() and vertex_rect_create(..., UI_ALIGN_V_TOP | UI_ALIGN_H_LEFT, ...)).
// If your mouse coordinates are y-down you need to convert them first: y = screen_height - mouse_y
//
// The dimensions are only valid after ui_cache()/ui_update() calculated them.
//
// Action bubbling:
//      Children are always tested before their parent (they are drawn on top of the parent).
//      If the deepest hit element has no action, the action "bubbles" up to the closest parent that has one.
//      This is the same behavior as DOM event bubbling, e.g.
//          click on window title label (no action) -> window title (movable) is returned
//          click on button label (no action) -> button (on_click) is returned
//
// Every find function below follows the same contract:
//      return value: the deepest element with an action at (x, y) within this subtree (or NULL)
//      *hit:         set to the deepest element at (x, y) regardless of actions (only set once)
//                    This is how we know that the UI consumed the input even if nothing is actionable
//                    (e.g. click on a plain panel must not "click through" to the window below or the game world)
//
// IMPORTANT: Children embedded in a parent struct (e.g. UIWindow::title, UIButton::label) are NOT dispatched
//      by core.type since their type is often not set (zero = UI_ELEMENT_TYPE_BUTTON after MEMORY_ELEMENT_ZERO).
//      We only dispatch by type for root elements and otherwise rely on the known struct layout.

#define UI_ELEMENT_FLAG_ACTION_MASK (UI_ELEMENT_FLAG_HITBOX \
    | UI_ELEMENT_FLAG_INTERACTABLE \
    | UI_ELEMENT_FLAG_MOVABLE \
    | UI_ELEMENT_FLAG_RESIZABLE \
    | UI_ELEMENT_FLAG_HOVERABLE \
    | UI_ELEMENT_FLAG_SELECTABLE)

static FORCE_INLINE
bool ui_element_contains(const UICore* const element, f32 x, f32 y) NO_EXCEPT
{
    const UIAttributeDimension* const dim = &element->dimension;

    return x >= dim->pos.x && x < dim->pos.x + dim->dim.x
        && y <= dim->pos.y && y > dim->pos.y - dim->dim.y;
}

static FORCE_INLINE
bool ui_element_has_action(const UICore* const element) NO_EXCEPT
{
    if (element->state & UI_STATE_DISABLED) {
        return false;
    }

    return (element->flags & UI_ELEMENT_FLAG_ACTION_MASK) != 0;
}

// Leaf element = element without (hit testable) children
static inline
UICore* ui_element_find_leaf(UICore* const element, f32 x, f32 y, UICore** const hit) NO_EXCEPT
{
    if (!ui_element_contains(element, x, y)) {
        return NULL;
    }

    *hit = element;

    return ui_element_has_action(element) ? element : NULL;
}

// Must be called by a parent AFTER testing all of its children
//      child_found: what the children returned
static inline
UICore* ui_element_find_bubble(
    UICore* const element, UICore* const child_found,
    f32 x, f32 y, UICore** const hit
) NO_EXCEPT
{
    if (*hit) {
        // A child was hit
        // -> child action wins, otherwise fall back to the parent (if it has an action)
        // We intentionally don't check the parent bounds here since children may overflow their parent
        if (child_found) {
            return child_found;
        }

        return ui_element_has_action(element) ? element : NULL;
    }

    // No child was hit -> the parent itself may still be hit
    return ui_element_find_leaf(element, x, y, hit);
}

static inline
UICore* ui_button_find(UIButton* const button, f32 x, f32 y, UICore** const hit) NO_EXCEPT
{
    UICore* found = ui_element_find_leaf(&button->label.core, x, y, hit);
    found = ui_element_find_bubble(&button->core, found, x, y, hit);

    // A button with a click handler is always actionable even without explicit flags
    if (!found
        && *hit
        && button->on_click
        && !(button->core.state & UI_STATE_DISABLED)
    ) {
        found = &button->core;
    }

    return found;
}

static inline
UICore* ui_window_title_find(UIWindowTitle* const title, f32 x, f32 y, UICore** const hit) NO_EXCEPT
{
    UICore* const found = ui_element_find_leaf(&title->label.core, x, y, hit);

    return ui_element_find_bubble(&title->core, found, x, y, hit);
}

static
UICore* ui_window_find(UIWindow* const window, f32 x, f32 y, UICore** const hit) NO_EXCEPT
{
    UICore* found = NULL;

    // Reverse render order -> top most child first
    // We stop at the first child that was hit (children don't overlap each other in a meaningful way)
    // NOTE: window->panel.core is not used for rendering (the window dimension is used instead)
    UIButton* const buttons[] = {
        &window->button_close,
        &window->button_minimize,
        &window->button_maximize
    };

    for (int32 i = 0; i < (int32) ARRAY_COUNT(buttons) && !*hit; ++i) {
        found = ui_button_find(buttons[i], x, y, hit);
    }

    if (!*hit) {
        found = ui_window_title_find(&window->title, x, y, hit);
    }

    return ui_element_find_bubble(&window->core, found, x, y, hit);
}

static
UICore* ui_element_find(UICore* const element, f32 x, f32 y, UICore** const hit) NO_EXCEPT
{
    switch (element->type) {
        case UI_ELEMENT_TYPE_BUTTON:
            return ui_button_find((UIButton *) element, x, y, hit);
        case UI_ELEMENT_TYPE_VIEW_WINDOW:
            return ui_window_find((UIWindow *) element, x, y, hit);
        case UI_ELEMENT_TYPE_CURSOR:
            // The cursor is always at the mouse position -> never a valid hit
            return NULL;
        case UI_ELEMENT_TYPE_TABLE:
            // @todo Check head/body/foot once their dimensions get calculated
        default:
            // @todo Once elements have dynamic children (child offsets) we need to iterate them here
            return ui_element_find_leaf(element, x, y, hit);
    }
}

/**
 * Finds the top most element at (x, y) that has an action bound.
 * If the deepest element at (x, y) has no action, the closest parent with an action is returned.
 *
 * @param hit_element (optional) Deepest element at (x, y) regardless of actions.
 *                    If it is NULL the UI wasn't hit at all (e.g. forward the input to the game world)
 *
 * @return Element with action or NULL
 */
UICore* ui_element_at_position(
    UILayout* const layout,
    f32 x, f32 y,
    UICore** hit_element = NULL
) NO_EXCEPT
{
    UICore* hit = NULL;
    UICore* found = NULL;

    // ui_element_root is in render order -> the last element is the top most one
    // That's why we iterate in reverse instead of using array_vector_iterate_start
    // This allows us to stop at the first root element that was hit
    for (int32 i = layout->ui_element_root.count - 1; i >= 0 && !hit; --i) {
        UICore* const element = (UICore *) (layout->ui_element_buffer.memory + layout->ui_element_root.elements[i]);
        found = ui_element_find(element, x, y, &hit);
    }

    if (hit_element) {
        *hit_element = hit;
    }

    return found;
}

#endif