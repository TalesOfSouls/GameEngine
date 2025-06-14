#ifndef COMS_UI_ALIGNMENT_H
#define COMS_UI_ALIGNMENT_H

#include "../stdlib/Types.h"

enum UIAlign : byte {
    UI_ALIGN_H_LEFT = 1 << 0,
    UI_ALIGN_H_CENTER = 1 << 1,
    UI_ALIGN_H_RIGHT = 1 << 2,

    UI_ALIGN_V_BOTTOM = 1 << 3,
    UI_ALIGN_V_CENTER = 1 << 4,
    UI_ALIGN_V_TOP = 1 << 5,
};

#endif