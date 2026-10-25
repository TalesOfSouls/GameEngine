/**
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 *
 * Raw input (keyboard, mouse and optionally controllers)
 *
 * Connects/disconnects:
 *      All devices are registered with RIDEV_DEVNOTIFY -> the window receives WM_INPUT_DEVICE_CHANGE
 *      GIDC_ARRIVAL = rawinput_device_add(), GIDC_REMOVAL = rawinput_device_remove()
 *
 *      WARNING: Windows also sends GIDC_ARRIVAL for all present devices right after the registration
 *               Adding a device is idempotent
 */
#pragma once
#ifndef COMS_PLATFORM_WIN32_INPUT_RAW_H
#define COMS_PLATFORM_WIN32_INPUT_RAW_H

#include <windows.h>

#include "../../../stdlib/Stdlib.h"
#include "../../../input/Input.cpp"
#include "../../../input/ControllerType.h"
#include "../../../input/ControllerInput.h"
#include "../../../memory/BufferMemory.cpp"
#include "InputDevice.h"
#include "ControllerReport.h"
#include <winDNS.h>
#include <hidusage.h>

// Max raw input devices we look at during the initialization (usually < 30)
#define RAWINPUT_MAX_DEVICES 256

static inline
bool rawinput_extract_vid_pid(
    const wchar_t* device_name,
    wchar_t* out_id
) {
    const wchar_t* vid = wcsstr(device_name, L"VID_");
    const wchar_t* pid = wcsstr(device_name, L"PID_");
    if (vid && pid) {
        memcpy(out_id, vid, 8 * sizeof(wchar_t));
        out_id[8] = L'&';
        memcpy(out_id + 9, pid, 8 * sizeof(wchar_t));

        return true;
    }

    // We need this code below since some peripheral devices get converted to mouse/keyboard devices
    // Such converted devices don't have VID/PID
    // Examples could be attachable keyboards for tables
    const wchar_t* id = wcschr(device_name, L'{');
    if (!id) {
        return false;
    }

    ++id;

    int32 len = 0;
    while (*id != L'}' && *id != L'\0' && len < 31) {
        out_id[len++] = *id;
        ++id;
    }

    return true;
}

/**
 * Identifies the physical device of a raw input device handle
 *
 * Remember one physical device may have multiple logical devices (e.g. additional keys = additional keyboard)
 * They all have the same id -> we group them together as one device
 *
 * @param out_id 32 characters, zero filled
 */
static inline
bool rawinput_device_id(
    HANDLE device,
    wchar_t* const out_id,
    wchar_t* const out_name = NULL, uint32 out_name_length = 0
) NO_EXCEPT
{
    wchar_t device_name[512];

    // WARNING: For RIDI_DEVICENAME the size is in characters, not bytes
    UINT name_size = ARRAY_COUNT(device_name);
    if (GetRawInputDeviceInfoW(device, RIDI_DEVICENAME, device_name, &name_size) == (UINT) -1) {
        return false;
    }

    device_name[ARRAY_COUNT(device_name) - 1] = L'\0';

    memset(out_id, 0, 32 * sizeof(wchar_t));
    if (!rawinput_extract_vid_pid(device_name, out_id)) {
        // Fallback = use the beginning of the name
        wcsncpy(out_id, device_name, 31);
    }

    if (out_name && out_name_length) {
        wcsncpy(out_name, device_name, out_name_length - 1);
        out_name[out_name_length - 1] = L'\0';
    }

    return true;
}

/**
 * Adds a keyboard/mouse to an input
 *
 * @return Input index (-1 = not added)
 */
