/**
 * Jingga
 *
 * @package   Utils
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_NOISE_VALUE_H
#define COMS_NOISE_VALUE_H

#include <stdio.h>
#include <stdlib.h>

#include "../animation/Animation.h"

// Initialize the grid with random values
void initialize_value_noise_2d(float* grid, int grid_size) {
    for (int i = 0; i < grid_size * grid_size; ++i) {
        grid[i] = (float) rand() / (float) RAND_MAX;
    }
}

void initialize_value_noise_3d(float* grid, int rows, int cols, int depth) {
    for (int i = 0; i < rows * cols * depth; ++i) {
        grid[i] = (float) rand() / (float) RAND_MAX;
    }
}

// @todo whenever we actually need this and we want to iterate over all x,y,z of a grid we will NOT iterate value_noise_2d.
//      this would be way to slow!!!
//      Instead we need to implement a simd version that performs the algorithm in value_noise_2d with a step size of 16/8/4

float value_noise_2d(float* grid, float x, float y, int rows, int cols) {
    int grid_size = rows * cols;

    // Calculate grid cell coordinates
    int x0 = (int) floor(x) % grid_size;
    int y0 = (int) floor(y) % grid_size;
    int x1 = (x0 + 1) % grid_size;
    int y1 = (y0 + 1) % grid_size;

    // Calculate interpolation weights
    float tx = x - floor(x);
    float ty = y - floor(y);

    // Smooth the weights using smoothstep function
    tx = smoothstep(tx);
    ty = smoothstep(ty);

    // Interpolate the four corner values
    float v00 = grid[y0 * rows + x0];
    float v10 = grid[y0 * rows + x1];
    float v01 = grid[y1 * rows + x0];
    float v11 = grid[y1 * rows + x1];

    // Interpolate along x direction
    float v0 = lerp(v00, v10, tx);
    float v1 = lerp(v01, v11, tx);

    // Interpolate along y direction and return the final noise value
    return lerp(v0, v1, ty);
}

float value_noise_3d(float* grid, float x, float y, float z, int rows, int cols, int depth) {
    int grid_size = rows * cols * depth;

    // Calculate grid cell coordinates
    int x0 = (int) floor(x) % grid_size;
    int y0 = (int) floor(y) % grid_size;
    int z0 = (int) floor(z) % grid_size;
    int x1 = (x0 + 1) % grid_size;
    int y1 = (y0 + 1) % grid_size;
    int z1 = (z0 + 1) % grid_size;

    // Calculate interpolation weights
    float tx = x - floor(x);
    float ty = y - floor(y);
    float tz = z - floor(z);

    // Smooth the weights using smoothstep function
    tx = smoothstep(tx);
    ty = smoothstep(ty);
    tz = smoothstep(tz);

    // Interpolate the eight corner values
    float v000 = grid[(y0 * rows + x0) * cols + z0];
    float v100 = grid[(y0 * rows + x1) * cols + z0];
    float v010 = grid[(y1 * rows + x0) * cols + z0];
    float v110 = grid[(y1 * rows + x1) * cols + z0];
    float v001 = grid[(y0 * rows + x0) * cols + z1];
    float v101 = grid[(y0 * rows + x1) * cols + z1];
    float v011 = grid[(y1 * rows + x0) * cols + z1];
    float v111 = grid[(y1 * rows + x1) * cols + z1];

    // Interpolate along x direction
    float v00 = lerp(v000, v100, tx);
    float v10 = lerp(v010, v110, tx);
    float v01 = lerp(v001, v101, tx);
    float v11 = lerp(v011, v111, tx);

    // Interpolate along y direction
    float v0 = lerp(v00, v10, ty);
    float v1 = lerp(v01, v11, ty);

    // Interpolate along z direction and return the final noise value
    return lerp(v0, v1, tz);
}

#endif