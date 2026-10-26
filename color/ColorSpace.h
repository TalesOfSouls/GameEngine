/**
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 *
 * Color space conversions between the display referred sRGB space (theme files, authored colors,
 * PNG/JPG images) and the linear light space used for blending and lighting.
 *
 * The render pipeline renders into an sRGB framebuffer, which means all shaders have to write
 * linear values. Anything that is authored (color constants, vertex colors, light colors) is
 * sRGB and has to be converted before it is used in linear math.
 */
#pragma once
#ifndef COMS_COLOR_COLOR_SPACE_H
#define COMS_COLOR_COLOR_SPACE_H

#include "../stdlib/Stdlib.h"

// IEC 61966-2-1 sRGB -> linear
FORCE_INLINE
f32 srgb_to_linear(f32 c) NO_EXCEPT
{
    return c <= 0.04045f ? c / 12.92f : powf((c + 0.055f) / 1.055f, 2.4f);
}

FORCE_INLINE
f64 srgb_to_linear(f64 c) NO_EXCEPT
{
    return c <= 0.04045 ? c / 12.92 : pow((c + 0.055) / 1.055, 2.4);
}

// IEC 61966-2-1 linear -> sRGB
FORCE_INLINE
f32 linear_to_srgb(f32 c) NO_EXCEPT
{
    return c <= 0.0031308f ? c * 12.92f : 1.055f * powf(c, 1.0f / 2.4f) - 0.055f;
}

FORCE_INLINE
f64 linear_to_srgb(f64 c) NO_EXCEPT
{
    return c <= 0.0031308 ? c * 12.92 : 1.055 * pow(c, 1.0 / 2.4) - 0.055;
}

/**
 * Converts a display referred (sRGB) color to linear
 *
 * The alpha channel is passed through untouched, it is always linear.
 */
FORCE_INLINE
void srgb_color_to_linear(f32* const color) NO_EXCEPT
{
    color[0] = srgb_to_linear(color[0]);
    color[1] = srgb_to_linear(color[1]);
    color[2] = srgb_to_linear(color[2]);
}

#endif
