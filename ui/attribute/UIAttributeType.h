#ifndef TOS_UI_ATTRIBUTE_TYPE_H
#define TOS_UI_ATTRIBUTE_TYPE_H

#include "../../stdlib/Types.h"

enum UIAttributeType : uint16 {
    UI_ATTRIBUTE_TYPE_NONE,

    UI_ATTRIBUTE_TYPE_TYPE,

    UI_ATTRIBUTE_TYPE_MIN_VALUE,
    UI_ATTRIBUTE_TYPE_MAX_VALUE,
    UI_ATTRIBUTE_TYPE_MAX_INPUT_LENGTH,

    UI_ATTRIBUTE_TYPE_POSITION_X,
    UI_ATTRIBUTE_TYPE_POSITION_Y,

    UI_ATTRIBUTE_TYPE_DIMENSION_WIDTH,
    UI_ATTRIBUTE_TYPE_DIMENSION_HEIGHT,
    UI_ATTRIBUTE_TYPE_DIMENSION_WIDTH_MIN,
    UI_ATTRIBUTE_TYPE_DIMENSION_HEIGHT_MIN,
    UI_ATTRIBUTE_TYPE_DIMENSION_WIDTH_MAX,
    UI_ATTRIBUTE_TYPE_DIMENSION_HEIGHT_MAX,

    // Allows elements to overflow their parent while still positioned relative to their parent element
    // e.g. Text in a button (e.g. a cooldown timer of a skill could be positioned below a button)
    UI_ATTRIBUTE_TYPE_DIMENSION_OVERFLOW,

    UI_ATTRIBUTE_TYPE_CONTENT,
    UI_ATTRIBUTE_TYPE_CONTENT_ALIGN_H,
    UI_ATTRIBUTE_TYPE_CONTENT_ALIGN_V,

    UI_ATTRIBUTE_TYPE_FONT_NAME,
    UI_ATTRIBUTE_TYPE_FONT_COLOR,
    UI_ATTRIBUTE_TYPE_FONT_SIZE,
    UI_ATTRIBUTE_TYPE_FONT_WEIGHT,
    UI_ATTRIBUTE_TYPE_FONT_LINE_HEIGHT,

    UI_ATTRIBUTE_TYPE_ALIGN_H,
    UI_ATTRIBUTE_TYPE_ALIGN_V,

    UI_ATTRIBUTE_TYPE_ZINDEX,
    UI_ATTRIBUTE_TYPE_PARENT,

    // Sub styles for components
    // E.g. scroll bar usess style1 for background box and style2 for movable bar
    UI_ATTRIBUTE_TYPE_STYLE1,
    UI_ATTRIBUTE_TYPE_STYLE2,
    UI_ATTRIBUTE_TYPE_STYLE3,
    UI_ATTRIBUTE_TYPE_STYLE4,
    UI_ATTRIBUTE_TYPE_STYLE5,
    UI_ATTRIBUTE_TYPE_STYLE6,
    UI_ATTRIBUTE_TYPE_STYLE7,
    UI_ATTRIBUTE_TYPE_STYLE8,

    UI_ATTRIBUTE_TYPE_FOREGROUND_COLOR_INDEX,
    UI_ATTRIBUTE_TYPE_FOREGROUND_COLOR,
    UI_ATTRIBUTE_TYPE_FOREGROUND_IMG,
    UI_ATTRIBUTE_TYPE_FOREGROUND_IMG_OPACITY,
    UI_ATTRIBUTE_TYPE_FOREGROUND_IMG_POSITION_V,
    UI_ATTRIBUTE_TYPE_FOREGROUND_IMG_POSITION_H,
    UI_ATTRIBUTE_TYPE_FOREGROUND_IMG_STYLE,

    UI_ATTRIBUTE_TYPE_BACKGROUND_COLOR_INDEX,
    UI_ATTRIBUTE_TYPE_BACKGROUND_COLOR,
    UI_ATTRIBUTE_TYPE_BACKGROUND_IMG,
    UI_ATTRIBUTE_TYPE_BACKGROUND_IMG_OPACITY,
    UI_ATTRIBUTE_TYPE_BACKGROUND_IMG_POSITION_V,
    UI_ATTRIBUTE_TYPE_BACKGROUND_IMG_POSITION_H,
    UI_ATTRIBUTE_TYPE_BACKGROUND_IMG_STYLE,

    UI_ATTRIBUTE_TYPE_BORDER_COLOR_INDEX,
    UI_ATTRIBUTE_TYPE_BORDER_COLOR,
    UI_ATTRIBUTE_TYPE_BORDER_WIDTH,
    UI_ATTRIBUTE_TYPE_BORDER_TOP_COLOR,
    UI_ATTRIBUTE_TYPE_BORDER_TOP_WIDTH,
    UI_ATTRIBUTE_TYPE_BORDER_RIGHT_COLOR,
    UI_ATTRIBUTE_TYPE_BORDER_RIGHT_WIDTH,
    UI_ATTRIBUTE_TYPE_BORDER_BOTTOM_COLOR,
    UI_ATTRIBUTE_TYPE_BORDER_BOTTOM_WIDTH,
    UI_ATTRIBUTE_TYPE_BORDER_LEFT_COLOR,
    UI_ATTRIBUTE_TYPE_BORDER_LEFT_WIDTH,

    UI_ATTRIBUTE_TYPE_PADDING,
    UI_ATTRIBUTE_TYPE_PADDING_TOP,
    UI_ATTRIBUTE_TYPE_PADDING_RIGHT,
    UI_ATTRIBUTE_TYPE_PADDING_BOTTOM,
    UI_ATTRIBUTE_TYPE_PADDING_LEFT,

    UI_ATTRIBUTE_TYPE_SCROLL_STYLE,
    UI_ATTRIBUTE_TYPE_SCROLL_X,
    UI_ATTRIBUTE_TYPE_SCROLL_Y,

    UI_ATTRIBUTE_TYPE_SHADOW_INNER_COLOR_INDEX,
    UI_ATTRIBUTE_TYPE_SHADOW_INNER_COLOR,
    UI_ATTRIBUTE_TYPE_SHADOW_INNER_ANGLE,
    UI_ATTRIBUTE_TYPE_SHADOW_INNER_DISTANCE,

    UI_ATTRIBUTE_TYPE_SHADOW_OUTER_COLOR_INDEX,
    UI_ATTRIBUTE_TYPE_SHADOW_OUTER_COLOR,
    UI_ATTRIBUTE_TYPE_SHADOW_OUTER_ANGLE,
    UI_ATTRIBUTE_TYPE_SHADOW_OUTER_DISTANCE,

    // @todo This isn't enough, we can have many animations (position, size, colors, ...)
    //  Maybe we need to define an animation child which overwrites the defined values
    //  Maybe it should use the same system as state dependent values like hover, active, ...
    UI_ATTRIBUTE_TYPE_TRANSITION_ANIMATION,
    UI_ATTRIBUTE_TYPE_TRANSITION_DURATION,

    UI_ATTRIBUTE_TYPE_TEXT_LIMIT,

    // How much memory do we reserve for custom data in this element
    UI_ATTRIBUTE_TYPE_CACHE_SIZE,

    UI_ATTRIBUTE_TYPE_ANIMATION,

    // What is the max amount of possible vertices for an element
    // This is used to reserve memory in our vertex cache
    UI_ATTRIBUTE_TYPE_VERTEX_COUNT,

    UI_ATTRIBUTE_TYPE_SIZE,
};


#endif