/**
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_PLATFORM_WIN32_CLIPBOARD_H
#define COMS_PLATFORM_WIN32_CLIPBOARD_H

#include "../../stdlib/Stdlib.h"
#include "../../utils/StringUtils.h"
#include <windows.h>

#ifdef NOVIRTUALKEYCODES
    #error "NOVIRTUALKEYCODES Must not be defined"
#endif

#ifdef NOCLIPBOARD
    #error "NOCLIPBOARD Must not be defined"
#endif

#ifdef NONLS
    #error "NONLS Must not be defined"
#endif

#ifdef NOMEMMGR
    #error "NOMEMMGR Must not be defined"
#endif

/**
 * Reads the clipboard text as utf8
 *
 * The text is cut if it doesn't fit (never in the middle of an utf8 character)
 *
 * @param max_length Size of text in bytes incl. \0
 *
 * @return Length of the text in bytes (without \0)
 */
inline
int32 clipboard_get(char* const text, int32 max_length) NO_EXCEPT
{
    if (max_length <= 0) {
        return 0;
    }

    *text = '\0';
    if (!OpenClipboard(NULL)) {
        return 0;
    }

    int32 length = 0;

    // CF_UNICODETEXT instead of CF_TEXT, CF_TEXT uses the ANSI code page and not utf8
    HANDLE clipboard_data = GetClipboardData(CF_UNICODETEXT);
    if (clipboard_data) {
        const wchar_t* const clipboard_text = (const wchar_t *) GlobalLock(clipboard_data);
        if (clipboard_text) {
            length = WideCharToMultiByte(CP_UTF8, 0, clipboard_text, -1, text, max_length, NULL, NULL);

            if (length <= 0) {
                // Doesn't fit -> convert as much as possible
                // Character by character to never cut an utf8 character in half
                length = 0;
                for (const wchar_t* c = clipboard_text; *c; ++c) {
                    // Surrogate pairs need to be converted together
                    const int32 units = (*c >= 0xD800 && *c <= 0xDBFF && c[1]) ? 2 : 1;

                    char encoded[8];
                    const int32 bytes = WideCharToMultiByte(CP_UTF8, 0, c, units, encoded, (int32) sizeof(encoded), NULL, NULL);
                    if (bytes <= 0 || length + bytes >= max_length) {
                        break;
                    }

                    memcpy(text + length, encoded, bytes);
                    length += bytes;
                    c += units - 1;
                }
            } else {
                // The returned length includes the \0
                --length;
            }

            text[length] = '\0';
            GlobalUnlock(clipboard_data);
        }
    }

    CloseClipboard();

    return length;
}

/**
 * Sets the clipboard text
 *
 * @param text utf8 text (doesn't need to be \0 terminated)
 * @param length Length in bytes
 *
 * @return true on success
 */
inline
bool clipboard_set(const char* const text, int32 length) NO_EXCEPT
{
    if (length < 0) {
        return false;
    }

    const int32 wide_length = length
        ? MultiByteToWideChar(CP_UTF8, 0, text, length, NULL, 0)
        : 0;

    if (length && wide_length <= 0) {
        return false;
    }

    HGLOBAL memory = GlobalAlloc(GMEM_MOVEABLE, (wide_length + 1) * sizeof(wchar_t));
    if (!memory) {
        return false;
    }

    wchar_t* const wide_text = (wchar_t *) GlobalLock(memory);
    if (!wide_text) {
        GlobalFree(memory);

        return false;
    }

    if (wide_length) {
        MultiByteToWideChar(CP_UTF8, 0, text, length, wide_text, wide_length);
    }

    wide_text[wide_length] = L'\0';
    GlobalUnlock(memory);

    if (!OpenClipboard(NULL)) {
        GlobalFree(memory);

        return false;
    }

    EmptyClipboard();

    // On success the clipboard owns the memory
    const bool success = SetClipboardData(CF_UNICODETEXT, memory) != NULL;
    CloseClipboard();

    if (!success) {
        GlobalFree(memory);
    }

    return success;
}

#endif
