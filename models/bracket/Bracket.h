/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_MODELS_BRACKET_SIMPLE_BRACKET_H
#define COMS_MODELS_BRACKET_SIMPLE_BRACKET_H

#include "BracketTeam.h"
#include "BracketSeeding.h"
#include "BracketMatch.h"

struct SimpleBracket {
    int32 size;
    BracketMatch* matches;
    BracketSeeding seeding;
};

struct DoubleEliminationBracket {
    SimpleBracket winners_bracket;
    SimpleBracket losers_bracket;
};

void bracket_single_elim_create(SimpleBracket* bracket)
{
    for (int32 i = 0; i < bracket->size / 2; ++i) {
        bracket->matches[i].teams[0] = &bracket->seeding.teams[bracket->seeding.team_seedings[i]];
        bracket->matches[i].teams[1] = &bracket->seeding.teams[bracket->seeding.team_seedings[bracket->size - i - 1]];
    }

    int32 j = 0;
    for (int32 i = bracket->size / 2; bracket->size - 1; ++i) {
        bracket->matches[i].teams[0] =  bracket->matches[j].winner;
        bracket->matches[i].teams[0] =  bracket->matches[++j].winner;
        ++j;
    }
}

void bracket_double_elim_create(DoubleEliminationBracket* bracket)
{
    bracket_single_elim_create(&bracket->winners_bracket);

    int32 match_index = 0;
    for (int32 i = 0; i < bracket->winners_bracket.size - 1; ++i) {
        if (i % 2 == 0) {
            bracket->losers_bracket.matches[match_index].teams[0] = bracket->winners_bracket.matches[i].loser;
            bracket->losers_bracket.matches[match_index].teams[1] = bracket->winners_bracket.matches[i + 1].loser;
            ++match_index;
        } else {
            bracket->losers_bracket.matches[match_index].teams[0] = bracket->losers_bracket.matches[match_index - 1].winner;
            bracket->losers_bracket.matches[match_index].teams[1] = bracket->winners_bracket.matches[i + 1].loser;
            ++match_index;
        }
    }

    int32 j = 0;
    for (int32 i = match_index; i < bracket->losers_bracket.size - 1; ++i) {
        bracket->losers_bracket.matches[i].teams[0] = bracket->losers_bracket.matches[j].winner;
        bracket->losers_bracket.matches[i].teams[1] = bracket->losers_bracket.matches[++j].winner;
        ++j;
    }
}

#endif