/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_INPUT_H
#define COMS_INPUT_H

#include "../stdlib/Types.h"
#include "../utils/BitUtils.h"
#include "../utils/StringUtils.h"
#include "../memory/BufferMemory.h"
#include "ControllerInput.h"
#include "InputConnectionType.h"

#define INPUT_MOUSE_BUTTON_1 1
#define INPUT_MOUSE_BUTTON_2 2
#define INPUT_MOUSE_BUTTON_3 3
#define INPUT_MOUSE_BUTTON_4 4
#define INPUT_MOUSE_BUTTON_5 5
#define INPUT_MOUSE_BUTTON_WHEEL 6
#define INPUT_MOUSE_BUTTON_HWHEEL 7

// How many concurrent mouse/secondary input device presses to we recognize
#define MAX_MOUSE_PRESSES 3

// How many concurrent primary key/button presses can be handled?
#define MAX_KEY_PRESSES 5
#define MAX_KEY_PRESS_TYPES (2 * MAX_KEY_PRESSES)

// How many keys/buttons do we support for the devices
#define MAX_KEYBOARD_KEYS 255
#define MAX_MOUSE_KEYS 10

#define MIN_INPUT_DEVICES 2

// How many buttons together are allowed to form a hotkey
#define MAX_HOTKEY_COMBINATION 3

// These values are used as bit flags to hint32 if a "key" is a keyboard/primary or mouse/secondary input
// When adding a keybind the "key" can only be uint8 but we expand it to an int and set the first bit accordingly
#define INPUT_MOUSE_PREFIX 0
#define INPUT_KEYBOARD_PREFIX 1 << 13
#define INPUT_CONTROLLER_PREFIX 1 << 14

enum InputType {
    INPUT_TYPE_NONE = 0,
    INPUT_TYPE_MOUSE_KEYBOARD = 1 << 0,
    INPUT_TYPE_CONTROLLER = 1 << 2,
    INPUT_TYPE_OTHER = 1 << 3,
};

#define MIN_CONTROLLER_DEVICES 4

#define INPUT_LONG_PRESS_DURATION 250

#ifdef _WIN32
    #include <windows.h>
    #include <dinput.h>
    #include "../platform/win32/UtilsWin32.h"
#endif

typedef void (*InputCallback)(void* data);

// @todo I'm not sure if I like the general input handling
//      Having separate keyboard_down and mouse_down etc. is a little bit weird in the functions below

enum KeyPressType : byte {
    KEY_PRESS_TYPE_NONE,
    KEY_PRESS_TYPE_PRESSED,
    KEY_PRESS_TYPE_HELD,
    KEY_PRESS_TYPE_RELEASED,
};

struct Hotkey {
    // negative hotkeys mean any of them needs to be matched, positive hotkeys means all of them need to be matched
    // mixing positive and negative keys for one hotkey is not possible
    // index = hotkey, value = key id
    int16 scan_codes[MAX_HOTKEY_COMBINATION];
    KeyPressType key_state;
    InputCallback callback;
};

struct InputMapping {
    // A hotkey can be bound to a combination of up to 3 key/button presses
    // The indices represent the hotkey id
    uint8 hotkey_count;
    Hotkey* hotkeys;
};

struct InputKey {
    // Includes flag for mouse, keyboard, controller
    uint16 scan_code;
    uint16 virtual_code;
    KeyPressType key_state;
    uint64 time; // when was this action performed (useful to decide if key state is held vs pressed)
    bool is_processed;
    int16 value; // e.g. stick/trigger keys have additional values
};

// @question Maybe we should also add a third key_down array for controllers and some special controller functions here to just handle everything in one struct
//      Or think about completely splitting all states (mouse, keyboard, other)
struct InputState {
    // Active hotkeys
    uint16 active_hotkeys[MAX_KEY_PRESSES];

    // Active keys
    InputKey active_keys[MAX_KEY_PRESS_TYPES];

    // @question Couldn't we make the values below 16 bit?
    int32 dx;
    int32 dy;

