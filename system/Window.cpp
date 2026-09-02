/**
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_SYSTEM_WINDOW_C
#define COMS_SYSTEM_WINDOW_C

#include "../stdlib/Stdlib.h"
#include "Window.h"

FORCE_INLINE
void window_state_restore(Window* const w) NO_EXCEPT
{
    memcpy(&w->state_current, &w->state_old, sizeof(w->state_current));
}

FORCE_INLINE
uint32 window_logical_to_physical(
    uint32 value,
    uint32 dpi
) NO_EXCEPT
{
    return (value * dpi + 48) / 96;
}

FORCE_INLINE
uint32 window_physical_to_logical(
    uint32 value,
    uint32 dpi
) NO_EXCEPT
{
    return (value * 96 + dpi / 2) / dpi;
}

#if _WIN32
    #include "../platform/win32/Window.cpp"
#endif

#endif