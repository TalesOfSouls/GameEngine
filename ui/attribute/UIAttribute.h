#ifndef COMS_UI_ATTRIBUTE_H
#define COMS_UI_ATTRIBUTE_H

#include "../../stdlib/Types.h"
#include "../../utils/StringUtils.h"
#include "../../math/Evaluator.h"
#include "../../compiler/CompilerUtils.h"
#include "UIAttributeType.h"
#include "UIAttributeDimension.h"
#include "UIAttributeFont.h"

enum UIAttributeDataType : byte {
    UI_ATTRIBUTE_DATA_TYPE_INT,
    UI_ATTRIBUTE_DATA_TYPE_F32,
    UI_ATTRIBUTE_DATA_TYPE_STR,
    UI_ATTRIBUTE_DATA_TYPE_V4_F32,
};

struct UIAttribute {
    // Attributes use ids (=type name) instead of strings
    UIAttributeType attribute_id;
    UIAttributeDataType datatype;

    union {
        // @performance The string makes this struct really large when it is not needed in 95% of the cases
        char value_str[32];
        int32 value_int;
        uint32 value_uint;
        f32 value_float;
        v4_f32 value_v4_f32;
    };
};

struct UIAttributeGroup {
    // @question Why is this a uint32 instead of uint16, I think it doesn't matter due to the alignment?!
    uint32 attribute_count;
    // We don't use a pointer since this would prevent us from copying around the main data owner
    // The UIAttribute values come directly after UIAttributeGroup (e.g. group + 1 in memory)
    //UIAttribute* attributes;
};

UIAttribute* ui_attribute_from_group(const UIAttributeGroup* group, UIAttributeType type)
{
    if (!group->attribute_count) {
        return NULL;
    }

    UIAttribute* attributes = (UIAttribute *) (group + 1);

    // The following code is an optimized binary search
    // @performance Consider to use EytzingerSearch for even better performance
    int32 length = group->attribute_count;
    while (length > 1) {
        int32 half = length / 2;
        length -= half;
        attributes += (attributes[half - 1].attribute_id < type) * half;
    }

    return attributes->attribute_id == type ? attributes : NULL;
}

