/**
 * Jingga
 *
 * @package   Utils
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_NOISE_SIMPLEX_H
#define COMS_NOISE_SIMPLEX_H

#include <stdio.h>

#define SIMPLEX_NOISE_F2 0.5 * (sqrt(3.0) - 1.0)
#define SIMPLEX_NOISE_G2 (3.0 - sqrt(3.0)) / 6.0

static const int perm[512] = {
    151,160,137,91,90,15,
    131,13,201,95,96,53,194,233,7,225,140,36,103,30,69,142,

    151,160,137,91,90,15,
    131,13,201,95,96,53,194,233,7,225,140,36,103,30,69,142,
};

static const int grad3_2[12][2] = {
    {1,1}, {-1,1}, {1,-1}, {-1,-1},
    {1,0}, {-1,0}, {1,0}, {-1,0},
    {0,1}, {0,-1}, {0,1}, {0,-1}
};

static const int grad3_3[12][3] = {
    {1,1,0}, {-1,1,0}, {1,-1,0}, {-1,-1,0},
    {1,0,1}, {-1,0,1}, {1,0,-1}, {-1,0,-1},
    {0,1,1}, {0,-1,1}, {0,1,-1}, {0,-1,-1}
};

static inline double simplex_noise_dot2(const int32* g, double x, double y) {
    return g[0] * x + g[1] * y;
}

static inline double simplex_noise_dot3(const int32* g, double x, double y, double z) {
    return g[0] * x + g[1] * y + g[2] * z;
}

double simplex_noise_2d(double x, double y) {
    double n0, n1, n2; // Noise contributions from the three corners

    // Skew the input space to determine which simplex cell we're in
    double s = (x + y) * SIMPLEX_NOISE_F2; // Hairy factor for 2D
    int32 i = floor(x + s);
    int32 j = floor(y + s);

    double t = (i + j) * SIMPLEX_NOISE_G2;
    double X0 = i - t; // Unskew the cell origin back to (x, y) space
    double Y0 = j - t;
    double x0 = x - X0; // The x, y distances from the cell origin
    double y0 = y - Y0;

    // For the 2D case, the simplex shape is an equilateral triangle.
    // Determine which simplex we are in.
    int32 i1, j1; // Offsets for the second (middle) corner of simplex in (i, j)
    if (x0 > y0) {
        i1 = 1; j1 = 0; // Lower triangle, XY order
    } else {
        i1 = 0; j1 = 1; // Upper triangle, YX order
    }

    // A step of (1, 0) in (i, j) means a step of (1 - c, -c) in (x, y), and
    // a step of (0, 1) in (i, j) means a step of (-c, 1 - c) in (x, y), where
    // c = (3 - sqrt(3)) / 6

    double x1 = x0 - i1 + SIMPLEX_NOISE_G2; // Offsets for middle corner in (x, y) unskewed coords
    double y1 = y0 - j1 + SIMPLEX_NOISE_G2;
    double x2 = x0 - 1.0 + 2.0 * SIMPLEX_NOISE_G2; // Offsets for last corner in (x, y) unskewed coords
    double y2 = y0 - 1.0 + 2.0 * SIMPLEX_NOISE_G2;

    // Work out the hashed gradient indices of the three simplex corners
    int32 ii = i & 255;
    int32 jj = j & 255;
    int32 gi0 = perm[ii + perm[jj]] % 12;
    int32 gi1 = perm[ii + i1 + perm[jj + j1]] % 12;
    int32 gi2 = perm[ii + 1 + perm[jj + 1]] % 12;

    // Calculate the contribution from the three corners
    double t0 = 0.5 - x0 * x0 - y0 * y0;
    if (t0 < 0) {
        n0 = 0.0;
    } else {
        t0 *= t0;
        n0 = t0 * t0 * simplex_noise_dot2(grad3_2[gi0], x0, y0); // (x,y) of grad3_2 used for 2D gradient
    }

    double t1 = 0.5 - x1 * x1 - y1 * y1;
    if (t1 < 0) {
        n1 = 0.0;
    } else {
        t1 *= t1;
        n1 = t1 * t1 * simplex_noise_dot2(grad3_2[gi1], x1, y1);
    }

    double t2 = 0.5 - x2 * x2 - y2 * y2;
    if (t2 < 0) {
        n2 = 0.0;
    } else {
        t2 *= t2;
        n2 = t2 * t2 * simplex_noise_dot2(grad3_2[gi2], x2, y2);
    }

    // Add contributions from each corner to get the final noise value.
    // The result is scaled to return values in the interval [-1,1].
    return 70.0 * (n0 + n1 + n2);
}

double simplex_noise_3d(double x, double y, double z) {
    double n0, n1, n2, n3; // Noise contributions from the four corners

    // Skew the input space to determine which simplex cell we're in
    double s = (x + y + z) * SIMPLEX_NOISE_F2; // Skew factor for 3D
    int i = floor(x + s);
    int j = floor(y + s);
    int k = floor(z + s);

    double t = (i + j + k) * SIMPLEX_NOISE_G2;
    double X0 = i - t; // Unskew the cell origin back to (x, y, z) space
    double Y0 = j - t;
    double Z0 = k - t;
    double x0 = x - X0; // The x, y, z distances from the cell origin
    double y0 = y - Y0;
    double z0 = z - Z0;

    // For the 3D case, the simplex shape is a slightly irregular tetrahedron.
    // Determine which simplex we are in.
    int i1, j1, k1; // Offsets for second corner of simplex in (i, j, k)
    int i2, j2, k2; // Offsets for third corner of simplex in (i, j, k)

    if (x0 >= y0) {
        if (y0 >= z0) {
            i1 = 1; j1 = 0; k1 = 0;  // X Y Z order
            i2 = 1; j2 = 1; k2 = 0;
        } else if (x0 >= z0) {
            i1 = 1; j1 = 0; k1 = 0;  // X Z Y order
            i2 = 1; j2 = 0; k2 = 1;
        } else {
            i1 = 0; j1 = 0; k1 = 1;  // Z X Y order
            i2 = 1; j2 = 0; k2 = 1;
        }
    } else { // x0 < y0
        if (y0 < z0) {
            i1 = 0; j1 = 0; k1 = 1;  // Z Y X order
            i2 = 0; j2 = 1; k2 = 1;
        } else if (x0 < z0) {
            i1 = 0; j1 = 1; k1 = 0;  // Y Z X order
            i2 = 0; j2 = 1; k2 = 1;
        } else {
            i1 = 0; j1 = 1; k1 = 0;  // Y X Z order
            i2 = 1; j2 = 1; k2 = 0;
        }
    }

    // Offsets for second corner in (x, y, z) unskewed coords
    double x1 = x0 - i1 + SIMPLEX_NOISE_G2;
    double y1 = y0 - j1 + SIMPLEX_NOISE_G2;
    double z1 = z0 - k1 + SIMPLEX_NOISE_G2;
    // Offsets for third corner in (x, y, z) unskewed coords
    double x2 = x0 - i2 + 2.0 * SIMPLEX_NOISE_G2;
    double y2 = y0 - j2 + 2.0 * SIMPLEX_NOISE_G2;
    double z2 = z0 - k2 + 2.0 * SIMPLEX_NOISE_G2;
    // Offsets for last corner in (x, y, z) unskewed coords
    double x3 = x0 - 1.0 + 3.0 * SIMPLEX_NOISE_G2;
    double y3 = y0 - 1.0 + 3.0 * SIMPLEX_NOISE_G2;
    double z3 = z0 - 1.0 + 3.0 * SIMPLEX_NOISE_G2;

    // Work out the hashed gradient indices of the four simplex corners
    int ii = i & 255;
    int jj = j & 255;
    int kk = k & 255;
    int gi0 = perm[ii + perm[jj + perm[kk]]] % 12;
    int gi1 = perm[ii + i1 + perm[jj + j1 + perm[kk + k1]]] % 12;
    int gi2 = perm[ii + i2 + perm[jj + j2 + perm[kk + k2]]] % 12;
    int gi3 = perm[ii + 1 + perm[jj + 1 + perm[kk + 1]]] % 12;

    // Calculate the contribution from the four corners
    double t0 = 0.6 - x0 * x0 - y0 * y0 - z0 * z0;
    if (t0 < 0) {
        n0 = 0.0;
    } else {
        t0 *= t0;
        n0 = t0 * t0 * simplex_noise_dot3(grad3_3[gi0], x0, y0, z0);
    }

    double t1 = 0.6 - x1 * x1 - y1 * y1 - z1 * z1;
    if (t1 < 0) {
        n1 = 0.0;
    } else {
        t1 *= t1;
        n1 = t1 * t1 * simplex_noise_dot3(grad3_3[gi1], x1, y1, z1);
    }

    double t2 = 0.6 - x2 * x2 - y2 * y2 - z2 * z2;
    if (t2 < 0) {
        n2 = 0.0;
    } else {
        t2 *= t2;
        n2 = t2 * t2 * simplex_noise_dot3(grad3_3[gi2], x2, y2, z2);
    }

    double t3 = 0.6 - x3 * x3 - y3 * y3 - z3 * z3;
    if (t3 < 0) {
        n3 = 0.0;
    } else {
        t3 *= t3;
        n3 = t3 * t3 * simplex_noise_dot3(grad3_3[gi3], x3, y3, z3);
    }

    // Add contributions from each corner to get the final noise value.
    // The result is scaled to return values in the interval [-1,1].
    return 32.0 * (n0 + n1 + n2 + n3);
}

#endif