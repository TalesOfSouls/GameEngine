/**
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_INPUT_C
#define COMS_INPUT_C

#include "../stdlib/Stdlib.h"
#include "../utils/BitUtils.h"
#include "../utils/StringUtils.h"
#include "../memory/BufferMemory.cpp"
#include "ControllerInput.h"
#include "InputConnectionType.h"
#include "Input.h"

#ifdef _WIN32
    #include "../platform/win32/UtilsWin32.h"
#endif

FORCE_INLINE
Input* input_kbm_find(Input* input, int count) NO_EXCEPT
{
    for (int i = 0; i < count; ++i) {
        if (input[i].mouse.handle[0]) {
            return &input[i];
        }
    }

    return NULL;
}

FORCE_INLINE
v2_int16 input_keyboard_find_by_handle(Input* input, int count, InputHandle handle) NO_EXCEPT
{
    for (int16 i = 0; i < count; ++i) {
        for (int16 h = 0; h < ARRAY_COUNT(input[i].keyboard.handle); ++h) {
            if (input[i].keyboard.handle[h] == handle) {
                return {i, h};
            }
        }
    }

    return {-1, -1};
}

FORCE_INLINE
v2_int16 input_mouse_find_by_handle(Input* input, int count, InputHandle handle) NO_EXCEPT
{
    for (int16 i = 0; i < count; ++i) {
        for (int16 h = 0; h < ARRAY_COUNT(input[i].mouse.handle); ++h) {
            if (input[i].mouse.handle[h] == handle) {
                return {i, h};
            }
        }
    }

    return {-1, -1};
}

FORCE_INLINE
v2_int16 input_controller_find_by_handle(Input* input, int count, InputHandle handle) NO_EXCEPT
{
    for (int16 i = 0; i < count; ++i) {
        if (input[i].controller.handle == handle) {
            return {i, 0};
        }
    }

    return {-1, -1};
}

FORCE_INLINE CONSTEXPR
size_t input_memory_size(uint8 count) NO_EXCEPT
{
    return count * sizeof(Hotkey) * 2;
}

// count = count of possible hotkeys
inline
void input_init(Input* const input, uint8 count, BufferMemory* const buf) NO_EXCEPT
{
    input->hotkey_count = count;
    input->managed_contexts = HOTKEY_CONTEXT_UI;

    const size_t hotkey_size = input_memory_size(input->hotkey_count);

    input->input_mapping1 = (Hotkey *) memory_get(buf, hotkey_size, alignof(Hotkey));
    input->input_mapping2 = input->input_mapping1 + input->hotkey_count;

    // This clears both mapping1 and mapping2
    memset(input->input_mapping1, 0, hotkey_size);

    DEBUG_MEMORY_SUBREGION((uintptr_t) input->input_mapping1, hotkey_size);
}

// Resets the mapping
// Usually used for re-assigning hotkeys
FORCE_INLINE
void input_mapping_reset(Input* const input) NO_EXCEPT
{
    // This clears both mapping1 and mapping2
    memset(input->input_mapping1, 0, input_memory_size(input->hotkey_count));
    memset(&input->state, 0, sizeof(input->state));
}

/**
 * Removes keys from the active keys
 *
 *      KEY_PRESS_TYPE_RELEASED = keys that were released (incl. PRESSED | RELEASED), @see input_frame_begin
 *      KEY_PRESS_TYPE_NONE     = ALL keys, hard reset (e.g. window lost focus, window changes, hotkey remapping)
 *                                otherwise keys could get stuck since we never receive their release event
 */
inline
void input_clean_state(InputKey* const active_keys, KeyPressType press_status = KEY_PRESS_TYPE_RELEASED) NO_EXCEPT
{
    if (press_status) {
        for (int32 i = 0; i < MAX_KEY_PRESSES; ++i) {
            if (active_keys[i].key_state & press_status) {
                memset(&active_keys[i], 0, sizeof(InputKey));
            }
        }
    } else {
        memset(active_keys, 0, MAX_KEY_PRESSES * sizeof(InputKey));
    }
}

