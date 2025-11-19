/**
 * Jingga
 *
 * @conyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_MODELS_SAMPLING_POISSON_DISK_H
#define COMS_MODELS_SAMPLING_POISSON_DISK_H

#include "../../../stdlib/Types.h"
#include "../../../memory/RingMemory.h"

// Calculates the amount of max points to generate
size_t poisson_disk_bridson_cap(f64 width, f64 height, f64 r)
{
    const f64 area = width * height;
    const f64 approx_num = area / (OMS_PI_F32 * r * r);

    return (size_t) (approx_num * 1.6 + 16.0);
}

/*
* poisson_disk_bridson
* - width, height : sampling rectangle size [0,width) x [0,height)
* - r : minimum distance between points
* - k : number of attempts per active sample (typical 30)
* - returns number of samples generated
*
* Implementation notes for performance:
* - grid cell size = r / intrin_sqrt_f64(2). Each occupied cell has at most one sample.
* - grid stored as int32 array of indices into points array; -1 means empty.
* - active list stored as int32 array used as stack with random removal implementation (swap-with-last)
*/
size_t poisson_disk_bridson(
    f64 width, f64 height, f64 r, int32 k,
    int32 points_size, v2_f64* __restrict out_points,
    RingMemory* __restrict ring
) {
    const f64 cell_size = r * OMS_INV_SQRT_2_F64;
    const int32 grid_w = (int32) OMS_CEIL_32(width / cell_size);
    const int32 grid_h = (int32) OMS_CEIL_32(height / cell_size);
    const size_t grid_size = (size_t) grid_w * (size_t) grid_h;

    int32* grid = (int32*) ring_get_memory(ring, grid_size * sizeof(int32), sizeof(size_t));
    for (size_t i = 0; i < grid_size; ++i) {
        grid[i] = -1;
    }

    size_t cap = points_size; //poisson_disk_bridson_points(width, height, r);

    v2_f64* points = (v2_f64 *) ring_get_memory(ring, cap * sizeof(v2_f64));
    int32* active = (int32 *) ring_get_memory(ring, cap * sizeof(int32));
    memset(active, 0, cap * sizeof(int32));

    size_t points_count = 0;
    size_t active_count = 0;

    while (active_count > 0) {
        // choose a random active index: uniform in [0, active_count)
        size_t rand_pos = (size_t) (rand_uniform01() * (f64) active_count);

        if (rand_pos >= active_count) {
            rand_pos = active_count - 1;
        }

        int32 active_idx = active[rand_pos];
        f64 ax = points[active_idx].x;
        f64 ay = points[active_idx].y;

        int32 found = 0;
        for (int32 attempt = 0; attempt < k; ++attempt) {
            /**
            * sample radius uniformly in [r, 2r) with area correction: use intrin_sqrt_f64(u)
            * sample angle uniformly in [0, 2pi)
            */
            f64 u = rand_uniform01();

            // in [r, 2r) biased appropriately
            // @performance we could pre-compute sqrt(u) in batches of 4/8
            // because we need this multiple times anyways
            f64 radius = r + r * intrin_sqrt_f64(u);

            f64 theta = OMS_TWO_PI_F32 * rand_uniform01();

            f64 s, c;
            SINCOS(theta, s, c);

            f64 nx = ax + radius * c;
            f64 ny = ay + radius * s;

            // reject if outside domain
            if (nx < 0.0 || nx >= width || ny < 0.0 || ny >= height)
                continue;

            /**
             * check neighbors fast via grid
             * compute candidate's cell
             */
            int32 cgx = (int32) (nx / cell_size);
            int32 cgy = (int32) (ny / cell_size);

            if (cgx < 0) {
                cgx = 0;
            }

            if (cgx >= grid_w) {
                cgx = grid_w - 1;
            }

            if (cgy < 0) {
                cgy = 0;
            }

            if (cgy >= grid_h) {
                cgy = grid_h - 1;
            }

            /* Check nearby cells for conflicts; uses macro that jumps to REJECTED label on violation */
            int32 min_gx = (int32) FLOORF(((nx) - r) / cell_size);
            int32 max_gx = (int32) FLOORF(((nx) + r) / cell_size);
            int32 min_gy = (int32) FLOORF(((ny) - r) / cell_size);
            int32 max_gy = (int32) FLOORF(((ny) + r) / cell_size);

            if (min_gx < 0) {
                min_gx = 0;
            }

            if (max_gx >= grid_w) {
                max_gx = grid_w - 1;
            }

            if (min_gy < 0) {
                min_gy = 0;
            }

            if (max_gy >= grid_h) {
                max_gy = grid_h - 1;
            }

            int32 too_close = 0;
            for (int32 gj = min_gy; gj <= max_gy && !too_close; ++gj) {
                size_t base = (size_t)gj * grid_w;
                for (int32 gi = min_gx; gi <= max_gx; ++gi) {
                    int32 idx = grid[base + gi];
                    if (idx != -1) {
                        f64 dx = points[idx].x - nx;
                        f64 dy = points[idx].y - ny;
                        if ((dx * dx + dy*dy) < (r * r)) {
                            too_close = 1;
                            break;
                        }
                    }
                }
            }

            if (too_close) {
                continue;
            }

            if (points_count >= cap) {
                // We don't allow more points than defined by the user
                return points_count;

                // No conflict — accept point
                /*
                // @todo: Ideally we don't want this to happen
                ASSERT_TRUE(points_count < cap);

                size_t newcap = cap * 2;

                active = (int32 *) ring_grow_memory(ring, active, newcap * sizeof(int32));
                points = (v2_f64 *) ring_grow_memory(ring, points, newcap * sizeof(v2_f64));
                memset(active + cap, 0, (newcap - cap) * sizeof(int32));

                cap = newcap;

                // grid stays same
                */
            }

            points[points_count].x = nx;
            points[points_count].y = ny;
            size_t cidx = (size_t)cgy * grid_w + cgx;
            grid[cidx] = (int32) points_count;
            active[active_count++] = (int32) points_count;
            ++points_count;
            found = 1;

            break;
        }

        if (!found) {
            // remove active_idx from active list by swapping with last element
            active[rand_pos] = active[active_count - 1];
            --active_count;
        }
    }

    return points_count;
}

