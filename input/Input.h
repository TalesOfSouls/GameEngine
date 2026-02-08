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

#include "../stdlib/Stdlib.h"
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
// @performance Can we make this 4? this way we could optimize some checks below by checking 2 hotkeys at the same time as uint32 against 0
#define MAX_KEY_PRESSES 5

// How many keys/buttons do we support for the devices
#define MAX_KEYBOARD_KEYS 255
#define MAX_MOUSE_KEYS 7

// How many buttons together are allowed to form a hotkey
#define MAX_HOTKEY_COMBINATION 3

// These values are used as bit flags to hint32 if a "key" is a keyboard/primary or mouse/secondary input
// When adding a keybind the "key" can only be uint8 but we expand it to an int and set the first bit accordingly
#define INPUT_MOUSE_PREFIX 0
#define INPUT_KEYBOARD_PREFIX 1 << 13
#define INPUT_CONTROLLER_PREFIX 1 << 14

enum InputType : byte {
    INPUT_TYPE_NONE = 0,
    INPUT_TYPE_MOUSE_KEYBOARD = 1 << 0,
    INPUT_TYPE_CONTROLLER = 1 << 1,
    INPUT_TYPE_OTHER = 1 << 2,
};

#define INPUT_LONG_PRESS_DURATION 250

#ifdef _WIN32
    #include <windows.h>
    #include <dinput.h>
    #include "../platform/win32/UtilsWin32.h"
#endif

/**
 * @param void* data General data to pass to the event function
 * @param int32 input_id Input state that triggered this callback
 */
typedef void (*InputCallback)(void* data, int32 input_id);

enum InputEventFlag : uint8 {
    INPUT_EVENT_FLAG_NONE = 0,
    INPUT_EVENT_FLAG_MULTIPLE = 1 << 0, // can run multiple times per frame/input handling
    INPUT_EVENT_FLAG_REPEAT = 1 << 1, // not getting removed after run
};

struct InputEvent {
    uint8 flag;
    InputCallback callback;
};

// @todo I'm not sure if I like the general input handling
//      Having separate keyboard_down and mouse_down etc. is a little bit weird in the functions below

enum KeyPressType : byte {
    KEY_PRESS_TYPE_NONE,
    KEY_PRESS_TYPE_PRESSED,
    KEY_PRESS_TYPE_HELD,
    KEY_PRESS_TYPE_RELEASED,
};

// This is probably never used but serves as a general idea how to handle input context
enum InputContext : byte {
    // used for typing
    HOTKEY_CONTEXT_TYPING = 1 << 0,

    HOTKEY_CONTEXT_MAIN_MENU = 1 << 1,
    HOTKEY_CONTEXT_INGAME_MENU = 1 << 2,

    // Different game modes
    HOTKEY_CONTEXT_INGAME_1 = 1 << 3,
    HOTKEY_CONTEXT_INGAME_2 = 1 << 4,
    HOTKEY_CONTEXT_INGAME_3 = 1 << 5,

    HOTKEY_CONTEXT_EDITOR_MODE = 1 << 6,
    HOTKEY_CONTEXT_ADMIN_MODE = 1 << 7
};

struct Hotkey {
    // negative hotkeys mean any of them needs to match, positive hotkeys means all of them need to match
    // mixing positive and negative keys for one hotkey is not possible
    // index = hotkey, value = key id
    // https://kbdlayout.info/
    int16 scan_codes[MAX_HOTKEY_COMBINATION];
    KeyPressType key_state;

    // Some hotkeys are only available in certain context
    // uses enum InputContext as a baseline
    byte context;
};

struct InputKey {
    // @question Do we really need scan_code and virtual_code both?
    uint16 scan_code;
    uint16 virtual_code;
    KeyPressType key_state;
    bool is_processed;
    int16 value; // e.g. stick/trigger keys have additional values
    uint64 time; // when was this action performed (useful to decide if key state is held vs pressed)
};

// @question Maybe we should also add a third key_down array for controllers and some special controller functions here to just handle everything in one struct
//      Or think about completely splitting all states (mouse, keyboard, other)
struct InputState {
    // Active hotkeys
    // @bug This should also contain the time of when the hotkey got activated
    //      We have this on a InputKey level but don't use it at the moment
    uint16 active_hotkeys[MAX_KEY_PRESSES];

