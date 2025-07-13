/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_MODELS_BRACKET_TEAM_H
#define COMS_MODELS_BRACKET_TEAM_H

#include "../../stdlib/Types.h"

struct BracketTeam {
    int32 size;
    uint64* player_ids;
    uint32 rating;
};

#endif