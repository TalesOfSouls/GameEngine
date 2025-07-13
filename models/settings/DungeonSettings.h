/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_MODELS_SETTINGS_DUNGEON_SETTINGS_H
#define COMS_MODELS_SETTINGS_DUNGEON_SETTINGS_H

#include "../../stdlib/Types.h"
#include "ItemDistributionType.h"

struct DungeonSettings {
    ItemDistributionType item_distribution;
    ItemDistributionType gold_distribution;
};

#endif