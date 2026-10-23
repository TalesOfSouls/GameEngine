/**
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 *
 * XInput controllers (max 4, XUSER_MAX_COUNT)
 *
 * XInput has no connect/disconnect events:
 *      connect:    xinput_controllers_scan() checks the free slots (e.g. after WM_DEVICECHANGE @see InputWrapper.h)
 *                  WARNING: XInputGetState() on an empty slot is slow -> don't scan every tick
 *      disconnect: xinput_controller_poll() fails
 */
#pragma once
#ifndef COMS_PLATFORM_WIN32_INPUT_XINPUT_H
#define COMS_PLATFORM_WIN32_INPUT_XINPUT_H

#include <windows.h>
#include <XInput.h>

#include "../../../stdlib/Stdlib.h"
#include "../../../input/Input.cpp"
#include "../../../input/ControllerType.h"
#include "../../../input/ControllerInput.h"
#include "../../../system/Library.h"
#include "../../../system/Library.cpp"

// BEGIN: Dynamically load XInput
typedef DWORD (WINAPI *XInputGetState_t)(DWORD, XINPUT_STATE*);
typedef DWORD (WINAPI *XInputSetState_t)(DWORD, XINPUT_VIBRATION*);

// WARNING: These are per module (e.g. the exe and a hot reloadable dll have their own pointers)
//          -> modules that only poll bind them lazily (@see xinput_bind_loaded)
static XInputGetState_t pXInputGetState = NULL;
static XInputSetState_t pXInputSetState = NULL;

static LibraryHandle _xinput_lib;

static int _xinput_lib_ref_count = 0;
// END: Dynamically load XInput

// Device ids of xinput controllers are their slot (+ marker to never collide with hid path hashes in practice)
#define XINPUT_DEVICE_ID(user_index) (0x58494E5055540000ULL | (uint64) (user_index))

bool xinput_load() NO_EXCEPT
{
    if (_xinput_lib_ref_count) {
        ++_xinput_lib_ref_count;
        return true;
    }

    bool success = library_dyn_load(&_xinput_lib, L"xinput1_4.dll");
    if (!success) {
        success = library_dyn_load(&_xinput_lib, L"xinput1_3.dll");
    }

    if (!success) {
        success = library_dyn_load(&_xinput_lib, L"xinput9_1_0.dll");
    }

    if (!success) {
        return false;
    }

    pXInputGetState = (XInputGetState_t) library_dyn_proc(_xinput_lib, "XInputGetState");
    pXInputSetState = (XInputSetState_t) library_dyn_proc(_xinput_lib, "XInputSetState");

    if (!pXInputGetState || !pXInputSetState) {
        library_dyn_unload(&_xinput_lib);
        pXInputGetState = NULL;
        pXInputSetState = NULL;

        return false;
    }

    ++_xinput_lib_ref_count;

    return true;
}

/**
 * Binds the function pointers of an already loaded XInput dll (e.g. loaded by the exe via xinput_load)
 *
 * Used by modules that only poll (e.g. hot reloadable game dll)
 * Doesn't change the dll reference count -> no xinput_free() needed
 */
inline
bool xinput_bind_loaded() NO_EXCEPT
{
    if (pXInputGetState) {
        return true;
    }

    const wchar_t* const dlls[] = { L"xinput1_4.dll", L"xinput1_3.dll", L"xinput9_1_0.dll" };
    for (int32 i = 0; i < ARRAY_COUNT(dlls); ++i) {
        HMODULE module = GetModuleHandleW(dlls[i]);
        if (!module) {
            continue;
        }

        XInputGetState_t get_state = (XInputGetState_t) GetProcAddress(module, "XInputGetState");
        if (get_state) {
            pXInputGetState = get_state;
            pXInputSetState = (XInputSetState_t) GetProcAddress(module, "XInputSetState");

            return true;
        }
    }

    return false;
}

inline
void xinput_free() NO_EXCEPT
{
    if (_xinput_lib_ref_count > 1) {
        --_xinput_lib_ref_count;
        return;
    }

    if (_xinput_lib_ref_count == 1) {
        library_dyn_unload(&_xinput_lib);
    }

    pXInputGetState = NULL;
    pXInputSetState = NULL;
    _xinput_lib_ref_count = 0;
}

