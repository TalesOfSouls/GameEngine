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
#include "InputConnectionType.h"

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

// @todo This should probably be a setting
#define INPUT_LONG_PRESS_DURATION 250

enum InputMouseAction {
    INPUT_MOUSE_BUTTON_1 = 1,
    INPUT_MOUSE_BUTTON_2 = 2,
    INPUT_MOUSE_BUTTON_3 = 3,
    INPUT_MOUSE_BUTTON_4 = 4,
    INPUT_MOUSE_BUTTON_5 = 5,
    INPUT_MOUSE_BUTTON_WHEEL = 6,
    INPUT_MOUSE_BUTTON_HWHEEL = 7,
    INPUT_MOUSE_WHEEL_UP = 8,
    INPUT_MOUSE_WHEEL_DOWN = 9,
    INPUT_MOUSE_HWHEEL_LEFT = 10,
    INPUT_MOUSE_HWHEEL_RIGHT = 11,
};

/**
 * @param void* data General data to pass to the event function
 * @param int32 input_id Input state that triggered this callback
 */
typedef void (*InputCallback)(void* data, int32 input_id);

enum InputEventFlag : uint8 {
    INPUT_EVENT_FLAG_NONE = 0,

    // can run multiple times per frame/input handling
    INPUT_EVENT_FLAG_MULTIPLE = 1 << 0,

    // not getting removed after run
    INPUT_EVENT_FLAG_REPEAT = 1 << 1,
};

struct InputEvent {
    // InputEventFlag
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

// @todo Do we even want to use these input states, I don't really see there use case
//      Instead the typing mode etc. we may want to use the context
enum GeneralInputState : byte {
    INPUT_STATE_GENERAL_BUTTON_CHANGE = 1 << 0,
    INPUT_STATE_GENERAL_MOUSE_CHANGE = 1 << 1,
    INPUT_STATE_GENERAL_MOUSE_MOVEMENT = 1 << 2,

    INPUT_STATE_GENERAL_TYPING_MODE = 1 << 3, // Used for typing in chat box etc. otherwise we could have conflicts with hotkeys
    INPUT_STATE_GENERAL_HOTKEY_ACTIVE = 1 << 4, // At least one hotkey is active
};

#ifdef _WIN32
    #include <windows.h>
    #include <dinput.h>
#endif

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

#endif