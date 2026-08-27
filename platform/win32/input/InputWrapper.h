/**
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_INPUT_WRAPPER_H
#define COMS_INPUT_WRAPPER_H

#include "../../../stdlib/Stdlib.h"
#include "../../../input/ControllerType.h"
#include "../../../input/InputMode.h"
#include "../../../memory/BufferMemory.cpp"

#include "XInput.h"
#include "PollingInput.h"
#include "RawInput.h"
#include "HidInput.h"

int32 input_controller_init(
    byte input_controller_api,
    Input* const __restrict states,
    BufferMemory* const __restrict mem
) {
    // @todo continue implementation
    for (int i = 0; i < 2; ++i) {
        switch (input_controller_api) {
            case CONTROLLER_INPUT_TYPE_HID: {
                return hid_init_controllers(states, mem);
            } break;
            case CONTROLLER_INPUT_TYPE_XINPUT: {
                bool success = xinput_load();
                if (!success) {
                    input_controller_api = CONTROLLER_INPUT_TYPE_HID;
                    break;
                }

                return xinput_init_controllers(states);
            } break;
            default:
                UNREACHABLE();
        }
    }

    /*
    This is possible but really bad, since it constantly pollutes the buffer
    The reason for this is that many controllers constantly send update information (gyro, accel and the worst of all sometimes even timer/index variables)
    rawinput_init_controllers(
        app->window->hwnd,
        input_states,
        ring
    );
    */

    return 0;
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