/**
 * Checks all xinput slots that are not assigned yet and assigns connected controllers to an input
 *
 * @param preferred_types InputType flags per input (@see input_state_assign), may be NULL
 *
 * @return Bit field of the inputs that got a new controller
 */
uint32 xinput_controllers_scan(
    Input* const __restrict states, int32 count,
    const byte* const __restrict preferred_types = NULL
) NO_EXCEPT
{
    if (!states || !pXInputGetState) {
        return 0;
    }

    uint32 changed = 0;
    for (uint32 user_index = 0; user_index < XUSER_MAX_COUNT; ++user_index) {
        const uint64 device_id = XINPUT_DEVICE_ID(user_index);

        // Slot already connected
        bool is_connected = false;
        for (int32 i = 0; i < count; ++i) {
            if (states[i].controller.connection_type
                && states[i].controller.api == CONTROLLER_INPUT_TYPE_XINPUT
                && states[i].controller.device_id == device_id
            ) {
                is_connected = true;
                break;
            }
        }

        if (is_connected) {
            continue;
        }

        XINPUT_STATE controller_state;
        if (pXInputGetState(user_index, &controller_state) != ERROR_SUCCESS) {
            continue;
        }

        // Same slot as before -> same input
        int32 index = input_controller_find_by_device_id(states, count, device_id);
        if (index < 0 || states[index].controller.connection_type) {
            index = input_state_assign(states, count, INPUT_DEVICE_KIND_CONTROLLER, preferred_types);
        }

        if (index < 0) {
            continue;
        }

        Input* const input = &states[index];

        // XInput doesn't tell us how the controller is connected
        input->controller.connection_type = INPUT_CONNECTION_TYPE_USB;
        input->controller.api = CONTROLLER_INPUT_TYPE_XINPUT;
        input->controller.device_id = device_id;
        input->controller.id = (int32) user_index;
        input->controller.handle = NULL;
        input->controller.has_report = false;
        input->controller_type = CONTROLLER_TYPE_XBOX_GENERIC;
        input->device_types |= INPUT_TYPE_CONTROLLER;
        input_state_connection_update(input);

        LOG_1(
            "[INFO] XInput controller %d connected to input %d",
            {DATA_TYPE_UINT32, &user_index}, {DATA_TYPE_INT32, &index}
        );

        changed |= 1U << index;
    }

    return changed;
}

// Thumb stick (-32768 - 32767) -> -128 - 127
FORCE_INLINE
int8 xinput_stick_value(SHORT value) NO_EXCEPT
{
    return (int8) (value >> 8);
}

// Y axis is inverted compared to the other controllers (up = negative, same as the mouse/window coordinates)
FORCE_INLINE
int8 xinput_stick_value_inverted(SHORT value) NO_EXCEPT
{
    // ~value = -value - 1 -> -32768 becomes 32767 (no overflow)
    return (int8) (((int32) ~value) >> 8);
}

