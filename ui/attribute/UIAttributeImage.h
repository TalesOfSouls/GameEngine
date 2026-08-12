#pragma once
#ifndef COMS_UI_ATTRIBUTE_IMAGE_H
#define COMS_UI_ATTRIBUTE_IMAGE_H

#include "../../stdlib/Stdlib.h"
#include "UIAttributeDimension.h"

struct UIAttributeImage {
    // 1-indexed texture_id
    uint32 texture_id;

    UIAttributeDimension dimension;

    v2_f32 tex_coord[4];
};

#endif