constexpr
int32 ui_attribute_type_to_id(const char* attribute_name)
{
    if (str_compare(attribute_name, "x") == 0) {
        return UI_ATTRIBUTE_TYPE_POSITION_X;
    } else if (str_compare(attribute_name, "y") == 0) {
        return UI_ATTRIBUTE_TYPE_POSITION_Y;
    } else if (str_compare(attribute_name, "width") == 0) {
        return UI_ATTRIBUTE_TYPE_DIMENSION_WIDTH;
    } else if (str_compare(attribute_name, "height") == 0) {
        return UI_ATTRIBUTE_TYPE_DIMENSION_HEIGHT;
    } else if (str_compare(attribute_name, "width_min") == 0) {
        return UI_ATTRIBUTE_TYPE_DIMENSION_WIDTH_MIN;
    } else if (str_compare(attribute_name, "height_min") == 0) {
        return UI_ATTRIBUTE_TYPE_DIMENSION_HEIGHT_MIN;
    } else if (str_compare(attribute_name, "width_max") == 0) {
        return UI_ATTRIBUTE_TYPE_DIMENSION_WIDTH_MAX;
    } else if (str_compare(attribute_name, "height_max") == 0) {
        return UI_ATTRIBUTE_TYPE_DIMENSION_HEIGHT_MAX;
    } else if (str_compare(attribute_name, "overflow") == 0) {
        return UI_ATTRIBUTE_TYPE_DIMENSION_OVERFLOW;
    } else if (str_compare(attribute_name, "font_name") == 0) {
        return UI_ATTRIBUTE_TYPE_FONT_NAME;
    } else if (str_compare(attribute_name, "font_color") == 0) {
        return UI_ATTRIBUTE_TYPE_FONT_COLOR;
    } else if (str_compare(attribute_name, "font_size") == 0) {
        return UI_ATTRIBUTE_TYPE_FONT_SIZE;
    } else if (str_compare(attribute_name, "font_weight") == 0) {
        return UI_ATTRIBUTE_TYPE_FONT_WEIGHT;
    } else if (str_compare(attribute_name, "font_line_height") == 0) {
        return UI_ATTRIBUTE_TYPE_FONT_LINE_HEIGHT;
    } else if (str_compare(attribute_name, "align_h") == 0) {
        return UI_ATTRIBUTE_TYPE_ALIGN_H;
    } else if (str_compare(attribute_name, "align_v") == 0) {
        return UI_ATTRIBUTE_TYPE_ALIGN_V;
    } else if (str_compare(attribute_name, "zindex") == 0) {
        return UI_ATTRIBUTE_TYPE_ZINDEX;
    } else if (str_compare(attribute_name, "style1") == 0) {
        return UI_ATTRIBUTE_TYPE_STYLE1;
    } else if (str_compare(attribute_name, "style2") == 0) {
        return UI_ATTRIBUTE_TYPE_STYLE2;
    } else if (str_compare(attribute_name, "style3") == 0) {
        return UI_ATTRIBUTE_TYPE_STYLE3;
    } else if (str_compare(attribute_name, "style4") == 0) {
        return UI_ATTRIBUTE_TYPE_STYLE4;
    } else if (str_compare(attribute_name, "style5") == 0) {
        return UI_ATTRIBUTE_TYPE_STYLE5;
    } else if (str_compare(attribute_name, "style6") == 0) {
        return UI_ATTRIBUTE_TYPE_STYLE6;
    } else if (str_compare(attribute_name, "style7") == 0) {
        return UI_ATTRIBUTE_TYPE_STYLE7;
    } else if (str_compare(attribute_name, "style8") == 0) {
        return UI_ATTRIBUTE_TYPE_STYLE8;
    } else if (str_compare(attribute_name, "foreground_color") == 0) {
        return UI_ATTRIBUTE_TYPE_FOREGROUND_COLOR;
    } else if (str_compare(attribute_name, "foreground_img") == 0) {
        return UI_ATTRIBUTE_TYPE_FOREGROUND_IMG;
    } else if (str_compare(attribute_name, "foreground_img_opacity") == 0) {
        return UI_ATTRIBUTE_TYPE_FOREGROUND_IMG_OPACITY;
    } else if (str_compare(attribute_name, "foreground_img_position_v") == 0) {
        return UI_ATTRIBUTE_TYPE_FOREGROUND_IMG_POSITION_V;
    } else if (str_compare(attribute_name, "foreground_img_position_h") == 0) {
        return UI_ATTRIBUTE_TYPE_FOREGROUND_IMG_POSITION_H;
    } else if (str_compare(attribute_name, "foreground_img_style") == 0) {
        return UI_ATTRIBUTE_TYPE_FOREGROUND_IMG_STYLE;
    } else if (str_compare(attribute_name, "background_color") == 0) {
        return UI_ATTRIBUTE_TYPE_BACKGROUND_COLOR;
    } else if (str_compare(attribute_name, "background_img") == 0) {
        return UI_ATTRIBUTE_TYPE_BACKGROUND_IMG;
    } else if (str_compare(attribute_name, "background_img_opacity") == 0) {
        return UI_ATTRIBUTE_TYPE_BACKGROUND_IMG_OPACITY;
    } else if (str_compare(attribute_name, "background_img_position_v") == 0) {
        return UI_ATTRIBUTE_TYPE_BACKGROUND_IMG_POSITION_V;
    } else if (str_compare(attribute_name, "background_img_position_h") == 0) {
        return UI_ATTRIBUTE_TYPE_BACKGROUND_IMG_POSITION_H;
    } else if (str_compare(attribute_name, "background_img_style") == 0) {
        return UI_ATTRIBUTE_TYPE_BACKGROUND_IMG_STYLE;
    } else if (str_compare(attribute_name, "border_color") == 0) {
        return UI_ATTRIBUTE_TYPE_BORDER_COLOR;
    } else if (str_compare(attribute_name, "border_width") == 0) {
        return UI_ATTRIBUTE_TYPE_BORDER_WIDTH;
    } else if (str_compare(attribute_name, "border_top_color") == 0) {
        return UI_ATTRIBUTE_TYPE_BORDER_TOP_COLOR;
    } else if (str_compare(attribute_name, "border_top_width") == 0) {
        return UI_ATTRIBUTE_TYPE_BORDER_TOP_WIDTH;
    } else if (str_compare(attribute_name, "border_right_color") == 0) {
        return UI_ATTRIBUTE_TYPE_BORDER_RIGHT_COLOR;
    } else if (str_compare(attribute_name, "border_right_width") == 0) {
        return UI_ATTRIBUTE_TYPE_BORDER_RIGHT_WIDTH;
    } else if (str_compare(attribute_name, "border_bottom_color") == 0) {
        return UI_ATTRIBUTE_TYPE_BORDER_BOTTOM_COLOR;
    } else if (str_compare(attribute_name, "border_bottom_width") == 0) {
        return UI_ATTRIBUTE_TYPE_BORDER_BOTTOM_WIDTH;
    } else if (str_compare(attribute_name, "border_left_color") == 0) {
        return UI_ATTRIBUTE_TYPE_BORDER_LEFT_COLOR;
    } else if (str_compare(attribute_name, "border_left_width") == 0) {
        return UI_ATTRIBUTE_TYPE_BORDER_LEFT_WIDTH;
    } else if (str_compare(attribute_name, "padding") == 0) {
        return UI_ATTRIBUTE_TYPE_PADDING;
    } else if (str_compare(attribute_name, "padding_top") == 0) {
        return UI_ATTRIBUTE_TYPE_PADDING_TOP;
    } else if (str_compare(attribute_name, "padding_right") == 0) {
        return UI_ATTRIBUTE_TYPE_PADDING_RIGHT;
    } else if (str_compare(attribute_name, "padding_bottom") == 0) {
        return UI_ATTRIBUTE_TYPE_PADDING_BOTTOM;
    } else if (str_compare(attribute_name, "padding_left") == 0) {
        return UI_ATTRIBUTE_TYPE_PADDING_LEFT;
    } else if (str_compare(attribute_name, "scroll_style") == 0) {
        return UI_ATTRIBUTE_TYPE_SCROLL_STYLE;
    } else if (str_compare(attribute_name, "scroll_x") == 0) {
        return UI_ATTRIBUTE_TYPE_SCROLL_X;
    } else if (str_compare(attribute_name, "scroll_y") == 0) {
        return UI_ATTRIBUTE_TYPE_SCROLL_Y;
    } else if (str_compare(attribute_name, "shadow_inner_color") == 0) {
        return UI_ATTRIBUTE_TYPE_SHADOW_INNER_COLOR;
    } else if (str_compare(attribute_name, "shadow_inner_angle") == 0) {
        return UI_ATTRIBUTE_TYPE_SHADOW_INNER_ANGLE;
    } else if (str_compare(attribute_name, "shadow_inner_distance") == 0) {
        return UI_ATTRIBUTE_TYPE_SHADOW_INNER_DISTANCE;
    } else if (str_compare(attribute_name, "shadow_outer_color") == 0) {
        return UI_ATTRIBUTE_TYPE_SHADOW_OUTER_COLOR;
    } else if (str_compare(attribute_name, "shadow_outer_angle") == 0) {
        return UI_ATTRIBUTE_TYPE_SHADOW_OUTER_ANGLE;
    } else if (str_compare(attribute_name, "shadow_outer_distance") == 0) {
        return UI_ATTRIBUTE_TYPE_SHADOW_OUTER_DISTANCE;
    } else if (str_compare(attribute_name, "transition_animation") == 0) {
        return UI_ATTRIBUTE_TYPE_TRANSITION_ANIMATION;
    } else if (str_compare(attribute_name, "transition_duration") == 0) {
        return UI_ATTRIBUTE_TYPE_TRANSITION_DURATION;
    } else if (str_compare(attribute_name, "text_limit") == 0) {
        return UI_ATTRIBUTE_TYPE_TEXT_LIMIT;
    } else if (str_compare(attribute_name, "cache_size") == 0) {
        return UI_ATTRIBUTE_TYPE_CACHE_SIZE;
    } else if (str_compare(attribute_name, "anim") == 0) {
        return UI_ATTRIBUTE_TYPE_ANIMATION;
    } else if (str_compare(attribute_name, "vertex_count") == 0) {
        return UI_ATTRIBUTE_TYPE_VERTEX_COUNT;
    }

    ASSERT_SIMPLE(false);

    return -1;
}

