/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_PLATFORM_WIN32_INPUT_POLLING_H
#define COMS_PLATFORM_WIN32_INPUT_POLLING_H

#include <windows.h>

#include "../../../stdlib/Stdlib.h"
#include "../../../input/Input.h"
#include "Keys.h"

FORCE_INLINE
void input_mouse_position(HWND hwnd, v2_int32* pos) NO_EXCEPT
{
    POINT p;
    if (GetCursorPos(&p) && ScreenToClient(hwnd, &p)) {
        pos->x = p.x;
        pos->y = p.y;
    }
}

// Mouse and Keyboard only work on the first element in the states
// @todo Instead of using states[0] as default kbm state, use settings for this (settings->input_device_types)
inline
int16 input_poll_handle(
    Input* const __restrict states,
    uint64 time
) {
    int16 input_count = 0;
    for (int vk = 0; vk < 256; ++vk) {
        const uint16 scan_code = (uint16) vk_to_scan_code(vk);
        const bool is_down = (GetAsyncKeyState(vk) & 0x8000) != 0;
        const bool was_down = input_is_down(states[0].state.active_keys, scan_code);

        if ((!is_down && !was_down) || (is_down && was_down)) {
            // No input change
            continue;
        }

        InputKey key = {
            scan_code,
            (uint16) vk,
            KEY_PRESS_TYPE_NONE, false, 0, time
        };

        // holding a button down is automatically handled in the input handling
        if (is_down && !was_down) {
            // new down
            key.key_state = KEY_PRESS_TYPE_PRESSED;
        } else {
            // new release
            key.key_state = KEY_PRESS_TYPE_RELEASED;
        }

        if (vk <= 0x7) {
            key.scan_code |= INPUT_MOUSE_PREFIX;
            key.virtual_code |= INPUT_MOUSE_PREFIX;
        } else {
            key.scan_code |= INPUT_KEYBOARD_PREFIX;
            key.virtual_code |= INPUT_KEYBOARD_PREFIX;
        }

        key.time = time;
        ++input_count;

        input_set_state(states[0].state.active_keys, &key);
        states[0].general_states |= INPUT_STATE_GENERAL_BUTTON_CHANGE;
    }

    return input_count;
}

#endif