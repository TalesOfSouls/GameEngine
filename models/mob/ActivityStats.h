/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_MODELS_MOB_ACTIVITY_STATS_H
#define COMS_MODELS_MOB_ACTIVITY_STATS_H

#include "../../stdlib/Types.h"

/**
 * @todo optimize order of struct members to ensure optimal struct size
 */

struct SActivityStats {
    int32 farming;
    int32 thiefing;
    int32 mining;
};

#endif