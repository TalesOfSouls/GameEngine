/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_PLATFORM_WIN32_INPUT_XINPUT_H
#define COMS_PLATFORM_WIN32_INPUT_XINPUT_H

#include <XInput.h>
#include <windows.h>

#include "../../../input/ControllerInput.h"
#include "../../../stdlib/Stdlib.h"
#include "../../../system/Library.h"
#include "../../../system/Library.cpp"

// @todo consider to remove some static and defines since we are never calling it somewhere else

// BEGIN: Dynamically load XInput
typedef DWORD (WINAPI *XInputGetState_t)(DWORD, XINPUT_STATE*);
typedef DWORD (WINAPI *XInputSetState_t)(DWORD, XINPUT_VIBRATION*);

static XInputGetState_t pXInputGetState = NULL;
static XInputSetState_t pXInputSetState = NULL;

static LibraryHandle _xinput_lib;

static int _xinput_lib_ref_count = 0;
// END: Dynamically load XInput

bool xinput_load() {
    if (_xinput_lib_ref_count) {
        ++_xinput_lib_ref_count;
        return true;
    }

    bool success = library_dyn_load(&_xinput_lib, L"xinput1_4.dll");
    if(!success) {
        success = library_dyn_load(&_xinput_lib, L"xinput1_3.dll");

        if (!success) {
            return false;
        }
    }

    pXInputGetState = (XInputGetState_t) library_dyn_proc(_xinput_lib, "XInputGetState");
    pXInputSetState = (XInputSetState_t) library_dyn_proc(_xinput_lib, "XInputSetState");

    if (!pXInputGetState || !pXInputSetState) {
        return false;
    }

    ++_xinput_lib_ref_count;

    return true;
}

inline
void xinput_free() NO_EXCEPT
{
    if (_xinput_lib_ref_count > 1) {
        --_xinput_lib_ref_count;
        return;
    }

    library_dyn_unload(&_xinput_lib);
    _xinput_lib_ref_count = 0;
}

ControllerInput* xinput_init_controllers()
{
    uint32 c = 0;
    for (uint32 controller_index = 0; controller_index < XUSER_MAX_COUNT; ++controller_index) {
        XINPUT_STATE controller_state;
        if (pXInputGetState(controller_index, &controller_state) == ERROR_SUCCESS) {
            ++c;
        }
    }

    // We always want at least one empty controller slot
    // @todo Change so that we store the actual number of devices
    ControllerInput* const controllers = (ControllerInput *) calloc((c + 1), sizeof(ControllerInput));

    if (c == 0) {
        return controllers;
    }

    c = 0;
    for (uint32 controller_index = 0; controller_index < XUSER_MAX_COUNT; ++controller_index) {
        XINPUT_STATE controller_state;
        if (pXInputGetState(controller_index, &controller_state) == ERROR_SUCCESS) {
            ++c;
        }
    }

    return controllers;
}

inline
void input_map_xinput(ControllerInput* controller, int32 controller_id) NO_EXCEPT
{
    XINPUT_STATE controller_state;
    if (pXInputGetState(controller_id, &controller_state) != ERROR_SUCCESS) {
        return;
    }

    controller->button[CONTROLLER_BUTTON_DPAD_LEFT] = (controller_state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_LEFT) * 127;
    controller->button[CONTROLLER_BUTTON_DPAD_RIGHT] = (controller_state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT) * 127;
    controller->button[CONTROLLER_BUTTON_DPAD_UP] = (controller_state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_UP) * 127;
    controller->button[CONTROLLER_BUTTON_DPAD_DOWN] = (controller_state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_DOWN) * 127;

    controller->button[CONTROLLER_BUTTON_OTHER_0] = controller_state.Gamepad.wButtons & XINPUT_GAMEPAD_START;
    controller->button[CONTROLLER_BUTTON_OTHER_1] = controller_state.Gamepad.wButtons & XINPUT_GAMEPAD_BACK;

    controller->button[CONTROLLER_BUTTON_SHOULDER_RIGHT_BUTTON] = (controller_state.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER) ? 1 : 0;
    controller->button[CONTROLLER_BUTTON_SHOULDER_LEFT_BUTTON] = (controller_state.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER) ? 1 : 0;

    controller->button[CONTROLLER_BUTTON_SHOULDER_RIGHT_TRIGGER] = controller_state.Gamepad.bRightTrigger;
    controller->is_analog[CONTROLLER_BUTTON_SHOULDER_RIGHT_TRIGGER] = true;

    controller->button[CONTROLLER_BUTTON_SHOULDER_LEFT_TRIGGER] = controller_state.Gamepad.bLeftTrigger;
    controller->is_analog[CONTROLLER_BUTTON_SHOULDER_LEFT_TRIGGER] = true;

    controller->button[CONTROLLER_BUTTON_T] = (controller_state.Gamepad.wButtons & XINPUT_GAMEPAD_Y) ? 1 : 0;
    controller->button[CONTROLLER_BUTTON_C] = (controller_state.Gamepad.wButtons & XINPUT_GAMEPAD_B) ? 1 : 0;
    controller->button[CONTROLLER_BUTTON_X] = (controller_state.Gamepad.wButtons & XINPUT_GAMEPAD_A) ? 1 : 0;
    controller->button[CONTROLLER_BUTTON_S] = (controller_state.Gamepad.wButtons & XINPUT_GAMEPAD_X) ? 1 : 0;

    controller->button[CONTROLLER_BUTTON_STICK_LEFT_HORIZONTAL] = (byte) OMS_MIN(controller_state.Gamepad.sThumbLX, (short) 127);
    controller->button[CONTROLLER_BUTTON_STICK_LEFT_VERTICAL] = (byte) OMS_MIN(controller_state.Gamepad.sThumbLY, (short) 127);
    controller->is_analog[CONTROLLER_BUTTON_STICK_LEFT_VERTICAL] = true;
    controller->button[CONTROLLER_BUTTON_STICK_LEFT_BUTTON] = controller_state.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_THUMB;

    controller->button[CONTROLLER_BUTTON_STICK_RIGHT_HORIZONTAL] = (byte) OMS_MIN(controller_state.Gamepad.sThumbRX, (short) 127);
    controller->button[CONTROLLER_BUTTON_STICK_RIGHT_VERTICAL] = (byte) OMS_MIN(controller_state.Gamepad.sThumbRY, (short) 127);
    controller->is_analog[CONTROLLER_BUTTON_STICK_RIGHT_VERTICAL] = true;
    controller->button[CONTROLLER_BUTTON_STICK_RIGHT_BUTTON] = controller_state.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_THUMB;
}

#endif