inline
void ui_attribute_parse_value(UIAttribute* attr, const char* attribute_name, const char* pos)
{
    attr->attribute_id = (UIAttributeType) ui_attribute_type_to_id(attribute_name);
    char value[64];

    str_copy_until(value, pos, "\r\n");

    if (str_is_integer(value)) {
        attr->value_int = (int32) str_to_int(value);
        attr->datatype = UI_ATTRIBUTE_DATA_TYPE_INT;
    } else if (str_is_float(value)) {
        attr->value_float = str_to_float(value, NULL);
        attr->datatype = UI_ATTRIBUTE_DATA_TYPE_F32;
    } else if (str_is_hex_color(value)) {
        ++pos; // Skip '#'
        hexstr_to_rgba(&attr->value_v4_f32, pos);
        attr->datatype = UI_ATTRIBUTE_DATA_TYPE_V4_F32;
    } else {
        str_copy_until(attr->value_str, value, "\r\n");
        attr->datatype = UI_ATTRIBUTE_DATA_TYPE_STR;
    }
}

inline
void ui_theme_assign_f32(f32* a, const UIAttribute* attr)
{
    if (attr->datatype == UI_ATTRIBUTE_DATA_TYPE_INT) {
        *a = (f32) attr->value_int;
    } else if (attr->datatype == UI_ATTRIBUTE_DATA_TYPE_F32) {
        *a = (f32) attr->value_float;
    } else if (attr->datatype == UI_ATTRIBUTE_DATA_TYPE_STR) {
        ASSERT_SIMPLE(str_length(attr->value_str) > 0);

        char value[32];
        memcpy(value, attr->value_str, ARRAY_COUNT(attr->value_str));
        *a = (f32) evaluator_evaluate(value);
    }
}

