/**
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_PLATFORM_WIN32_UTILS_H
#define COMS_PLATFORM_WIN32_UTILS_H

#include "../../stdlib/Stdlib.h"
#include "../../utils/StringUtils.h"
#include <windows.h>

/**
 * Converts a key into a unicode codepoint (0 = no character, e.g. dead key or function key)
 *
 * @param keyboard_state Pressed keys (0x80 = down), used for modifiers (shift, alt gr, ...)
 */
uint32 key_to_unicode(byte scan_code, byte vkey, byte keyboard_state[256]) NO_EXCEPT
{
    WCHAR char_buffer[4] = {0};
    const int32 result = ToUnicode(vkey, scan_code, keyboard_state, char_buffer, (int32) ARRAY_COUNT(char_buffer), 0);

    if (result == 1) {
        return (uint32) char_buffer[0];
    }

    // UTF-16 surrogate pair (e.g. emoji keyboard layouts)
    if (result == 2
        && char_buffer[0] >= 0xD800 && char_buffer[0] <= 0xDBFF
        && char_buffer[1] >= 0xDC00 && char_buffer[1] <= 0xDFFF
    ) {
        return 0x10000 + ((((uint32) char_buffer[0]) - 0xD800) << 10) + (((uint32) char_buffer[1]) - 0xDC00);
    }

    // < 0 = dead key (e.g. ^ on german keyboards), it is combined with the next key by ToUnicode
    // > 1 = e.g. dead key that couldn't be combined + character (we only support one code per key)
    return 0;
}

#endif