static
int32 rawinput_kbm_add(
    Input* const __restrict states, int32 count,
    HANDLE device,
    InputDeviceKind kind,
    const byte* const __restrict preferred_types
) NO_EXCEPT
{
    // Already added
    for (int32 i = 0; i < count; ++i) {
        const KBMDevice* const kbm = kind == INPUT_DEVICE_KIND_KEYBOARD
            ? &states[i].keyboard
            : &states[i].mouse;

        for (int32 h = 0; h < ARRAY_COUNT(kbm->handle) && kbm->handle[h]; ++h) {
            if (kbm->handle[h] == device) {
                return -1;
            }
        }
    }

    wchar_t device_name_id[32];
    if (!rawinput_device_id(device, device_name_id)) {
        return -1;
    }

    // We only store a hash, the name itself isn't needed
    const uint64 device_id = input_device_path_hash(device_name_id);

    // Same physical device as an already connected/previously connected device
    // -> add the logical device to that input (also handles reconnects)
    int32 index = -1;
    for (int32 i = 0; i < count; ++i) {
        const KBMDevice* const kbm = kind == INPUT_DEVICE_KIND_KEYBOARD
            ? &states[i].keyboard
            : &states[i].mouse;

        if (kbm->device_id == device_id) {
            index = i;

            // Connected devices have priority over previously connected devices
            if (kbm->handle[0]) {
                break;
            }
        }
    }

    if (index < 0) {
        index = input_state_assign(states, count, kind, preferred_types);
    }

    if (index < 0) {
        LOG_1("[INFO] No free input for keyboard/mouse");
        return -1;
    }

    Input* const input = &states[index];
    KBMDevice* const kbm = kind == INPUT_DEVICE_KIND_KEYBOARD ? &input->keyboard : &input->mouse;

    // A different physical device took over the input slot
    if (!kbm->handle[0]) {
        kbm->device_id = device_id;
    }

    if (!input_kbm_handle_add(kbm, device)) {
        // Too many logical devices, the input of this logical device is ignored
        return -1;
    }

    input->device_types |= INPUT_TYPE_MOUSE_KEYBOARD;
    input_state_connection_update(input);

    return index;
}

/**
 * Adds a controller (only used if raw input is the controller api)
 *
 * @return Input index (-1 = not added)
 */
static
int32 rawinput_controller_add(
    Input* const __restrict states, int32 count,
    HANDLE device,
    const RID_DEVICE_INFO* const __restrict rdi,
    const byte* const __restrict preferred_types
) NO_EXCEPT
{
    if (input_controller_find_by_handle(states, count, device).vec[0] >= 0) {
        return -1;
    }

    wchar_t device_id[32];
    wchar_t device_name[512];
    if (!rawinput_device_id(device, device_id, device_name, ARRAY_COUNT(device_name))) {
        return -1;
    }

    const uint64 id = input_device_path_hash(device_name);

    int32 index = input_controller_find_by_device_id(states, count, id);
    if (index < 0 || states[index].controller.connection_type) {
        index = input_state_assign(states, count, INPUT_DEVICE_KIND_CONTROLLER, preferred_types);
    }

    if (index < 0) {
        LOG_1("[INFO] No free input for raw input controller");
        return -1;
    }

    Input* const input = &states[index];
    ControllerDevice* const controller = &input->controller;

    controller->api = CONTROLLER_INPUT_TYPE_RAWINPUT;
    controller->device_id = id;
    controller->handle = device;
    controller->has_report = false;
    controller->report_size = 0;
    controller->connection_type = input_device_path_connection(device_name);

    input->controller.controller_type = input_controller_type_get(rdi->hid.dwVendorId, rdi->hid.dwProductId);
    input->device_types |= INPUT_TYPE_CONTROLLER;
    input_state_connection_update(input);

    LOG_1(
        "[INFO] Raw input controller connected to input %d (type %d)",
        {DATA_TYPE_INT32, &index}, {DATA_TYPE_INT32, &input->controller.controller_type}
    );

    return index;
}

/**
 * Adds a raw input device (e.g. GIDC_ARRIVAL)
 *
 * @param controllers       Also add controllers (= raw input is the controller api)
 * @param preferred_types   Which InputType is preferred for which state
 *
 * @return Input index the device was added to (-1 = not added, e.g. already known)
 */
int32 rawinput_device_add(
    Input* const __restrict states, int32 count,
    HANDLE device,
    bool controllers,
    const byte* const __restrict preferred_types = NULL
) NO_EXCEPT
{
    RID_DEVICE_INFO rdi;
    rdi.cbSize = sizeof(RID_DEVICE_INFO);
    UINT cb_size = sizeof(RID_DEVICE_INFO);
    if (GetRawInputDeviceInfoW(device, RIDI_DEVICEINFO, &rdi, &cb_size) == (UINT) -1) {
        return -1;
    }

    switch (rdi.dwType) {
        case RIM_TYPEMOUSE:
            return rawinput_kbm_add(states, count, device, INPUT_DEVICE_KIND_MOUSE, preferred_types);
        case RIM_TYPEKEYBOARD:
            return rawinput_kbm_add(states, count, device, INPUT_DEVICE_KIND_KEYBOARD, preferred_types);
        case RIM_TYPEHID: {
            if (!controllers
                || rdi.hid.usUsagePage != HID_USAGE_PAGE_GENERIC
                || (rdi.hid.usUsage != HID_USAGE_GENERIC_GAMEPAD && rdi.hid.usUsage != HID_USAGE_GENERIC_JOYSTICK)
            ) {
                return -1;
            }

            return rawinput_controller_add(states, count, device, &rdi, preferred_types);
        }
        default:
            return -1;
    }
}

