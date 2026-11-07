/**
 * Display names of input keys (keyboard, mouse, controller) e.g. for hotkey settings
 *
 * The names are English (US keyboard layout), the keyboard keys are scan codes (= position on the keyboard)
 * WARNING: Keys with an E0 prefix share the scan code with the numpad keys (e.g. up arrow = numpad 8)
 *          -> such keys have a combined name (e.g. "Up / Num 8")
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_INPUT_KEY_NAMES_H
#define COMS_INPUT_KEY_NAMES_H

#include "../stdlib/Stdlib.h"
#include "../utils/StringUtils.h"
#include "Input.h"
#include "ControllerInput.h"

// Index = scan code (without INPUT_KEYBOARD_PREFIX), NULL = unknown key
// @bug This should not be hard coded, there are different languages!!!!
static const char* const _input_keyboard_names[0x59] = {
    NULL, "Esc", "1", "2", "3", "4", "5", "6",                                      // 0x00
    "7", "8", "9", "0", "-", "=", "Backspace", "Tab",                               // 0x08
    "Q", "W", "E", "R", "T", "Y", "U", "I",                                         // 0x10
    "O", "P", "[", "]", "Enter", "Ctrl", "A", "S",                                  // 0x18
    "D", "F", "G", "H", "J", "K", "L", ";",                                         // 0x20
    "'", "`", "Left Shift", "\\", "Z", "X", "C", "V",                               // 0x28
    "B", "N", "M", ",", ".", "/", "Right Shift", "Num *",                           // 0x30
    "Alt", "Space", "Caps Lock", "F1", "F2", "F3", "F4", "F5",                      // 0x38
    "F6", "F7", "F8", "F9", "F10", "Num Lock", "Scroll Lock", "Home / Num 7",       // 0x40
    "Up / Num 8", "Page Up / Num 9", "Num -", "Left / Num 4", "Num 5", "Right / Num 6", "Num +", "End / Num 1", // 0x48
    "Down / Num 2", "Page Down / Num 3", "Insert / Num 0", "Delete / Num .", NULL, NULL, NULL, "F11", // 0x50
    "F12",                                                                          // 0x58
};

// Index = InputMouseAction, NULL = unknown
static const char* const _input_mouse_names[] = {
    NULL,
    "Left Mouse",
    "Right Mouse",
    "Middle Mouse",
    "Mouse 4",
    "Mouse 5",
    "Mouse Wheel",
    "Mouse Wheel Horizontal",
    "Mouse Wheel Up",
    "Mouse Wheel Down",
    "Mouse Wheel Left",
    "Mouse Wheel Right",
    "Mouse Move",
};

// Index = ControllerButton
// The face buttons are named by their position (e.g. cross = bottom button, A on an Xbox controller)
static const char* const _input_controller_names[CONTROLLER_BUTTON_SIZE] = {
    "Left Stick Button",
    "Left Stick Horizontal",
    "Left Stick Vertical",
    "Right Stick Button",
    "Right Stick Horizontal",
    "Right Stick Vertical",
    "Left Trigger",
    "Left Shoulder",
    "Right Trigger",
    "Right Shoulder",
    "Cross / A",
    "Circle / B",
    "Triangle / Y",
    "Square / X",
    "D-Pad Left",
    "D-Pad Right",
    "D-Pad Up",
    "D-Pad Down",
    "Button 1",
    "Button 2",
    "Button 3",
    "Button 4",
    "Button 5",
    "Button 6",
    "Button 7",
    "Button 8",
    "Left Stick Up",
    "Left Stick Down",
    "Left Stick Left",
    "Left Stick Right",
    "Right Stick Up",
    "Right Stick Down",
    "Right Stick Left",
    "Right Stick Right",
};

/**
 * Name of a single key (with device prefix, e.g. INPUT_KEYBOARD_PREFIX | KEY_SCAN_CODE_W)
 *
 * @return NULL for unknown keys
 */
inline
const char* input_key_name(int32 key) NO_EXCEPT
{
    const uint32 code = (uint32) key & 0xFFFF;

    if (code & INPUT_CONTROLLER_PREFIX) {
        const uint32 button = code & ~INPUT_CONTROLLER_PREFIX;

        return button < (uint32) CONTROLLER_BUTTON_SIZE ? _input_controller_names[button] : NULL;
    }

    if (code & INPUT_KEYBOARD_PREFIX) {
        const uint32 scan_code = code & 0xFF;

        return scan_code < (uint32) ARRAY_COUNT(_input_keyboard_names) ? _input_keyboard_names[scan_code] : NULL;
    }

    return code < (uint32) ARRAY_COUNT(_input_mouse_names) ? _input_mouse_names[code] : NULL;
}

/**
 * Text of a key combination, e.g. "Ctrl + S"
 *
 * @param keys      Keys of the combination (0 = unused)
 * @param count     Amount of keys
 * @param out       Output buffer
 * @param out_size  Size of the output buffer (incl. \0)
 *
 * @return Length of the text (0 = no keys)
 */
inline
int32 input_key_combination_name(const int16* const keys, int32 count, char* const out, int32 out_size) NO_EXCEPT
{
    int32 length = 0;
    out[0] = '\0';

    for (int32 i = 0; i < count; ++i) {
        if (!keys[i]) {
            continue;
        }

        char unknown[16];
        const char* name = input_key_name(keys[i]);
        if (!name) {
            // Unknown keys show their code (same as in the settings files)
            sprintf_fast(unknown, (int32) sizeof(unknown), "#%d", (int32) (uint16) keys[i]);
            name = unknown;
        }

        const int32 name_length = (int32) strlen(name);
        const int32 separator = length ? 3 : 0;
        if (length + separator + name_length >= out_size) {
            break;
        }

        if (separator) {
            memcpy(out + length, " + ", 3);
            length += 3;
        }

        memcpy(out + length, name, name_length);
        length += name_length;
        out[length] = '\0';
    }

    return length;
}

#endif
