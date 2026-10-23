/**
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 *
 * Maps controller input reports (hid, raw input) to the input state
 */
#pragma once
#ifndef COMS_PLATFORM_WIN32_INPUT_CONTROLLER_REPORT_H
#define COMS_PLATFORM_WIN32_INPUT_CONTROLLER_REPORT_H

#include "../../../stdlib/Stdlib.h"
#include "../../../input/Input.cpp"
#include "../../../input/ControllerType.h"
#include "../../../input/ControllerInput.h"
#include "controller/DualShock4.h"
#include "controller/DualSense.h"
#include "controller/XBoxS.h"

/**
 * Maps a report to the controller keys of the input
 *
 * @return false if there is no mapping for this controller type
 */
inline
bool input_controller_report_handle(Input* const input, const byte* const report, uint64 time) NO_EXCEPT
{
    ControllerInput controller = {};
    switch (input->controller_type) {
        case CONTROLLER_TYPE_DUALSHOCK4: {
            input_map_dualshock4(&controller, input->controller.connection_type, report);
        } break;
        case CONTROLLER_TYPE_DUALSENSE: {
            input_map_dualsense(&controller, input->controller.connection_type, report);
        } break;
        case CONTROLLER_TYPE_XBOX_S: {
            input_map_xboxs(&controller, input->controller.connection_type, report);
        } break;
        default: {
            // @todo Generic mapping based on the hid report descriptor (HidP_GetUsages/HidP_GetUsageValue)
            return false;
        }
    }

    input_set_controller_state(input, &controller, time);
    input->time_last_input_check = time;

    return true;
}

#endif
