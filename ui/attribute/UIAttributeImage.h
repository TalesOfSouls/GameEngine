#pragma once
#ifndef COMS_UI_ATTRIBUTE_IMAGE_H
#define COMS_UI_ATTRIBUTE_IMAGE_H

#include "../../stdlib/Stdlib.h"

// Sampler/texture slot used by the ui shader
// Font = 1, UI atlas = 2 (@see ui shader)
#define UI_SAMPLER_NONE 0
#define UI_SAMPLER_FONT 1
#define UI_SAMPLER_ATLAS 2

// Image data is resolved ONCE (e.g. from the texture atlas) and stored inline
// This way rendering never has to look up the atlas
//
// uv values are stored as unorm16 (0 - 65535 = 0.0 - 1.0)
// For a 4096px texture the error is < 0.04px which is plenty
//
// 16 bytes (the previous version was 72 bytes)
struct UIAttributeImage {
    // UI_SAMPLER_NONE = no image
    int16 sampler;

    // Natural size of the image in px
    // Used if the element doesn't define a dimension (dim_raw == 0)
    uint16 width;
    uint16 height;

    uint16 _pad;

    // Bottom left and top right uv coordinate
    // bl.x, bl.y, tr.x, tr.y
    uint16 uv[4];
};

FORCE_INLINE
uint16 ui_uv_encode(f32 value) NO_EXCEPT
{
    value = value < 0.0f ? 0.0f : (value > 1.0f ? 1.0f : value);

    return (uint16) (value * 65535.0f + 0.5f);
}

FORCE_INLINE
f32 ui_uv_decode(uint16 value) NO_EXCEPT
{
    return (f32) value * (1.0f / 65535.0f);
}

#endif
