#pragma once
#ifndef COMS_UI_ATTRIBUTE_C
#define COMS_UI_ATTRIBUTE_C

#include "UIAttribute.h"
#include "../UIUber.h"

const UIAttribute* ui_attribute_from_group(const UIAttributeGroup* const group, UIAttributeType type)
{
    if (!group->attribute_count) {
        return NULL;
    }

    // The attributes start aligned after the group (@see theme_from_file_txt)
    const UIAttribute* attributes = (const UIAttribute *) ALIGN_UP((uintptr_t) (group + 1), alignof(UIAttribute));

    // The following code is an optimized binary search
    // @performance Consider to use EytzingerSearch for even better performance
    int32 length = group->attribute_count;
    while (length > 1) {
        const int32 half = length / 2;
        length -= half;
        attributes += (attributes[half - 1].attribute_id < type) * half;
    }

    return attributes->attribute_id == type ? attributes : NULL;
}

// Parses a color in the format #RRGGBBAA or #RRGGBB
static inline
uint32 ui_color_from_str(const char* value) NO_EXCEPT
{
    if (*value == '#') {
        ++value;
    }

    int32 length = 0;
    while (HEX_LOOKUP_TABLE[(byte) value[length]]) {
        ++length;
    }

    uint32 color = (uint32) hex_to_int(value);

    // No alpha defined -> opaque
    if (length <= 6) {
        color = (color << 8) | 0xFF;
    }

    return color;
}

static inline
byte ui_anchor_align_from_str(const char* value) NO_EXCEPT
{
    if (strncmp(value, "topleft", sizeof("topleft") - 1) == 0) {
        return UI_ANCHOR_ALIGN_TOP_LEFT;
    } else if (strncmp(value, "topright", sizeof("topright") - 1) == 0) {
        return UI_ANCHOR_ALIGN_TOP_RIGHT;
    } else if (strncmp(value, "bottomleft", sizeof("bottomleft") - 1) == 0) {
        return UI_ANCHOR_ALIGN_BOTTOM_LEFT;
    } else if (strncmp(value, "bottomright", sizeof("bottomright") - 1) == 0) {
        return UI_ANCHOR_ALIGN_BOTTOM_RIGHT;
    } else if (strncmp(value, "center", sizeof("center") - 1) == 0) {
        return UI_ANCHOR_ALIGN_CENTER;
    }

    return UI_ANCHOR_ALIGN_TOP_LEFT;
}

/**
 * Parses a single "name: value" line of a layout file into the temporary uber element
 * The string values point into the file content (not copied)
 */
inline
void ui_uber_from_txt(
    UIUber* __restrict uber,
    const char* __restrict value_type,
    const char* value
) NO_EXCEPT
{
    if (strcmp(value_type, "pattern_length") == 0) {
        uber->pattern_length = (int32) str_to_int(value, &value);
    } else if (strcmp(value_type, "pattern") == 0) {
        // @todo How to write wchar_t in a text file when everything else is ascii?
        uber->pattern.str = value;
        str_move_to(&value, "\r\n");
        uber->pattern.length = (int32) ((uintptr_t) value - (uintptr_t) uber->pattern.str);
    } else if (strcmp(value_type, "content_length") == 0) {
        uber->content_length = (int32) str_to_int(value, &value);
    } else if (strcmp(value_type, "content") == 0) {
        // @todo How to write wchar_t in a text file when everything else is ascii?
        uber->content.str = value;
        str_move_to(&value, "\r\n");
        uber->content.length = (int32) ((uintptr_t) value - (uintptr_t) uber->content.str);
    } else if (strcmp(value_type, "font_size") == 0) {
        uber->font.size = (f32) str_to_float(value, &value);
    } else if (strcmp(value_type, "font_color") == 0) {
        uber->font.color = ui_color_from_str(value);
    } else if (strcmp(value_type, "background_color") == 0) {
        uber->background_color = ui_color_from_str(value);
    } else if (strcmp(value_type, "anchor") == 0) {
        uber->core.dimension.align_anchor = ui_anchor_align_from_str(value);
    } else if (strcmp(value_type, "x") == 0) {
        uber->core.dimension.pos_raw.x = (f32) str_to_float(value, &value);
    } else if (strcmp(value_type, "y") == 0) {
        uber->core.dimension.pos_raw.y = (f32) str_to_float(value, &value);
    } else if (strcmp(value_type, "width") == 0) {
        uber->core.dimension.dim_raw.x = (f32) str_to_float(value, &value);
    } else if (strcmp(value_type, "height") == 0) {
        uber->core.dimension.dim_raw.y = (f32) str_to_float(value, &value);
    } else if (strcmp(value_type, "update") == 0) {
        uber->core.update_func = (int16) str_to_int(value, &value);
    }
}

