/**
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
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
#include "controller/DualShock4.h"
#include <winDNS.h>

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
    while (*id != L'}' && *id != L'\0' && len < 32) {
        out_id[len++] = *id;
        ++id;
    }

    return true;
}

uint32 rawinput_kbm_init(
    HWND hwnd,
    Input* __restrict states,
    BufferMemory* const __restrict mem
) NO_EXCEPT
{
    uint32 device_count;
    GetRawInputDeviceList(NULL, &device_count, sizeof(RAWINPUTDEVICELIST));
    if (device_count == 0 || device_count > 1000) {
        return 0;
    }

    PRAWINPUTDEVICELIST pRawInputDeviceList;
    BUFFER_STACK_MEMORY(
        mem,
        (byte **) &pRawInputDeviceList,
        sizeof(RAWINPUTDEVICELIST) * device_count,
        alignof(RAWINPUTDEVICELIST)
    );
    device_count = GetRawInputDeviceList(pRawInputDeviceList, &device_count, sizeof(RAWINPUTDEVICELIST));

    // We always want at least one empty input device slot
    // @todo Change so that we store the actual number of devices
    if (device_count == 0 || device_count > 1000) {
        return 0;
    }

    int32 mouse_found = 0;
    int32 keyboard_found = 0;

    uint32 i;

    // @test Test with 5 devices connected
    for (i = 0; i < device_count; ++i) {
        uint32 cb_size = sizeof(RID_DEVICE_INFO);
        RID_DEVICE_INFO rdi;
        GetRawInputDeviceInfoW(pRawInputDeviceList[i].hDevice, RIDI_DEVICEINFO, &rdi, &cb_size);

        switch (rdi.dwType) {
            case RIM_TYPEMOUSE: {
                // Extract device ID
                wchar_t device_name[512];
                UINT name_size = sizeof(device_name);

                GetRawInputDeviceInfoW(
                    pRawInputDeviceList[i].hDevice,
                    RIDI_DEVICENAME,
                    device_name,
                    &name_size
                );

                wchar_t device_id[32] = {0};
                if (!rawinput_extract_vid_pid(device_name, device_id)) {
                    ASSERT_THROW();

                    continue;
                }

                // We now check if the device is already bound
                // Remember one physical device may have multiple logical devices
                // We need to group them together as one device
                int32 found_state = -1;
                for (int32 c = 0; c < 4; ++c) {
                    if (memcmp(states[c].mouse.device_name, device_id, sizeof(device_id)) == 0) {
                        found_state = c;
                        break;
                    }
                }

                if (found_state >= 0) {
                    // Physical device already bound
                    // We need to add the handle to the handle list of that device

                    // Find empty handle
                    for (int32 c = 0; c < ARRAY_COUNT(states[found_state].mouse.handle); ++c) {
                        if (!states[found_state].mouse.handle[c]) {
                            states[found_state].mouse.handle[c] = pRawInputDeviceList[i].hDevice;
                            break;
                        }
                    }
                } else {
                    // New device added
                    memcpy(states[mouse_found].mouse.device_name, device_id, sizeof(device_id));

                    states[mouse_found].mouse.handle[0] = pRawInputDeviceList[i].hDevice;
                    states[mouse_found].connection_type = INPUT_CONNECTION_TYPE_USB;
                    ++mouse_found;
                }
            } break;
            case RIM_TYPEKEYBOARD: {
                // Extract device ID
                wchar_t device_name[512];
                UINT name_size = sizeof(device_name);

                GetRawInputDeviceInfoW(
                    pRawInputDeviceList[i].hDevice,
                    RIDI_DEVICENAME,
                    device_name,
                    &name_size
                );

                wchar_t device_id[32] = {0};
                if (!rawinput_extract_vid_pid(device_name, device_id)) {
                    ASSERT_THROW();

                    continue;
                }

                // We now check if the device is already bound
                // Remember one physical device may have multiple logical devices
                // We need to group them together as one device
                int32 found_state = -1;
                for (int32 c = 0; c < 4; ++c) {
                    if (memcmp(states[c].keyboard.device_name, device_id, sizeof(device_id)) == 0) {
                        found_state = c;
                        break;
                    }
                }

                if (found_state >= 0) {
                    // Physical device already bound
                    // We need to add the handle to the handle list of that device

                    // Find empty handle
                    for (int32 c = 0; c < ARRAY_COUNT(states[found_state].keyboard.handle); ++c) {
                        if (!states[found_state].keyboard.handle[c]) {
                            states[found_state].keyboard.handle[c] = pRawInputDeviceList[i].hDevice;
                            break;
                        }
                    }
                } else {
                    // New device added
                    memcpy(states[keyboard_found].keyboard.device_name, device_id, sizeof(device_id));

                    states[keyboard_found].keyboard.handle[0] = pRawInputDeviceList[i].hDevice;
                    states[keyboard_found].connection_type = INPUT_CONNECTION_TYPE_USB;
                    ++keyboard_found;
                }
            } break;
            default: break;
        }
    }

    if (mouse_found || keyboard_found) {
        alignas(8) RAWINPUTDEVICE rid[2];

        // Mouse
        rid[0].usUsagePage = 0x01;
        rid[0].usUsage     = 0x02;
        rid[0].dwFlags     = RIDEV_DEVNOTIFY;
        rid[0].hwndTarget  = hwnd;

        // Keyboard
        rid[1].usUsagePage = 0x01;
        rid[1].usUsage     = 0x06;
        rid[1].dwFlags     = RIDEV_DEVNOTIFY;
        rid[1].hwndTarget  = hwnd;

        if (!RegisterRawInputDevices((PCRAWINPUTDEVICE) rid, 2, sizeof(RAWINPUTDEVICE))) {
            LOG_1("[WARNING] Couldn't register keyboard raw input device");
            ASSERT_THROW();
        }
    }

    return i;
}

// WARNING: While this works we highly recommend to use hid_init_controllers
uint32 rawinput_init_controllers(
    HWND hwnd,
    Input* __restrict states,
    RingMemory* const __restrict ring
) NO_EXCEPT
{
    uint32 device_count;
    GetRawInputDeviceList(NULL, &device_count, sizeof(RAWINPUTDEVICELIST));
    if (device_count == 0 || device_count > 1000) {
        return 0;
    }

    PRAWINPUTDEVICELIST pRawInputDeviceList = (PRAWINPUTDEVICELIST) memory_get(
        ring,
        sizeof(RAWINPUTDEVICELIST) * device_count,
        alignof(RAWINPUTDEVICELIST)
    );
    device_count = GetRawInputDeviceList(pRawInputDeviceList, &device_count, sizeof(RAWINPUTDEVICELIST));

    // We always want at least one empty input device slot
    if (device_count == 0 || device_count > 1000) {
        LOG_1("[WARNING] Found %d raw input devices", {DATA_TYPE_INT32, &device_count});
        return 0;
    }

    int32 controller_found = 0;

    uint32 i;
    for (i = 0; i < device_count; ++i) {
        uint32 cb_size = sizeof(RID_DEVICE_INFO);
        RID_DEVICE_INFO rdi;
        GetRawInputDeviceInfoW(pRawInputDeviceList[i].hDevice, RIDI_DEVICEINFO, &rdi, &cb_size);

        alignas(8) RAWINPUTDEVICE rid[1];

        switch (rdi.dwType) {
            case RIM_TYPEHID: {
                if (rdi.hid.usUsage == 0x05) {
                    if (states[controller_found].controller.handle != NULL) {
                        ++controller_found;
                    }

                    states[controller_found].controller.handle = pRawInputDeviceList[i].hDevice;
                    // @bug This is not always true, how to check?
                    states[controller_found].connection_type = INPUT_CONNECTION_TYPE_USB;

                    if (rdi.hid.dwVendorId == 0x054C
                        && (rdi.hid.dwProductId == 0x05C4 || rdi.hid.dwProductId == 0x09CC)
                    ) {
                        states[controller_found].controller_type = CONTROLLER_TYPE_DUALSHOCK4;
                    } else if (rdi.hid.dwVendorId == 0x054C
                        && (rdi.hid.dwProductId == 0x0CE6 || rdi.hid.dwProductId == 0x0DF2)
                    ) {
                        states[controller_found].controller_type = CONTROLLER_TYPE_DUALSENSE;
                    } else if (rdi.hid.dwVendorId == 0x045E && rdi.hid.dwProductId == 0x02E0) {
                        states[controller_found].controller_type = CONTROLLER_TYPE_XBOX_360;
                    } else if (rdi.hid.dwVendorId == 0x045E && rdi.hid.dwProductId == 0x02FF) {
                        states[controller_found].controller_type = CONTROLLER_TYPE_XBOX_ONE;
                    } else if (rdi.hid.dwVendorId == 0x045E && rdi.hid.dwProductId == 0x028E) {
                        states[controller_found].controller_type = CONTROLLER_TYPE_XBOX_S;
                    } else {
                        states[controller_found].controller_type = CONTROLLER_TYPE_OTHER;
                    }

                    // Gamepad
                    rid[0].usUsagePage = 0x01;
                    rid[0].usUsage	   = 0x05;
                    rid[0].dwFlags     = RIDEV_DEVNOTIFY;
                    rid[0].hwndTarget = hwnd;

                    if (!RegisterRawInputDevices((PCRAWINPUTDEVICE) rid, 1, sizeof(RAWINPUTDEVICE))) {
                        LOG_1("[WARNING] Couldn't register gamepad raw input device");
                        ASSERT_THROW();
                    }
                } else if (rdi.hid.usUsage == 0x04) {
                    if (states[controller_found].controller.handle != NULL) {
                        ++controller_found;
                    }

                    states[controller_found].controller.handle = pRawInputDeviceList[i].hDevice;
                    // @bug This is not always true, how to check?
                    states[controller_found].connection_type = INPUT_CONNECTION_TYPE_USB;
                    states[controller_found].controller_type = CONTROLLER_TYPE_OTHER;

                    // Joystick
                    rid[0].usUsagePage = 0x01;
                    rid[0].usUsage	   = 0x04;
                    rid[0].dwFlags     = RIDEV_DEVNOTIFY;
                    rid[0].hwndTarget = hwnd;

                    if (!RegisterRawInputDevices((PCRAWINPUTDEVICE) rid, 1, sizeof(RAWINPUTDEVICE))) {
                        LOG_1("[WARNING] Couldn't register joystick raw input device");
                        ASSERT_THROW();
                    }
                }
            } break;
            default: {}
        }
    }

    return i;
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
        const v2_int16 input_handle = input_mouse_find_by_handle(
            states,
            state_count,
            raw->header.hDevice
        );

        if (input_handle.vec[0] < 0
            || !states[input_handle.vec[0]].connection_type
        ) {
            return 0;
        }

        if (raw->data.mouse.usButtonFlags) {
            InputKey key;

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
            states[input_handle.vec[0]].state.x[0] = OMS_CLAMP(states[input_handle.vec[0]].state.x[0], (int16) 0, window_dim.width);
            states[input_handle.vec[0]].state.y[0] = OMS_CLAMP(states[input_handle.vec[0]].state.y[0], (int16) 0, window_dim.height);

            // We need to signal that the mouse was moved
            // We basically fake that a mouse movement is the same as a button
            const InputKey key = {
                SMN(scan_code) INPUT_MOUSE_PREFIX | INPUT_MOUSE_MOVE,
                SMN(virtual_code) 0,
                SMN(key_state) KEY_PRESS_TYPE_RELEASED,
                SMN(is_processed) false,
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
        const v2_int16 input_handle = input_keyboard_find_by_handle(
            states,
            state_count,
            raw->header.hDevice
        );

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

        // @todo we need to support vkey and MakeCode/ScanCode for input mode -> typing and recognizing the respective unicode
        const InputKey key = {
            (uint16) (raw->data.keyboard.MakeCode | INPUT_KEYBOARD_PREFIX),
            (uint16) (raw->data.keyboard.VKey | INPUT_KEYBOARD_PREFIX),
            new_state, false, 0, time
        };

        input_set_state(states[input_handle.vec[0]].state.active_keys, &key);
        states[input_handle.vec[0]].general_states |= INPUT_STATE_GENERAL_INPUT_CHANGE;
    } else if (raw->header.dwType == RIM_TYPEHID
        && raw->header.dwSize > sizeof(RAWINPUT)
    ) {
        // @performance This shouldn't be done every time, it should be polling based
        // Controllers often CONSTANTLY send data -> really bad
        // Maybe we can add timer usage instead of polling?
        // But we would still need to register them, right?
        // Ideally we wouldn't even have to register them then because they would still pollute the general buffer
        const v2_int16 input_handle = input_controller_find_by_handle(
            states,
            state_count,
            raw->header.hDevice
        );

        if (input_handle.vec[0] < 0
            || !states[input_handle.vec[0]].connection_type
            || time - states[input_handle.vec[0]].time_last_input_check < 5
        ) {
            return 0;
        }

        ControllerInput controller = {0};
        switch(states[input_handle.vec[0]].controller_type) {
            case CONTROLLER_TYPE_DUALSHOCK4: {
                input_map_dualshock4(&controller, states[input_handle.vec[0]].connection_type, raw->data.hid.bRawData);
            } break;
            default: {
            };
        }
        input_set_controller_state(&states[input_handle.vec[0]], &controller, time);

        states[input_handle.vec[0]].general_states |= INPUT_STATE_GENERAL_INPUT_CHANGE;
        states[input_handle.vec[0]].time_last_input_check = time;
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
            if (!pri->header.hDevice) {
                break;
            }

            // @performance Instead of passing all input states we should only pass the state that actually matters
            input_count += input_raw_handle(pri, states, state_count, window_dim, time);

            pri = NEXTRAWINPUTBLOCK(pri);
        }
    }

    return input_count;
}

#endif