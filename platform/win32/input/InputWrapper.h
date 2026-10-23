/**
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 *
 * Input device management (initialization, connects/disconnects, polling, cleanup)
 *
 * Keyboard + mouse:
 *      raw input (event based) or GetAsyncKeyState (polling, fallback)
 *      connects/disconnects: WM_INPUT_DEVICE_CHANGE -> input_raw_device_change()
 *
 * Controllers (ControllerInputType):
 *      HID         async hid reads + XInput for XInput compatible controllers (e.g. Xbox controllers)
 *      XINPUT      only XInput
 *      RAWINPUT    raw input, only the latest report per tick is used
 *      All of them are polled once per tick (input_controllers_poll)
 *      connects/disconnects: WM_DEVICECHANGE -> input_hid_device_change() (requires input_device_notification_register)
 *                            WM_INPUT_DEVICE_CHANGE (raw input api)
 *                            failed poll (all apis)
 *
 * All connect/disconnect functions return a bit field of the inputs that changed (bit n = input n)
 * -> the application can update its input specific data (e.g. hotkeys)
 */

/**
 * Input flow overview
 * ===================
 *
 * 1. Setup (setup_input)
 *      a) input_init()                 Allocates the hotkey mappings of every Input
 *      b) rawinput_kbm_init()          Registers keyboard + mouse raw input (RIDEV_DEVNOTIFY)
 *                                      and adds all present keyboards/mice
 *                                      Fails -> INPUT_MODE_POLLING (GetAsyncKeyState, only Input 0)
 *      c) input_controller_init()      Initializes the controller api (settings->input_controller_api)
 *                                          HID       async hid reads, XInput for XInput compatible controllers
 *                                          XINPUT    only XInput (max 4)
 *                                          RAWINPUT  registers gamepad/joystick raw input
 *                                      and adds all present controllers
 *      d) input_device_notification_register()
 *                                      Registers for hid connect/disconnect notifications (WM_DEVICECHANGE)
 *      e) input_hotkey_setup()         Loads the hotkeys of every Input (defaults + input<n>_kbm/controller.cfg)
 *
 * 2. Device discovery + assignment (input_state_assign)
 *      A new device is assigned to an Input:
 *          0. Known device (same device id as before) -> same Input as before (reconnect)
 *          1. Input whose settings->input_device_types allow this device kind and the slot is free
 *          2. Unconfigured Input (type 0) that isn't used by another device kind
 *      Default settings [3, 0, 0, 0]:
 *          first keyboard + mouse + first controller -> Input 0 (shared)
 *          additional devices                        -> their own Input (1, 2, 3)
 *      The device identification (KBMDevice::device_id, ControllerDevice::device_id) is kept after a
 *      disconnect -> a reconnected device returns to its old Input
 *
 * 3. Connect / disconnect (window procedure)
 *      WM_INPUT_DEVICE_CHANGE  -> input_raw_device_change()    keyboard, mouse, raw input controllers
 *                                 WARNING: Windows also sends ARRIVAL for all present devices after the
 *                                          registration -> adding a device is idempotent
 *      WM_DEVICECHANGE         -> input_hid_device_change()    hid controllers (by device path)
 *                                                              XInput controllers (rescan)
 *      WM_TIMER                -> input_controllers_rescan()   XInput needs some time until a new controller
 *                                                              is available -> a few delayed rescans
 *      Failed controller poll  -> input_controller_disconnect()
 *
 *      Disconnect: the handles are closed and only the keys of that device are released
 *                  (other devices of the same Input keep working, e.g. keyboard stays if the controller is removed)
 *      All functions return a bit field of the changed Inputs -> input_devices_changed():
 *          - new device kind for an Input -> hotkeys are set up again (rebind)
 *          - active_input_states is updated (= highest connected Input + 1, indices stay stable)
 *
 * 4. Hotkey mapping (input_hotkey_setup)
 *      Device types of an Input = settings->input_device_types[i] | Input::device_types (devices that were connected)
 *          only kbm         -> input_mappings[0] = kbm hotkeys
 *          only controller  -> input_mappings[0] = controller hotkeys
 *          both (shared)    -> input_mappings[0] = kbm hotkeys, input_mappings[1] = controller hotkeys
 *      Rebinding = changing the config/mapping and calling input_hotkey_setup() again
 *
 * 5. Per tick (on_update_input)
 *      a) input_frame_begin()          released keys are removed, pressed -> held, deltas/text reset
 *      b) input_kbm_handle()           drains the raw input buffer (keyboard, mouse, raw input controller reports)
 *                                      keyboard/mouse events -> input_set_state()
 *                                      raw input controllers -> only the latest report is stored
 *      c) input_controllers_poll()     only while the window has the focus
 *                                          HID       collects all reports since the last tick (never blocks),
 *                                                    maps the latest one (re-applied if no new report)
 *                                          XINPUT    XInputGetState()
 *                                          RAWINPUT  maps the stored latest report
 *                                      -> input_set_controller_state() (buttons/axes = keys, sticks = dx/dy)
 *      d) input_hotkey_state_update()  active keys -> active hotkeys (context + key count priority)
 *      e) input_hotkeys_handle()       runs the InputEvent callbacks of the active hotkeys
 *
 *      Why polling for controllers: many controllers constantly send reports (gyro, timestamps)
 *      -> event based handling would flood the message/raw input queue
 *
 * 6. Focus
 *      WM_KILLFOCUS clears all active keys (no stuck keys), controllers aren't polled without focus
 *
 * 7. Cleanup (cleanup_input)
 *      Unregisters the device notifications, closes the controller handles, unloads XInput
 *
 * WARNING: The game loop is a hot reloadable dll -> static function pointers are per module
 *          Device setup/connects happen in the exe, the dll only polls (XInput is bound via xinput_bind_loaded)
 */