struct AttributeStringTypeMap {
    const char* name;
    int32 type;
};

static const CONSTEXPR
AttributeStringTypeMap attribute_string_type_map[] = {
    {"align_h", UI_ATTRIBUTE_TYPE_ALIGN_H},
    {"align_v", UI_ATTRIBUTE_TYPE_ALIGN_V},
    {"alignment", UI_ATTRIBUTE_TYPE_ALIGNMENT},
    {"anchor", UI_ATTRIBUTE_TYPE_ANCHOR},
    {"anim", UI_ATTRIBUTE_TYPE_ANIMATION},
    {"background_color", UI_ATTRIBUTE_TYPE_BACKGROUND_COLOR},
    {"background_img_opacity", UI_ATTRIBUTE_TYPE_BACKGROUND_IMG_OPACITY},
    {"background_img_position_h", UI_ATTRIBUTE_TYPE_BACKGROUND_IMG_POSITION_H},
    {"background_img_position_v", UI_ATTRIBUTE_TYPE_BACKGROUND_IMG_POSITION_V},
    {"background_img_style", UI_ATTRIBUTE_TYPE_BACKGROUND_IMG_STYLE},
    {"background_img", UI_ATTRIBUTE_TYPE_BACKGROUND_IMG},
    {"border_bottom_color", UI_ATTRIBUTE_TYPE_BORDER_BOTTOM_COLOR},
    {"border_bottom_width", UI_ATTRIBUTE_TYPE_BORDER_BOTTOM_WIDTH},
    {"border_color", UI_ATTRIBUTE_TYPE_BORDER_COLOR},
    {"border_left_color", UI_ATTRIBUTE_TYPE_BORDER_LEFT_COLOR},
    {"border_left_width", UI_ATTRIBUTE_TYPE_BORDER_LEFT_WIDTH},
    {"border_right_color", UI_ATTRIBUTE_TYPE_BORDER_RIGHT_COLOR},
    {"border_right_width", UI_ATTRIBUTE_TYPE_BORDER_RIGHT_WIDTH},
    {"border_top_color", UI_ATTRIBUTE_TYPE_BORDER_TOP_COLOR},
    {"border_top_width", UI_ATTRIBUTE_TYPE_BORDER_TOP_WIDTH},
    {"border_width", UI_ATTRIBUTE_TYPE_BORDER_WIDTH},
    {"cache_size", UI_ATTRIBUTE_TYPE_CACHE_SIZE},
    {"class", UI_ATTRIBUTE_TYPE_CLASS},
    {"content_length", UI_ATTRIBUTE_TYPE_CONTENT_LENGTH},
    {"content", UI_ATTRIBUTE_TYPE_CONTENT},
    {"font_color", UI_ATTRIBUTE_TYPE_FONT_COLOR},
    {"font_line_height", UI_ATTRIBUTE_TYPE_FONT_LINE_HEIGHT},
    {"font_name", UI_ATTRIBUTE_TYPE_FONT_NAME},
    {"font_size", UI_ATTRIBUTE_TYPE_FONT_SIZE},
    {"font_weight", UI_ATTRIBUTE_TYPE_FONT_WEIGHT},
    {"foreground_color", UI_ATTRIBUTE_TYPE_FOREGROUND_COLOR},
    {"foreground_img_opacity", UI_ATTRIBUTE_TYPE_FOREGROUND_IMG_OPACITY},
    {"foreground_img_position_h", UI_ATTRIBUTE_TYPE_FOREGROUND_IMG_POSITION_H},
    {"foreground_img_position_v", UI_ATTRIBUTE_TYPE_FOREGROUND_IMG_POSITION_V},
    {"foreground_img_style", UI_ATTRIBUTE_TYPE_FOREGROUND_IMG_STYLE},
    {"foreground_img", UI_ATTRIBUTE_TYPE_FOREGROUND_IMG},
    {"height_max", UI_ATTRIBUTE_TYPE_DIMENSION_HEIGHT_MAX},
    {"height_min", UI_ATTRIBUTE_TYPE_DIMENSION_HEIGHT_MIN},
    {"height", UI_ATTRIBUTE_TYPE_DIMENSION_HEIGHT},
    {"inherit", UI_ATTRIBUTE_TYPE_INHERIT},
    {"overflow", UI_ATTRIBUTE_TYPE_DIMENSION_OVERFLOW},
    {"padding_bottom", UI_ATTRIBUTE_TYPE_PADDING_BOTTOM},
    {"padding_left", UI_ATTRIBUTE_TYPE_PADDING_LEFT},
    {"padding_right", UI_ATTRIBUTE_TYPE_PADDING_RIGHT},
    {"padding_top", UI_ATTRIBUTE_TYPE_PADDING_TOP},
    {"padding", UI_ATTRIBUTE_TYPE_PADDING},
    {"pattern_length", UI_ATTRIBUTE_TYPE_PATTERN_LENGTH},
    {"pattern", UI_ATTRIBUTE_TYPE_PATTERN},
    {"scroll_style", UI_ATTRIBUTE_TYPE_SCROLL_STYLE},
    {"scroll_x", UI_ATTRIBUTE_TYPE_SCROLL_X},
    {"scroll_y", UI_ATTRIBUTE_TYPE_SCROLL_Y},
    {"shadow_inner_angle", UI_ATTRIBUTE_TYPE_SHADOW_INNER_ANGLE},
    {"shadow_inner_color", UI_ATTRIBUTE_TYPE_SHADOW_INNER_COLOR},
    {"shadow_inner_distance", UI_ATTRIBUTE_TYPE_SHADOW_INNER_DISTANCE},
    {"shadow_outer_angle", UI_ATTRIBUTE_TYPE_SHADOW_OUTER_ANGLE},
    {"shadow_outer_color", UI_ATTRIBUTE_TYPE_SHADOW_OUTER_COLOR},
    {"shadow_outer_distance", UI_ATTRIBUTE_TYPE_SHADOW_OUTER_DISTANCE},
    {"style1", UI_ATTRIBUTE_TYPE_STYLE1},
    {"style2", UI_ATTRIBUTE_TYPE_STYLE2},
    {"style3", UI_ATTRIBUTE_TYPE_STYLE3},
    {"style4", UI_ATTRIBUTE_TYPE_STYLE4},
    {"style5", UI_ATTRIBUTE_TYPE_STYLE5},
    {"style6", UI_ATTRIBUTE_TYPE_STYLE6},
    {"style7", UI_ATTRIBUTE_TYPE_STYLE7},
    {"style8", UI_ATTRIBUTE_TYPE_STYLE8},
    {"text_limit", UI_ATTRIBUTE_TYPE_TEXT_LIMIT},
    {"transition_animation", UI_ATTRIBUTE_TYPE_TRANSITION_ANIMATION},
    {"transition_duration", UI_ATTRIBUTE_TYPE_TRANSITION_DURATION},
    {"type", UI_ATTRIBUTE_TYPE_TYPE},
    {"update", UI_ATTRIBUTE_TYPE_UPDATE},
    {"vertex_count", UI_ATTRIBUTE_TYPE_VERTEX_COUNT},
    {"width_max", UI_ATTRIBUTE_TYPE_DIMENSION_WIDTH_MAX},
    {"width_min", UI_ATTRIBUTE_TYPE_DIMENSION_WIDTH_MIN},
    {"width", UI_ATTRIBUTE_TYPE_DIMENSION_WIDTH},
    {"x", UI_ATTRIBUTE_TYPE_POSITION_X},
    {"y", UI_ATTRIBUTE_TYPE_POSITION_Y},
    {"zindex", UI_ATTRIBUTE_TYPE_ZINDEX},
};