/**
 * Removes a raw input device (e.g. GIDC_REMOVAL)
 * The device handle is already invalid at this point -> only handle comparisons
 *
 * The active keys of the device are released (no stuck keys)
 *
 * @param kind Kind of the removed device (output, optional)
 *
 * @return Input index the device was removed from (-1 = unknown device)
 */
int32 rawinput_device_remove(
    Input* const __restrict states, int32 count,
    HANDLE device,
    InputDeviceKind* const __restrict kind = NULL
) NO_EXCEPT
{
    for (int32 i = 0; i < count; ++i) {
        Input* const input = &states[i];

        if (input_kbm_handle_remove(&input->keyboard, device)) {
            if (!input->keyboard.handle[0]) {
                // @question Do I even need to use this complicated function?
                //          couldn't Just memset it to 0 using input_clean_state
                input_release_keys(input, INPUT_DEVICE_KIND_KEYBOARD);
            }

            input_state_connection_update(input);

            if (kind) {
                *kind = INPUT_DEVICE_KIND_KEYBOARD;
            }

            return i;
        } else if (input_kbm_handle_remove(&input->mouse, device)) {
            if (!input->mouse.handle[0]) {
                // @question Do I even need to use this complicated function?
                //          couldn't Just memset it to 0 using input_clean_state
                input_release_keys(input, INPUT_DEVICE_KIND_MOUSE);
            }

            input_state_connection_update(input);

            if (kind) {
                *kind = INPUT_DEVICE_KIND_MOUSE;
            }

            return i;
        } else if (input->controller.connection_type
            && input->controller.api == CONTROLLER_INPUT_TYPE_RAWINPUT
            && input->controller.handle == device
        ) {
            input->controller.connection_type = INPUT_CONNECTION_TYPE_NONE;
            input->controller.handle = NULL;
            input->controller.has_report = false;
            input_release_keys(input, INPUT_DEVICE_KIND_CONTROLLER);
            input_state_connection_update(input);

            if (kind) {
                *kind = INPUT_DEVICE_KIND_CONTROLLER;
            }

            return i;
        }
    }

    return -1;
}

/**
 * Adds all present raw input devices
 *
 * @return Amount of devices added
 */
static
int32 rawinput_devices_add_all(
    Input* const __restrict states, int32 count,
    bool controllers,
    const byte* const __restrict preferred_types
) NO_EXCEPT
{
    RAWINPUTDEVICELIST device_list[RAWINPUT_MAX_DEVICES];
    UINT device_count = ARRAY_COUNT(device_list);

    const UINT found = GetRawInputDeviceList(device_list, &device_count, sizeof(RAWINPUTDEVICELIST));
    if (found == (UINT) -1) {
        LOG_1("[WARNING] Couldn't get raw input device list");
        return 0;
    }

    int32 added = 0;
    for (UINT i = 0; i < found; ++i) {
        if (rawinput_device_add(
            states,
            count,
            device_list[i].hDevice,
            controllers,
            preferred_types) >= 0
        ) {
            ++added;
        }
    }

    return added;
}

/**
 * Registers keyboard + mouse raw input and adds all present devices
 *
 * The registration also happens if no device is present -> devices connected later on are handled
 *
 * @return false = raw input couldn't be registered (-> use polling)
 */
bool rawinput_kbm_init(
    HWND hwnd,
    Input* const __restrict states, int32 count,
    const byte* const __restrict preferred_types = NULL
) NO_EXCEPT
{
    alignas(8) RAWINPUTDEVICE rid[2];

    // Mouse
    rid[0].usUsagePage = HID_USAGE_PAGE_GENERIC;
    rid[0].usUsage = HID_USAGE_GENERIC_MOUSE;
    rid[0].dwFlags = RIDEV_DEVNOTIFY;
    rid[0].hwndTarget = hwnd;

    // Keyboard
    rid[1].usUsagePage = HID_USAGE_PAGE_GENERIC;
    rid[1].usUsage = HID_USAGE_GENERIC_KEYBOARD;
    rid[1].dwFlags = RIDEV_DEVNOTIFY;
    rid[1].hwndTarget = hwnd;

    if (!RegisterRawInputDevices((PCRAWINPUTDEVICE) rid, 2, sizeof(RAWINPUTDEVICE))) {
        LOG_1("[WARNING] Couldn't register keyboard/mouse raw input device");
        return false;
    }

    // @test Test with 5 devices connected
    rawinput_devices_add_all(states, count, false, preferred_types);

    return true;
}

