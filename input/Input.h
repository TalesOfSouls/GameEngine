/**
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_INPUT_H
#define COMS_INPUT_H

#include "../stdlib/Stdlib.h"
#include "InputConnectionType.h"
#include "ControllerType.h"
#include "ControllerInput.h"

// How many concurrent key/button presses can be handled (per input device)?
// Realistic worst cases are higher than it seems:
//      keyboard + mouse: 2 movement keys (diagonal) + modifier + skill key + 2 mouse buttons
//                        + mouse move + wheel (both are "fake" keys for one tick) = 8
//      controller:       every analog axis outside of the deadzone is an active key
//                        -> 2 sticks (4 axes) + 2 triggers + 2 buttons = 8
// Released keys also stay in their slot until the next tick (@see input_frame_begin).
#define MAX_KEY_PRESSES 16

// How many keys/buttons do we support for the devices
#define MAX_KEYBOARD_KEYS 255
#define MAX_MOUSE_KEYS 7

// How many buttons together are allowed to form a hotkey
#define MAX_HOTKEY_COMBINATION 3

// These values are used as bit flags to hint32 if a "key" is a keyboard/primary or mouse/secondary input
// When adding a keybind the "key" can only be uint8 but we expand it to an int and set the first bit accordingly
// IMPORTANT: The parentheses are required, e.g. ~INPUT_CONTROLLER_PREFIX would otherwise be (~1U) << 14
#define INPUT_MOUSE_PREFIX 0
#define INPUT_KEYBOARD_PREFIX (1U << 13)
#define INPUT_CONTROLLER_PREFIX (1U << 14)

enum InputType : byte {
    INPUT_TYPE_NONE = 0,
    INPUT_TYPE_MOUSE_KEYBOARD = 1 << 0,
    INPUT_TYPE_CONTROLLER = 1 << 1,
    INPUT_TYPE_OTHER = 1 << 2,
};

// Physical device kinds that can be (dis-)connected individually
// One Input may have one device of every kind (e.g. keyboard + mouse + controller of the same player)
enum InputDeviceKind : byte {
    INPUT_DEVICE_KIND_KEYBOARD,
    INPUT_DEVICE_KIND_MOUSE,
    INPUT_DEVICE_KIND_CONTROLLER,
};

// @todo This should probably be a setting
#define INPUT_LONG_PRESS_DURATION 250

enum InputMouseAction : uint8 {
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
    INPUT_MOUSE_MOVE = 12,
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

enum KeyPressType : byte {
    KEY_PRESS_TYPE_NONE = 0,
    KEY_PRESS_TYPE_PRESSED = 1 << 0,
    KEY_PRESS_TYPE_HELD = 1 << 1,
    KEY_PRESS_TYPE_RELEASED = 1 << 2,
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

    // The ui has the keyboard/controller focus (e.g. tab navigation, open popup)
    // UI navigation hotkeys (next/prev element, confirm, ...) are usually bound to this context
    // so the same keys can be used for game hotkeys while the ui isn't focused (e.g. tab = tab target)
    HOTKEY_CONTEXT_UI = 1 << 5,

    // @question Do I even need this?
    HOTKEY_CONTEXT_EDITOR_MODE = 1 << 6,
    HOTKEY_CONTEXT_ADMIN_MODE = 1 << 7
};

struct Hotkey {
    // negative hotkeys mean any of them needs to match, positive hotkeys means all of them need to match
    // mixing positive and negative keys for one hotkey is not possible
    // index = hotkey, value = key id
    // https://kbdlayout.info/
    int16 scan_codes[MAX_HOTKEY_COMBINATION];

    // @type KeyPressType
    byte key_state;

    // Some hotkeys are only available in certain context (@see InputContext)
    //      0 = always available (but a hotkey of the current context with the same keys has priority)
    //      only managed contexts (@see Input::managed_contexts) = the input must be in at least one of them
    //      any unmanaged context bit = treated like 0 (backwards compatible, contexts used to be ignored)
    byte context;
};

enum InputKeyFlag : byte {
    // This is needed if different hotkey use the same button (e.g. ctrl+s and s =backwards movement)
    // First we check for the more complex hotkey ctrl+s,
    // then we set s as handled so it doesn't get handled again for s =backwards movement
    INPUT_KEY_FLAG_PROCESSED = 1 << 0,

    // The key produced text (@see InputState::text)
    // While typing (HOTKEY_CONTEXT_TYPING) such keys don't trigger hotkeys
    INPUT_KEY_FLAG_TEXT = 1 << 1,
};

/**
 * Key state lifecycle (@see input_frame_begin):
 *      PRESSED             = went down this tick
 *      HELD                = went down in an earlier tick and is still down
 *      RELEASED            = went up this tick (removed at the beginning of the next tick)
 *      PRESSED | RELEASED  = went down AND up within this tick (e.g. very fast click)
 */
