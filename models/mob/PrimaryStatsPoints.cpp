/**
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_MODELS_MOB_PRIMARY_STATS_POINTS_C
#define COMS_MODELS_MOB_PRIMARY_STATS_POINTS_C

#include "../../stdlib/Simd.h"
#include "PrimaryStatsPoints.h"

void calculate_primary_values(const PrimaryStatsPoints* __restrict points, PrimaryStatsValues* __restrict values, int step = 8)
{
    simd_mult((int16 *) points, 1.3f, (int32 *) values, sizeof(PrimaryStatsPoints), step);
}

void calculate_primary_relatives(const PrimaryStatsPoints* __restrict points, PrimaryStatsRelValues* __restrict values, int step = 8)
{
    simd_mult((int16 *) points, 0.01f, (int32 *) values, sizeof(PrimaryStatsPoints), step);
}

#endif