static inline
int32 ui_attribute_type_to_id(const char* attribute_name) NO_EXCEPT
{
    const size_t count = ARRAY_COUNT(attribute_string_type_map);
    const char c = attribute_name[0];

    size_t left = 0;
    size_t right = count;

    // Binary search for any entry starting with c
    // @performance If we ever make this a runtime function instead of build time we could heavily optimize this
    //              Use 2 separate arrays or even have a string with only the first character to find the index
    //              Since this is only get called during build time we don't really care
    while (left < right) {
        size_t mid = left + (right - left) / 2;

        if (attribute_string_type_map[mid].name[0] < c) {
            left = mid + 1;
        } else {
            right = mid;
        }
    }

    // No entry starts with this character
    if (left == count || attribute_string_type_map[left].name[0] != c) {
        ASSERT_THROW();
        return -1;
    }

    // Scan forward until the first character changes
    for (size_t i = left;
        i < count &&
        attribute_string_type_map[i].name[0] == c;
        ++i
    ) {
        if (strcmp(attribute_name, attribute_string_type_map[i].name) == 0) {
            return attribute_string_type_map[i].type;
        }
    }

    ASSERT_THROW();
    return -1;
}

inline
void ui_attribute_parse_value(UIAttribute* const attr, const char* attribute_name, const char* pos)
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
void ui_theme_assign_f32(f32* a, const UIAttribute* const attr)
{
    if (attr->datatype == UI_ATTRIBUTE_DATA_TYPE_INT) {
        *a = (f32) attr->value_int;
    } else if (attr->datatype == UI_ATTRIBUTE_DATA_TYPE_F32) {
        *a = (f32) attr->value_float;
    } else if (attr->datatype == UI_ATTRIBUTE_DATA_TYPE_STR) {
        ASSERT_TRUE(strlen(attr->value_str) > 0);

        char value[32];
        memcpy(value, attr->value_str, ARRAY_COUNT(attr->value_str));
        *a = (f32) evaluator_evaluate(value);
    }
}

