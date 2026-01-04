/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_ARCHITECTURE_INTRINSICS_H
#define COMS_ARCHITECTURE_INTRINSICS_H

// Adjusts the step size based on the memory alignment
inline
int32 intrin_validate_steps(const byte* mem, int32 steps) NO_EXCEPT
{
    if (steps >= 16 && ((uintptr_t) mem & 63) == 0) {
        return 16;
    } else if (steps >= 8 && ((uintptr_t) mem & 31) == 0) {
        return 8;
    } else if (steps >= 4 && ((uintptr_t) mem & 15) == 0) {
        return 4;
    } else {
        return 1;
    }
}

#ifdef __aarch64__
    #include "arm/Intrinsics.h"
#else
    #include "x86/Intrinsics.h"
#endif

#endif