    uint32 x;
    uint32 y;

    // Secondary coordinate input (usually used by controllers)
    int32 dx2;
    int32 dy2;

    uint32 x2;
    uint32 y2;

    // Tertiary coordinate input (usually used by controllers)
    int32 dx3;
    int32 dy3;

    uint32 x3;
    uint32 y3;
};

enum GeneralInputState : byte {
    INPUT_STATE_GENERAL_BUTTON_CHANGE = 1 << 0,
    INPUT_STATE_GENERAL_MOUSE_CHANGE = 1 << 1,
    INPUT_STATE_GENERAL_MOUSE_MOVEMENT = 1 << 2,
    INPUT_STATE_GENERAL_TYPING_MODE = 1 << 3,
};

struct Input {
    // Device
    InputConnectionType connection_type;

    #ifdef _WIN32
        // @question maybe replace with id?!
        //      -> remove _WIN32 section?
        HANDLE handle_keyboard;
        HANDLE handle_mouse;

        // @todo support all three versions
        int32 controller_id; // used by XInput
        HANDLE handle_controller; // used by raw input controller
        LPDIRECTINPUTDEVICE8* direct_controller; // used by direct input controller
    #endif

    byte controller_type;
    byte general_states;

    InputState state;
    uint64 time_last_input_check;

    // @todo this should probably be somewhere else
    // @todo don't we need multiple deadzones? triggers, sticks
    uint32 deadzone = 10;
    char text[512];

    // This data is passed to the hotkey callback
    void* callback_data;

    InputMapping input_mapping1;
    InputMapping input_mapping2;
};

inline
void input_init(Input* input, uint8 size, void* callback_data, BufferMemory* buf) noexcept
{
    // Init input
    input->callback_data = callback_data;

    // Init mapping1
    input->input_mapping1.hotkey_count = size;
    input->input_mapping1.hotkeys = (Hotkey *) buffer_get_memory(
        buf,
        input->input_mapping1.hotkey_count * sizeof(Hotkey),
        4, true
    );

    // Init mapping2
    input->input_mapping2.hotkey_count = size;
    input->input_mapping2.hotkeys = (Hotkey *) buffer_get_memory(
        buf,
        input->input_mapping2.hotkey_count * sizeof(Hotkey),
        4, true
    );
}

inline
void input_clean_state(InputKey* active_keys, KeyPressType press_status = KEY_PRESS_TYPE_RELEASED) noexcept
{
    if (press_status) {
        for (int32 i = 0; i < MAX_KEY_PRESS_TYPES; ++i) {
            if (active_keys[i].key_state == press_status) {
                memset(&active_keys[i], 0, sizeof(InputKey));
            }
        }
    } else {
        memset(active_keys, 0, MAX_KEY_PRESS_TYPES * sizeof(InputKey));
    }
}

inline
bool input_action_exists(const InputKey* active_keys, int16 key, KeyPressType press_type = KEY_PRESS_TYPE_PRESSED) noexcept
{
    return (active_keys[0].scan_code == key && active_keys[0].key_state == press_type)
        || (active_keys[1].scan_code == key && active_keys[1].key_state == press_type)
        || (active_keys[2].scan_code == key && active_keys[2].key_state == press_type)
        || (active_keys[3].scan_code == key && active_keys[3].key_state == press_type)
        || (active_keys[4].scan_code == key && active_keys[4].key_state == press_type)
        || (active_keys[4].scan_code == key && active_keys[4].key_state == press_type)
        || (active_keys[5].scan_code == key && active_keys[5].key_state == press_type)
        || (active_keys[6].scan_code == key && active_keys[6].key_state == press_type)
        || (active_keys[7].scan_code == key && active_keys[7].key_state == press_type)
        || (active_keys[8].scan_code == key && active_keys[8].key_state == press_type)
        || (active_keys[9].scan_code == key && active_keys[9].key_state == press_type);
}