/**
 * Registers gamepad + joystick raw input and adds all present controllers
 *
 * WARNING: While this works we highly recommend to use hid_init_controllers
 *          Many controllers constantly send reports (gyro etc.) -> floods the raw input buffer
 *
 * @return Amount of controllers found
 */
int32 rawinput_init_controllers(
    HWND hwnd,
    Input* const __restrict states, int32 count,
    const byte* const __restrict preferred_types = NULL
) NO_EXCEPT
{
    alignas(8) RAWINPUTDEVICE rid[2];

    // Gamepad
    rid[0].usUsagePage = HID_USAGE_PAGE_GENERIC;
    rid[0].usUsage = HID_USAGE_GENERIC_GAMEPAD;
    rid[0].dwFlags = RIDEV_DEVNOTIFY;
    rid[0].hwndTarget = hwnd;

    // Joystick
    rid[1].usUsagePage = HID_USAGE_PAGE_GENERIC;
    rid[1].usUsage = HID_USAGE_GENERIC_JOYSTICK;
    rid[1].dwFlags = RIDEV_DEVNOTIFY;
    rid[1].hwndTarget = hwnd;

    if (!RegisterRawInputDevices((PCRAWINPUTDEVICE) rid, 2, sizeof(RAWINPUTDEVICE))) {
        LOG_1("[WARNING] Couldn't register gamepad/joystick raw input device");
        return 0;
    }

    int32 found = 0;
    RAWINPUTDEVICELIST device_list[RAWINPUT_MAX_DEVICES];
    UINT device_count = ARRAY_COUNT(device_list);
    const UINT listed = GetRawInputDeviceList(device_list, &device_count, sizeof(RAWINPUTDEVICELIST));
    if (listed == (UINT) -1) {
        return 0;
    }

    for (UINT i = 0; i < listed; ++i) {
        if (device_list[i].dwType == RIM_TYPEHID
            && rawinput_device_add(states, count, device_list[i].hDevice, true, preferred_types) >= 0
        ) {
            ++found;
        }
    }

    return found;
}

// Stops the raw input for controllers (e.g. switching to a different controller api)
inline
void rawinput_controllers_unregister() NO_EXCEPT
{
    alignas(8) RAWINPUTDEVICE rid[2];

    rid[0].usUsagePage = HID_USAGE_PAGE_GENERIC;
    rid[0].usUsage = HID_USAGE_GENERIC_GAMEPAD;
    rid[0].dwFlags = RIDEV_REMOVE;
    rid[0].hwndTarget = NULL;

    rid[1].usUsagePage = HID_USAGE_PAGE_GENERIC;
    rid[1].usUsage = HID_USAGE_GENERIC_JOYSTICK;
    rid[1].dwFlags = RIDEV_REMOVE;
    rid[1].hwndTarget = NULL;

    RegisterRawInputDevices((PCRAWINPUTDEVICE) rid, 2, sizeof(RAWINPUTDEVICE));
}

#ifdef NOVIRTUALKEYCODES
    #error "NOVIRTUALKEYCODES Must not be defined"
#endif

// Key code without the device prefix = scan code / virtual key code of the OS
#define RAWINPUT_KEY_CODE_MASK 0x00FF

// Bits of a key in the keyboard state array of ToUnicode/GetKeyState
#define RAWINPUT_KEY_STATE_DOWN 0x80 // key is pressed
#define RAWINPUT_KEY_STATE_TOGGLED 0x01 // toggle key is on (e.g. caps lock)

// Character codes ToUnicode creates for some key combinations
#define RAWINPUT_CHAR_CTRL_A 0x01 // ctrl + a
#define RAWINPUT_CHAR_CTRL_C 0x03 // ctrl + c
#define RAWINPUT_CHAR_CTRL_V 0x16 // ctrl + v
#define RAWINPUT_CHAR_CTRL_X 0x18 // ctrl + x
#define RAWINPUT_CHAR_FIRST_PRINTABLE 0x20 // space, everything below is a control character
#define RAWINPUT_CHAR_CTRL_BACKSPACE 0x7F // ctrl + backspace (DEL character)