/**
 * Converts the attribute value to a RGBA color
 * Colors are parsed as v4_f32 (0.0 - 1.0) from the theme text file (@see ui_attribute_parse_value)
 */
inline
uint32 ui_theme_color(const UIAttribute* const attr)
{
    if (attr->datatype == UI_ATTRIBUTE_DATA_TYPE_V4_F32) {
        return ((uint32) (attr->value_v4_f32.r * 255.0f + 0.5f) << 24)
            | ((uint32) (attr->value_v4_f32.g * 255.0f + 0.5f) << 16)
            | ((uint32) (attr->value_v4_f32.b * 255.0f + 0.5f) << 8)
            | (uint32) (attr->value_v4_f32.a * 255.0f + 0.5f);
    }

    return attr->value_uint;
}

// Themes define the raw values, the calculated values are created by ui_dimension_calculate()
inline
void ui_theme_assign_dimension(UIAttributeDimension* const dimension, const UIAttribute* const attr)
{
    switch (attr->attribute_id) {
        case UI_ATTRIBUTE_TYPE_POSITION_X: {
                ui_theme_assign_f32(&dimension->pos_raw.x, attr);
            } break;
        case UI_ATTRIBUTE_TYPE_DIMENSION_WIDTH: {
                ui_theme_assign_f32(&dimension->dim_raw.x, attr);
            } break;
        case UI_ATTRIBUTE_TYPE_POSITION_Y: {
                ui_theme_assign_f32(&dimension->pos_raw.y, attr);
            } break;
        case UI_ATTRIBUTE_TYPE_DIMENSION_HEIGHT: {
                ui_theme_assign_f32(&dimension->dim_raw.y, attr);
            } break;
        default: {
            UNREACHABLE();
        }
    }
}

inline
void ui_theme_assign_font(UIAttributeFont* const font, const UIAttribute* const attr)
{
    switch (attr->attribute_id) {
        case UI_ATTRIBUTE_TYPE_FONT_COLOR: {
                font->color = ui_theme_color(attr);
            } break;
        case UI_ATTRIBUTE_TYPE_FONT_SIZE: {
                ui_theme_assign_f32(&font->size, attr);
            } break;
        case UI_ATTRIBUTE_TYPE_FONT_LINE_HEIGHT: {
                ui_theme_assign_f32(&font->line_height, attr);
            } break;
        default: {
            // @todo FONT_NAME, FONT_WEIGHT are not supported by the text rendering yet
        }
    }
}

#endif