inline
bool input_is_down(const InputKey* active_keys, int16 key) noexcept
{
    return (active_keys[0].scan_code == key && active_keys[0].key_state != KEY_PRESS_TYPE_RELEASED)
        || (active_keys[1].scan_code == key && active_keys[1].key_state != KEY_PRESS_TYPE_RELEASED)
        || (active_keys[2].scan_code == key && active_keys[2].key_state != KEY_PRESS_TYPE_RELEASED)
        || (active_keys[3].scan_code == key && active_keys[3].key_state != KEY_PRESS_TYPE_RELEASED)
        || (active_keys[4].scan_code == key && active_keys[4].key_state != KEY_PRESS_TYPE_RELEASED)
        || (active_keys[5].scan_code == key && active_keys[5].key_state != KEY_PRESS_TYPE_RELEASED)
        || (active_keys[6].scan_code == key && active_keys[6].key_state != KEY_PRESS_TYPE_RELEASED)
        || (active_keys[7].scan_code == key && active_keys[7].key_state != KEY_PRESS_TYPE_RELEASED)
        || (active_keys[8].scan_code == key && active_keys[8].key_state != KEY_PRESS_TYPE_RELEASED)
        || (active_keys[9].scan_code == key && active_keys[9].key_state != KEY_PRESS_TYPE_RELEASED);
}

inline
bool input_is_pressed(const InputKey* active_keys, int16 key) noexcept
{
    return (active_keys[0].scan_code == key && active_keys[0].key_state == KEY_PRESS_TYPE_PRESSED)
        || (active_keys[1].scan_code == key && active_keys[1].key_state == KEY_PRESS_TYPE_PRESSED)
        || (active_keys[2].scan_code == key && active_keys[2].key_state == KEY_PRESS_TYPE_PRESSED)
        || (active_keys[3].scan_code == key && active_keys[3].key_state == KEY_PRESS_TYPE_PRESSED)
        || (active_keys[4].scan_code == key && active_keys[4].key_state == KEY_PRESS_TYPE_PRESSED)
        || (active_keys[5].scan_code == key && active_keys[5].key_state == KEY_PRESS_TYPE_PRESSED)
        || (active_keys[6].scan_code == key && active_keys[6].key_state == KEY_PRESS_TYPE_PRESSED)
        || (active_keys[7].scan_code == key && active_keys[7].key_state == KEY_PRESS_TYPE_PRESSED)
        || (active_keys[8].scan_code == key && active_keys[8].key_state == KEY_PRESS_TYPE_PRESSED)
        || (active_keys[9].scan_code == key && active_keys[9].key_state == KEY_PRESS_TYPE_PRESSED);
}

inline
bool input_is_held(const InputKey* active_keys, int16 key) noexcept
{
    return (active_keys[0].scan_code == key && active_keys[0].key_state == KEY_PRESS_TYPE_HELD)
        || (active_keys[1].scan_code == key && active_keys[1].key_state == KEY_PRESS_TYPE_HELD)
        || (active_keys[2].scan_code == key && active_keys[2].key_state == KEY_PRESS_TYPE_HELD)
        || (active_keys[3].scan_code == key && active_keys[3].key_state == KEY_PRESS_TYPE_HELD)
        || (active_keys[4].scan_code == key && active_keys[4].key_state == KEY_PRESS_TYPE_HELD)
        || (active_keys[4].scan_code == key && active_keys[4].key_state == KEY_PRESS_TYPE_HELD)
        || (active_keys[5].scan_code == key && active_keys[5].key_state == KEY_PRESS_TYPE_HELD)
        || (active_keys[6].scan_code == key && active_keys[6].key_state == KEY_PRESS_TYPE_HELD)
        || (active_keys[7].scan_code == key && active_keys[7].key_state == KEY_PRESS_TYPE_HELD)
        || (active_keys[8].scan_code == key && active_keys[8].key_state == KEY_PRESS_TYPE_HELD)
        || (active_keys[9].scan_code == key && active_keys[9].key_state == KEY_PRESS_TYPE_HELD);
}