inline
void input_map_xinput(ControllerInput* const __restrict controller, const XINPUT_GAMEPAD* const __restrict pad) NO_EXCEPT
{
    const WORD buttons = pad->wButtons;

    controller->button[CONTROLLER_BUTTON_DPAD_LEFT] = (buttons & XINPUT_GAMEPAD_DPAD_LEFT) ? 127 : 0;
    controller->button[CONTROLLER_BUTTON_DPAD_RIGHT] = (buttons & XINPUT_GAMEPAD_DPAD_RIGHT) ? 127 : 0;
    controller->button[CONTROLLER_BUTTON_DPAD_UP] = (buttons & XINPUT_GAMEPAD_DPAD_UP) ? 127 : 0;
    controller->button[CONTROLLER_BUTTON_DPAD_DOWN] = (buttons & XINPUT_GAMEPAD_DPAD_DOWN) ? 127 : 0;

    controller->button[CONTROLLER_BUTTON_OTHER_0] = (buttons & XINPUT_GAMEPAD_START) ? 1 : 0;
    controller->button[CONTROLLER_BUTTON_OTHER_1] = (buttons & XINPUT_GAMEPAD_BACK) ? 1 : 0;

    controller->button[CONTROLLER_BUTTON_SHOULDER_RIGHT_BUTTON] = (buttons & XINPUT_GAMEPAD_RIGHT_SHOULDER) ? 1 : 0;
    controller->button[CONTROLLER_BUTTON_SHOULDER_LEFT_BUTTON] = (buttons & XINPUT_GAMEPAD_LEFT_SHOULDER) ? 1 : 0;

    // 0 - 255 -> 0 - 127
    controller->button[CONTROLLER_BUTTON_SHOULDER_RIGHT_TRIGGER] = (int8) (pad->bRightTrigger >> 1);
    controller->is_analog[CONTROLLER_BUTTON_SHOULDER_RIGHT_TRIGGER] = true;

    controller->button[CONTROLLER_BUTTON_SHOULDER_LEFT_TRIGGER] = (int8) (pad->bLeftTrigger >> 1);
    controller->is_analog[CONTROLLER_BUTTON_SHOULDER_LEFT_TRIGGER] = true;

    // Same positions as on a playstation controller (Y = triangle, B = circle, A = cross, X = square)
    controller->button[CONTROLLER_BUTTON_T] = (buttons & XINPUT_GAMEPAD_Y) ? 1 : 0;
    controller->button[CONTROLLER_BUTTON_C] = (buttons & XINPUT_GAMEPAD_B) ? 1 : 0;
    controller->button[CONTROLLER_BUTTON_X] = (buttons & XINPUT_GAMEPAD_A) ? 1 : 0;
    controller->button[CONTROLLER_BUTTON_S] = (buttons & XINPUT_GAMEPAD_X) ? 1 : 0;

    controller->button[CONTROLLER_BUTTON_STICK_LEFT_HORIZONTAL] = xinput_stick_value(pad->sThumbLX);
    controller->is_analog[CONTROLLER_BUTTON_STICK_LEFT_HORIZONTAL] = true;
    controller->button[CONTROLLER_BUTTON_STICK_LEFT_VERTICAL] = xinput_stick_value_inverted(pad->sThumbLY);
    controller->is_analog[CONTROLLER_BUTTON_STICK_LEFT_VERTICAL] = true;
    controller->button[CONTROLLER_BUTTON_STICK_LEFT_BUTTON] = (buttons & XINPUT_GAMEPAD_LEFT_THUMB) ? 1 : 0;

    controller->button[CONTROLLER_BUTTON_STICK_RIGHT_HORIZONTAL] = xinput_stick_value(pad->sThumbRX);
    controller->is_analog[CONTROLLER_BUTTON_STICK_RIGHT_HORIZONTAL] = true;
    controller->button[CONTROLLER_BUTTON_STICK_RIGHT_VERTICAL] = xinput_stick_value_inverted(pad->sThumbRY);
    controller->is_analog[CONTROLLER_BUTTON_STICK_RIGHT_VERTICAL] = true;
    controller->button[CONTROLLER_BUTTON_STICK_RIGHT_BUTTON] = (buttons & XINPUT_GAMEPAD_RIGHT_THUMB) ? 1 : 0;
}

/**
 * Polls the controller state
 *
 * @return false = controller disconnected (the caller must disconnect the controller)
 */
inline
bool xinput_controller_poll(Input* const input, uint64 time) NO_EXCEPT
{
    // The function pointers are per module (e.g. hot reloadable game dll)
    // The dll itself was loaded during the setup (@see input_controller_init)
    if (!pXInputGetState && !xinput_bind_loaded()) {
        return false;
    }

    XINPUT_STATE controller_state;
    const DWORD result = pXInputGetState((DWORD) input->controller.id, &controller_state);
    if (result != ERROR_SUCCESS) {
        return false;
    }

    ControllerInput controller = {};
    input_map_xinput(&controller, &controller_state.Gamepad);
    input_set_controller_state(input, &controller, time);
    input->time_last_input_check = time;

    return true;
}

#endif