// Maps non character keys that are relevant for text editing to InputTextControl values (0 = none)
inline
uint32 rawinput_key_to_text_control(byte vkey) NO_EXCEPT
{
    switch (vkey) {
        case VK_LEFT:
            return INPUT_TEXT_LEFT;
        case VK_RIGHT:
            return INPUT_TEXT_RIGHT;
        case VK_UP:
            return INPUT_TEXT_UP;
        case VK_DOWN:
            return INPUT_TEXT_DOWN;
        case VK_HOME:
            return INPUT_TEXT_HOME;
        case VK_END:
            return INPUT_TEXT_END;
        case VK_DELETE:
            return INPUT_TEXT_DELETE;
        case VK_BACK:
            // e.g. ctrl + backspace doesn't create 0x08
            return INPUT_TEXT_BACKSPACE;
        default:
            return 0;
    }
}

/**
 * Converts a keyboard key event into text
 * MUST be called for every key down event (incl. key repeat) AFTER input_set_state() so that modifiers are known
 * The key event itself doesn't need an active key slot -> text is never lost if many keys are pressed
 *
 * Only call this while typing (HOTKEY_CONTEXT_TYPING), the conversion is not free and may change the dead key state
 *
 * @return true if the key is a text key (the caller should set INPUT_KEY_FLAG_TEXT on the active key)
 */
inline
bool input_text_add(InputState* const __restrict state, const InputKey* const __restrict key) NO_EXCEPT
{
    // Modifier state (e.g. shift, alt gr) for the character conversion
    byte keyboard_state[256] = {0};
    for (int32 i = 0; i < MAX_KEY_PRESSES; ++i) {
        const InputKey* const active = &state->active_keys[i];
        if ((active->scan_code & INPUT_KEYBOARD_PREFIX)
            && !(active->key_state & KEY_PRESS_TYPE_RELEASED)
        ) {
            keyboard_state[active->virtual_code & RAWINPUT_KEY_CODE_MASK] = RAWINPUT_KEY_STATE_DOWN;
        }
    }

    // Caps lock is a toggle and not part of the pressed keys
    keyboard_state[VK_CAPITAL] = (byte) (GetKeyState(VK_CAPITAL) & RAWINPUT_KEY_STATE_TOGGLED);

    const byte vkey = (byte) (key->virtual_code & RAWINPUT_KEY_CODE_MASK);

    // ctrl + alt = alt gr -> creates normal characters on many keyboard layouts
    const bool shift = (keyboard_state[VK_SHIFT] & RAWINPUT_KEY_STATE_DOWN) != 0;
    const bool ctrl = (keyboard_state[VK_CONTROL] & RAWINPUT_KEY_STATE_DOWN)
        && !(keyboard_state[VK_MENU] & RAWINPUT_KEY_STATE_DOWN);

    uint32 code = key_to_unicode((byte) (key->scan_code & RAWINPUT_KEY_CODE_MASK), vkey, keyboard_state);

    if (ctrl) {
        // ctrl + letter creates the control characters 0x01 - 0x1A
        switch (code) {
            case RAWINPUT_CHAR_CTRL_A: {
                code = INPUT_TEXT_SELECT_ALL;
            } break;
            case RAWINPUT_CHAR_CTRL_C: {
                code = INPUT_TEXT_COPY;
            } break;
            case RAWINPUT_CHAR_CTRL_V: {
                code = INPUT_TEXT_PASTE;
            } break;
            case RAWINPUT_CHAR_CTRL_X: {
                code = INPUT_TEXT_CUT;
            } break;
            default: {
                // Other ctrl combinations are not text (e.g. ctrl + s = save hotkey)
                // ctrl + backspace is handled as backspace below (rawinput_key_to_text_control)
                if (code < RAWINPUT_CHAR_FIRST_PRINTABLE || code == RAWINPUT_CHAR_CTRL_BACKSPACE) {
                    code = 0;
                }
            }
        }
    } else if (code < RAWINPUT_CHAR_FIRST_PRINTABLE
        && code != INPUT_TEXT_BACKSPACE && code != INPUT_TEXT_TAB
        && code != INPUT_TEXT_ENTER && code != INPUT_TEXT_ESCAPE
    ) {
        // Control characters except the ones used for editing are not text
        code = 0;
    }

    if (!code) {
        code = rawinput_key_to_text_control(vkey);
    }

    // Modifiers for navigation/editing keys (e.g. shift + left = select, shift + tab = previous element)
    if ((code >= INPUT_TEXT_LEFT && code <= INPUT_TEXT_END)
        || code == INPUT_TEXT_BACKSPACE || code == INPUT_TEXT_DELETE || code == INPUT_TEXT_TAB
    ) {
        if (shift) {
            code |= INPUT_TEXT_MOD_SHIFT;
        }

        if (ctrl) {
            code |= INPUT_TEXT_MOD_CTRL;
        }
    }

    if (!code) {
        return false;
    }

    if (state->text_count < INPUT_TEXT_MAX_CODES) {
        state->text[state->text_count++] = code;
    }

    return true;
}