inline
bool input_is_released(const InputKey* active_keys, int16 key) noexcept
{
    return (active_keys[0].scan_code == key && active_keys[0].key_state == KEY_PRESS_TYPE_RELEASED)
        || (active_keys[1].scan_code == key && active_keys[1].key_state == KEY_PRESS_TYPE_RELEASED)
        || (active_keys[2].scan_code == key && active_keys[2].key_state == KEY_PRESS_TYPE_RELEASED)
        || (active_keys[3].scan_code == key && active_keys[3].key_state == KEY_PRESS_TYPE_RELEASED)
        || (active_keys[4].scan_code == key && active_keys[4].key_state == KEY_PRESS_TYPE_RELEASED)
        || (active_keys[4].scan_code == key && active_keys[4].key_state == KEY_PRESS_TYPE_RELEASED)
        || (active_keys[5].scan_code == key && active_keys[5].key_state == KEY_PRESS_TYPE_RELEASED)
        || (active_keys[6].scan_code == key && active_keys[6].key_state == KEY_PRESS_TYPE_RELEASED)
        || (active_keys[7].scan_code == key && active_keys[7].key_state == KEY_PRESS_TYPE_RELEASED)
        || (active_keys[8].scan_code == key && active_keys[8].key_state == KEY_PRESS_TYPE_RELEASED)
        || (active_keys[9].scan_code == key && active_keys[9].key_state == KEY_PRESS_TYPE_RELEASED);
}

inline
bool input_was_down(const InputKey* active_keys, int16 key) noexcept
{
    return (active_keys[0].scan_code == key && active_keys[0].key_state == KEY_PRESS_TYPE_RELEASED)
        || (active_keys[1].scan_code == key && active_keys[1].key_state == KEY_PRESS_TYPE_RELEASED)
        || (active_keys[2].scan_code == key && active_keys[2].key_state == KEY_PRESS_TYPE_RELEASED)
        || (active_keys[3].scan_code == key && active_keys[3].key_state == KEY_PRESS_TYPE_RELEASED)
        || (active_keys[4].scan_code == key && active_keys[4].key_state == KEY_PRESS_TYPE_RELEASED)
        || (active_keys[4].scan_code == key && active_keys[4].key_state == KEY_PRESS_TYPE_RELEASED)
        || (active_keys[5].scan_code == key && active_keys[5].key_state == KEY_PRESS_TYPE_RELEASED)
        || (active_keys[6].scan_code == key && active_keys[6].key_state == KEY_PRESS_TYPE_RELEASED)
        || (active_keys[7].scan_code == key && active_keys[7].key_state == KEY_PRESS_TYPE_RELEASED)
        || (active_keys[8].scan_code == key && active_keys[8].key_state == KEY_PRESS_TYPE_RELEASED)
        || (active_keys[9].scan_code == key && active_keys[9].key_state == KEY_PRESS_TYPE_RELEASED);
}

inline
bool inputs_are_down(
    const InputKey* active_keys,
    int16 key0, int16 key1 = 0, int16 key2 = 0, int16 key3 = 0, int16 key4 = 0
) noexcept {
    return (key0 != 0 && input_is_down(active_keys, key0))
        && (key1 == 0 || input_is_down(active_keys, key1))
        && (key2 == 0 || input_is_down(active_keys, key2))
        && (key3 == 0 || input_is_down(active_keys, key3))
        && (key4 == 0 || input_is_down(active_keys, key4));
}

void input_add_callback(InputMapping* mapping, uint8 hotkey, InputCallback callback) noexcept
{
    mapping->hotkeys[hotkey].callback = callback;
}