inline
void ui_theme_assign_dimension(UIAttributeDimension* dimension, const UIAttribute* attr)
{
    switch (attr->attribute_id) {
        case UI_ATTRIBUTE_TYPE_POSITION_X: {
                ui_theme_assign_f32(&dimension->dimension.x, attr);
            } break;
        case UI_ATTRIBUTE_TYPE_DIMENSION_WIDTH: {
                ui_theme_assign_f32(&dimension->dimension.width, attr);
            } break;
        case UI_ATTRIBUTE_TYPE_POSITION_Y: {
                ui_theme_assign_f32(&dimension->dimension.y, attr);
            } break;
        case UI_ATTRIBUTE_TYPE_DIMENSION_HEIGHT: {
                ui_theme_assign_f32(&dimension->dimension.height, attr);
            } break;
        default: {
            UNREACHABLE();
        }
    }
}

inline
void ui_theme_assign_font(UIAttributeFont* font, const UIAttribute* attr)
{
    switch (attr->attribute_id) {
        case UI_ATTRIBUTE_TYPE_FONT_NAME: {
                UNREACHABLE();
            } break;
        case UI_ATTRIBUTE_TYPE_FONT_COLOR: {
                font->color = attr->value_uint;
            } break;
        case UI_ATTRIBUTE_TYPE_FONT_SIZE: {
                font->size = attr->value_float;
            } break;
        case UI_ATTRIBUTE_TYPE_FONT_WEIGHT: {
                font->weight = attr->value_float;
            } break;
        case UI_ATTRIBUTE_TYPE_FONT_LINE_HEIGHT: {
                font->line_height = attr->value_float;
            } break;
        default: {
            UNREACHABLE();
        }
    }
}

#endif