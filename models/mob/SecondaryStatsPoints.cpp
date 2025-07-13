/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_MODELS_MOB_SECONDARY_STATS_POINTS_C
#define COMS_MODELS_MOB_SECONDARY_STATS_POINTS_C

#include "../../stdlib/Simd.h"
#include "SecondaryStatsPoints.h"

void calculate_primary_values(const SecondaryStatsPoints* points, SecondaryStatsValues* values, int step = 8)
{
    simd_mult((int16 *) points, 1.3f, (int32 *) values, sizeof(SecondaryStatsPoints), step);
}

void calculate_primary_relatives(const SecondaryStatsRelPoints* points, SecondaryStatsRelValues* values, int step = 8)
{
    simd_mult((int16 *) points, 0.01f, (int32 *) values, sizeof(SecondaryStatsPoints), step);
}

#endif