// We are binding hotkeys bi-directional:
//      Which keys are required for a certain hotkey
//      What are the hotkeys a key can trigger
void
input_add_hotkey(
    InputMapping* mapping, uint8 hotkey,
    int16 key0, int16 key1 = 0, int16 key2 = 0,
    KeyPressType press_type = KEY_PRESS_TYPE_PRESSED
) noexcept
{
    int32 count = 0;

    mapping->hotkeys[(hotkey - 1)].key_state = press_type;

    // Define required keys for hotkey
    // Note: -1 since the hotkeys always MUST start at 1 (0 is a special value for empty)
    if (key0) {
        mapping->hotkeys[(hotkey - 1)].scan_codes[count++] = key0;
        key0 = OMS_ABS_INT16(key0);
    }

    if (key1) {
        // Note: -1 since the hotkeys MUST start at 1 (0 is a special value for empty)
        mapping->hotkeys[(hotkey - 1)].scan_codes[count++] = key1;
        key1 = OMS_ABS_INT16(key1);
    }

    if (key2) {
        // Note: -1 since the hotkeys MUST start at 1 (0 is a special value for empty)
        mapping->hotkeys[(hotkey - 1)].scan_codes[count++] = key2;
        key2 = OMS_ABS_INT16(key2);
    }
}

inline
bool hotkey_is_active(const uint16* active_hotkeys, uint16 hotkey) noexcept
{
    return active_hotkeys[0] == hotkey
        || active_hotkeys[1] == hotkey
        || active_hotkeys[2] == hotkey
        || active_hotkeys[3] == hotkey
        || active_hotkeys[4] == hotkey;
}

// similar to hotkey_is_active but instead of just performing a lookup in the input_hotkey_state created results
// this is actively checking the current input state (not the hotkey state)
inline
bool hotkey_keys_are_active(const InputKey* active_keys, const InputMapping* mapping, uint16 hotkey) noexcept
{
    int16 key0 = mapping->hotkeys[(hotkey - 1)].scan_codes[0];
    int16 key1 = mapping->hotkeys[(hotkey - 1)].scan_codes[1];
    int16 key2 = mapping->hotkeys[(hotkey - 1)].scan_codes[2];

    if (!key0 && !key1 && !key2) {
        return false;
    }

    // This may seem a little bit confusing but we don't care if a input key is down or up
    // Any state means it was used recently BUT NOT YET HANDLED
    // If it was handled it would've been removed (at least in case of RELEASED)
    // Therefore, if a key has a state -> treat it as if active

    // The code below also allows optional keys which have a negative sign (at least one of the optional keys must be valid)
    bool is_active = input_action_exists(active_keys, OMS_ABS_INT16(key0), mapping->hotkeys[(hotkey - 1)].key_state);
    if ((!is_active && (key0 > 0 || key1 >= 0)) || (is_active && key0 < 0) || (key1 == 0 && key2 == 0)) {
        return is_active;
    }

    is_active = input_action_exists(active_keys, OMS_ABS_INT16(key1), mapping->hotkeys[(hotkey - 1)].key_state);
    if ((!is_active && (key1 > 0 || key2 >= 0)) || (is_active && key1 < 0) || (key2 == 0)) {
        return is_active;
    }

    return input_action_exists(active_keys, OMS_ABS_INT16(key2), mapping->hotkeys[(hotkey - 1)].key_state);
}

inline
void input_set_state(InputKey* __restrict active_keys, const InputKey* __restrict new_key) noexcept
{
    InputKey* free_state = NULL;

    // Insert new key state or change if key already exists
    for (int32 i = 0; i < MAX_KEY_PRESS_TYPES; ++i) {
        if (!free_state && active_keys[i].scan_code == 0) {
            free_state = &active_keys[i];
        } else if (active_keys[i].scan_code == new_key->scan_code) {
            active_keys[i].key_state = new_key->key_state;
            active_keys[i].value += new_key->value;
            active_keys[i].time = new_key->time;

            return;
        }
    }

    if (!free_state) {
        return;
    }

    free_state->scan_code = new_key->scan_code;
    free_state->virtual_code = new_key->virtual_code;
    free_state->key_state = new_key->key_state;
    free_state->value = new_key->value;
    free_state->time = new_key->time;
}

