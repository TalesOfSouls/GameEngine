/**
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_MODELS_MOB_SECONDARY_STATS_POINTS_C
#define COMS_MODELS_MOB_SECONDARY_STATS_POINTS_C

#include "../../stdlib/Simd.h"
#include "SecondaryStatsPoints.h"

void calculate_primary_values(const SecondaryStatsPoints* __restrict points, SecondaryStatsValues* __restrict values, int step = 8)
{
    simd_mult((int16 *) points, 1.3f, (int32 *) values, sizeof(SecondaryStatsPoints), step);
}

void calculate_primary_relatives(const SecondaryStatsRelPoints* __restrict points, SecondaryStatsRelValues* __restrict values, int step = 8)
{
    simd_mult((int16 *) points, 0.01f, (int32 *) values, sizeof(SecondaryStatsPoints), step);
}

#endif
