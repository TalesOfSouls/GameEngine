/**
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_PLATFORM_WIN32_INPUT_POLLING_H
#define COMS_PLATFORM_WIN32_INPUT_POLLING_H

#include <windows.h>

#include "../../../stdlib/Stdlib.h"
#include "../../../input/Input.cpp"
#include "../Window.h"
#include "Keys.h"

/**
 * Finds the system mouse sensitivity
 */
inline
f32 input_mouse_sensitivity() NO_EXCEPT
{
    int mouse_speed = 10;
    SystemParametersInfo(SPI_GETMOUSESPEED, 0, &mouse_speed, 0);
    return (f32) mouse_speed / 10.0f;
}

inline
void input_mouse_position(HWND hwnd, v2_int32* pos) NO_EXCEPT
{
    POINT p;
    if (GetCursorPos(&p) && ScreenToClient(hwnd, &p)) {
        pos->x = p.x;
        pos->y = p.y;
    }
}

inline
v2_int32 input_mouse_position(HWND hwnd) NO_EXCEPT
{
    POINT p;
    if (GetCursorPos(&p) && ScreenToClient(hwnd, &p)) {
        return {p.x, p.y};
    }

    return {0, 0};
}

inline
void input_mouse_update(HWND hwnd, Input* input_states) NO_EXCEPT
{
    v2_int32 mouse_pos = input_mouse_position(hwnd);
    Input* const input = input_kbm_find(input_states, INPUT_DEVICE_COUNT);

    input->state.x[0] = (int16) mouse_pos.x;
    input->state.y[0] = (int16) mouse_pos.y;
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
        states[0].general_states |= INPUT_STATE_GENERAL_INPUT_CHANGE;
    }

    return input_count;
}

#endif