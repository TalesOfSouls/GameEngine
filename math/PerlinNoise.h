#include <stdlib.h>
#include <stdio.h>
#include "../stdlib/Intrinsics.h"
#include "Animation.h"

double fade(double t) {
    return t * t * t * (t * (t * 6 - 15) + 10);
}

double grad(int hash, double x, double y, double z)
{
    int h = hash & 15;

    double u = h < 8 ? x : y;
    double v = h < 4 ? y : h == 12 || h == 14 ? x : z;

    return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
}

// permutation = 512 elements
double noise(int* permutation, double x, double y, double z)
{
    int X = (int) floor(x) & 255;
    int Y = (int) floor(y) & 255;
    int Z = (int) floor(z) & 255;

    x -= floor(x);
    y -= floor(y);
    z -= floor(z);

    double u = fade(x), v = fade(y), w = fade(z);

    int A = permutation[X] + Y;
    int AA = permutation[A] + Z;
    int AB = permutation[A + 1] + Z;

    int B = permutation[X + 1] + Y;
    int BA = permutation[B] + Z;
    int BB = permutation[B + 1] + Z;

    return lerp_approx(
        w,
        lerp_approx(
            v,
            lerp_approx(u, grad(permutation[AA], x, y, z), grad(permutation[BA], x - 1, y, z)),
            lerp_approx(u, grad(permutation[AB], x, y - 1, z), grad(permutation[BB], x - 1, y - 1, z))
        ),
        lerp_approx(
            v,
            lerp_approx(u, grad(permutation[AA+1], x, y, z - 1), grad(permutation[BA + 1], x - 1, y, z - 1)),
            lerp_approx(u, grad(permutation[AB+1], x, y - 1, z - 1), grad(permutation[BB+1], x - 1, y - 1, z - 1))
        )
    );
}