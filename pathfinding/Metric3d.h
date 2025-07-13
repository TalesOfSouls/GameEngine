/**
 * Jingga
 *
 * @package   Utils
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_PATHFINDING_METRIC3D_H
#define COMS_PATHFINDING_METRIC3D_H

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "../stdlib/Types.h"

// Manhattan distance for 3D
f32 manhattan_3d(v3_f32 a, v3_f32 b) {
    return fabs(a.x - b.x)
        + fabs(a.y - b.y)
        + fabs(a.z - b.z);
}

// Euclidean distance for 3D
f32 euclidean_3d(v3_f32 a, v3_f32 b) {
    f32 dx = a.x - b.x;
    f32 dy = a.y - b.y;
    f32 dz = a.z - b.z;

    return sqrt(dx * dx + dy * dy + dz * dz);
}

// Chebyshev distance for 3D
f32 chebyshev_3d(v3_f32 a, v3_f32 b) {
    f32 dx = fabs(a.x - b.x);
    f32 dy = fabs(a.y - b.y);
    f32 dz = fabs(a.z - b.z);

    return fmax(fmax(dx, dy), dz);
}

// Minkowski distance for 3D
f32 minkowski_3d(v3_f32 a, v3_f32 b, int32 lambda) {
    f32 dx = fabs(a.x - b.x);
    f32 dy = fabs(a.y - b.y);
    f32 dz = fabs(a.z - b.z);

    return pow(
        pow(dx, lambda)
        + pow(dy, lambda)
        + pow(dz, lambda),
        1.0 / lambda
    );
}

// Canberra distance for 3D
f32 canberra_3d(v3_f32 a, v3_f32 b) {
    return fabs(a.x - b.x) / (fabs(a.x) + fabs(b.x))
        + fabs(a.y - b.y) / (fabs(a.y) + fabs(b.y))
        + fabs(a.z - b.z) / (fabs(a.z) + fabs(b.z));
}

// Bray-Curtis distance for 3D
f32 bray_curtis_3d(v3_f32 a, v3_f32 b) {
    return (fabs(a.x - b.x) + fabs(a.y - b.y) + fabs(a.z - b.z))
        / ((a.x + b.x) + (a.y + b.y) + (a.z + b.z));
}

// Angular separation for 3D
f32 angular_separation_3d(v3_f32 a, v3_f32 b) {
    return (a.x * b.x + a.y * b.y + a.z * b.z)
        / (sqrt(a.x * a.x + a.y * a.y + a.z * a.z) * sqrt(b.x * b.x + b.y * b.y + b.z * b.z));
}

// Hamming distance for arrays
int32 hamming(int32* a, int32* b, int32 size) {
    int32 dist = 0;

    for (int32 i = 0; i < size; ++i) {
        if (a[i] != b[i]) {
            ++dist;
        }
    }

    return dist;
}

#endif