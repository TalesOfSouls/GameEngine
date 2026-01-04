#ifndef COMS_UI_UBER_H
#define COMS_UI_UBER_H

#include "../stdlib/Stdlib.h"
#include "../camera/Camera.h"

// A Uber element that is very versatile and can hold basically all possible atributes
// Using this type is not encouraged since it is expensive in memory and when rendering
struct UIUber {
    UIAttributeDimension dimension;
    byte opacity; // 1 byte alpha channel
    byte padding;

    UIAttributeBackground background;
    UIAttributeBorder border;
    UIAttributeShadow shadow_outer;
    UIAttributeShadow shadow_inner;

    // @todo implement more
}

#endif