struct InputKey {
    // @question Do we really need scan_code and virtual_code both?
    uint16 scan_code;
    uint16 virtual_code;

    // @type KeyPressType
    byte key_state;

    // @type InputKeyFlag
    byte flags;

    // e.g. stick/trigger keys have additional values
    int16 value;

    // when was this action performed (useful to decide if key state is held vs pressed)
    uint64 time;
};

// @question Maybe we should also add a third key_down array for controllers and some special controller functions here
//          to just handle everything in one struct
//          Or think about completely splitting all states (mouse, keyboard, other)

// Text input codes that are not unicode characters (outside of the unicode range)
// Control characters that ToUnicode already returns are used as is (backspace, tab, enter, escape)
enum InputTextControl : uint32 {
    INPUT_TEXT_BACKSPACE = 0x08,
    INPUT_TEXT_TAB = 0x09,
    INPUT_TEXT_ENTER = 0x0D,
    INPUT_TEXT_ESCAPE = 0x1B,
    INPUT_TEXT_DELETE = 0x7F,

    INPUT_TEXT_LEFT = 0x110000,
    INPUT_TEXT_RIGHT,
    INPUT_TEXT_UP,
    INPUT_TEXT_DOWN,
    INPUT_TEXT_HOME,
    INPUT_TEXT_END,

    // ctrl + a/c/x/v (layout independent since they are based on the generated control characters)
    INPUT_TEXT_SELECT_ALL,
    INPUT_TEXT_COPY,
    INPUT_TEXT_CUT,
    INPUT_TEXT_PASTE,

    // Modifier bits, only set on navigation codes (LEFT ... END) and BACKSPACE/DELETE
    // e.g. INPUT_TEXT_LEFT | INPUT_TEXT_MOD_SHIFT = extend the selection by one character
    //      INPUT_TEXT_LEFT | INPUT_TEXT_MOD_CTRL = jump one word
    INPUT_TEXT_MOD_CTRL = 1U << 29,
    INPUT_TEXT_MOD_SHIFT = 1U << 30,
    INPUT_TEXT_MOD_MASK = INPUT_TEXT_MOD_CTRL | INPUT_TEXT_MOD_SHIFT,
};

// Max text codes per tick and input device (more are dropped, e.g. during a long frame)
#define INPUT_TEXT_MAX_CODES 16

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

    // Text typed this tick (only while HOTKEY_CONTEXT_TYPING is set)
    // Unicode codepoints or InputTextControl values in the order the key events arrived
    // Every key event (incl. OS key repeat) produces text exactly once -> independent from the frame rate
    int32 text_count;
    uint32 text[INPUT_TEXT_MAX_CODES];
};

enum GeneralInputState : byte {
    INPUT_STATE_GENERAL_INPUT_CHANGE = 1 << 0,
    INPUT_STATE_GENERAL_HOTKEY_ACTIVE = 1 << 1, // At least one hotkey is active
};

#ifdef _WIN32
    #include <windows.h>
    #include <dinput.h>

    typedef HANDLE InputHandle;
#endif

struct KBMDevice {
    // Identifies the physical device (hash of the VID/PID part of the device name, @see rawinput_device_id)
    //      * groups the logical devices (handles) of one physical device
    //      * stays set after a disconnect -> the same device reconnects to the same Input
    // 0 = no device was ever connected
    uint64 device_id;

    // Windows is nuts, one device can have multiple handles
    // Sometimes to simulate additional buttons additional devices are created
    // There are probably more insane reasons
    // handle[0] == NULL -> no device of this kind connected (the list is always compact)
    InputHandle handle[8];
};

