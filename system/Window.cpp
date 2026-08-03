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

#if _WIN32
    #include "../platform/win32/Window.cpp"
#endif

#endif