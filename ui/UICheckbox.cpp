#pragma once
#ifndef COMS_UI_CHECKBOX_C
#define COMS_UI_CHECKBOX_C

#include "../stdlib/Stdlib.h"
#include "../gpuapi/RenderUtils.h"
#include "../camera/Camera.cpp"
#include "attribute/UIAttributeDimension.cpp"
#include "attribute/UIAttributeBorder.cpp"
#include "UIAlignment.h"
#include "UIStyleDefaults.h"
#include "UICore.cpp"
#include "UILabel.cpp"
#include "UICheckbox.h"

#define UI_CHECKBOX_BOX_SIZE 16.0f
#define UI_CHECKBOX_LABEL_GAP 6.0f

/**
 * @param text NULL = no label
 * @param parent NULL = root
 */
inline
UICheckbox* ui_checkbox_create(
    UILayout* const layout,
    const char* text = NULL,
    UICore* parent = NULL,
    UIElementType type = UI_ELEMENT_TYPE_CHECKBOX
) NO_EXCEPT
{
    UICheckbox* const checkbox = UI_ELEMENT_ALLOC(layout, type, UICheckbox, parent);
    checkbox->core.flags = UI_ELEMENT_FLAG_INTERACTABLE | UI_ELEMENT_FLAG_HOVERABLE;
    checkbox->core.dimension.align_anchor = UI_ANCHOR_ALIGN_TOP_LEFT;

    checkbox->box_color = UI_DEFAULT_CONTROL_BACKGROUND;
    checkbox->border_color = UI_DEFAULT_CONTROL_BORDER;
    checkbox->border_color_hover = UI_DEFAULT_CONTROL_BORDER_FOCUS;
    checkbox->check_color = UI_DEFAULT_CONTROL_ACCENT;

    if (text) {
        // The label is a child -> allocated directly after the checkbox
        UILabel* const label = ui_label_create(layout, CHAR_TYPE_CHAR, 0, 0, &checkbox->core);
        label->core.dimension.align_anchor = UI_ANCHOR_ALIGN_LEFT_CENTER;
        label->core.dimension.pos_raw = {UI_CHECKBOX_BOX_SIZE + UI_CHECKBOX_LABEL_GAP, 0.0f};
        ui_label_set_text(layout, label, text);

        checkbox->label = ui_element_offset(layout, label);
    }

    return checkbox;
}

/**
 * @param group Radios with the same group and parent are mutually exclusive
 */
inline
UIRadio* ui_radio_create(
    UILayout* const layout,
    const char* text = NULL,
    int16 group = 0,
    UICore* parent = NULL
) NO_EXCEPT
{
    UIRadio* const radio = ui_checkbox_create(layout, text, parent, UI_ELEMENT_TYPE_RADIO);
    radio->group = group;

    return radio;
}

/**
 * Changes the checked state
 * Checking a radio unchecks all other radios of the same group (and parent)
 *
 * @return true if the value changed
 */
inline
bool ui_checkbox_set(UILayout* const layout, UICheckbox* const checkbox, bool checked) NO_EXCEPT
{
    if (checkbox->checked == checked) {
        return false;
    }

    if (checkbox->core.type == UI_ELEMENT_TYPE_RADIO && checked) {
        UICore* const parent = ui_element_parent(layout, &checkbox->core);
        UICore* sibling;
        ui_element_children_iterate_start(layout, parent, sibling) {
            if (sibling->type == UI_ELEMENT_TYPE_RADIO
                && sibling != &checkbox->core
                && ((UIRadio *) sibling)->group == checkbox->group
            ) {
                ((UIRadio *) sibling)->checked = false;
                sibling->change_type |= UI_ELEMENT_CHANGE_CONTENT;
            }
        } ui_element_children_iterate_end;
    }

    checkbox->checked = checked;
    checkbox->core.change_type |= UI_ELEMENT_CHANGE_CONTENT;
    ui_layout_mark_dirty(layout);

    return true;
}

void ui_checkbox_cache(
    UICheckbox* const checkbox,
    GpuApiType gpu_api_type,
    UILayout* const layout,
    f32* const zindex
) NO_EXCEPT
{
    PROFILE_DEBUG(PROFILE_UI_CACHE_CHECKBOX);

    // The size includes the label, so clicking the label also toggles the checkbox
    // The label size is from the last ui_cache() since children are calculated after their parent
    v2_f32 content_dim = {UI_CHECKBOX_BOX_SIZE, UI_CHECKBOX_BOX_SIZE};
    if (checkbox->label) {
        const UICore* const label = ui_element_get(layout, checkbox->label);
        content_dim.x += UI_CHECKBOX_LABEL_GAP + label->dimension.dim.x;
        content_dim.y = max_branched(content_dim.y, label->dimension.dim.y);
    }

    ui_dimension_calculate(layout, &checkbox->core, content_dim);
    const UIAttributeDimension* const dim = &checkbox->core.dimension;

    // Box is vertically centered on the left side
    const v4_f32 box = {
        dim->pos.x,
        dim->pos.y - (dim->dim.y - UI_CHECKBOX_BOX_SIZE) * 0.5f,
        UI_CHECKBOX_BOX_SIZE,
        UI_CHECKBOX_BOX_SIZE
    };

    const uint32 border_color = (checkbox->core.state & UI_STATE_HOVER)
        ? checkbox->border_color_hover
        : checkbox->border_color;

    ArrayVector<Vertex3DSamplerTextureColor>* const vertices = &layout->ui_vertex_cache;
    ArrayVector<int32>* const indices = &layout->ui_index_cache;

    if (checkbox->core.type == UI_ELEMENT_TYPE_RADIO) {
        // Circle: border circle, background circle, check circle
        vertex_circle_create(vertices, indices, *zindex, 0, box, UI_ALIGN_V_TOP | UI_ALIGN_H_LEFT, 16, border_color);
        *zindex = camera_step_closer(gpu_api_type, *zindex);

        vertex_circle_create(
            vertices, indices, *zindex, 0,
            {box.x + 1.0f, box.y - 1.0f, box.width - 2.0f, box.height - 2.0f},
            UI_ALIGN_V_TOP | UI_ALIGN_H_LEFT, 16, checkbox->box_color
        );

        if (checkbox->checked) {
            *zindex = camera_step_closer(gpu_api_type, *zindex);
            vertex_circle_create(
                vertices, indices, *zindex, 0,
                {box.x + 4.0f, box.y - 4.0f, box.width - 8.0f, box.height - 8.0f},
                UI_ALIGN_V_TOP | UI_ALIGN_H_LEFT, 16, checkbox->check_color
            );
        }
    } else {
        ui_rect_vertices(box, checkbox->box_color, vertices, indices, *zindex);
        *zindex = camera_step_closer(gpu_api_type, *zindex);

        ui_border_solid_vertices(box, border_color, 1, vertices, indices, *zindex);

        if (checkbox->checked) {
            ui_rect_vertices(
                {box.x + 4.0f, box.y - 4.0f, box.width - 8.0f, box.height - 8.0f},
                checkbox->check_color, vertices, indices, *zindex
            );
        }
    }

    *zindex = camera_step_closer(gpu_api_type, *zindex);
}

#endif