// Max size of a controller input report
//      DS4/DualSense bluetooth = 78 bytes
//      usb = 64 bytes
#define INPUT_CONTROLLER_REPORT_SIZE 128

struct ControllerDevice {
    // Connection of the controller itself
    // The Input may still have a keyboard/mouse connected if this is NONE
    InputConnectionType connection_type;

    // API that handles this controller
    ControllerInputType api;

    // A new report is available in report (only used by raw input, @see input_controller_poll)
    bool has_report;

    // HID only: an async read is in flight
    bool read_pending;

    // Size of a single input report
    uint16 report_size;

    // Identifies the physical device (e.g. hash of the device path, xinput slot)
    // Stays set after a disconnect -> the same controller reconnects to the same Input
    uint64 device_id;

    #ifdef _WIN32
        int32 id; // used by XInput (user index)
        InputHandle handle; // used by hid (file handle) and raw input (device handle) controllers
        LPDIRECTINPUTDEVICE8* direct; // used by direct input controller

        // HID only: async (overlapped) reads -> polling never blocks
        // WARNING: The Input must not be moved/copied while a read is pending
        OVERLAPPED overlapped;
        byte read_buffer[INPUT_CONTROLLER_REPORT_SIZE];
    #endif

    // Latest complete input report
    byte report[INPUT_CONTROLLER_REPORT_SIZE];
};

struct Input {
    // Device
    // NONE = no device connected to this input at all (@see input_state_connection_update)
    InputConnectionType connection_type;

    // InputType flags of the devices that were assigned to this input at some point
    // Used to decide which hotkey mappings the input needs (kbm, controller or both)
    // Not reset on disconnect -> the hotkeys stay valid for a reconnect
    byte device_types;

    // GeneralInputState
    byte general_states;

    // InputContext
    byte context;

    // Contexts the application actually sets/removes in Input::context (@see Hotkey::context)
    // Hotkeys with context bits outside of this mask are general hotkeys
    // Default = HOTKEY_CONTEXT_UI
    // InputContext
    byte managed_contexts;

    // ControllerType
    ControllerType controller_type;

    uint8 hotkey_count;

    KBMDevice keyboard;
    KBMDevice mouse;
    ControllerDevice controller;

    f32 cursor_sensitivity;

    InputState state;
    uint64 time_last_input_check;

    // @todo this should probably be somewhere else
    // @todo don't we need multiple deadzones? triggers, sticks
    int32 deadzone;

    // Controller axes (ControllerButton) that create the pointer movement
    // [0] = dx[0], [1] = dy[0] (e.g. camera / virtual cursor), [2] = dx[1], [3] = dy[1] (e.g. movement)
    // 0xFF = not mapped
    ControllerButton controller_axes[4];

    // Buffer for text (e.g. text copy/clipboard)
    // In case of multiple Input elements we may even reference the same buffer
    // Used in input_text_add()
    char* text;

    // We allow a hotkey to be mapped in two different ways = allow alternative hotkey combination
    // This allows to use controller + keyboard or alternative hotkey mapping but not both at the same time
    // This trade off seems fine since both situations are edge cases
    Hotkey* input_mappings[2];

    // This contains a full list off REFERENCES to hotkey events
    // The length of this array is the exact amount of possible hotkeys
    // It doesn't hold the actual event but a REFERENCE to it
    // The reason for that is that we don't necessarily need n-to-n events.
    // Usually we need less events than hotkeys (e.g. all movement hotkeys are handled by one movement event function)
    const InputEvent* hotkey_event_list;
};

// Application wide input device data (one instance per application, not per player like Input)
struct InputDeviceManager {
    #ifdef _WIN32
        // Registration of the hid device notifications (WM_DEVICECHANGE)
        // Keyboards/mice (raw input) have their own notification (WM_INPUT_DEVICE_CHANGE)
        // but hid/XInput controllers don't -> without this we wouldn't notice controller connects/disconnects
        // @see input_device_notification_register
        HDEVNOTIFY device_notification;

        // Remaining delayed XInput rescans after a device change (WM_TIMER)
        // XInput has no connect events and a new controller is only available a little bit after WM_DEVICECHANGE
        // -> we check a couple of times for new controllers (@see input_controllers_rescan)
        int32 rescan_count;
    #else
        // Empty structs are not allowed in C
        byte unused;
    #endif
};

#endif