// Controllers are a little bit special
// We need to manually check the specific buttons and set their key
// Since some controllers are constantly sending data like mad it's not possible to handle them event based
// We need to poll them and then check the old state against this new state (annoying but necessary)
// Mice are fully supported by RawInput and are fairly generalized in terms of their buttons -> no special function needed
inline
void input_set_controller_state(Input* input, ControllerInput* controller, uint64 time) noexcept
{
    // Check active keys that might need to be set to inactive
    for (int32 i = 0; i < MAX_KEY_PRESSES; ++i) {
        if ((input->state.active_keys[i].scan_code & INPUT_CONTROLLER_PREFIX)
            && input->state.active_keys[i].key_state != KEY_PRESS_TYPE_RELEASED
        ) {
            uint32 scan_code = input->state.active_keys[i].scan_code & ~INPUT_CONTROLLER_PREFIX;

            if ((controller->is_analog[scan_code] && OMS_ABS_INT8(controller->button[scan_code]) < input->deadzone)
                || (!controller->is_analog[scan_code] && controller->button[scan_code] == 0)
            ) {
                input->state.active_keys[i].key_state = KEY_PRESS_TYPE_RELEASED;
            }
        }
    }

    // Special keys
    // @todo this code means we cannot change this behavior (e.g. swap mouse view to dpad, swap sticks, ...)
    // @todo This is also not very general, maybe we can fix it like we did with analog vs digital key (instead of bool flag maybe bit flag)
    if (OMS_ABS_INT8(controller->button[CONTROLLER_BUTTON_STICK_RIGHT_HORIZONTAL]) > input->deadzone) {
        input->state.dx += controller->button[CONTROLLER_BUTTON_STICK_RIGHT_HORIZONTAL] / 8;
        input->general_states |= INPUT_STATE_GENERAL_MOUSE_CHANGE;
    } else {
        input->state.dx = 0;
    }

    if (OMS_ABS_INT8(controller->button[CONTROLLER_BUTTON_STICK_RIGHT_VERTICAL]) > input->deadzone) {
        input->state.dy += controller->button[CONTROLLER_BUTTON_STICK_RIGHT_VERTICAL] / 8;
        input->general_states |= INPUT_STATE_GENERAL_MOUSE_CHANGE;
    } else {
        input->state.dy = 0;
    }

    if (OMS_ABS_INT8(controller->button[CONTROLLER_BUTTON_STICK_LEFT_HORIZONTAL]) > input->deadzone) {
        input->state.dx2 += controller->button[CONTROLLER_BUTTON_STICK_LEFT_HORIZONTAL] / 8;
        // @todo needs state change flag like mouse?!
    } else {
        input->state.dx2 = 0;
    }

    if (OMS_ABS_INT8(controller->button[CONTROLLER_BUTTON_STICK_LEFT_HORIZONTAL]) > input->deadzone) {
        input->state.dy2 += controller->button[CONTROLLER_BUTTON_STICK_LEFT_HORIZONTAL] / 8;
        // @todo needs state change flag like mouse?!
    } else {
        input->state.dy2 = 0;
    }

    // General Keys
    int32 count = 0;
    InputKey keys[5];

    for (uint16 i = 0; i < 32; ++i) {
        if ((controller->is_analog[i] && OMS_ABS_INT8(controller->button[i]) > input->deadzone)
            || (!controller->is_analog[i] && controller->button[i] != 0)
        ) {
            keys[count].scan_code = i | INPUT_CONTROLLER_PREFIX;
            keys[count].key_state = KEY_PRESS_TYPE_PRESSED;
            keys[count].value = controller->button[i];
            keys[count].time = time;

            ++count;
        }
    }

    if (count > 0) {
        for (int32 i = 0; i < count; ++i) {
            input_set_state(input->state.active_keys, &keys[i]);
        }
    }

    input->general_states |= INPUT_STATE_GENERAL_BUTTON_CHANGE;
}

