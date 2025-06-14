/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_MODELS_MOB_PLAYER_REPUTATION_H
#define COMS_MODELS_MOB_PLAYER_REPUTATION_H

#include "../../../stdlib/Types.h"

struct Reputation {
    // Character trait
    int8 bravery;
    int8 skill;
    int8 wisdom;
    int8 goodness;
    int8 loyalty;
    int8 gratefulness;
    int8 persistence;
    int8 love;
    int8 creativity;
    int8 patience;
    int8 ambition;
    int8 honesty;
    int8 empathy;
    int8 discretion;
    int8 fairness;
    int8 kindness;
    int8 selflessness;
    int8 confidence;
    int8 curiousity;
    int8 discipline;
    int8 flexibility;
    int8 humility;
    int8 openness;
    int8 reliability;
    int8 lawfulness;

    // Character activity
    int8 thief_threat_level;
    int8 brawler;
    int8 murderer;
    int8 trader;
    int8 ripoff;
    int8 dungeoneer;
    int8 raider;
    int8 pvpler;
};

#endif