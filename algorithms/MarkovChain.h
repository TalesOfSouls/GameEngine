#pragma once
#ifndef COMS_ALGORITHMS_MARKOV_CHAIN_H
#define COMS_ALGORITHMS_MARKOV_CHAIN_H

/**
 * Example state probability matrix
 */
// static const f32 transition_matrix[STATE_COUNT][STATE_COUNT] = {
//     /*              IDLE   PATROL  CHASE  ATTACK  FLEE  */
//     /* IDLE   */ { 0.90f,  0.10f,   0.00f,  0.00f,   0.00f },
//     /* PATROL */ { 0.05f,  0.90f,   0.05f,  0.00f,   0.00f },
//     /* CHASE  */ { 0.00f,  0.05f,   0.70f,  0.20f,   0.05f },
//     /* ATTACK */ { 0.00f,  0.00f,   0.10f,  0.80f,   0.10f },
//     /* FLEE   */ { 0.00f,  0.10f,   0.00f,  0.00f,   0.90f },
// };

int markov_chain_next(f32* transition_matrix, int state_count, int current, f32 r) {
    f32 cumulative = 0.0f;

    for (int i = 0; i < state_count; ++i) {
        cumulative += transition_matrix[state_count * current + i];
        if (r < cumulative) {
            return i;
        }
    }

    return current;
}

#endif