#pragma once
#ifndef COMS_INPUT_WRAPPER_H
#define COMS_INPUT_WRAPPER_H

#include <windows.h>
#include <dbt.h>

#include "../../../stdlib/Stdlib.h"
#include "../../../input/Input.cpp"
#include "../../../input/ControllerType.h"
#include "../../../input/InputMode.h"
#include "../../../memory/BufferMemory.cpp"

#include "XInput.h"
#include "PollingInput.h"
#include "RawInput.h"
#include "HidInput.h"

/**
 * Device interface class of all hid devices (= GUID_DEVINTERFACE_HID from hidclass.h)
 *
 * Why we need it:
 *      Windows only sends WM_DEVICECHANGE with DBT_DEVICEARRIVAL/DBT_DEVICEREMOVECOMPLETE for device interfaces
 *      a window registered for (RegisterDeviceNotificationW + DBT_DEVTYP_DEVICEINTERFACE + class guid).
 *      Controllers (hid and XInput, which also exposes a hid interface) belong to this class
 *      -> registering for it gives us connect/disconnect events incl. the device path (dbcc_name)
 *      The device path is what hid_controller_add() opens and what identifies a hid controller on removal.
 *      Without it we would have to re-enumerate all devices periodically (slow).
 *      Keyboards/mice don't need it, they get WM_INPUT_DEVICE_CHANGE through raw input (RIDEV_DEVNOTIFY).
 *
 * Why it is defined here:
 *      GUID_DEVINTERFACE_HID is only declared in hidclass.h with DEFINE_GUID, which only creates the actual
 *      variable if <initguid.h> is included before it (in exactly one translation unit) or hid.lib is linked.
 *      HidD_GetHidGuid() returns the same guid but requires hid.dll (dynamic linking).
 *
 * @question Consider to remove if HidD_GetHidGuid() returns the same
 */
static const GUID INPUT_GUID_DEVINTERFACE_HID = {
    0x4D1E55B2, 0xF16F, 0x11CF,
    { 0x88, 0xCB, 0x00, 0x11, 0x11, 0x00, 0x00, 0x30 }
};

FORCE_INLINE
bool input_controller_api_uses_xinput(byte controller_api) NO_EXCEPT
{
    return (controller_api == CONTROLLER_INPUT_TYPE_HID || controller_api == CONTROLLER_INPUT_TYPE_XINPUT)
        && pXInputGetState;
}

/**
 * Disconnects the controller of an input
 *
 * The keys of the controller are released, keyboard/mouse of the same input are not affected
 * The device id is kept -> the same controller reconnects to the same input
 */
inline
void input_controller_disconnect(Input* const input) NO_EXCEPT
{
    switch (input->controller.api) {
        case CONTROLLER_INPUT_TYPE_HID: {
            hid_controller_close(&input->controller);
        } break;
        default: {
            input->controller.handle = NULL;
            input->controller.has_report = false;
            input->controller.connection_type = INPUT_CONNECTION_TYPE_NONE;
        }
    }

    input_release_keys(input, INPUT_DEVICE_KIND_CONTROLLER);
    input_state_connection_update(input);

    LOG_1("[INFO] Controller disconnected");
}

/**
 * Initializes the controller api and adds all connected controllers
 *
 * @param controller_api    ControllerInputType (in/out), changed if the api is not available (fallback = HID)
 * @param preferred_types   InputType flags per input (@see input_state_assign), may be NULL
 *
 * @return Amount of controllers found
 */
