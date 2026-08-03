/**
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_MODELS_BRACKET_MATCH_H
#define COMS_MODELS_BRACKET_MATCH_H

#include "BracketTeam.h"

struct BracketMatch {
    BracketTeam* teams[2];
    BracketTeam* winner;
    BracketTeam* loser;
};

#endif