void input_hotkey_state(Input* input) noexcept
{
    InputState* state = &input->state;
    memset(state->active_hotkeys, 0, sizeof(uint16) * MAX_KEY_PRESSES);

    // Check if we have any active keys
    if (memcmp(state->active_keys, ((byte *) state->active_keys) + 1, sizeof(state->active_keys) - 1) == 0) {
        input_clean_state(state->active_keys);
        return;
    }

    // Check typing mode
    if (input->general_states & INPUT_STATE_GENERAL_TYPING_MODE) {
        *input->text = '\0';
        int32 input_characters = 0;
        uint32 characters[10];

        // Create keyboard state array
        byte keyboard_state[256] = {};
        for (int32 key_state = 0; key_state < MAX_KEY_PRESS_TYPES; ++key_state) {
            if (state->active_keys[key_state].scan_code == 0
                || state->active_keys[key_state].key_state == KEY_PRESS_TYPE_RELEASED
            ) {
                // no key defined for this down state
                continue;
            }

            keyboard_state[state->active_keys[key_state].virtual_code & 0x00FF] = 0x80;
        }

        // Check if all keys result in text, if not -> is potential hotkey -> shouldn't output any text
        for (int32 key_state = 0; key_state < MAX_KEY_PRESS_TYPES; ++key_state) {
            if ((input->general_states & INPUT_STATE_GENERAL_TYPING_MODE)
                && (state->active_keys[key_state].scan_code & INPUT_KEYBOARD_PREFIX)
                && state->active_keys[key_state].key_state != KEY_PRESS_TYPE_RELEASED
            ) {
                if (input_characters >= ARRAY_COUNT(characters)) {
                    break;
                }

                uint32 code = key_to_unicode(
                    state->active_keys[key_state].scan_code & 0x00FF,
                    state->active_keys[key_state].virtual_code & 0x00FF,
                    keyboard_state
                );

                // Is the pressed key a keyboard input
                if (!code) {
                    // Is not text -> we have to reset characters
                    memset(characters, 0, sizeof(uint32) * input_characters);
                    input_characters = 0;

                    break;
                }

                characters[input_characters++] = code;
            }
        }

        if (input_characters) {
            // Mark keys
            for (int32 key_state = 0; key_state < MAX_KEY_PRESS_TYPES; ++key_state) {
                state->active_keys[key_state].is_processed = true;
                state->active_keys[key_state].time = 0; // @todo fix
            }

            // Create text from input
            char* pos = input->text;
            for (uint32 i = 0; i < ARRAY_COUNT(characters); ++i) {
                pos += utf8_decode(characters[i], pos);
            }

            *pos = '\0';

            input_clean_state(state->active_keys);
            return;
        }
    }

    int32 active_hotkeys = 0;

    // Check every mapping
    for (int32 i = 0; i < 2; ++i) {
        InputMapping* mapping = i == 0 ? &input->input_mapping1 : &input->input_mapping2;

        // Check all possible hotkeys if all of their required keys are active
        for (int16 hotkey_idx = 1; hotkey_idx <= mapping->hotkey_count; ++hotkey_idx) {
            // We only support a limited amount of active hotkeys
            if (active_hotkeys >= MAX_KEY_PRESSES) {
                hotkey_idx = mapping->hotkey_count + 1;
                i = 2;
                break;
            }

            if (hotkey_is_active(state->active_hotkeys, hotkey_idx),
                !hotkey_keys_are_active(state->active_keys, mapping, hotkey_idx)
            ) {
                // Hotkey already active, we don't need to check if it needs to be activated
                // Or not all keys for the hotkey are pressed or the KeyPressType is not the same
                continue;
            }

            state->active_hotkeys[active_hotkeys++] = hotkey_idx;

            // Run callback if defined
            if (mapping->hotkeys[hotkey_idx].callback != 0) {
                mapping->hotkeys[hotkey_idx].callback(input->callback_data);
            }
        }
    }

    input_clean_state(state->active_keys);

    // @bug how to handle priority? e.g. there might be a hotkey for 1 and one for alt+1
    //      in this case only the hotkey for alt+1 should be triggered
    // @bug how to handle other conditions besides buttons pressed together? some hotkeys are only available in certain situations
    // @bug how to handle values (e.g. stick may or may not set the x/y or dx/dy in some situations)
    // @bug how to allow rebinding/swapping of left and right stick? (maybe create handful of events e.g. set dx/dy that fire based on the input?)
    // @bug There is a bug ONLY with the controller, when doing camera look around and holding the stick at and angle
    //          The hotkey seemingly loses activity after 1-2 sec if you then move the stick a little bit it works again
    //          It doesn't always happen but you can test it rather consistently within a couple of seconds
}

