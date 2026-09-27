#pragma once
#ifndef COMS_UI_ALIGNMENT_H
#define COMS_UI_ALIGNMENT_H

#include "../stdlib/Stdlib.h"

// Default alignment is BOTTOM + LEFT
#define UI_ALIGN_DEFAULT 0
#define UI_ANCHOR_DEFAULT 0

#define UI_ALIGN_V_BOTTOM 0
#define UI_ALIGN_H_LEFT 0
#define UI_ANCHOR_H_LEFT 0
#define UI_ANCHOR_V_BOTTOM 0

enum UIAlignAnchor : byte {
    // Align
    // If none of the following two is set -> UI_ALIGN_H_LEFT
    UI_ALIGN_H_CENTER = 1 << 0,
    UI_ALIGN_H_RIGHT = 1 << 1,
    // Helper to check for !/not the above two
    UI_ALIGN_H_LEFT_NOT = UI_ALIGN_H_CENTER | UI_ALIGN_H_RIGHT,

    // If none of the following two is set -> UI_ALIGN_V_TOP
    UI_ALIGN_V_TOP = 1 << 2,
    UI_ALIGN_V_CENTER = 1 << 3,
    // Helper to check for !/not the above two
    UI_ALIGN_V_BOTTOM_NOT = UI_ALIGN_V_TOP | UI_ALIGN_V_CENTER,

    // Anchor
    // If none of the following two is set -> UI_ANCHOR_H_LEFT
    UI_ANCHOR_H_CENTER = 1 << 4,
    UI_ANCHOR_H_RIGHT = 1 << 5,
    // Helper to check for !/not the above two
    UI_ANCHOR_H_LEFT_NOT = UI_ANCHOR_H_CENTER | UI_ANCHOR_H_RIGHT,

    // If none of the following two is set -> UI_ANCHOR_V_TOP
    UI_ANCHOR_V_TOP = 1 << 6,
    UI_ANCHOR_V_CENTER = 1 << 7,
    // Helper to check for !/not the above two
    UI_ANCHOR_V_BOTTOM_NOT = UI_ANCHOR_V_TOP | UI_ANCHOR_V_CENTER,
};

#endif