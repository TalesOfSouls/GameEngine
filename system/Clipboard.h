/**
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_SYSTEM_CLIPBOARD_H
#define COMS_SYSTEM_CLIPBOARD_H

// Every platform provides the same functions:
//      int32 clipboard_get(char* text, int32 max_length)   utf8 text incl. \0, returns the length without \0
//      bool clipboard_set(const char* text, int32 length)  utf8 text (doesn't need to be \0 terminated)
#if _WIN32
    #include "../platform/win32/Clipboard.h"
#elif __linux__
    #include "../platform/linux/Clipboard.h"
#endif

#endif
