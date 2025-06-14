/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_MODELS_SETTINGS_ITEM_DISTRIBUTION_TYPE_H
#define COMS_MODELS_SETTINGS_ITEM_DISTRIBUTION_TYPE_H

enum ItemDistributionType {
    ITEM_DISTRIBUTION_TYPE_DEFAULT, // Mix of individual and shared drops
    ITEM_DISTRIBUTION_TYPE_INDIVIDUAL, // All loot is individual BUT not class specific
    ITEM_DISTRIBUTION_TYPE_LEADER, // Leader receives all loot
    ITEM_DISTRIBUTION_TYPE_GUILD_BANK, // Items go into guild bank
};

#endif