    // Active keys
    alignas(8) InputKey active_keys[MAX_KEY_PRESSES];

    // Usually used by controllers
    // E.g. index 0 = primary stick or mouse, index 1 = secondary stick, index 2 = thumb trackpad
    int16 dx[3];
    int16 dy[3];

    int16 x[3];
    int16 y[3];
};

enum GeneralInputState : byte {
    INPUT_STATE_GENERAL_BUTTON_CHANGE = 1 << 0,
    INPUT_STATE_GENERAL_MOUSE_CHANGE = 1 << 1,
    INPUT_STATE_GENERAL_MOUSE_MOVEMENT = 1 << 2,
    INPUT_STATE_GENERAL_TYPING_MODE = 1 << 3, // Used for typing in chat box etc. otherwise we could have conflicts with hotkeys
    INPUT_STATE_GENERAL_HOTKEY_ACTIVE = 1 << 4, // At least one hotkey is active
};

struct KBMDevice {
    wchar_t device_name[128];

    #ifdef _WIN32
        // Windows is nuts, one device can have multiple handles
        // Sometimes to simulate additional buttons additional devices are created
        // There are probably more insane reasons
        HANDLE handle[8];
    #endif
};

union ControllerDevice {
    #ifdef _WIN32
        int32 id; // used by XInput
        HANDLE handle; // used by raw input controller
        LPDIRECTINPUTDEVICE8* direct; // used by direct input controller
    #endif
};

struct Input {
    // Device
    InputConnectionType connection_type;

    byte general_states;
    byte controller_type;
    uint8 hotkey_count;

    KBMDevice keyboard;
    KBMDevice mouse;

    ControllerDevice controller;

    InputState state;
    uint64 time_last_input_check;

    // @todo this should probably be somewhere else
    // @todo don't we need multiple deadzones? triggers, sticks
    int32 deadzone;
    char text[512];

    // We allow a hotkey to be mapped in two different ways = allow alternative hotkey combination
    // @bug This doesn't allow alternative mappings + controller support together with keyboard
    //      Sometimes a player wants to use controller but still use keyboard hotkeys at the same time
    //      Sure mapping1 could be keyboard and mapping2 controller but then we cannot have alternate mapping for keyboard
    Hotkey* input_mapping1;
    Hotkey* input_mapping2;

    // This contains a full list off REFERENCES to hotkey events
    // The length of this array is the exact amount of possible hotkeys
    // It doesn't hold the actual event but a REFERENCE to it
    // The reason for that is that we don't necessarily need n-to-n events.
    // Usually we need less events than hotkeys (e.g. all movement hotkeys are handled by one movement event function)
    const InputEvent* const* hotkey_event_list;
};

// count = count of possible hotkeys
inline
void input_init(Input* const input, uint8 count, BufferMemory* const buf) NO_EXCEPT
{
    input->hotkey_count = count;

    // Init mapping1
    input->input_mapping1 = (Hotkey *) buffer_get_memory(
        buf,
        input->hotkey_count * sizeof(Hotkey),
        sizeof(size_t)
    );
    memset(input->input_mapping1, 0, input->hotkey_count * sizeof(Hotkey));

    // Init mapping2
    input->input_mapping2 = (Hotkey *) buffer_get_memory(
        buf,
        input->hotkey_count * sizeof(Hotkey),
        sizeof(size_t)
    );
    memset(input->input_mapping2, 0, input->hotkey_count * sizeof(Hotkey));
}

// Resets the mapping
// Usually used for re-assigning hotkeys
inline
void input_mapping_reset(Input* const input) NO_EXCEPT {
    memset(input->input_mapping1, 0, input->hotkey_count * sizeof(Hotkey));
    memset(input->input_mapping2, 0, input->hotkey_count * sizeof(Hotkey));
    memset(&input->state, 0, sizeof(input->state));
}

// Resets keys by status
inline
void input_clean_state(InputKey* const active_keys, KeyPressType press_status = KEY_PRESS_TYPE_RELEASED) NO_EXCEPT
{
    if (press_status) {
        for (int32 i = 0; i < MAX_KEY_PRESSES; ++i) {
            if (active_keys[i].key_state == press_status) {
                memset(&active_keys[i], 0, sizeof(InputKey));
            }
        }
    } else {
        memset(active_keys, 0, MAX_KEY_PRESSES * sizeof(InputKey));
    }
}

