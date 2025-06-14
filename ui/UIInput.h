#ifndef COMS_UI_INPUT_H
#define COMS_UI_INPUT_H

#include "../stdlib/Types.h"
#include "../camera/Camera.h"
#include "attribute/UIAttribute.h"
#include "attribute/UIAttributeBorder.h"
#include "attribute/UIAttributeShadow.h"
#include "attribute/UIAttributeBackground.h"
#include "attribute/UIAttributeDimension.h"
#include "UIAnimation.h"
#include "UIStyleType.h"
#include "UIElement.h"
#include "UICursor.h"
#include "UIWindow.h"
#include "UIPanel.h"
#include "UILayout.h"
#include "../math/Evaluator.h"
#include "../gpuapi/RenderUtils.h"
#include "../object/Vertex.h"

enum UIInputType : byte {
    UI_INPUT_TYPE_TEXT,
    UI_INPUT_TYPE_NUMERIC,
    UI_INPUT_TYPE_DATE,
    UI_INPUT_TYPE_DATE_TIME,
    UI_INPUT_TYPE_TIME,
};

struct UIInputState {
    uint16 cursor_pos_x;
    uint16 cursor_pos_y;
    UIInputType type;
    int32 min_value;
    int32 max_value;
    uint16 max_input_length;
};

struct UIInput {
    UIAttributeDimension dimension;
    byte opacity; // 1 byte alpha channel
    byte padding;

    UIAttributeBackground background;
    UIAttributeBorder border;
    UIAttributeShadow shadow_outer;
    UIAttributeShadow shadow_inner;
};

void ui_input_state_serialize(const UIInputState* __restrict state, byte** __restrict pos)
{
    *((uint16 *) *pos) = SWAP_ENDIAN_LITTLE(state->cursor_pos_x);
    *pos += sizeof(state->cursor_pos_x);

    *((uint16 *) *pos) = SWAP_ENDIAN_LITTLE(state->cursor_pos_y);
    *pos += sizeof(state->cursor_pos_y);

    **pos = state->type;
    *pos += sizeof(state->type);

    *((int32 *) *pos) = SWAP_ENDIAN_LITTLE(state->min_value);
    *pos += sizeof(state->min_value);

    *((int32 *) *pos) = SWAP_ENDIAN_LITTLE(state->max_value);
    *pos += sizeof(state->max_value);

    *((uint16 *) *pos) = SWAP_ENDIAN_LITTLE(state->max_input_length);
    *pos += sizeof(state->max_input_length);
}

void ui_input_state_unserialize(UIInputState* __restrict state, const byte** __restrict pos)
{
    state->cursor_pos_x = SWAP_ENDIAN_LITTLE(*((uint16 *) *pos));
    *pos += sizeof(state->cursor_pos_x);

    state->cursor_pos_y = SWAP_ENDIAN_LITTLE(*((uint16 *) *pos));
    *pos += sizeof(state->cursor_pos_y);

    state->type = (UIInputType) **pos;
    *pos += sizeof(state->type);

    state->min_value = SWAP_ENDIAN_LITTLE(*((int32 *) *pos));
    *pos += sizeof(state->min_value);

    state->max_value = SWAP_ENDIAN_LITTLE(*((int32 *) *pos));
    *pos += sizeof(state->max_value);

    state->max_input_length = SWAP_ENDIAN_LITTLE(*((uint16 *) *pos));
    *pos += sizeof(state->max_input_length);
}

void ui_input_state_populate(const UIAttributeGroup* __restrict group, UIInputState* __restrict state) {

    UIAttribute* attributes = (UIAttribute *) (group + 1);
    for (uint32 i = 0; i < group->attribute_count; ++i) {
        switch (attributes[i].attribute_id) {
            case UI_ATTRIBUTE_TYPE_TYPE: {
                    state->type = (UIInputType) attributes[i].value_int;
                } break;
            case UI_ATTRIBUTE_TYPE_MIN_VALUE: {
                    state->min_value = attributes[i].value_int;
                } break;
            case UI_ATTRIBUTE_TYPE_MAX_VALUE: {
                    state->max_value = attributes[i].value_int;
                } break;
            case UI_ATTRIBUTE_TYPE_MAX_INPUT_LENGTH: {
                    state->max_input_length = (uint16) attributes[i].value_int;
                } break;
            default: {}
        }
    }
}

void ui_input_element_serialize(const UIInput* __restrict details, byte** __restrict pos)
{
    ui_attr_dimension_serialize(&details->dimension, pos);

    **pos = details->opacity;
    *pos += sizeof(details->opacity);

    **pos = details->padding;
    *pos += sizeof(details->padding);

    ui_attr_background_serialize(&details->background, pos);
    ui_attr_border_serialize(&details->border, pos);
    ui_attr_shadow_serialize(&details->shadow_outer, pos);
    ui_attr_shadow_serialize(&details->shadow_inner, pos);
}

