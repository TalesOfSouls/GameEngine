/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_MODELS_BRACKET_SEEDING_H
#define COMS_MODELS_BRACKET_SEEDING_H

#include "../../stdlib/Types.h"
#include "BracketTeam.h"

struct BracketSeeding {
    BracketTeam* teams;
    uint32* team_seedings; // stores the index of the respective seed, index + 1 = seed
};

#endif