FORCE_INLINE
bool input_action_exists(const InputKey* const active_keys, int16 key, KeyPressType press_type = KEY_PRESS_TYPE_PRESSED) NO_EXCEPT
{
    return (active_keys[0].scan_code == key && active_keys[0].key_state == press_type)
        || (active_keys[1].scan_code == key && active_keys[1].key_state == press_type)
        || (active_keys[2].scan_code == key && active_keys[2].key_state == press_type)
        || (active_keys[3].scan_code == key && active_keys[3].key_state == press_type)
        || (active_keys[4].scan_code == key && active_keys[4].key_state == press_type)
        || (active_keys[5].scan_code == key && active_keys[5].key_state == press_type)
        || (active_keys[6].scan_code == key && active_keys[6].key_state == press_type);
}

FORCE_INLINE
bool input_is_down(const InputKey* const active_keys, int16 key) NO_EXCEPT
{
    return (active_keys[0].scan_code == key && active_keys[0].key_state != KEY_PRESS_TYPE_RELEASED)
        || (active_keys[1].scan_code == key && active_keys[1].key_state != KEY_PRESS_TYPE_RELEASED)
        || (active_keys[2].scan_code == key && active_keys[2].key_state != KEY_PRESS_TYPE_RELEASED)
        || (active_keys[3].scan_code == key && active_keys[3].key_state != KEY_PRESS_TYPE_RELEASED)
        || (active_keys[4].scan_code == key && active_keys[4].key_state != KEY_PRESS_TYPE_RELEASED)
        || (active_keys[5].scan_code == key && active_keys[5].key_state != KEY_PRESS_TYPE_RELEASED)
        || (active_keys[6].scan_code == key && active_keys[6].key_state != KEY_PRESS_TYPE_RELEASED);
}

FORCE_INLINE
bool input_is_pressed(const InputKey* const active_keys, int16 key) NO_EXCEPT
{
    return (active_keys[0].scan_code == key && active_keys[0].key_state == KEY_PRESS_TYPE_PRESSED)
        || (active_keys[1].scan_code == key && active_keys[1].key_state == KEY_PRESS_TYPE_PRESSED)
        || (active_keys[2].scan_code == key && active_keys[2].key_state == KEY_PRESS_TYPE_PRESSED)
        || (active_keys[3].scan_code == key && active_keys[3].key_state == KEY_PRESS_TYPE_PRESSED)
        || (active_keys[4].scan_code == key && active_keys[4].key_state == KEY_PRESS_TYPE_PRESSED)
        || (active_keys[5].scan_code == key && active_keys[5].key_state == KEY_PRESS_TYPE_PRESSED)
        || (active_keys[6].scan_code == key && active_keys[6].key_state == KEY_PRESS_TYPE_PRESSED);
}

FORCE_INLINE
bool input_is_held(const InputKey* const active_keys, int16 key) NO_EXCEPT
{
    return (active_keys[0].scan_code == key && active_keys[0].key_state == KEY_PRESS_TYPE_HELD)
        || (active_keys[1].scan_code == key && active_keys[1].key_state == KEY_PRESS_TYPE_HELD)
        || (active_keys[2].scan_code == key && active_keys[2].key_state == KEY_PRESS_TYPE_HELD)
        || (active_keys[3].scan_code == key && active_keys[3].key_state == KEY_PRESS_TYPE_HELD)
        || (active_keys[4].scan_code == key && active_keys[4].key_state == KEY_PRESS_TYPE_HELD)
        || (active_keys[5].scan_code == key && active_keys[5].key_state == KEY_PRESS_TYPE_HELD)
        || (active_keys[6].scan_code == key && active_keys[6].key_state == KEY_PRESS_TYPE_HELD);
}

FORCE_INLINE
bool input_is_released(const InputKey* const active_keys, int16 key) NO_EXCEPT
{
    return (active_keys[0].scan_code == key && active_keys[0].key_state == KEY_PRESS_TYPE_RELEASED)
        || (active_keys[1].scan_code == key && active_keys[1].key_state == KEY_PRESS_TYPE_RELEASED)
        || (active_keys[2].scan_code == key && active_keys[2].key_state == KEY_PRESS_TYPE_RELEASED)
        || (active_keys[3].scan_code == key && active_keys[3].key_state == KEY_PRESS_TYPE_RELEASED)
        || (active_keys[4].scan_code == key && active_keys[4].key_state == KEY_PRESS_TYPE_RELEASED)
        || (active_keys[5].scan_code == key && active_keys[5].key_state == KEY_PRESS_TYPE_RELEASED)
        || (active_keys[6].scan_code == key && active_keys[6].key_state == KEY_PRESS_TYPE_RELEASED);
}

