/**
 * Jingga
 *
 * @package   Utils
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_PATHFINDING_METRIC2D_H
#define COMS_PATHFINDING_METRIC2D_H

#include <stdio.h>
#include <stdlib.h>

#include "../stdlib/Types.h"

f32 manhattan_2d(v2_f32 a, v2_f32 b) {
    return fabs(a.x - b.x) + fabs(a.y - b.y);
}

f32 euclidean_2d(v2_f32 a, v2_f32 b) {
    f32 dx = fabs(a.x - b.x);
    f32 dy = fabs(a.y - b.y);

    return sqrt(dx * dx + dy * dy);
}

f32 octile_2d(v2_f32 a, v2_f32 b) {
    f32 dx = fabs(a.x - b.x);
    f32 dy = fabs(a.y - b.y);

    return dx < dy
        ? (SQRT_2 - 1) * dx + dy
        : (SQRT_2 - 1) * dy + dx;
}

f32 chebyshev_2d(v2_f32 a, v2_f32 b) {
    f32 dx = fabs(a.x - b.x);
    f32 dy = fabs(a.y - b.y);

    return fmax(dx, dy);
}

f32 minkowski_2d(v2_f32 a, v2_f32 b, int32 lambda) {
    f32 dx = fabs(a.x - b.x);
    f32 dy = fabs(a.y - b.y);

    return pow(pow(dx, lambda) + pow(dy, lambda), 1.0 / lambda);
}

f32 canberra_2d(v2_f32 a, v2_f32 b) {
    return (fabs(a.x - b.x) / (fabs(a.x) + fabs(b.x)))
        + (fabs(a.y - b.y) / (fabs(a.y) + fabs(b.y)));
}

f32 bray_curtis_2d(v2_f32 a, v2_f32 b) {
    return (fabs(a.x - b.x) + fabs(a.y - b.y))
        / ((a.x + b.x) + (a.y + b.y));
}

f32 angular_separation_2d(v2_f32 a, v2_f32 b) {
    return (a.x * b.x + a.y * b.y)
        / (sqrt(a.x * a.x + a.y * a.y) * sqrt(b.x * b.x + b.y * b.y));
}

#endif