/**
 * MUST be called once per tick for every input BEFORE the new input events are applied
 *
 *      - keys released in the last tick are removed
 *      - keys pressed in the last tick become held -> KEY_PRESS_TYPE_PRESSED is a real "went down this tick" edge
 *      - per tick values (mouse/stick delta, typed text, change flag) are reset
 *
 * This way everybody in the same tick (hotkeys, ui, game logic) sees the same key states incl. releases
 */
inline
void input_frame_begin(Input* const input) NO_EXCEPT
{
    InputState* const state = &input->state;

    // Keys released last tick are gone
    input_clean_state(state->active_keys, KEY_PRESS_TYPE_RELEASED);

    // Keys pressed last tick are now held
    for (int32 i = 0; i < MAX_KEY_PRESSES; ++i) {
        if (state->active_keys[i].key_state & KEY_PRESS_TYPE_PRESSED) {
            state->active_keys[i].key_state = KEY_PRESS_TYPE_HELD;
        }
    }

    memset(state->dx, 0, sizeof(state->dx));
    memset(state->dy, 0, sizeof(state->dy));
    state->text_count = 0;

    input->general_states &= ~INPUT_STATE_GENERAL_INPUT_CHANGE;
}

// @param exclude_flags Keys with any of these InputKeyFlag are ignored (e.g. text keys while typing)
FORCE_INLINE
bool input_action_exists(
    const InputKey* const active_keys, int16 key,
    byte press_type = KEY_PRESS_TYPE_PRESSED, byte exclude_flags = 0
) NO_EXCEPT
{
    for (int32 i = 0; i < MAX_KEY_PRESSES; ++i) {
        if (active_keys[i].scan_code == (uint16) key
            && (active_keys[i].key_state & press_type)
            && !(active_keys[i].flags & exclude_flags)
        ) {
            return true;
        }
    }

    return false;
}

FORCE_INLINE
bool input_action_processed(const InputKey* const active_keys, int16 key) NO_EXCEPT
{
    for (int32 i = 0; i < MAX_KEY_PRESSES; ++i) {
        if (active_keys[i].scan_code == (uint16) key && (active_keys[i].flags & INPUT_KEY_FLAG_PROCESSED)) {
            return true;
        }
    }

    return false;
}

// Active key slot of a key (NULL = not active)
FORCE_INLINE
const InputKey* input_key_find(const InputKey* const active_keys, int16 key) NO_EXCEPT
{
    for (int32 i = 0; i < MAX_KEY_PRESSES; ++i) {
        if (active_keys[i].scan_code == (uint16) key) {
            return &active_keys[i];
        }
    }

    return NULL;
}

FORCE_INLINE
bool input_is_down(const InputKey* const active_keys, int16 key) NO_EXCEPT
{
    const InputKey* const k = input_key_find(active_keys, key);
    return k && !(k->key_state & KEY_PRESS_TYPE_RELEASED);
}

FORCE_INLINE
bool input_is_pressed(const InputKey* const active_keys, int16 key) NO_EXCEPT
{
    const InputKey* const k = input_key_find(active_keys, key);
    return k && (k->key_state & KEY_PRESS_TYPE_PRESSED);
}

FORCE_INLINE
bool input_is_held(const InputKey* const active_keys, int16 key) NO_EXCEPT
{
    const InputKey* const k = input_key_find(active_keys, key);
    return k && (k->key_state & KEY_PRESS_TYPE_HELD);
}

FORCE_INLINE
bool input_is_released(const InputKey* const active_keys, int16 key) NO_EXCEPT
{
    const InputKey* const k = input_key_find(active_keys, key);
    return k && (k->key_state & KEY_PRESS_TYPE_RELEASED);
}