FORCE_INLINE
bool input_was_down(const InputKey* const active_keys, int16 key) NO_EXCEPT
{
    return (active_keys[0].scan_code == key && active_keys[0].key_state == KEY_PRESS_TYPE_RELEASED)
        || (active_keys[1].scan_code == key && active_keys[1].key_state == KEY_PRESS_TYPE_RELEASED)
        || (active_keys[2].scan_code == key && active_keys[2].key_state == KEY_PRESS_TYPE_RELEASED)
        || (active_keys[3].scan_code == key && active_keys[3].key_state == KEY_PRESS_TYPE_RELEASED)
        || (active_keys[4].scan_code == key && active_keys[4].key_state == KEY_PRESS_TYPE_RELEASED)
        || (active_keys[5].scan_code == key && active_keys[5].key_state == KEY_PRESS_TYPE_RELEASED)
        || (active_keys[6].scan_code == key && active_keys[6].key_state == KEY_PRESS_TYPE_RELEASED);
}

FORCE_INLINE
bool inputs_are_down(
    const InputKey* const active_keys,
    int16 key0, int16 key1 = 0, int16 key2 = 0, int16 key3 = 0, int16 key4 = 0
) NO_EXCEPT
{
    return (key0 != 0 && input_is_down(active_keys, key0))
        && (key1 == 0 || input_is_down(active_keys, key1))
        && (key2 == 0 || input_is_down(active_keys, key2))
        && (key3 == 0 || input_is_down(active_keys, key3))
        && (key4 == 0 || input_is_down(active_keys, key4));
}

// We are binding hotkeys bi-directional:
//      Which keys are required for a certain hotkey
//      What are the hotkeys a key can trigger
inline void
input_add_hotkey(
    Hotkey* const mapping, uint8 hotkey,
    int16 key0, int16 key1 = 0, int16 key2 = 0,
    KeyPressType press_type = KEY_PRESS_TYPE_PRESSED
) NO_EXCEPT
{
    int32 count = 0;

    // Hotkey enums should start at 1 but in our array we are 0-indexed
    --hotkey;

    Hotkey* key = &mapping[hotkey];

    key->key_state = press_type;

    // Define required keys for hotkey
    // Note: -1 since the hotkeys always MUST start at 1 (0 is a special value for empty)
    key->scan_codes[count++] = key0;
    //key0 = abs(key0);

    if (key1) {
        // Note: -1 since the hotkeys MUST start at 1 (0 is a special value for empty)
        key->scan_codes[count++] = key1;
        //key1 = abs(key1);
    }

    if (key2) {
        // Note: -1 since the hotkeys MUST start at 1 (0 is a special value for empty)
        key->scan_codes[count++] = key2;
        //key2 = abs(key2);
    }
}

inline void
input_add_hotkey(
    Hotkey* const __restrict mapping, uint8 hotkey,
    const Hotkey* const __restrict key
) NO_EXCEPT
{
    // Hotkey enums should start at 1 but in our array we are 0-indexed
    --hotkey;
    memcpy(&mapping[hotkey], key, sizeof(*key));
}

FORCE_INLINE HOT_CODE
bool hotkey_is_active(const uint16* const active_hotkeys, uint16 hotkey) NO_EXCEPT
{
    return active_hotkeys[0] == hotkey
        || active_hotkeys[1] == hotkey
        || active_hotkeys[2] == hotkey
        || active_hotkeys[3] == hotkey
        || active_hotkeys[4] == hotkey;
}

