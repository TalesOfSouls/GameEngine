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
#include "../../../stdlib/Types.h"

// @todo consider to remove some global_persist and defines since we are never calling it somewhere else

// BEGIN: Dynamically load XInput
typedef DWORD WINAPI x_input_get_state(DWORD, XINPUT_STATE*);
DWORD WINAPI XInputGetStateStub(DWORD, XINPUT_STATE*) {
    return 0;
}
global_persist x_input_get_state* XInputGetState_ = XInputGetStateStub;
#define XInputGetState XInputGetState_

typedef DWORD WINAPI x_input_set_state(DWORD, XINPUT_VIBRATION*);
DWORD WINAPI XInputSetStateStub(DWORD, XINPUT_VIBRATION*) {
    return 0;
}
global_persist x_input_set_state* XInputSetState_ = XInputSetStateStub;
#define XInputSetState XInputSetState_

void xinput_load() {
    HMODULE lib = LoadLibraryExA((LPCSTR) "xinput1_4.dll", NULL, LOAD_LIBRARY_SEARCH_SYSTEM32);
    if(!lib) {
        // @todo Log
        lib = LoadLibraryExA((LPCSTR) "xinput1_3.dll", NULL, LOAD_LIBRARY_SEARCH_SYSTEM32);
    }

    if (!lib) {
        // @todo Log
        return;
    }

    XInputGetState = (x_input_get_state *) GetProcAddress(lib, "XInputGetState");
    XInputSetState = (x_input_set_state *) GetProcAddress(lib, "XInputSetState");

    if (!XInputGetState || !XInputSetState) {
        // @todo Log
        return;
    }
}
// END: Dynamically load XInput

ControllerInput* xinput_init_controllers()
{
    uint32 c = 0;
    for (uint32 controller_index = 0; controller_index < XUSER_MAX_COUNT; ++controller_index) {
        XINPUT_STATE controller_state;
        if (XInputGetState(controller_index, &controller_state) == ERROR_SUCCESS) {
            ++c;
        }
    }

    // We always want at least one empty controller slot
    // @todo Change so that we store the actual number of devices
    ControllerInput *controllers = (ControllerInput *) calloc((c + 1), sizeof(ControllerInput));

    if (c == 0) {
        return controllers;
    }

    c = 0;
    for (uint32 controller_index = 0; controller_index < XUSER_MAX_COUNT; ++controller_index) {
        XINPUT_STATE controller_state;
        if (XInputGetState(controller_index, &controller_state) == ERROR_SUCCESS) {
            ++c;
        }
    }

    return controllers;
}

inline
void input_map_xinput(ControllerInput* controller, int32 controller_id)
{
    XINPUT_STATE controller_state;
    if (XInputGetState(controller_id, &controller_state) != ERROR_SUCCESS) {
        return;
    }

    controller->button[CONTROLLER_BUTTON_DPAD_LEFT] = (controller_state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_LEFT) * 127;
    controller->button[CONTROLLER_BUTTON_DPAD_RIGHT] = (controller_state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT) * 127;
    controller->button[CONTROLLER_BUTTON_DPAD_UP] = (controller_state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_UP) * 127;
    controller->button[CONTROLLER_BUTTON_DPAD_DOWN] = (controller_state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_DOWN) * 127;

    controller->button[CONTROLLER_BUTTON_OTHER_0] = controller_state.Gamepad.wButtons & XINPUT_GAMEPAD_START;
    controller->button[CONTROLLER_BUTTON_OTHER_1] = controller_state.Gamepad.wButtons & XINPUT_GAMEPAD_BACK;

    controller->button[CONTROLLER_BUTTON_SHOULDER_RIGHT_BUTTON] = controller_state.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER ? 1: 0;
    controller->button[CONTROLLER_BUTTON_SHOULDER_LEFT_BUTTON] = controller_state.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER ? 1: 0;

    controller->button[CONTROLLER_BUTTON_SHOULDER_RIGHT_TRIGGER] = controller_state.Gamepad.bRightTrigger;
    controller->is_analog[CONTROLLER_BUTTON_SHOULDER_RIGHT_TRIGGER] = true;

    controller->button[CONTROLLER_BUTTON_SHOULDER_LEFT_TRIGGER] = controller_state.Gamepad.bLeftTrigger;
    controller->is_analog[CONTROLLER_BUTTON_SHOULDER_LEFT_TRIGGER] = true;

    controller->button[CONTROLLER_BUTTON_T] = controller_state.Gamepad.wButtons & XINPUT_GAMEPAD_Y ? 1: 0;
    controller->button[CONTROLLER_BUTTON_C] = controller_state.Gamepad.wButtons & XINPUT_GAMEPAD_B ? 1: 0;
    controller->button[CONTROLLER_BUTTON_X] = controller_state.Gamepad.wButtons & XINPUT_GAMEPAD_A ? 1: 0;
    controller->button[CONTROLLER_BUTTON_S] = controller_state.Gamepad.wButtons & XINPUT_GAMEPAD_X ? 1: 0;

    controller->button[CONTROLLER_BUTTON_STICK_LEFT_HORIZONTAL] = (byte) OMS_MIN(controller_state.Gamepad.sThumbLX, 127);
    controller->button[CONTROLLER_BUTTON_STICK_LEFT_VERTICAL] = (byte) OMS_MIN(controller_state.Gamepad.sThumbLY, 127);
    controller->is_analog[CONTROLLER_BUTTON_STICK_LEFT_VERTICAL] = true;
    controller->button[CONTROLLER_BUTTON_STICK_LEFT_BUTTON] = controller_state.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_THUMB;

    controller->button[CONTROLLER_BUTTON_STICK_RIGHT_HORIZONTAL] = (byte) OMS_MIN(controller_state.Gamepad.sThumbRX, 127);
    controller->button[CONTROLLER_BUTTON_STICK_RIGHT_VERTICAL] = (byte) OMS_MIN(controller_state.Gamepad.sThumbRY, 127);
    controller->is_analog[CONTROLLER_BUTTON_STICK_RIGHT_VERTICAL] = true;
    controller->button[CONTROLLER_BUTTON_STICK_RIGHT_BUTTON] = controller_state.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_THUMB;
}

#endif