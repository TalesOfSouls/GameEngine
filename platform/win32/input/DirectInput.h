/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_PLATFORM_WIN32_INPUT_DIRECTINPUT_H
#define COMS_PLATFORM_WIN32_INPUT_DIRECTINPUT_H

#include <dinput.h>
#include "../../../input/Input.h"
#include "../../../input/ControllerInput.h"
#include "../../../stdlib/Types.h"

BOOL CALLBACK EnumJoysticksCallback(const DIDEVICEINSTANCE* instance, void* pDevicePtr)
{
    LPDIRECTINPUT8 pDirectInput = (LPDIRECTINPUT8)pDevicePtr;
    LPDIRECTINPUTDEVICE8* pDIDevice = (LPDIRECTINPUTDEVICE8*)pDevicePtr;

    if (SUCCEEDED(pDirectInput->CreateDevice(instance->guidInstance, pDIDevice, NULL))) {
        return DIENUM_STOP;
    }

    return DIENUM_CONTINUE;
}

int32 directinput_init_controllers(HWND hwnd, Input* __restrict states)
{
    LPDIRECTINPUT8* pDirectInput = NULL;
    HRESULT hr;

    // Initialize DirectInput interface if it’s not already initialized
    hr = DirectInput8Create(GetModuleHandle(NULL), DIRECTINPUT_VERSION, IID_IDirectInput8, (void**)pDirectInput, NULL);
    if (FAILED(hr)) {
        return 0;
    }

    // Enumerate devices to find a connected game controller
    hr = (*pDirectInput)->EnumDevices(DI8DEVCLASS_GAMECTRL, EnumJoysticksCallback, states->direct_controller, DIEDFL_ATTACHEDONLY);
    if (FAILED(hr) || *states->direct_controller == NULL) {
        return 0;
    }

    // Set data format for the device to use the joystick state format
    hr = (*states->direct_controller)->SetDataFormat(&c_dfDIJoystick2);
    if (FAILED(hr)) {
        (*states->direct_controller)->Release();
        *states->direct_controller = NULL;
        return 0;
    }

    // Set the cooperative level for the device
    hr = (*states->direct_controller)->SetCooperativeLevel(hwnd, DISCL_FOREGROUND | DISCL_EXCLUSIVE);
    if (FAILED(hr)) {
        (*states->direct_controller)->Release();
        *states->direct_controller = NULL;
        return 0;
    }

    // Acquire the device for use
    hr = (*states->direct_controller)->Acquire();
    if (FAILED(hr)) {
        (*states->direct_controller)->Release();
        *states->direct_controller = NULL;
        return 0;
    }

    return 1;
}

inline void input_map_directinput(ControllerInput* controller, LPDIRECTINPUTDEVICE8 pDIDevice)
{
    if (FAILED(pDIDevice->Poll())) {
        if (FAILED(pDIDevice->Acquire())) {
            return;
        }
    }

    // Define a buffer for DirectInput state
    DIJOYSTATE2 controller_state;
    if (FAILED(pDIDevice->GetDeviceState(sizeof(DIJOYSTATE2), &controller_state))) {
        return;
    }

    controller->button[CONTROLLER_BUTTON_DPAD_UP] = (controller_state.rgdwPOV[0] == 0) ? 127 : 0;
    controller->button[CONTROLLER_BUTTON_DPAD_RIGHT] = (controller_state.rgdwPOV[0] == 9000) ? 127 : 0;
    controller->button[CONTROLLER_BUTTON_DPAD_DOWN] = (controller_state.rgdwPOV[0] == 18000) ? 127 : 0;
    controller->button[CONTROLLER_BUTTON_DPAD_LEFT] = (controller_state.rgdwPOV[0] == 27000) ? 127 : 0;

    controller->button[CONTROLLER_BUTTON_OTHER_0] = (controller_state.rgbButtons[7] & 0x80) ? 127 : 0; // Start
    controller->button[CONTROLLER_BUTTON_OTHER_1] = (controller_state.rgbButtons[6] & 0x80) ? 127 : 0; // Back

    controller->button[CONTROLLER_BUTTON_SHOULDER_RIGHT_BUTTON] = (controller_state.rgbButtons[5] & 0x80) ? 127 : 0;
    controller->button[CONTROLLER_BUTTON_SHOULDER_LEFT_BUTTON] = (controller_state.rgbButtons[4] & 0x80) ? 127 : 0;

    controller->button[CONTROLLER_BUTTON_SHOULDER_RIGHT_TRIGGER] = controller_state.lZ > 0 ? controller_state.lZ : 0;
    controller->is_analog[CONTROLLER_BUTTON_SHOULDER_RIGHT_TRIGGER] = true;

    controller->button[CONTROLLER_BUTTON_SHOULDER_LEFT_TRIGGER] = controller_state.lZ < 0 ? -controller_state.lZ : 0;
    controller->is_analog[CONTROLLER_BUTTON_SHOULDER_LEFT_TRIGGER] = true;

    controller->button[CONTROLLER_BUTTON_T] = (controller_state.rgbButtons[3] & 0x80) ? 127 : 0;
    controller->button[CONTROLLER_BUTTON_C] = (controller_state.rgbButtons[1] & 0x80) ? 127 : 0;
    controller->button[CONTROLLER_BUTTON_X] = (controller_state.rgbButtons[0] & 0x80) ? 127 : 0;
    controller->button[CONTROLLER_BUTTON_S] = (controller_state.rgbButtons[2] & 0x80) ? 127 : 0;

    controller->button[CONTROLLER_BUTTON_STICK_LEFT_HORIZONTAL] = controller_state.lX;
    controller->button[CONTROLLER_BUTTON_STICK_LEFT_VERTICAL] = controller_state.lY;
    controller->is_analog[CONTROLLER_BUTTON_STICK_LEFT_HORIZONTAL] = true;
    controller->is_analog[CONTROLLER_BUTTON_STICK_LEFT_VERTICAL] = true;
    controller->button[CONTROLLER_BUTTON_STICK_LEFT_BUTTON] = (controller_state.rgbButtons[8] & 0x80) ? 127 : 0;

    controller->button[CONTROLLER_BUTTON_STICK_RIGHT_HORIZONTAL] = controller_state.lRx;
    controller->button[CONTROLLER_BUTTON_STICK_RIGHT_VERTICAL] = controller_state.lRy;
    controller->is_analog[CONTROLLER_BUTTON_STICK_RIGHT_HORIZONTAL] = true;
    controller->is_analog[CONTROLLER_BUTTON_STICK_RIGHT_VERTICAL] = true;
    controller->button[CONTROLLER_BUTTON_STICK_RIGHT_BUTTON] = (controller_state.rgbButtons[9] & 0x80) ? 127 : 0;

    controller->button[CONTROLLER_BUTTON_OTHER_2] = (controller_state.rgbButtons[10] & 0x80) ? 127 : 0;
    controller->button[CONTROLLER_BUTTON_OTHER_3] = (controller_state.rgbButtons[11] & 0x80) ? 127 : 0;
    controller->button[CONTROLLER_BUTTON_OTHER_4] = (controller_state.rgbButtons[12] & 0x80) ? 127 : 0;
    controller->button[CONTROLLER_BUTTON_OTHER_5] = (controller_state.rgbButtons[13] & 0x80) ? 127 : 0;
    controller->button[CONTROLLER_BUTTON_OTHER_6] = (controller_state.rgbButtons[14] & 0x80) ? 127 : 0;
    controller->button[CONTROLLER_BUTTON_OTHER_7] = (controller_state.rgbButtons[15] & 0x80) ? 127 : 0;
}

#endif