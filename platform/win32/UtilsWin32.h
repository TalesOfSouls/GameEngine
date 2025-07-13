/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_PLATFORM_WIN32_UTILS_H
#define COMS_PLATFORM_WIN32_UTILS_H

#include "../../stdlib/Types.h"
#include "../../utils/StringUtils.h"
#include <windows.h>

#define strtok_r strtok_s

uint32 key_to_unicode(byte scan_code, byte vkey, byte keyboard_state[256]) noexcept
{
    WCHAR char_buffer[5] = {};
    int32 result = ToUnicode(vkey, scan_code, keyboard_state, char_buffer, 5, 0);

    if (result == 1) {
        return (uint32) char_buffer[0];
    } else if (result == 2) {
        return (uint32) *((uint16 *) char_buffer);
    } else if (result == 4) {
        return *((uint32 *) char_buffer);
    } else {
        return 0;
    }
}

#endif