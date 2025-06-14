/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_COLOR_VISION_DEFICIENCY_H
#define COMS_COLOR_VISION_DEFICIENCY_H

#include "../stdlib/Types.h"

f32 protanopia_matrix[9] = {
    0.567f, 0.433f, 0.000f,
    0.558f, 0.442f, 0.000f,
    0.000f, 0.242f, 0.758f
};

f32 deuteranopia_matrix[9] = {
    0.625f, 0.375f, 0.000f,
    0.700f, 0.300f, 0.000f,
    0.000f, 0.300f, 0.700f
};

f32 tritanopia_matrix[9] = {
    0.950f, 0.050f, 0.000f,
    0.000f, 0.433f, 0.567f,
    0.000f, 0.475f, 0.525f
};

f32 protanomaly_matrix[9] = {
    0.817f, 0.183f, 0.000f,
    0.333f, 0.667f, 0.000f,
    0.000f, 0.125f, 0.875f
};

f32 deuteranomaly_matrix[9] = {
    0.800f, 0.200f, 0.000f,
    0.258f, 0.742f, 0.000f,
    0.000f, 0.142f, 0.858f
};

f32 tritanomaly_matrix[9] = {
    0.967f, 0.033f, 0.000f,
    0.000f, 0.733f, 0.267f,
    0.000f, 0.183f, 0.817f
};

f32 achromatopsia_matrix[9] = {
    0.299f, 0.587f, 0.114f,
    0.299f, 0.587f, 0.114f,
    0.299f, 0.587f, 0.114f
};

#endif