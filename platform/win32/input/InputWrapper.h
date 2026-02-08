/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_INPUT_WRAPPER_H
#define COMS_INPUT_WRAPPER_H

#include "../../../stdlib/Stdlib.h"
#include "../../../input/ControllerType.h"
#include "../../../input/InputMode.h"

#include "XInput.h"
#include "PollingInput.h"
#include "RawInput.h"

void input_controller_init(
    byte input_controller_api,
    Input* const __restrict states,
    RingMemory* const __restrict ring
) {
    // @todo continue implementation
    for (int i = 0; i < 2; ++i) {
        switch (input_controller_api) {
            case CONTROLLER_INPUT_TYPE_HID: {
                hid_init_controllers(
                    states,
                    ring
                );

                return;
            } break;
            case CONTROLLER_INPUT_TYPE_XINPUT: {
                bool success = xinput_load();
                if (!success) {
                    input_controller_api = CONTROLLER_INPUT_TYPE_HID;
                    break;
                }

                xinput_init_controllers();

                return;
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
}

int16 input_kbm_handle(
    InputMode mode,
    int32 max_inputs,
    Input* __restrict states, int32 state_count,
    RingMemory* const __restrict ring,
    uint64 time
) NO_EXCEPT
{
    switch (mode) {
        case INPUT_MODE_EVENT: {
            return input_raw_handle_buffered(max_inputs, states, state_count, ring, time);
        };
        case INPUT_MODE_POLLING: {
            return input_poll_handle(states, time);
        };
        default:
            UNREACHABLE();
    };
}

#endif