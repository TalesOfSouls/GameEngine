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

#endif