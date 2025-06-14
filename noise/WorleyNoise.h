/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_NOISE_WORLEY_H
#define COMS_NOISE_WORLEY_H

#include <stdio.h>
#include <stdlib.h>

#include "../stdlib/Types.h"

#define WORLEY_FEATURE_POINTS 4

void generate_feature_points_2d(v2_f32* points, int feature_points, int rows, int cols) {
    for (int i = 0; i < rows * cols * feature_points; ++i) {
        points[i].x = (float) rand() / RAND_MAX;
        points[i].y = (float) rand() / RAND_MAX;
    }
}

void generate_feature_points_3d(v3_f32* points, int feature_points, int rows, int cols, int depth) {
    for (int i = 0; i < rows * cols * depth * feature_points; ++i) {
        points[i].x = (float) rand() / RAND_MAX;
        points[i].y = (float) rand() / RAND_MAX;
        points[i].z = (float) rand() / RAND_MAX;
    }
}

float distance_squared_2d(float x1, float y1, float x2, float y2) {
    return (x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1);
}

float distance_squared_3d(float x1, float y1, float z1, float x2, float y2, float z2) {
    return (x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1) + (z2 - z1) * (z2 - z1);
}

float worley_noise_2d(v2_f32* points, int feature_points, float x, float y, int rows, int cols) {
    int grid_size = rows * cols;

    int cell_x = (int) floor(x);
    int cell_y = (int) floor(y);

    float min_dist_squared = INFINITY;

    // Check the surrounding cells and the current cell
    for (int i = -1; i <= 1; ++i) {
        for (int j = -1; j <= 1; ++j) {
            int neighbor_x = cell_x + i;
            int neighbor_y = cell_y + j;

            // Ensure the neighboring cell is within bounds
            if (neighbor_x < 0 || neighbor_x >= cols
                || neighbor_y < 0 || neighbor_y >= rows
            ) {
                continue;
            }

            for (int p = 0; p < feature_points; p++) {
                v2_f32 fp = points[(neighbor_y * rows + neighbor_x) * cols + p];
                float dist_squared = distance_squared_2d(x, y, fp.x, fp.y);

                if (dist_squared < min_dist_squared) {
                    min_dist_squared = dist_squared;
                }
            }
        }
    }

    return sqrt(min_dist_squared);
}

float worley_noise_3d(v3_f32* points, int feature_points, float x, float y, float z, int rows, int cols, int depth) {
    int grid_size = rows * cols * depth;

    int cell_x = (int) floor(x);
    int cell_y = (int) floor(y);
    int cell_z = (int) floor(z);

    float min_dist_squared = INFINITY;

    // Check the surrounding cells and the current cell
    for (int i = -1; i <= 1; i++) {
        for (int j = -1; j <= 1; j++) {
            for (int k = -1; k <= 1; k++) {
                int neighbor_x = cell_x + i;
                int neighbor_y = cell_y + j;
                int neighbor_z = cell_z + k;

                // Ensure the neighboring cell is within bounds
                if (neighbor_x < 0 || neighbor_x >= cols
                    || neighbor_y < 0 || neighbor_y >= rows
                    || neighbor_z < 0 || neighbor_z >= depth
                ) {
                    continue;
                }

                for (int p = 0; p < feature_points; ++p) {
                    v3_f32 fp = points[((neighbor_y * rows + neighbor_x) * cols + neighbor_z) * depth + p];
                    float dist_squared = distance_squared_3d(x, y, z, fp.x, fp.y, fp.z);

                    if (dist_squared < min_dist_squared) {
                        min_dist_squared = dist_squared;
                    }
                }
            }
        }
    }

    return sqrt(min_dist_squared);
}

#endif