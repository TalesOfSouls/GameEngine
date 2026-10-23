/**
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_PLATFORM_LINUX_CLIPBOARD_H
#define COMS_PLATFORM_LINUX_CLIPBOARD_H

#include "../../stdlib/Stdlib.h"

// @todo Implement with X11/Wayland (@see GuiUtils.h for an older X11 draft)
//      Same function layout as platform/win32/Clipboard.h

inline
int32 clipboard_get(char* const text, int32 max_length) NO_EXCEPT
{
    if (max_length > 0) {
        *text = '\0';
    }

    return 0;
}

inline
bool clipboard_set(const char* const, int32) NO_EXCEPT
{
    return false;
}

#endif
