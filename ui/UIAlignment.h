#pragma once
#ifndef COMS_UI_ALIGNMENT_H
#define COMS_UI_ALIGNMENT_H

#include "../stdlib/Stdlib.h"

// Coordinate system of the UI:
//      y-up, origin = bottom left of the screen
//      UIAttributeDimension::pos is the TOP-LEFT corner of an element
//      An element covers x in [pos.x, pos.x + dim.x] and y in [pos.y - dim.y, pos.y]
//
// Anchor = which point of the PARENT the element is positioned relative to
// Align  = which point of the ELEMENT is placed at that (anchor + offset) position
//
// Default (= 0) for both is BOTTOM + LEFT
// That means with the defaults an element sits inside the bottom left corner of its parent
// For an element inside the top left corner use UI_ANCHOR_ALIGN_TOP_LEFT

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

    // If none of the following two is set -> UI_ALIGN_V_BOTTOM
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

    // If none of the following two is set -> UI_ANCHOR_V_BOTTOM
    UI_ANCHOR_V_TOP = 1 << 6,
    UI_ANCHOR_V_CENTER = 1 << 7,
    // Helper to check for !/not the above two
    UI_ANCHOR_V_BOTTOM_NOT = UI_ANCHOR_V_TOP | UI_ANCHOR_V_CENTER,
};

// Common combinations that place the element inside the respective corner/side of its parent
#define UI_ANCHOR_ALIGN_TOP_LEFT (UI_ANCHOR_H_LEFT | UI_ANCHOR_V_TOP | UI_ALIGN_H_LEFT | UI_ALIGN_V_TOP)
#define UI_ANCHOR_ALIGN_TOP_RIGHT (UI_ANCHOR_H_RIGHT | UI_ANCHOR_V_TOP | UI_ALIGN_H_RIGHT | UI_ALIGN_V_TOP)
#define UI_ANCHOR_ALIGN_BOTTOM_LEFT (UI_ANCHOR_H_LEFT | UI_ANCHOR_V_BOTTOM | UI_ALIGN_H_LEFT | UI_ALIGN_V_BOTTOM)
#define UI_ANCHOR_ALIGN_BOTTOM_RIGHT (UI_ANCHOR_H_RIGHT | UI_ANCHOR_V_BOTTOM | UI_ALIGN_H_RIGHT | UI_ALIGN_V_BOTTOM)
#define UI_ANCHOR_ALIGN_CENTER (UI_ANCHOR_H_CENTER | UI_ANCHOR_V_CENTER | UI_ALIGN_H_CENTER | UI_ALIGN_V_CENTER)
#define UI_ANCHOR_ALIGN_LEFT_CENTER (UI_ANCHOR_H_LEFT | UI_ANCHOR_V_CENTER | UI_ALIGN_H_LEFT | UI_ALIGN_V_CENTER)
#define UI_ANCHOR_ALIGN_RIGHT_CENTER (UI_ANCHOR_H_RIGHT | UI_ANCHOR_V_CENTER | UI_ALIGN_H_RIGHT | UI_ALIGN_V_CENTER)

#endif