void ui_input_element_unserialize(UIInput* __restrict details, const byte** __restrict pos)
{
    ui_attr_dimension_unserialize(&details->dimension, pos);

    details->opacity = **pos;
    *pos += sizeof(details->opacity);

    details->padding = **pos;
    *pos += sizeof(details->padding);

    ui_attr_background_unserialize(&details->background, pos);
    ui_attr_border_unserialize(&details->border, pos);
    ui_attr_shadow_unserialize(&details->shadow_outer, pos);
    ui_attr_shadow_unserialize(&details->shadow_inner, pos);
}

void ui_input_element_populate(
    UILayout* layout,
    UIElement* element,
    const UIAttributeGroup* __restrict  group,
    UIInput* __restrict input
) {
    v4_f32 parent_dimension = {};
    if (element->parent) {
        UIElement* parent = element->parent ? (UIElement *) (layout->data + element->parent) : NULL;
        // @bug How to ensure that the parent is initialized before the child element
        // Currently the order of the initialization depends on the theme file, NOT the layout file
        // We could fix it by loading the style based on the layout order but this would result in many misses when looking up styles
        //      The reason for these misses are, that often only 1-2 style_types exist per element
        switch (parent->type) {
            case UI_ELEMENT_TYPE_VIEW_WINDOW: {
                    UIWindow* parent_window = (UIWindow *) (layout->data + parent->style_types[UI_STYLE_TYPE_ACTIVE]);
                    parent_dimension = parent_window->dimension.dimension;
                } break;
            case UI_ELEMENT_TYPE_VIEW_PANEL: {
                    UIPanel* parent_window = (UIPanel *) (layout->data + parent->style_types[UI_STYLE_TYPE_ACTIVE]);
                    parent_dimension = parent_window->dimension.dimension;
                } break;
            default:
                UNREACHABLE();
        }
    }

    if (!element->vertices_active_offset && !element->vertex_count_max) {
        element->vertices_active_offset = layout->active_vertex_offset;
        UIAttribute* vertex_attr = ui_attribute_from_group(group, UI_ATTRIBUTE_TYPE_VERTEX_COUNT);
        element->vertex_count_max = (uint16) (vertex_attr ? vertex_attr->value_int : 8);

        layout->active_vertex_offset += element->vertex_count_max;
    }

    UIAttribute* attributes = (UIAttribute *) (group + 1);

    // First set all values, which we can set immediately
    for (uint32 i = 0; i < group->attribute_count; ++i) {
        switch (attributes[i].attribute_id) {
            case UI_ATTRIBUTE_TYPE_POSITION_X:
            case UI_ATTRIBUTE_TYPE_DIMENSION_WIDTH:
            case UI_ATTRIBUTE_TYPE_POSITION_Y:
            case UI_ATTRIBUTE_TYPE_DIMENSION_HEIGHT: {
                    ui_theme_assign_dimension(&input->dimension, &attributes[i]);
                } break;
        }
    }
}

int32 ui_input_element_update(UILayout* layout, UIElement* element)
{
    UIInput* input = (UIInput *) (layout->data + element->style_types[element->style_new]);
    //UIInputState* state = (UIInputState *) (layout->data + element->state);

    int32 idx = 0;
    f32 zindex = element->zindex;

    v4_f32 dimension = input->dimension.dimension;

    // Border
    if (input->border.thickness) {
        idx += vertex_rect_create(
            layout->vertices_active + element->vertices_active_offset, zindex, -1,
            dimension, input->dimension.alignment,
            input->border.color
        );

        // Adjusting dimension based on border
        // border is part of width/height
        dimension.x += input->border.thickness;
        dimension.y += input->border.thickness;
        dimension.width -= input->border.thickness;
        dimension.height -= input->border.thickness;

        // @bug change to camera_step_closer()
        zindex = nextafterf(zindex, INFINITY);
    }

    // Background
    if (input->background.background_color) {
        idx += vertex_rect_create(
            layout->vertices_active + element->vertices_active_offset + idx, zindex, -1,
            dimension, input->dimension.alignment,
            input->background.background_color
        );

        // @bug change to camera_step_closer()
        zindex = nextafterf(zindex, INFINITY);
    }

    // Cursor
    HashEntryInt32* cursor_entry = (HashEntryInt32 *) hashmap_get_entry(&layout->hash_map, "cursor");
    if (cursor_entry) {
        UIElement* cursor_element = (UIElement *) (layout->data + cursor_entry->value);
        // This requires the cursor position is already updated wherever we handle the input update (on_enter, on_leave)
        idx += ui_cursor_element_update(layout, cursor_element);

        memcpy(
            layout->vertices_active + element->vertices_active_offset + idx,
            layout->vertices_active + cursor_element->vertices_active_offset,
            cursor_element->vertex_count_active * sizeof(*(layout->vertices_active))
        );
    }

    element->vertex_count_active = (uint16) idx;

    return idx;
}

#endif