int32 input_controller_init(
    byte* const __restrict controller_api,
    HWND hwnd,
    Input* const __restrict states, int32 count,
    const byte* const __restrict preferred_types = NULL
) NO_EXCEPT
{
    if (*controller_api >= CONTROLLER_INPUT_TYPE_SIZE) {
        *controller_api = CONTROLLER_INPUT_TYPE_HID;
    }

    switch (*controller_api) {
        case CONTROLLER_INPUT_TYPE_RAWINPUT: {
            // This is possible but not recommended, many controllers constantly send update information
            // (gyro, accel and the worst of all sometimes even timer/index variables)
            return rawinput_init_controllers(hwnd, states, count, preferred_types);
        };
        case CONTROLLER_INPUT_TYPE_XINPUT: {
            if (xinput_load()) {
                return (int32) intrin_bits_count_32(xinput_controllers_scan(states, count, preferred_types));
            }

            LOG_1("[WARNING] Couldn't load XInput, falling back to HID");
            *controller_api = CONTROLLER_INPUT_TYPE_HID;
        } FALLTHROUGH;
        case CONTROLLER_INPUT_TYPE_HID: {
            // XInput compatible controllers are handled by XInput if available
            const bool has_xinput = xinput_load();

            int32 found = 0;
            if (has_xinput) {
                found += (int32) intrin_bits_count_32(xinput_controllers_scan(states, count, preferred_types));
            }

            found += hid_init_controllers(states, count, preferred_types, has_xinput);

            return found;
        };
        default:
            UNREACHABLE();
    }
}

/**
 * Registers for hid device interface notifications (WM_DEVICECHANGE)
 *
 * This tells the window to receive a WM whenever a input device changes
 *
 * @return Notification handle (NULL = failed)
 */
inline
HDEVNOTIFY input_device_notification_register(HWND hwnd) NO_EXCEPT
{
    DEV_BROADCAST_DEVICEINTERFACE_W filter = {};
    filter.dbcc_size = sizeof(filter);
    filter.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
    filter.dbcc_classguid = INPUT_GUID_DEVINTERFACE_HID;

    HDEVNOTIFY notification = RegisterDeviceNotificationW(hwnd, &filter, DEVICE_NOTIFY_WINDOW_HANDLE);
    LOG_TRUE_1(!notification, "[WARNING] Couldn't register device notifications");

    return notification;
}

// We unregister the WM messaging
inline
void input_device_notification_unregister(HDEVNOTIFY* const notification) NO_EXCEPT
{
    if (*notification) {
        UnregisterDeviceNotification(*notification);
        *notification = NULL;
    }
}

/**
 * Checks for new controllers that don't have connect events (XInput)
 *
 * WARNING: Slow (XInputGetState on empty slots), only call after device changes (e.g. timer after WM_DEVICECHANGE)
 *
 * @return Bit field of the inputs that changed
 */
inline
uint32 input_controllers_rescan(
    Input* const __restrict states, int32 count,
    ControllerInputType controller_api,
    const byte* const __restrict preferred_types = NULL
) NO_EXCEPT
{
    if (!input_controller_api_uses_xinput(controller_api)) {
        return 0;
    }

    return xinput_controllers_scan(states, count, preferred_types);
}

/**
 * Handles WM_INPUT_DEVICE_CHANGE (raw input devices: keyboard, mouse and raw input controllers)
 *
 * @return Bit field of the inputs that changed
 */
inline
uint32 input_raw_device_change(
    WPARAM wparam, LPARAM lparam,
    Input* const __restrict states, int32 count,
    ControllerInputType controller_api,
    const byte* const __restrict preferred_types = NULL
) NO_EXCEPT
{
    HANDLE device = (HANDLE) lparam;

    int32 index = -1;
    if (wparam == GIDC_ARRIVAL) {
        index = rawinput_device_add(
            states, count,
            device,
            controller_api == CONTROLLER_INPUT_TYPE_RAWINPUT,
            preferred_types
        );

        LOG_TRUE_1(index >= 0, "[INFO] Input device connected to input %d", {DATA_TYPE_INT32, &index});
    } else if (wparam == GIDC_REMOVAL) {
        index = rawinput_device_remove(states, count, device);

        LOG_TRUE_1(index >= 0, "[INFO] Input device disconnected from input %d", {DATA_TYPE_INT32, &index});
    }

    return index >= 0 ? (1U << index) : 0;
}

/**
 * Handles WM_DEVICECHANGE (hid device interfaces)
 *
 * @param needs_rescan Set to true if input_controllers_rescan() should be called again a little bit later
 *                     (e.g. XInput needs some time until a new controller is available)
 *
 * @return Bit field of the inputs that changed
 */