static
int16 input_raw_handle(
    const RAWINPUT* const __restrict raw,
    Input* const __restrict states,
    int32 state_count,
    v2_int16 window_dim,
    uint64 time
) NO_EXCEPT
{
    int16 input_count = 0;

    if (raw->header.dwType == RIM_TYPEMOUSE) {
        v2_int16 input_handle = input_mouse_find_by_handle(
            states,
            state_count,
            raw->header.hDevice
        );

        // Injected/synthesized input has no device (e.g. remote desktop, touch emulation)
        // -> belongs to the primary keyboard + mouse
        if (!raw->header.hDevice) {
            const Input* const kbm = input_kbm_find(states, state_count);
            input_handle.vec[0] = kbm ? (int16) (kbm - states) : -1;
        }

        if (input_handle.vec[0] < 0
            || !states[input_handle.vec[0]].connection_type
        ) {
            return 0;
        }

        if (raw->data.mouse.usButtonFlags) {
            InputKey key = {};

            if (raw->data.mouse.usButtonFlags & RI_MOUSE_LEFT_BUTTON_DOWN) {
                key.key_state = KEY_PRESS_TYPE_PRESSED;
                key.scan_code = INPUT_MOUSE_BUTTON_1;
            } else if (raw->data.mouse.usButtonFlags & RI_MOUSE_LEFT_BUTTON_UP) {
                key.key_state = KEY_PRESS_TYPE_RELEASED;
                key.scan_code = INPUT_MOUSE_BUTTON_1;
            } else if (raw->data.mouse.usButtonFlags & RI_MOUSE_RIGHT_BUTTON_DOWN) {
                key.key_state = KEY_PRESS_TYPE_PRESSED;
                key.scan_code = INPUT_MOUSE_BUTTON_2;
            } else if (raw->data.mouse.usButtonFlags & RI_MOUSE_RIGHT_BUTTON_UP) {
                key.key_state = KEY_PRESS_TYPE_RELEASED;
                key.scan_code = INPUT_MOUSE_BUTTON_2;
            } else if (raw->data.mouse.usButtonFlags & RI_MOUSE_MIDDLE_BUTTON_DOWN) {
                key.key_state = KEY_PRESS_TYPE_PRESSED;
                key.scan_code = INPUT_MOUSE_BUTTON_3;
            } else if (raw->data.mouse.usButtonFlags & RI_MOUSE_MIDDLE_BUTTON_UP) {
                key.key_state = KEY_PRESS_TYPE_RELEASED;
                key.scan_code = INPUT_MOUSE_BUTTON_3;
            } else if (raw->data.mouse.usButtonFlags & RI_MOUSE_BUTTON_4_DOWN) {
                key.key_state = KEY_PRESS_TYPE_PRESSED;
                key.scan_code = INPUT_MOUSE_BUTTON_4;
            } else if (raw->data.mouse.usButtonFlags & RI_MOUSE_BUTTON_4_UP) {
                key.key_state = KEY_PRESS_TYPE_RELEASED;
                key.scan_code = INPUT_MOUSE_BUTTON_4;
            } else if (raw->data.mouse.usButtonFlags & RI_MOUSE_BUTTON_5_DOWN) {
                key.key_state = KEY_PRESS_TYPE_PRESSED;
                key.scan_code = INPUT_MOUSE_BUTTON_5;
            } else if (raw->data.mouse.usButtonFlags & RI_MOUSE_BUTTON_5_UP) {
                key.key_state = KEY_PRESS_TYPE_RELEASED;
                key.scan_code = INPUT_MOUSE_BUTTON_5;
            } else if (raw->data.mouse.usButtonFlags & RI_MOUSE_WHEEL) {
                key.key_state = KEY_PRESS_TYPE_RELEASED;
                key.scan_code = INPUT_MOUSE_BUTTON_WHEEL;
                key.value = (int16) raw->data.mouse.usButtonData;
            } else if (raw->data.mouse.usButtonFlags & RI_MOUSE_HWHEEL) {
                key.key_state = KEY_PRESS_TYPE_RELEASED;
                key.scan_code = INPUT_MOUSE_BUTTON_HWHEEL;
                key.value = (int16) raw->data.mouse.usButtonData;
            } else {
                return 0;
            }

            ++input_count;

            key.scan_code |= INPUT_MOUSE_PREFIX;
            key.time = time;

            input_set_state(states[input_handle.vec[0]].state.active_keys, &key);
            states[input_handle.vec[0]].general_states |= INPUT_STATE_GENERAL_INPUT_CHANGE;
        }

        if (raw->data.mouse.lLastX || raw->data.mouse.lLastY) {
            // @question do we want to handle mouse movement for every individual movement,
            //          or do we want to pull it at the end if we have ANY mouse movement

            // @bug Consider to change dx and x to float values
            //      Currently we are jumping in unsmooth ways since we always round down to int values
            //      However, in reality the movement is much smoother
            //      I couldn't feel it on my crappy mouse, low monitor resolution etc. but it could be an issue
            const int16 dy = (int16) (raw->data.mouse.lLastY * states[input_handle.vec[0]].cursor_sensitivity);
            const int16 dx = (int16) (raw->data.mouse.lLastX * states[input_handle.vec[0]].cursor_sensitivity);

            states[input_handle.vec[0]].state.dx[0] += dx;
            states[input_handle.vec[0]].state.dy[0] += dy;

            states[input_handle.vec[0]].state.x[0] += dx;
            states[input_handle.vec[0]].state.y[0] += dy;

            // Since we are only using relative movement we need to clip to the window dimensions
            states[input_handle.vec[0]].state.x[0] = OMS_CLAMP(
                states[input_handle.vec[0]].state.x[0],
                (int16) 0,
                window_dim.width
            );
            states[input_handle.vec[0]].state.y[0] = OMS_CLAMP(
                states[input_handle.vec[0]].state.y[0],
                (int16) 0,
                window_dim.height
            );

            // We need to signal that the mouse was moved
            // We basically fake that a mouse movement is the same as a button
            const InputKey key = {
                SMN(scan_code) INPUT_MOUSE_PREFIX | INPUT_MOUSE_MOVE,
                SMN(virtual_code) 0,
                SMN(key_state) KEY_PRESS_TYPE_RELEASED,
                SMN(flags) 0,
                SMN(value) 0,
                SMN(time) time
            };

            input_set_state(states[input_handle.vec[0]].state.active_keys, &key);
            states[input_handle.vec[0]].general_states |= INPUT_STATE_GENERAL_INPUT_CHANGE;

            ++input_count;

            // @todo Maybe we need to enforce the same coordinates between platforms?!
            //          Windows top left    = 0;0
            //                  bottom left = 0;height
        }
    } else if (raw->header.dwType == RIM_TYPEKEYBOARD) {
        v2_int16 input_handle = input_keyboard_find_by_handle(
            states,
            state_count,
            raw->header.hDevice
        );

        // Injected/synthesized input has no device (e.g. SendInput, on screen keyboard)
        if (!raw->header.hDevice) {
            const Input* const kbm = input_kbm_find(states, state_count);
            input_handle.vec[0] = kbm ? (int16) (kbm - states) : -1;
        }

        if (input_handle.vec[0] < 0
            || !states[input_handle.vec[0]].connection_type
        ) {
            return 0;
        }

        KeyPressType new_state;
        if (raw->data.keyboard.Flags == RI_KEY_MAKE
            || raw->data.keyboard.Flags == RI_KEY_E0
            || raw->data.keyboard.Flags == RI_KEY_E1
        ) {
            new_state = KEY_PRESS_TYPE_PRESSED;
        } else if (raw->data.keyboard.Flags & RI_KEY_BREAK) {
            new_state = KEY_PRESS_TYPE_RELEASED;
        } else {
            return 0;
        }

        ++input_count;

        Input* const input = &states[input_handle.vec[0]];

        const InputKey key = {
            (uint16) (raw->data.keyboard.MakeCode | INPUT_KEYBOARD_PREFIX),
            (uint16) (raw->data.keyboard.VKey | INPUT_KEYBOARD_PREFIX),
            new_state, 0, 0, time
        };

        InputKey* const active_key = input_set_state(input->state.active_keys, &key);
        input->general_states |= INPUT_STATE_GENERAL_INPUT_CHANGE;

        // Text is created directly from the key event (also for OS key repeat events)
        // Every event produces text exactly once, independent from the frame rate
        // Text keys don't trigger hotkeys while typing
        if (new_state == KEY_PRESS_TYPE_PRESSED
            && (input->context & HOTKEY_CONTEXT_TYPING)
            && input_text_add(&input->state, &key)
            && active_key
        ) {
            active_key->flags |= INPUT_KEY_FLAG_TEXT;
        }
    } else if (raw->header.dwType == RIM_TYPEHID) {
        // Controllers often CONSTANTLY send data (gyro, timestamps)
        // We only store the latest report here, it is mapped once per tick
        const v2_int16 input_handle = input_controller_find_by_handle(
            states,
            state_count,
            raw->header.hDevice
        );

        if (input_handle.vec[0] < 0
            || states[input_handle.vec[0]].controller.api != CONTROLLER_INPUT_TYPE_RAWINPUT
            || raw->data.hid.dwCount == 0
            || raw->data.hid.dwSizeHid == 0
        ) {
            return 0;
        }

        ControllerDevice* const controller = &states[input_handle.vec[0]].controller;

        // One message may contain multiple reports, the last one is the latest
        const uint32 report_size = OMS_MIN(
            (uint32) raw->data.hid.dwSizeHid,
            (uint32) INPUT_CONTROLLER_REPORT_SIZE
        );
        memcpy(
            controller->report,
            raw->data.hid.bRawData + (raw->data.hid.dwCount - 1) * raw->data.hid.dwSizeHid,
            report_size
        );

        controller->report_size = (uint16) report_size;
        controller->has_report = true;
    }

    return input_count;
}