// similar to hotkey_is_active but instead of just performing a lookup in the input_hotkey_state created results
// this is actively checking the current input state (not the hotkey state)
inline HOT_CODE
bool hotkey_keys_are_active(const InputKey* const active_keys, const Hotkey* mapping, uint16 hotkey) NO_EXCEPT
{
    // Hotkeys are 0-indexed but the enum starts at 1
    --hotkey;

    const Hotkey* key = &mapping[hotkey];

    const int16 key0 = key->scan_codes[0];
    const int16 key1 = key->scan_codes[1];
    const int16 key2 = key->scan_codes[2];

    if (!key0 && !key1 && !key2) {
        return false;
    }

    // This may seem a little bit confusing but we don't care if a input key is down or up
    // Any state means it was used recently BUT NOT YET HANDLED
    // If it was handled it would've been removed (at least in case of RELEASED)
    // Therefore, if a key has a state -> treat it as if active

    // The code below also allows optional keys which have a negative sign (at least one of the optional keys must be valid)
    bool is_active = input_action_exists(active_keys, (int16) abs(key0), key->key_state);
    if ((!is_active && (key0 > 0 || key1 >= 0)) || (is_active && key0 < 0) || (key1 == 0 && key2 == 0)) {
        return is_active;
    }

    is_active = input_action_exists(active_keys, (int16) abs(key1), key->key_state);
    if ((!is_active && (key1 > 0 || key2 >= 0)) || (is_active && key1 < 0) || (key2 == 0)) {
        return is_active;
    }

    return input_action_exists(active_keys, (int16) abs(key2), key->key_state);
}