// @todo We probably need a way to unset a specific key and hotkey after processing it
bool input_key_is_longpress(const InputState* state, int16 key, uint64 time, f32 dt = 0.0f) noexcept {
    for (int32 i = 0; i < MAX_KEY_PRESS_TYPES; ++i) {
        if (state->active_keys[i].scan_code == key) {
            return (f32) (time - state->active_keys[i].time) / 1000.0f >= (dt == 0.0f ? INPUT_LONG_PRESS_DURATION : dt);
        }
    }

    return false;
}

// @todo I wrote this code at 9am after staying awake for the whole night and that is how that code looks like... fix it!
bool input_hotkey_is_longpress(const Input* input, uint8 hotkey, uint64 time, f32 dt = 0.0f) noexcept {
    bool is_longpress = false;
    for (int32 i = 0; i < MAX_KEY_PRESSES; ++i) {
        if (input->state.active_hotkeys[i] != hotkey) {
            continue;
        }

        is_longpress = true;

        for (int32 j = 0; j < MAX_HOTKEY_COMBINATION; ++j) {
            bool potential_miss = true;
            bool both_empty = false;
            if (input->input_mapping1.hotkeys[hotkey].scan_codes[j] > 0) {
                if(!input_key_is_longpress(&input->state, input->input_mapping1.hotkeys[hotkey].scan_codes[j], time, dt)) {
                    potential_miss = true;
                } else {
                    potential_miss = false;
                }
            } else {
                both_empty = true;
            }

            if (!potential_miss) {
                continue;
            }

            if (input->input_mapping2.hotkeys[hotkey].scan_codes[j] > 0) {
                if(!input_key_is_longpress(&input->state, input->input_mapping2.hotkeys[hotkey].scan_codes[j], time, dt)) {
                    potential_miss = true;
                } else {
                    potential_miss = false;
                }
            } else {
                both_empty &= true;
            }

            if (both_empty) {
                continue;
            } else if (potential_miss) {
                return false;
            }
        }
    }

    return is_longpress;
}

uint32 input_get_typed_character(InputState* state, uint64 time, uint64 dt) noexcept
{
    byte keyboard_state[256] = {};
    for (int32 key_state = 0; key_state < MAX_KEY_PRESS_TYPES; ++key_state) {
        if (state->active_keys[key_state].scan_code == 0
            || state->active_keys[key_state].key_state == KEY_PRESS_TYPE_RELEASED
        ) {
            // no key defined for this down state
            continue;
        }

        keyboard_state[state->active_keys[key_state].virtual_code & 0x00FF] = 0x80;
    }

    for (int32 key_state = 0; key_state < MAX_KEY_PRESS_TYPES; ++key_state) {
        if (state->active_keys[key_state].scan_code == 0
            || (state->active_keys[key_state].is_processed
                && state->active_keys[key_state].time - time <= dt)
        ) {
            // no key defined for this down state
            // key is already released
            // key was already processed and is not yet eligible for continuous output
            continue;
        }

        uint32 code = key_to_unicode(
            state->active_keys[key_state].scan_code & 0x00FF,
            state->active_keys[key_state].virtual_code & 0x00FF,
            keyboard_state
        );

        if (code) {
            // We are not outputting a repeat character multiple times in low fps situations.
            // It is annoying as a user to suddenly have 10s of repeated character just because the game lagged
            state->active_keys[key_state].is_processed = true;
            state->active_keys[key_state].time = time;

            return code;
        }
    }

    return 0;
}

#endif