uint32 input_hid_device_change(
    WPARAM wparam, LPARAM lparam,
    Input* const __restrict states, int32 count,
    ControllerInputType controller_api,
    const byte* const __restrict preferred_types,
    bool* const __restrict needs_rescan
) NO_EXCEPT
{
    if ((wparam != DBT_DEVICEARRIVAL && wparam != DBT_DEVICEREMOVECOMPLETE)
        || controller_api == CONTROLLER_INPUT_TYPE_RAWINPUT
    ) {
        return 0;
    }

    const DEV_BROADCAST_HDR* const header = (const DEV_BROADCAST_HDR *) lparam;
    if (!header || header->dbch_devicetype != DBT_DEVTYP_DEVICEINTERFACE) {
        return 0;
    }

    const DEV_BROADCAST_DEVICEINTERFACE_W* const device = (const DEV_BROADCAST_DEVICEINTERFACE_W *) header;
    const bool has_xinput = input_controller_api_uses_xinput(controller_api);

    uint32 changed = 0;
    if (wparam == DBT_DEVICEARRIVAL) {
        if (controller_api == CONTROLLER_INPUT_TYPE_HID) {
            const int32 index = hid_controller_add(states, count, device->dbcc_name, preferred_types, has_xinput);
            if (index >= 0) {
                changed |= 1U << index;
            }
        }

        if (has_xinput && input_device_path_contains(device->dbcc_name, L"ig_")) {
            changed |= xinput_controllers_scan(states, count, preferred_types);

            // XInput may not see the controller yet
            *needs_rescan = true;
        }
    } else {
        const int32 index = hid_controller_find_by_path(states, count, device->dbcc_name);
        if (index >= 0) {
            input_controller_disconnect(&states[index]);
            changed |= 1U << index;
        }

        // XInput controllers can't be identified by their path, check all of them
        if (has_xinput && input_device_path_contains(device->dbcc_name, L"ig_")) {
            for (int32 i = 0; i < count; ++i) {
                XINPUT_STATE controller_state;
                if (states[i].controller.connection_type
                    && states[i].controller.api == CONTROLLER_INPUT_TYPE_XINPUT
                    && pXInputGetState((DWORD) states[i].controller.id, &controller_state) != ERROR_SUCCESS
                ) {
                    input_controller_disconnect(&states[i]);
                    changed |= 1U << i;
                }
            }
        }
    }

    return changed;
}

/**
 * Polls all connected controllers (once per tick, after input_frame_begin)
 *
 * @return Bit field of the inputs whose controller disconnected
 */
uint32 input_controllers_poll(
    Input* const states, int32 count,
    uint64 time
) NO_EXCEPT
{
    uint32 disconnected = 0;
    for (int32 i = 0; i < count; ++i) {
        Input* const input = &states[i];
        if (!input->controller.connection_type) {
            continue;
        }

        bool is_connected = true;
        switch (input->controller.api) {
            case CONTROLLER_INPUT_TYPE_HID: {
                is_connected = hid_controller_poll(input, time);
            } break;
            case CONTROLLER_INPUT_TYPE_XINPUT: {
                is_connected = xinput_controller_poll(input, time);
            } break;
            case CONTROLLER_INPUT_TYPE_RAWINPUT: {
                // The latest report was stored by the raw input handling (@see input_raw_handle)
                if (input->controller.has_report) {
                    input_controller_report_handle(input, input->controller.report, time);
                }
            } break;
            default: {}
        }

        if (!is_connected) {
            input_controller_disconnect(input);
            disconnected |= 1U << i;
        }
    }

    return disconnected;
}

// Closes all controller handles
inline
void input_controllers_cleanup(Input* const states, int32 count) NO_EXCEPT
{
    for (int32 i = 0; i < count; ++i) {
        if (states[i].controller.connection_type) {
            input_controller_disconnect(&states[i]);
        }
    }
}

int16 input_kbm_handle(
    InputMode mode,
    int32 max_inputs,
    Input* __restrict states, int32 state_count,
    v2_int16 window_dim,
    BufferMemory* const __restrict mem,
    uint64 time
) NO_EXCEPT
{
    PROFILE_DEBUG(PROFILE_INPUT_POLL);

    switch (mode) {
        case INPUT_MODE_EVENT: {
            return input_raw_handle_buffered(
                max_inputs,
                states,
                state_count,
                window_dim,
                mem,
                time
            );
        };
        case INPUT_MODE_POLLING: {
            return input_poll_handle(states, time);
        };
        default:
            UNREACHABLE();
    };
}

#endif