inline HOT_CODE
void input_set_state(InputKey* const __restrict active_keys, const InputKey* const __restrict new_key) NO_EXCEPT
{
    InputKey* free_state = NULL;

    // Insert new key state or change if key already exists
    for (int32 i = 0; i < MAX_KEY_PRESSES; ++i) {
        InputKey* key = &active_keys[i];

        if (!free_state && key->scan_code == 0) {
            free_state = key;
        } else if (key->scan_code == new_key->scan_code) {
            key->key_state = new_key->key_state;
            key->value += new_key->value;
            key->time = new_key->time;

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
void input_set_controller_state(Input* input, ControllerInput* controller, uint64 time) NO_EXCEPT
{
    // Check active keys that might need to be set to inactive
    for (int32 i = 0; i < MAX_KEY_PRESSES; ++i) {
        InputKey* key = &input->state.active_keys[i];

        if ((key->scan_code & INPUT_CONTROLLER_PREFIX)
            && key->key_state != KEY_PRESS_TYPE_RELEASED
        ) {
            const uint32 scan_code = ((uint32) key->scan_code) & ~INPUT_CONTROLLER_PREFIX;

            if ((controller->is_analog[scan_code] && abs(controller->button[scan_code]) < input->deadzone)
                || (!controller->is_analog[scan_code] && controller->button[scan_code] == 0)
            ) {
                key->key_state = KEY_PRESS_TYPE_RELEASED;
            }
        }
    }

    // Special keys
    // @todo this code means we cannot change this behavior (e.g. swap mouse view to dpad, swap sticks, ...)
    // @todo This is also not very general, maybe we can fix it like we did with analog vs digital key (instead of bool flag maybe bit flag)
    if (abs(controller->button[CONTROLLER_BUTTON_STICK_RIGHT_HORIZONTAL]) > input->deadzone) {
        input->state.dx[0] += controller->button[CONTROLLER_BUTTON_STICK_RIGHT_HORIZONTAL] / 8;
        input->general_states |= INPUT_STATE_GENERAL_MOUSE_CHANGE;
    } else {
        input->state.dx[0] = 0;
    }

    if (abs(controller->button[CONTROLLER_BUTTON_STICK_RIGHT_VERTICAL]) > input->deadzone) {
        input->state.dy[0] += controller->button[CONTROLLER_BUTTON_STICK_RIGHT_VERTICAL] / 8;
        input->general_states |= INPUT_STATE_GENERAL_MOUSE_CHANGE;
    } else {
        input->state.dy[0] = 0;
    }

    if (abs(controller->button[CONTROLLER_BUTTON_STICK_LEFT_HORIZONTAL]) > input->deadzone) {
        input->state.dx[1] += controller->button[CONTROLLER_BUTTON_STICK_LEFT_HORIZONTAL] / 8;
        // @todo needs state change flag like mouse?!
    } else {
        input->state.dx[1] = 0;
    }

    if (abs(controller->button[CONTROLLER_BUTTON_STICK_LEFT_HORIZONTAL]) > input->deadzone) {
        input->state.dy[1] += controller->button[CONTROLLER_BUTTON_STICK_LEFT_HORIZONTAL] / 8;
        // @todo needs state change flag like mouse?!
    } else {
        input->state.dy[1] = 0;
    }

    // General Keys
    int32 count = 0;

    // @question Shouldn't the array size be a macro?
    InputKey keys[5];

    for (uint16 i = 0; i < 32; ++i) {
        if ((controller->is_analog[i] && abs(controller->button[i]) > input->deadzone)
            || (!controller->is_analog[i] && controller->button[i] != 0)
        ) {
            keys[count].scan_code = i | INPUT_CONTROLLER_PREFIX;
            keys[count].key_state = KEY_PRESS_TYPE_PRESSED;
            keys[count].value = controller->button[i];
            keys[count].time = time;

            ++count;
        }
    }

    for (int32 i = 0; i < count; ++i) {
        input_set_state(input->state.active_keys, &keys[i]);
    }

    input->general_states |= INPUT_STATE_GENERAL_BUTTON_CHANGE;
}

HOT_CODE
void input_hotkey_state(Input* const input) NO_EXCEPT
{
    // @performance Can't we have a input state that checks if we even have to check the input?
    // careful even no active keys may require a minor update because we need to set it to inactive

    // @todo implement _HELD state based on time
    // @bug Maybe we then need to check if it is still held down through a poll event to avoid bugs when tabbing etc.

    InputState* const state = &input->state;
    memset(state->active_hotkeys, 0, sizeof(uint16) * MAX_KEY_PRESSES);

    InputKey* state_active_keys = state->active_keys;

    // Check if we have any active keys
    if (memcmp(state_active_keys, ((byte *) state_active_keys) + 1, sizeof(*state_active_keys) - 1) == 0) {
        input_clean_state(state_active_keys);
        return;
    }

    // Check typing mode
    if (input->general_states & INPUT_STATE_GENERAL_TYPING_MODE) {
        *input->text = '\0';
        int32 input_characters = 0;
        uint32 characters[10];

        // Create keyboard state array
        byte keyboard_state[256] = {0};
        for (int32 key_state = 0; key_state < ARRAY_COUNT(state->active_keys); ++key_state) {
            const InputKey* const key = &state_active_keys[key_state];
            if (key->scan_code == 0
                || key->key_state == KEY_PRESS_TYPE_RELEASED
            ) {
                // no key defined for this down state
                continue;
            }

            keyboard_state[key->virtual_code & 0x00FF] = 0x80;
        }

        // Check if all keys result in text, if not -> is potential hotkey -> shouldn't output any text
        for (int key_state = 0; key_state < ARRAY_COUNT(state->active_keys); ++key_state) {
            const InputKey* const key = &state_active_keys[key_state];
            if ((input->general_states & INPUT_STATE_GENERAL_TYPING_MODE)
                && (key->scan_code & INPUT_KEYBOARD_PREFIX)
                && key->key_state != KEY_PRESS_TYPE_RELEASED
            ) {
                if (input_characters >= ARRAY_COUNT(characters)) {
                    break;
                }

                const uint32 code = key_to_unicode(
                    key->scan_code & 0x00FF,
                    key->virtual_code & 0x00FF,
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
            for (int key_state = 0; key_state < ARRAY_COUNT(state->active_keys); ++key_state) {
                InputKey* const key = &state_active_keys[key_state];

                key->is_processed = true;
                key->time = 0; // @todo fix
            }

            // Create text from input
            char* pos = input->text;
            for (int i = 0; i < ARRAY_COUNT(characters); ++i) {
                pos += utf8_decode(characters[i], pos);
            }

            *pos = '\0';

            input_clean_state(state_active_keys);
            return;
        }
    }

    int32 active_hotkeys = 0;

    // Check every mapping
    for (int32 i = 0; i < 2; ++i) {
        const Hotkey* const mapping = i == 0 ? input->input_mapping1 : input->input_mapping2;

        // Check all possible hotkeys if all of their required keys are active
        for (int16 hotkey_idx = 1; hotkey_idx <= input->hotkey_count; ++hotkey_idx) {
            // We only support a limited amount of active hotkeys
            if (active_hotkeys >= MAX_KEY_PRESSES) { UNLIKELY
                i = 2;
                break;
            }

            if (hotkey_is_active(state->active_hotkeys, hotkey_idx)
                || !hotkey_keys_are_active(state->active_keys, mapping, hotkey_idx)
            ) {
                // Hotkey already active, we don't need to check if it needs to be activated
                // Or not all keys for the hotkey are pressed or the KeyPressType is not the same
                continue;
            }

            state->active_hotkeys[active_hotkeys++] = hotkey_idx;
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
inline
bool input_key_is_longpress(const InputState* state, int16 key, uint64 time, f32 dt = 0.0f) NO_EXCEPT
{
    for (int i = 0; i < ARRAY_COUNT(state->active_keys); ++i) {
        if (state->active_keys[i].scan_code == key) {
            return (f32) (time - state->active_keys[i].time) / 1000.0f >= (dt == 0.0f ? INPUT_LONG_PRESS_DURATION : dt);
        }
    }

    return false;
}

// @todo I wrote this code at 9am after staying awake for the whole night and that is how that code looks like... fix it!
bool input_hotkey_is_longpress(const Input* input, uint8 hotkey, uint64 time, f32 dt = 0.0f) NO_EXCEPT
{
    bool is_longpress = false;
    for (int i = 0; i < ARRAY_COUNT(input->state.active_hotkeys); ++i) {
        if (input->state.active_hotkeys[i] != hotkey) {
            continue;
        }

        is_longpress = true;

        for (int j = 0; j < MAX_HOTKEY_COMBINATION; ++j) {
            bool potential_miss = true;
            bool both_empty = false;
            if (input->input_mapping1[hotkey].scan_codes[j] > 0) {
                if(!input_key_is_longpress(&input->state, input->input_mapping1[hotkey].scan_codes[j], time, dt)) {
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

            if (input->input_mapping2[hotkey].scan_codes[j] > 0) {
                if(!input_key_is_longpress(&input->state, input->input_mapping2[hotkey].scan_codes[j], time, dt)) {
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

inline
uint32 input_get_typed_character(InputState* state, uint64 time, uint64 dt) NO_EXCEPT
{
    byte keyboard_state[256] = {0};
    for (int32 key_state = 0; key_state < ARRAY_COUNT(state->active_keys); ++key_state) {
        if (state->active_keys[key_state].scan_code == 0
            || state->active_keys[key_state].key_state == KEY_PRESS_TYPE_RELEASED
        ) {
            // no key defined for this down state
            continue;
        }

        keyboard_state[state->active_keys[key_state].virtual_code & 0x00FF] = 0x80;
    }

    for (int32 key_state = 0; key_state < ARRAY_COUNT(state->active_keys); ++key_state) {
        if (state->active_keys[key_state].scan_code == 0
            || (state->active_keys[key_state].is_processed
                && state->active_keys[key_state].time - time <= dt)
        ) {
            // no key defined for this down state
            // key is already released
            // key was already processed and is not yet eligible for continuous output
            continue;
        }

        const uint32 code = key_to_unicode(
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

inline
void input_handle_hotkeys(const Input* const input, void* data) NO_EXCEPT {
    // @question One hotkey can trigger one function, do we want multiple functions per hotkey?
    const InputEvent* input_events[MAX_KEY_PRESSES] = {0};
    int32 input_event_count = 0;

    // Identify possible input events
    for (int32 i = 0; i < MAX_KEY_PRESSES; ++i) {
        if (!input->state.active_hotkeys[i]) {
            continue;
        }

        int32 j = 0;
        for (; j < input_event_count; ++j) {
            if (input_events[j]
                && input_events[j] == input->hotkey_event_list[input->state.active_hotkeys[i]]
            ) {
                break;
            }
        }

        if ((j < input_event_count && (input_events[j]->flag & INPUT_EVENT_FLAG_MULTIPLE))
            || j >= input_event_count
        ) {
            // Input event already exists, check if we are allowed to add it another time
            // OR, First time input event is added
            input_events[input_event_count++] = input->hotkey_event_list[input->state.active_hotkeys[i]];
        }
    }

    // Run all input events
    for (int32 i = 0; i < input_event_count; ++i) {
        // @bug The order of the hotkeys is not based on timing, that could potentially be an issue
        // @performance Instead of doing pointer chasing maybe we should have one index array and a reference to the event array
        // @bug we need to pass the input id but we only have the input pointer. Currently always passing 0
        input_events[i]->callback(data, 0);
    }
}

#endif