// This function returns samples based on an importance function
// The importance function could even use a input image / shadow map for this
// This would allow to generate more sample points based on focus points
size_t poisson_disk_bridson_importance(
    f64 width, f64 height, f64 r, int32 k,
    int32 points_size, v2_f64* __restrict out_points,
    RingMemory* __restrict ring,
    ImportanceFn importance, void *userdata,
    f64 max_importance
) {
    /* sanity clamp for provided bounds */
    if (max_importance < 1e-6) max_importance = 1.0;

    /* cell size uses the *smallest* local radius = r / intrin_sqrt_f64(max_importance) */
    const f64 min_local_r = r / intrin_sqrt_f64(max_importance);
    const f64 cell_size = min_local_r * OMS_INV_SQRT_2_F64;
    const int32 grid_w = (int32) OMS_CEIL_32(width / cell_size);
    const int32 grid_h = (int32) OMS_CEIL_32(height / cell_size);
    const size_t grid_size = (size_t) grid_w * (size_t) grid_h;

    int32* grid = (int32*) ring_get_memory(ring, grid_size * sizeof(int32), sizeof(size_t));
    if (!grid) return 0;
    for (size_t i = 0; i < grid_size; ++i) grid[i] = -1;

    size_t cap = (size_t) points_size;

    v2_f64* points = out_points; /* already provided by caller */
    int32* active = (int32*) ring_get_memory(ring, cap * sizeof(int32), sizeof(int32));
    if (!active) return 0;
    memset(active, 0, cap * sizeof(int32));

    size_t points_count = 0;
    size_t active_count = 0;

    /* small clamp to avoid divide by zero or insanely large local radii */
    const f64 min_importance_clamp = 1e-6;

    /* --- create first sample uniformly --- */
    {
        f64 x0 = rand_uniform01() * width;
        f64 y0 = rand_uniform01() * height;

        /* accept immediately (no neighbor conflict yet) */
        points[points_count].x = x0;
        points[points_count].y = y0;

        int32 gx = (int32) (x0 / cell_size);
        int32 gy = (int32) (y0 / cell_size);
        if (gx < 0) gx = 0; if (gx >= grid_w) gx = grid_w - 1;
        if (gy < 0) gy = 0; if (gy >= grid_h) gy = grid_h - 1;
        size_t gidx = (size_t)gy * grid_w + gx;
        grid[gidx] = (int32) points_count;

        active[active_count++] = (int32) points_count;
        ++points_count;
    }

    while (active_count > 0) {
        size_t rand_pos = (size_t) (rand_uniform01() * (f64) active_count);
        if (rand_pos >= active_count) rand_pos = active_count - 1;

        int32 active_idx = active[rand_pos];
        f64 ax = points[active_idx].x;
        f64 ay = points[active_idx].y;

        int32 found = 0;
        for (int32 attempt = 0; attempt < k; ++attempt) {
            /* sample radius and angle (area-corrected) */
            f64 u = rand_uniform01();
            f64 radius = r + r * intrin_sqrt_f64(u); /* in [r, 2r) like original */
            f64 theta = OMS_TWO_PI_F32 * rand_uniform01();
            f64 s, c; SINCOS(theta, s, c);
            f64 nx = ax + radius * c;
            f64 ny = ay + radius * s;

            if (nx < 0.0 || nx >= width || ny < 0.0 || ny >= height) {
                continue;
            }

            /* clamp candidate importance */
            f64 w_cand = importance ? importance(nx, ny, userdata) : 1.0;
            if (w_cand < min_importance_clamp) w_cand = min_importance_clamp;
            if (w_cand > max_importance) w_cand = max_importance;

            /* candidate's local radius used as guide (conservative mapping) */
            f64 local_r_cand = r / intrin_sqrt_f64(w_cand);

            /* find candidate's cell (clamp) */
            int32 cgx = (int32) (nx / cell_size);
            int32 cgy = (int32) (ny / cell_size);
            if (cgx < 0) cgx = 0;
            if (cgx >= grid_w) cgx = grid_w - 1;
            if (cgy < 0) cgy = 0;
            if (cgy >= grid_h) cgy = grid_h - 1;

            /* compute neighborhood window in cells to check.
               conservative radius: use candidate's local radius rounded up to cells,
               but also ensure at least one-cell radius to catch neighbors that might have
               larger local radii. Because grid cell_size is derived from the smallest
               local radius (densest region) this remains safe and reasonably small. */
            int32 cell_radius = (int32) OMS_CEIL_32(local_r_cand / cell_size);
            if (cell_radius < 1) cell_radius = 1;
            /* also clamp to a small max to avoid huge sweeps; in practice importance maps should be bounded */
            const int32 MAX_CELL_RADIUS = 8;
            if (cell_radius > MAX_CELL_RADIUS) cell_radius = MAX_CELL_RADIUS;

            int32 min_gx = cgx - cell_radius;
            int32 max_gx = cgx + cell_radius;
            int32 min_gy = cgy - cell_radius;
            int32 max_gy = cgy + cell_radius;

            if (min_gx < 0) min_gx = 0;
            if (min_gy < 0) min_gy = 0;
            if (max_gx >= grid_w) max_gx = grid_w - 1;
            if (max_gy >= grid_h) max_gy = grid_h - 1;

            // neighbor check
            int32 too_close = 0;
            for (int32 gj = min_gy; gj <= max_gy && !too_close; ++gj) {
                size_t base = (size_t) gj * grid_w;
                for (int32 gi = min_gx; gi <= max_gx; ++gi) {
                    int32 idx = grid[base + gi];
                    if (idx != -1) {
                        /* neighbor position */
                        f64 dx = points[idx].x - nx;
                        f64 dy = points[idx].y - ny;
                        /* neighbor importance and local radius (clamped) */
                        f64 w_nei = importance ? importance(points[idx].x, points[idx].y, userdata) : 1.0;
                        if (w_nei < min_importance_clamp) w_nei = min_importance_clamp;
                        if (w_nei > max_importance) w_nei = max_importance;
                        f64 local_r_nei = r / intrin_sqrt_f64(w_nei);

                        /* conservative combined distance: use max(local_r_cand, local_r_nei) */
                        f64 forbid_r = (local_r_cand > local_r_nei) ? local_r_cand : local_r_nei;
                        if ((dx * dx + dy * dy) < (forbid_r * forbid_r)) {
                            too_close = 1;
                            break;
                        }
                    }
                }
            }

            if (too_close) {
                continue;
            }

            // if buffer full, return early (user requested fixed capacity)
            if (points_count >= cap) {
                return points_count;
            }

            // accept candidate
            points[points_count].x = nx;
            points[points_count].y = ny;
            size_t cidx = (size_t) cgy * grid_w + cgx;
            grid[cidx] = (int32) points_count;
            active[active_count++] = (int32) points_count;
            ++points_count;
            found = 1;
            break;
        }

        if (!found) {
            // remove active_idx from active list by swapping with last element
            active[rand_pos] = active[active_count - 1];
            --active_count;
        }
    }

    return points_count;
}

#endif