// Value of an active key (e.g. mouse wheel movement, stick value), 0 = key not active
FORCE_INLINE
int16 input_key_value(const InputKey* const active_keys, int16 key) NO_EXCEPT
{
    for (int32 i = 0; i < MAX_KEY_PRESSES; ++i) {
        if (active_keys[i].scan_code == key) {
            return active_keys[i].value;
        }
    }

    return 0;
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
    byte press_type = KEY_PRESS_TYPE_PRESSED
) NO_EXCEPT
{
    // Hotkey enums should start at 1 but in our array we are 0-indexed
    --hotkey;

    Hotkey* key = &mapping[hotkey];

    key->key_state = press_type;

    // Define required keys for hotkey
    int32 count = 0;
    key->scan_codes[count++] = key0;

    if (key1) {
        key->scan_codes[count++] = key1;
    }

    if (key2) {
        key->scan_codes[count++] = key2;
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
    for (int32 i = 0; i < MAX_KEY_PRESSES; ++i) {
        if (active_hotkeys[i] == hotkey) {
            return true;
        }
    }

    return false;
}

FORCE_INLINE
int32 hotkey_key_count(const Hotkey* const key) NO_EXCEPT
{
    return (key->scan_codes[0] != 0) + (key->scan_codes[1] != 0) + (key->scan_codes[2] != 0);
}

/**
 * State of the keys of a hotkey independent from the hotkey evaluation (@see input_hotkey_state_update)
 * Useful for hotkeys that need the full press/hold/release cycle (e.g. ui pointer button, drag & drop)
 * Both mappings are checked, the first mapping where all keys are active wins
 *
 * @return KeyPressType flags:
 *      PRESSED  = all keys are down and at least one went down this tick
 *      HELD     = all keys are down
 *      RELEASED = all keys are active and at least one went up this tick
 *      0        = not active
 */
inline
byte input_hotkey_key_state(const Input* const input, uint16 hotkey) NO_EXCEPT
{
    if (!hotkey || hotkey > input->hotkey_count) {
        return 0;
    }

    const Hotkey* const mappings[] = { &input->input_mapping1[hotkey - 1], &input->input_mapping2[hotkey - 1] };
    for (int32 m = 0; m < 2; ++m) {
        const Hotkey* const key = mappings[m];

        byte state = KEY_PRESS_TYPE_HELD;
        bool any = false;
        for (int32 i = 0; i < MAX_HOTKEY_COMBINATION; ++i) {
            if (!key->scan_codes[i]) {
                continue;
            }

            const InputKey* const active = input_key_find(input->state.active_keys, key->scan_codes[i]);
            if (!active) {
                any = false;
                break;
            }

            any = true;
            state = (byte) (state | (active->key_state & (KEY_PRESS_TYPE_PRESSED | KEY_PRESS_TYPE_RELEASED)));

            // A released key is not down anymore
            if ((active->key_state & KEY_PRESS_TYPE_RELEASED) && !(active->key_state & KEY_PRESS_TYPE_PRESSED)) {
                state = (byte) (state & ~KEY_PRESS_TYPE_HELD);
            }
        }

        if (any) {
            // pressed AND released within one tick -> not held
            if (state & KEY_PRESS_TYPE_RELEASED) {
                state = (byte) (state & ~KEY_PRESS_TYPE_HELD);
            }

            return state;
        }
    }

    return 0;
}

// Hotkey keys are currently down (pressed or held)
FORCE_INLINE
bool input_hotkey_is_down(const Input* const input, uint16 hotkey) NO_EXCEPT
{
    const byte state = input_hotkey_key_state(input, hotkey);
    return (state & KEY_PRESS_TYPE_HELD) || ((state & KEY_PRESS_TYPE_PRESSED) && !(state & KEY_PRESS_TYPE_RELEASED));
}

/**
 * Value of the first key of a hotkey (e.g. mouse wheel delta, stick deflection)
 *
 * @return 0 if the hotkey is not active
 */
inline
int16 input_hotkey_value(const Input* const input, uint16 hotkey, int16* const scan_code = NULL) NO_EXCEPT
{
    if (!hotkey || hotkey > input->hotkey_count) {
        return 0;
    }

    const Hotkey* const mappings[] = { &input->input_mapping1[hotkey - 1], &input->input_mapping2[hotkey - 1] };
    for (int32 m = 0; m < 2; ++m) {
        for (int32 i = 0; i < MAX_HOTKEY_COMBINATION; ++i) {
            const int16 code = mappings[m]->scan_codes[i];
            if (!code) {
                continue;
            }

            const InputKey* const active = input_key_find(input->state.active_keys, code);
            if (active) {
                if (scan_code) {
                    *scan_code = code;
                }

                return active->value;
            }
        }
    }

    return 0;
}

// similar to hotkey_is_active but instead of just performing a lookup in the input_hotkey_state created results
// this is actively checking the current input state (not the hotkey state)
//
// A hotkey is active if ALL of its keys are down (or in the hotkey key state)
// and at least ONE key is in the hotkey key state
//      e.g. shift + tab (PRESSED): shift is held since a while, tab was pressed this tick -> active
inline HOT_CODE
bool hotkey_keys_are_active(
    const InputKey* const __restrict active_keys,
    const Hotkey* const __restrict mapping,
    uint16 hotkey,
    byte exclude_flags = 0
) NO_EXCEPT
{
    // Hotkeys are 0-indexed but the enum starts at 1
    --hotkey;

    const Hotkey* key = &mapping[hotkey];

    bool any_in_state = false;
    bool has_keys = false;
    for (int32 i = 0; i < MAX_HOTKEY_COMBINATION; ++i) {
        const int16 code = key->scan_codes[i];
        if (!code) {
            continue;
        }

        has_keys = true;

        const InputKey* const active = input_key_find(active_keys, code);
        if (!active || (active->flags & exclude_flags)) {
            return false;
        }

        const bool in_state = (active->key_state & key->key_state) != 0;
        const bool down = !(active->key_state & KEY_PRESS_TYPE_RELEASED) || (active->key_state & KEY_PRESS_TYPE_PRESSED);
        if (!in_state && !down) {
            return false;
        }

        any_in_state |= in_state;
    }

    return has_keys && any_in_state;
}

inline HOT_CODE
bool hotkey_keys_already_processed(
    const InputKey* const __restrict active_keys,
    const Hotkey* const __restrict mapping,
    uint16 hotkey
) NO_EXCEPT
{
    // Hotkeys are 0-indexed but the enum starts at 1
    --hotkey;

    const Hotkey* key = &mapping[hotkey];
    // If the hotkey allows held state (instead of just press/release),
    // the key is never considered processed
    if (key->key_state & KEY_PRESS_TYPE_HELD) {
        return false;
    }

    const int16 key0 = key->scan_codes[0];
    const int16 key1 = key->scan_codes[1];
    const int16 key2 = key->scan_codes[2];

    return (key0 || key1 || key2)
        && input_action_processed(active_keys, key0)
        && (!key1 || input_action_processed(active_keys, key1))
        && (!key2 || input_action_processed(active_keys, key2));
}

inline HOT_CODE
void hotkey_set_processed(
    InputKey* const __restrict active_keys,
    const Hotkey* const __restrict mapping,
    uint16 hotkey
) NO_EXCEPT
{
    // Hotkeys are 0-indexed but the enum starts at 1
    --hotkey;

    const Hotkey* key = &mapping[hotkey];
    const int16 key0 = key->scan_codes[0];
    const int16 key1 = key->scan_codes[1];
    const int16 key2 = key->scan_codes[2];

    for (int i = 0; i < MAX_KEY_PRESSES; ++i) {
        if ((key0 && key0 == active_keys[i].scan_code)
            || (key1 && key1 == active_keys[i].scan_code)
            || (key2 && key2 == active_keys[i].scan_code)
        ) {
            active_keys[i].flags |= INPUT_KEY_FLAG_PROCESSED;
        }
    }
}

/**
 * Applies a key event to the active keys (@see InputKey for the key state lifecycle)
 *
 * @return The active key (NULL if all key slots are in use)
 */
inline HOT_CODE
InputKey* input_set_state(
    InputKey* const __restrict active_keys,
    const InputKey* const __restrict new_key
) NO_EXCEPT
{
    InputKey* free_state = NULL;

    // Insert new key state or change if key already exists
    for (int i = 0; i < MAX_KEY_PRESSES; ++i) {
        InputKey* key = &active_keys[i];

        if (!free_state && key->scan_code == 0) {
            free_state = key;
        } else if (key->scan_code == new_key->scan_code) {
            if (new_key->key_state & KEY_PRESS_TYPE_PRESSED) {
                // Key repeat (e.g. OS key repeat, controllers report pressed buttons on every poll)
                // A key that went down this tick stays PRESSED, otherwise it is HELD
                // A key that was released this tick and pressed again is PRESSED again
                if (key->key_state & KEY_PRESS_TYPE_RELEASED) {
                    key->key_state = KEY_PRESS_TYPE_PRESSED;
                    key->flags = 0;
                } else if (!(key->key_state & KEY_PRESS_TYPE_PRESSED)) {
                    key->key_state = KEY_PRESS_TYPE_HELD;
                }
            } else if (new_key->key_state & KEY_PRESS_TYPE_RELEASED) {
                // Pressed AND released within this tick -> keep the press, otherwise it would get lost
                key->key_state = (key->key_state & KEY_PRESS_TYPE_PRESSED)
                    ? (KEY_PRESS_TYPE_PRESSED | KEY_PRESS_TYPE_RELEASED)
                    : KEY_PRESS_TYPE_RELEASED;
            } else {
                key->key_state = new_key->key_state;
            }

            key->value += new_key->value;
            key->time = new_key->time;

            return key;
        }
    }

    if (!free_state) {
        return NULL;
    }

    memcpy(free_state, new_key, sizeof(*new_key));

    return free_state;
}


inline HOT_CODE
bool input_has_active_keys(InputKey* const __restrict active_keys) NO_EXCEPT {
    for (int i = 0; i < MAX_KEY_PRESSES; ++i) {
        if (active_keys[i].scan_code && active_keys[i].key_state) {
            return true;
        }
    }

    return false;
}

// Default controller pointer axes: right stick = dx/dy[0] (camera, virtual cursor), left stick = dx/dy[1] (movement)
FORCE_INLINE
void input_controller_axes_default(Input* const input) NO_EXCEPT
{
    input->controller_axes[0] = CONTROLLER_BUTTON_STICK_RIGHT_HORIZONTAL;
    input->controller_axes[1] = CONTROLLER_BUTTON_STICK_RIGHT_VERTICAL;
    input->controller_axes[2] = CONTROLLER_BUTTON_STICK_LEFT_HORIZONTAL;
    input->controller_axes[3] = CONTROLLER_BUTTON_STICK_LEFT_VERTICAL;
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
    for (int i = 0; i < MAX_KEY_PRESSES; ++i) {
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

    // Pointer movement axes (e.g. right stick = camera/virtual cursor, left stick = movement)
    // The mapping is part of the input (@see Input::controller_axes), the default is set by input_controller_axes_default()
    for (int32 axis = 0; axis < 4; ++axis) {
        const byte button = input->controller_axes[axis];
        if (button >= CONTROLLER_BUTTON_SIZE) {
            continue;
        }

        int16* const delta = (axis & 1) ? &input->state.dy[axis >> 1] : &input->state.dx[axis >> 1];
        if (abs(controller->button[button]) > input->deadzone) {
            *delta = (int16) (*delta + controller->button[button] / 8);
            input->general_states |= INPUT_STATE_GENERAL_INPUT_CHANGE;
        } else {
            *delta = 0;
        }
    }

    // General Keys
    // Every button/axis is a key -> it can be used in hotkeys (e.g. shoulder button = next ui element)
    for (uint16 i = 0; i < CONTROLLER_BUTTON_SIZE; ++i) {
        if ((controller->is_analog[i] && abs(controller->button[i]) > input->deadzone)
            || (!controller->is_analog[i] && controller->button[i] != 0)
        ) {
            InputKey key = {};
            key.scan_code = i | INPUT_CONTROLLER_PREFIX;
            key.key_state = KEY_PRESS_TYPE_PRESSED;
            key.value = controller->button[i];
            key.time = time;

            // Analog values are absolute, they must not accumulate (@see input_set_state)
            InputKey* const active = input_set_state(input->state.active_keys, &key);
            if (active) {
                active->value = controller->button[i];
            }
        }
    }

    input->general_states |= INPUT_STATE_GENERAL_INPUT_CHANGE;
}

HOT_CODE
int32 input_hotkey_state_update(Input* const input, uint64 /* time */ = 0) NO_EXCEPT
{
    PROFILE_DEBUG(PROFILE_INPUT_UPDATE);

    // @todo implement _HELD state based on time
    // @bug Maybe we then need to check if it is still held down through a poll event to avoid bugs when tabbing etc.

    InputState* const state = &input->state;
    memset(state->active_hotkeys, 0, sizeof(uint16) * ARRAY_COUNT(state->active_hotkeys));

    // While typing, keys that produce text don't trigger hotkeys (e.g. "w" = move forward)
    // Other keys (e.g. F1, ctrl+c) still work
    const byte exclude_flags = (input->context & HOTKEY_CONTEXT_TYPING) ? INPUT_KEY_FLAG_TEXT : 0;

    if ((input->context & HOTKEY_CONTEXT_TYPING) && input->text) {
        // Legacy text buffer (e.g. console), contains the printable text typed this tick
        char* pos = input->text;
        for (int32 i = 0; i < state->text_count; ++i) {
            if (state->text[i] >= 0x20 && state->text[i] != INPUT_TEXT_DELETE && state->text[i] < 0x110000) {
                pos += utf8_decode(state->text[i], pos);
            }
        }

        *pos = '\0';
    }

    int32 active_hotkeys = 0;

    // Priority (one key can only trigger one non-held hotkey @see INPUT_KEY_FLAG_PROCESSED):
    //      1. hotkeys of the current context before general hotkeys (context = 0)
    //          e.g. tab = next ui element while the ui is focused, otherwise tab = tab target
    //      2. hotkeys with more keys before hotkeys with less keys
    //          e.g. shift + tab = previous ui element, NOT tab = next ui element
    for (int32 pass = 0; pass < 2 * MAX_HOTKEY_COMBINATION; ++pass) {
        const bool context_pass = pass < MAX_HOTKEY_COMBINATION;
        const int32 key_count = MAX_HOTKEY_COMBINATION - (pass % MAX_HOTKEY_COMBINATION);

        for (int16 hotkey_idx = 1; hotkey_idx <= input->hotkey_count; ++hotkey_idx) {
            if (active_hotkeys >= MAX_KEY_PRESSES) UNLIKELY {
                break;
            }

            const Hotkey* const mappings[] = {
                &input->input_mapping1[hotkey_idx - 1],
                &input->input_mapping2[hotkey_idx - 1]
            };

            for (int32 m = 0; m < 2; ++m) {
                const Hotkey* const key = mappings[m];

                // Only hotkeys that exclusively use managed contexts are context hotkeys
                // Unused/unmanaged context values (e.g. 0xFF or HOTKEY_CONTEXT_EDITOR_MODE in older configs) = general hotkey
                const byte context = (key->context & ~input->managed_contexts) ? 0 : key->context;
                if ((context != 0) != context_pass
                    || (context && !(context & input->context))
                    || hotkey_key_count(key) != key_count
                ) {
                    continue;
                }

                const Hotkey* const mapping = m == 0 ? input->input_mapping1 : input->input_mapping2;
                if (!hotkey_keys_are_active(state->active_keys, mapping, hotkey_idx, exclude_flags)) {
                    continue;
                }

                if (!hotkey_keys_already_processed(state->active_keys, mapping, hotkey_idx)
                    && !hotkey_is_active(state->active_hotkeys, hotkey_idx)
                ) {
                    state->active_hotkeys[active_hotkeys++] = hotkey_idx;
                    hotkey_set_processed(state->active_keys, mapping, hotkey_idx);
                }

                // The second mapping is only an alternative
                break;
            }
        }
    }

    // WARNING: Released keys are NOT removed here, they stay visible for the rest of the tick (e.g. ui click handling)
    //          They are removed by input_frame_begin() at the beginning of the next tick

    // @bug how to handle values (e.g. stick may or may not set the x/y or dx/dy in some situations)
    // @bug how to allow rebinding/swapping of left and right stick? (maybe create handful of events e.g. set dx/dy that fire based on the input?)
    // @bug There is a bug ONLY with the controller, when doing camera look around and holding the stick at and angle
    //          The hotkey seemingly loses activity after 1-2 sec if you then move the stick a little bit it works again
    //          It doesn't always happen but you can test it rather consistently within a couple of seconds

    return active_hotkeys;
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
                potential_miss = !input_key_is_longpress(
                    &input->state,
                    input->input_mapping1[hotkey].scan_codes[j],
                    time,
                    dt
                );

                if (!potential_miss) {
                    continue;
                }
            } else {
                both_empty = true;
            }

            if (input->input_mapping2[hotkey].scan_codes[j] > 0) {
                potential_miss = !input_key_is_longpress(
                    &input->state,
                    input->input_mapping2[hotkey].scan_codes[j],
                    time,
                    dt
                );
            } else {
                both_empty &= true;
            }

            if (!both_empty && potential_miss) {
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
            || ((state->active_keys[key_state].flags & INPUT_KEY_FLAG_PROCESSED)
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
            state->active_keys[key_state].flags |= INPUT_KEY_FLAG_PROCESSED;
            state->active_keys[key_state].time = time;

            return code;
        }
    }

    return 0;
}

/**
 * Runs the events of the active hotkeys
 *
 * @param input_id Index of the input (passed to the event callbacks, @see InputCallback)
 */
inline
void input_hotkeys_handle(const Input* const input, void* data, int32 input_id = 0) NO_EXCEPT
{
    PROFILE_DEBUG(PROFILE_INPUT_HANDLE);

    const InputEvent* input_events[MAX_KEY_PRESSES] = {0};
    int32 input_event_count = 0;

    // Identify possible input events
    for (int i = 0; i < ARRAY_COUNT(input_events); ++i) {
        const uint16 active_hotkey = input->state.active_hotkeys[i];

        const InputEvent* active_event = &input->hotkey_event_list[active_hotkey];
        if (!active_hotkey || !active_event->callback) {
            // This does NOT mean a hotkey that the user unbound
            // Tt means a hotkey is not assigned a function
            // Which doesn't make sense because it would mean the hotkey exists but never does anything.
            ASSERT_TRUE(!active_hotkey || (active_hotkey && !active_event->callback));

            continue;
        }

        int32 j = 0;
        // Checks if the input event is already added to the events to run or not
        for (; j < input_event_count; ++j) {
            if (input_events[j] == active_event) {
                break;
            }
        }

        // Input event already exists, check if we are allowed to add it another time (INPUT_EVENT_FLAG_MULTIPLE)
        // OR, First time input event is added (j >= input_event_count)
        if ((j < input_event_count && (input_events[j]->flag & INPUT_EVENT_FLAG_MULTIPLE))
            || j >= input_event_count
        ) {
            input_events[input_event_count++] = active_event;

            // @bug The order of the hotkeys is not based on timing, that could potentially be an issue
            active_event->callback(data, input_id);
        }
    }
}

#endif