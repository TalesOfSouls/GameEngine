/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_MODELS_MOB_PRIMARY_STATS_POINTS_H
#define COMS_MODELS_MOB_PRIMARY_STATS_POINTS_H

#include "../../stdlib/Types.h"

#define PRIMARY_STAT_SIZE 7
static const int PRIMARY_STAT_INDICES[] = {0, 1, 2, 3, 4, 5, 6, 7};

// Character stats modifiable through leveling (simple +/- buttons)
struct PrimaryStatsPoints {
    uint16 stat_str; // strength      : effects health + base damage
    uint16 stat_int; // intelligence   : effects resource + base damage
    uint16 stat_acc; // accuracy      : effects critical chance + base damage + miss chance
    uint16 stat_agi; // agility       : effects resource + base damage + dodge chance
    // @todo not implemented in database
    uint16 stat_def; // defense       : effects resource + base defense + dodge chance
    uint16 stat_sta; // stamina       : effects health regen + resource regen
    uint16 stat_dex; // dexterity     : effects health regen + resource regen
    // @question do we need dex and acc or only one?
};

struct PrimaryStatsValues {
};

struct PrimaryStatsRelValues {
};

#endif