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
#include "../utils/SimpleStringUtils.h"
#include "Input.h"
#include "ControllerInput.h"

// The names are stored with their length (known at compile time) -> no strlen when they are used
// KN = key name, only defined for the tables below
#define KN(name) simple_string_literal(name)
#define KN_NONE simple_string_empty_init<char>()

// Index = scan code (without INPUT_KEYBOARD_PREFIX), str == NULL = unknown key
// @bug This should not be hard coded, there are different languages!!!!
static const SimpleString<char> _input_keyboard_names[0x59] = {
    KN_NONE, KN("Esc"), KN("1"), KN("2"), KN("3"), KN("4"), KN("5"), KN("6"),                                      // 0x00
    KN("7"), KN("8"), KN("9"), KN("0"), KN("-"), KN("="), KN("Backspace"), KN("Tab"),                               // 0x08
    KN("Q"), KN("W"), KN("E"), KN("R"), KN("T"), KN("Y"), KN("U"), KN("I"),                                         // 0x10
    KN("O"), KN("P"), KN("["), KN("]"), KN("Enter"), KN("Ctrl"), KN("A"), KN("S"),                                  // 0x18
    KN("D"), KN("F"), KN("G"), KN("H"), KN("J"), KN("K"), KN("L"), KN(";"),                                         // 0x20
    KN("'"), KN("`"), KN("Left Shift"), KN("\\"), KN("Z"), KN("X"), KN("C"), KN("V"),                               // 0x28
    KN("B"), KN("N"), KN("M"), KN(","), KN("."), KN("/"), KN("Right Shift"), KN("Num *"),                           // 0x30
    KN("Alt"), KN("Space"), KN("Caps Lock"), KN("F1"), KN("F2"), KN("F3"), KN("F4"), KN("F5"),                      // 0x38
    KN("F6"), KN("F7"), KN("F8"), KN("F9"), KN("F10"), KN("Num Lock"), KN("Scroll Lock"), KN("Home / Num 7"),       // 0x40
    KN("Up / Num 8"), KN("Page Up / Num 9"), KN("Num -"), KN("Left / Num 4"), KN("Num 5"), KN("Right / Num 6"), KN("Num +"), KN("End / Num 1"), // 0x48
    KN("Down / Num 2"), KN("Page Down / Num 3"), KN("Insert / Num 0"), KN("Delete / Num ."), KN_NONE, KN_NONE, KN_NONE, KN("F11"), // 0x50
    KN("F12"),                                                                          // 0x58
};

// Index = InputMouseAction, str == NULL = unknown
static const SimpleString<char> _input_mouse_names[] = {
    KN_NONE,
    KN("Left Mouse"),
    KN("Right Mouse"),
    KN("Middle Mouse"),
    KN("Mouse 4"),
    KN("Mouse 5"),
    KN("Mouse Wheel"),
    KN("Mouse Wheel Horizontal"),
    KN("Mouse Wheel Up"),
    KN("Mouse Wheel Down"),
    KN("Mouse Wheel Left"),
    KN("Mouse Wheel Right"),
    KN("Mouse Move"),
};

// Index = ControllerButton
// The face buttons are named by their position (e.g. cross = bottom button, A on an Xbox controller)
static const SimpleString<char> _input_controller_names[CONTROLLER_BUTTON_SIZE] = {
    KN("Left Stick Button"),
    KN("Left Stick Horizontal"),
    KN("Left Stick Vertical"),
    KN("Right Stick Button"),
    KN("Right Stick Horizontal"),
    KN("Right Stick Vertical"),
    KN("Left Trigger"),
    KN("Left Shoulder"),
    KN("Right Trigger"),
    KN("Right Shoulder"),
    KN("Cross / A"),
    KN("Circle / B"),
    KN("Triangle / Y"),
    KN("Square / X"),
    KN("D-Pad Left"),
    KN("D-Pad Right"),
    KN("D-Pad Up"),
    KN("D-Pad Down"),
    KN("Button 1"),
    KN("Button 2"),
    KN("Button 3"),
    KN("Button 4"),
    KN("Button 5"),
    KN("Button 6"),
    KN("Button 7"),
    KN("Button 8"),
    KN("Left Stick Up"),
    KN("Left Stick Down"),
    KN("Left Stick Left"),
    KN("Left Stick Right"),
    KN("Right Stick Up"),
    KN("Right Stick Down"),
    KN("Right Stick Left"),
    KN("Right Stick Right"),
};

#undef KN
#undef KN_NONE

/**
 * Name of a single key incl. its length (with device prefix, e.g. INPUT_KEYBOARD_PREFIX | KEY_SCAN_CODE_W)
 *
 * @return {0, 0, NULL} for unknown keys
 */
inline
SimpleString<char> input_key_name_string(int32 key) NO_EXCEPT
{
    const uint32 code = (uint32) key & 0xFFFF;

    if (code & INPUT_CONTROLLER_PREFIX) {
        const uint32 button = code & ~INPUT_CONTROLLER_PREFIX;

        return button < (uint32) CONTROLLER_BUTTON_SIZE ? _input_controller_names[button] : simple_string_empty_init<char>();
    }

    if (code & INPUT_KEYBOARD_PREFIX) {
        const uint32 scan_code = code & 0xFF;

        return scan_code < (uint32) ARRAY_COUNT(_input_keyboard_names) ? _input_keyboard_names[scan_code] : simple_string_empty_init<char>();
    }

    return code < (uint32) ARRAY_COUNT(_input_mouse_names) ? _input_mouse_names[code] : simple_string_empty_init<char>();
}

/**
 * Name of a single key (with device prefix, e.g. INPUT_KEYBOARD_PREFIX | KEY_SCAN_CODE_W)
 *
 * @return NULL for unknown keys
 */
FORCE_INLINE
const char* input_key_name(int32 key) NO_EXCEPT
{
    return input_key_name_string(key).str;
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
        SimpleString<char> name = input_key_name_string(keys[i]);
        if (!name.str) {
            // Unknown keys show their code (same as in the settings files)
            name.length = sprintf_fast(unknown, (int32) sizeof(unknown), "#%d", (int32) (uint16) keys[i]);
            name.str = unknown;
        }

        const int32 name_length = name.length;
        const int32 separator = length ? 3 : 0;
        if (length + separator + name_length >= out_size) {
            break;
        }

        if (separator) {
            memcpy(out + length, " + ", 3);
            length += 3;
        }

        memcpy(out + length, name.str, name_length);
        length += name_length;
        out[length] = '\0';
    }

    return length;
}

#endif
