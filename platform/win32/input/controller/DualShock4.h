/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 * @see       https://dsremap.readthedocs.io/en/latest/reverse.html
 */
#ifndef COMS_PLATFORM_WIN32_INPUT_CONTROLLER_DUALSHOCK4_H
#define COMS_PLATFORM_WIN32_INPUT_CONTROLLER_DUALSHOCK4_H

#include "../../../../stdlib/Stdlib.h"
#include "../../../../input/ControllerInput.h"
#include "../../../../input/InputConnectionType.h"
#include "../../../../utils/BitUtils.h"

inline
void input_map_dualshock4(
    ControllerInput* controller,
    InputConnectionType connection_type,
    const byte* data
) NO_EXCEPT
{
    // The report id decides the layout, not the connection type:
    //      0x01 = usb and bluetooth "simple" report -> data starts at byte 1
    //      0x11 = bluetooth extended report         -> data starts at byte 3, otherwise identical
    PSEUDO_USE(connection_type);
    data += (data[0] == 0x11) ? 3 : 1;

    controller->button[CONTROLLER_BUTTON_STICK_LEFT_HORIZONTAL] = (int8) ((int32) *data++ - 128);
    controller->is_analog[CONTROLLER_BUTTON_STICK_LEFT_HORIZONTAL] = true;

    controller->button[CONTROLLER_BUTTON_STICK_LEFT_VERTICAL] = (int8) ((int32) *data++ - 128);
    controller->is_analog[CONTROLLER_BUTTON_STICK_LEFT_VERTICAL] = true;

    controller->button[CONTROLLER_BUTTON_STICK_RIGHT_HORIZONTAL] = (int8) ((int32) *data++ - 128);
    controller->is_analog[CONTROLLER_BUTTON_STICK_RIGHT_HORIZONTAL] = true;

    controller->button[CONTROLLER_BUTTON_STICK_RIGHT_VERTICAL] = (int8) ((int32) *data++ - 128);
    controller->is_analog[CONTROLLER_BUTTON_STICK_RIGHT_VERTICAL] = true;

    controller->button[CONTROLLER_BUTTON_T] = BITS_GET_8_L2R(*data, 0, 1);
    controller->button[CONTROLLER_BUTTON_C] = BITS_GET_8_L2R(*data, 1, 1);
    controller->button[CONTROLLER_BUTTON_X] = BITS_GET_8_L2R(*data, 2, 1);
    controller->button[CONTROLLER_BUTTON_S] = BITS_GET_8_L2R(*data, 3, 1);

    uint32 d_pad_state = BITS_GET_8_L2R(*data, 4, 4);
    if (d_pad_state == 8) {
        controller->button[CONTROLLER_BUTTON_DPAD_LEFT] = 0;
        controller->button[CONTROLLER_BUTTON_DPAD_RIGHT] = 0;
        controller->button[CONTROLLER_BUTTON_DPAD_UP] = 0;
        controller->button[CONTROLLER_BUTTON_DPAD_DOWN] = 0;
    } else if (d_pad_state == 0) {
        controller->button[CONTROLLER_BUTTON_DPAD_LEFT] = 0;
        controller->button[CONTROLLER_BUTTON_DPAD_RIGHT] = 0;
        controller->button[CONTROLLER_BUTTON_DPAD_UP] = 127;
        controller->button[CONTROLLER_BUTTON_DPAD_DOWN] = 0;
    } else if (d_pad_state == 1) {
        controller->button[CONTROLLER_BUTTON_DPAD_LEFT] = 0;
        controller->button[CONTROLLER_BUTTON_DPAD_RIGHT] = 127;
        controller->button[CONTROLLER_BUTTON_DPAD_UP] = 127;
        controller->button[CONTROLLER_BUTTON_DPAD_DOWN] = 0;
    } else if (d_pad_state == 2) {
        controller->button[CONTROLLER_BUTTON_DPAD_LEFT] = 0;
        controller->button[CONTROLLER_BUTTON_DPAD_RIGHT] = 127;
        controller->button[CONTROLLER_BUTTON_DPAD_UP] = 0;
        controller->button[CONTROLLER_BUTTON_DPAD_DOWN] = 0;
    } else if (d_pad_state == 3) {
        controller->button[CONTROLLER_BUTTON_DPAD_LEFT] = 0;
        controller->button[CONTROLLER_BUTTON_DPAD_RIGHT] = 127;
        controller->button[CONTROLLER_BUTTON_DPAD_UP] = 0;
        controller->button[CONTROLLER_BUTTON_DPAD_DOWN] = 127;
    } else if (d_pad_state == 4) {
        controller->button[CONTROLLER_BUTTON_DPAD_LEFT] = 0;
        controller->button[CONTROLLER_BUTTON_DPAD_RIGHT] = 0;
        controller->button[CONTROLLER_BUTTON_DPAD_UP] = 0;
        controller->button[CONTROLLER_BUTTON_DPAD_DOWN] = 127;
    } else if (d_pad_state == 5) {
        controller->button[CONTROLLER_BUTTON_DPAD_LEFT] = 127;
        controller->button[CONTROLLER_BUTTON_DPAD_RIGHT] = 0;
        controller->button[CONTROLLER_BUTTON_DPAD_UP] = 0;
        controller->button[CONTROLLER_BUTTON_DPAD_DOWN] = 127;
    } else if (d_pad_state == 6) {
        controller->button[CONTROLLER_BUTTON_DPAD_LEFT] = 127;
        controller->button[CONTROLLER_BUTTON_DPAD_RIGHT] = 0;
        controller->button[CONTROLLER_BUTTON_DPAD_UP] = 0;
        controller->button[CONTROLLER_BUTTON_DPAD_DOWN] = 0;
    } else if (d_pad_state == 7) {
        controller->button[CONTROLLER_BUTTON_DPAD_LEFT] = 127;
        controller->button[CONTROLLER_BUTTON_DPAD_RIGHT] = 0;
        controller->button[CONTROLLER_BUTTON_DPAD_UP] = 127;
        controller->button[CONTROLLER_BUTTON_DPAD_DOWN] = 0;
    }

    ++data;

    // bit 7 = R3, bit 6 = L3
    controller->button[CONTROLLER_BUTTON_STICK_RIGHT_BUTTON] = BITS_GET_8_L2R(*data, 0, 1);
    controller->button[CONTROLLER_BUTTON_STICK_LEFT_BUTTON] = BITS_GET_8_L2R(*data, 1, 1);

    controller->button[CONTROLLER_BUTTON_OTHER_0] = BITS_GET_8_L2R(*data, 2, 1); // option
    controller->button[CONTROLLER_BUTTON_OTHER_1] = BITS_GET_8_L2R(*data, 3, 1); // share

    controller->button[CONTROLLER_BUTTON_SHOULDER_RIGHT_BUTTON] = BITS_GET_8_L2R(*data, 6, 1);
    controller->button[CONTROLLER_BUTTON_SHOULDER_LEFT_BUTTON] = BITS_GET_8_L2R(*data, 7, 1);

    ++data;

    controller->button[CONTROLLER_BUTTON_OTHER_2] = BITS_GET_8_L2R(*data, 6, 1); // tpad
    controller->button[CONTROLLER_BUTTON_OTHER_3] = BITS_GET_8_L2R(*data, 7, 1); // ps

    ++data;

    // L2 comes before R2
    controller->button[CONTROLLER_BUTTON_SHOULDER_LEFT_TRIGGER] = (int8) (*data++ >> 1); // 0 - 255 -> 0 - 127
    controller->is_analog[CONTROLLER_BUTTON_SHOULDER_LEFT_TRIGGER] = true;

    controller->button[CONTROLLER_BUTTON_SHOULDER_RIGHT_TRIGGER] = (int8) (*data++ >> 1); // 0 - 255 -> 0 - 127
    controller->is_analog[CONTROLLER_BUTTON_SHOULDER_RIGHT_TRIGGER] = true;

    data += 3;

    controller->gyro_x = *data++;
    controller->gyro_y = *data++;
    controller->gyro_z = *data++;
}

#endif