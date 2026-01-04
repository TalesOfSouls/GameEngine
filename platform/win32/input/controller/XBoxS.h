// Maybe: https://xboxdevwiki.net/Xbox_Input_Devices

/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 *
 * Xbox Series S / X Controller (HID – Win32)
 */
#ifndef COMS_PLATFORM_WIN32_INPUT_CONTROLLER_XBOXS_H
#define COMS_PLATFORM_WIN32_INPUT_CONTROLLER_XBOXS_H

#include "../../../../stdlib/Stdlib.h"
#include "../../../../input/ControllerInput.h"
#include "../../../../input/InputConnectionType.h"
#include "../../../../utils/BitUtils.h"

inline
void input_map_xboxs(
    ControllerInput* controller,
    InputConnectionType /*connection_type*/,
    const byte* data
) NO_EXCEPT
{
    /*
     * Typical HID layout (simplified):
     * 0: Report ID
     * 1: LX
     * 2: LY
     * 3: RX
     * 4: RY
     * 5: Buttons (A, B, X, Y, LB, RB, View, Menu)
     * 6: Buttons (DPad + Stick buttons)
     * 7: LT
     * 8: RT
     */

    ++data; // skip report id

    // --- Left Stick ---
    controller->button[CONTROLLER_BUTTON_STICK_LEFT_HORIZONTAL] = (int8) (*data++ - 128);
    controller->is_analog[CONTROLLER_BUTTON_STICK_LEFT_HORIZONTAL] = true;

    controller->button[CONTROLLER_BUTTON_STICK_LEFT_VERTICAL] = (int8) (*data++ - 128);
    controller->is_analog[CONTROLLER_BUTTON_STICK_LEFT_VERTICAL] = true;

    // --- Right Stick ---
    controller->button[CONTROLLER_BUTTON_STICK_RIGHT_HORIZONTAL] = (int8) (*data++ - 128);
    controller->is_analog[CONTROLLER_BUTTON_STICK_RIGHT_HORIZONTAL] = true;

    controller->button[CONTROLLER_BUTTON_STICK_RIGHT_VERTICAL] = (int8) (*data++ - 128);
    controller->is_analog[CONTROLLER_BUTTON_STICK_RIGHT_VERTICAL] = true;

    // --- Face & Shoulder Buttons ---
    controller->button[CONTROLLER_BUTTON_X] = BITS_GET_8_L2R(*data, 0, 1); // A
    controller->button[CONTROLLER_BUTTON_S] = BITS_GET_8_L2R(*data, 1, 1); // B
    controller->button[CONTROLLER_BUTTON_T] = BITS_GET_8_L2R(*data, 2, 1); // X
    controller->button[CONTROLLER_BUTTON_C] = BITS_GET_8_L2R(*data, 3, 1); // Y

    controller->button[CONTROLLER_BUTTON_SHOULDER_LEFT_BUTTON]  = BITS_GET_8_L2R(*data, 4, 1); // LB
    controller->button[CONTROLLER_BUTTON_SHOULDER_RIGHT_BUTTON] = BITS_GET_8_L2R(*data, 5, 1); // RB

    controller->button[CONTROLLER_BUTTON_OTHER_1] = BITS_GET_8_L2R(*data, 6, 1); // View
    controller->button[CONTROLLER_BUTTON_OTHER_0] = BITS_GET_8_L2R(*data, 7, 1); // Menu

    ++data;

    // --- D-Pad (bitmask, not hat-switch) ---
    controller->button[CONTROLLER_BUTTON_DPAD_UP]    = BITS_GET_8_L2R(*data, 0, 1) ? 127 : 0;
    controller->button[CONTROLLER_BUTTON_DPAD_DOWN]  = BITS_GET_8_L2R(*data, 1, 1) ? 127 : 0;
    controller->button[CONTROLLER_BUTTON_DPAD_LEFT]  = BITS_GET_8_L2R(*data, 2, 1) ? 127 : 0;
    controller->button[CONTROLLER_BUTTON_DPAD_RIGHT] = BITS_GET_8_L2R(*data, 3, 1) ? 127 : 0;

    controller->button[CONTROLLER_BUTTON_STICK_LEFT_BUTTON]  = BITS_GET_8_L2R(*data, 4, 1); // L3
    controller->button[CONTROLLER_BUTTON_STICK_RIGHT_BUTTON] = BITS_GET_8_L2R(*data, 5, 1); // R3

    controller->button[CONTROLLER_BUTTON_OTHER_2] = BITS_GET_8_L2R(*data, 6, 1); // Xbox button

    ++data;

    // --- Triggers ---
    controller->button[CONTROLLER_BUTTON_SHOULDER_LEFT_TRIGGER] = *data++;
    controller->is_analog[CONTROLLER_BUTTON_SHOULDER_LEFT_TRIGGER] = true;

    controller->button[CONTROLLER_BUTTON_SHOULDER_RIGHT_TRIGGER] = *data++;
    controller->is_analog[CONTROLLER_BUTTON_SHOULDER_RIGHT_TRIGGER] = true;

    // --- No gyro on Xbox controllers ---
    controller->gyro_x = 0;
    controller->gyro_y = 0;
    controller->gyro_z = 0;
}

#endif
