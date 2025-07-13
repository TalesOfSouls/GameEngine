/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_PLATFORM_WIN32_CLIPBOARD_H
#define COMS_PLATFORM_WIN32_CLIPBOARD_H

#include "../../stdlib/Types.h"
#include "../../utils/StringUtils.h"
#include <windows.h>
#include <string.h>

#define strtok_r strtok_s

void clipboard_get(char* text, int32 max_length)
{
    *text = '\0';
    if (OpenClipboard(NULL)) {
        HANDLE clipboard_data = GetClipboardData(CF_TEXT);
        if (clipboard_data) {
            const char* clipboard_text = (const char*) GlobalLock(clipboard_data);
            if (clipboard_text) {
                str_copy_short(text, clipboard_text, max_length);
                GlobalUnlock(clipboard_data);
            }
        }

        CloseClipboard();
    }
}

#endif