void input_raw_handle(
    LPARAM lParam,
    Input* __restrict states, int32 state_count,
    v2_int16 window_dim,
    BufferMemory* const __restrict mem,
    uint64 time
) NO_EXCEPT
{
    uint32 db_size;
    GetRawInputData((HRAWINPUT) lParam, RID_INPUT, NULL, &db_size, sizeof(RAWINPUTHEADER));

    LPBYTE lpb;
    BUFFER_STACK_MEMORY(mem, (byte **) &lpb, db_size * sizeof(BYTE), alignof(size_t));

    uint32 size = GetRawInputData((HRAWINPUT) lParam, RID_INPUT, lpb, &db_size, sizeof(RAWINPUTHEADER));
    LOG_TRUE_3(size > 100 * KILOBYTE, "Very large input data %d bytes", {DATA_TYPE_UINT32, &size});

    if (db_size != size) {
        return;
    }

    input_raw_handle((RAWINPUT *) lpb, states, state_count, window_dim, time);
}

// max_inputs = max input messages
int16 input_raw_handle_buffered(
    int32 max_inputs,
    Input* __restrict states, int32 state_count,
    v2_int16 window_dim,
    BufferMemory* const __restrict mem,
    uint64 time
) NO_EXCEPT
{
    uint32 cb_size;
    GetRawInputBuffer(NULL, &cb_size, sizeof(RAWINPUTHEADER));
    if (!cb_size) {
        return 0;
    }

    // Max input messages (e.g. 16)
    cb_size *= max_inputs;

    LOG_TRUE_3(cb_size > 100 * KILOBYTE, "Very large input data %d bytes", {DATA_TYPE_UINT32, &cb_size});

    PRAWINPUT raw_input;
    BUFFER_STACK_MEMORY(mem, (byte **) &raw_input, cb_size, alignof(size_t));

    int16 input_count = 0;

    while (true) {
        const uint32 input = GetRawInputBuffer(raw_input, &cb_size, sizeof(RAWINPUTHEADER));
        if (input == 0 || input == (uint32) -1) {
            break;
        }

        PRAWINPUT pri = raw_input;
        for (uint32 i = 0; i < input; ++i) {
            // WARNING: Messages without a device (injected input) must not stop the loop
            //          otherwise the rest of the buffer is lost

            // @performance Instead of passing all input states we should only pass the state that actually matters
            input_count += input_raw_handle(pri, states, state_count, window_dim, time);

            pri = NEXTRAWINPUTBLOCK(pri);
        }
    }

    return input_count;
}

#endif