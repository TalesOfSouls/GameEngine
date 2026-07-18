#pragma once
#ifndef COMS_UI_ALIGNMENT_H
#define COMS_UI_ALIGNMENT_H

#include "../stdlib/Stdlib.h"

// @performance We could probably remove H_LEFT and V_TOP in the following two enums
// Why? because if _CENTER AND _RIGHT are not set it is automatically _LEFT/_TOP
// If we do this we could even combine align and anchor into one byte

enum UIAlign : byte {
    UI_ALIGN_H_LEFT = 1 << 0,
    UI_ALIGN_H_CENTER = 1 << 1,
    UI_ALIGN_H_RIGHT = 1 << 2,

    UI_ALIGN_V_BOTTOM = 1 << 3,
    UI_ALIGN_V_CENTER = 1 << 4,
    UI_ALIGN_V_TOP = 1 << 5,
};

enum UIAnchor : byte {
    UI_ANCHOR_H_LEFT = 1 << 0,
    UI_ANCHOR_H_CENTER = 1 << 1,
    UI_ANCHOR_H_RIGHT = 1 << 2,

    UI_ANCHOR_V_BOTTOM = 1 << 3,
    UI_ANCHOR_V_CENTER = 1 << 4,
    UI_ANCHOR_V_TOP = 1 << 5,
};

#endif