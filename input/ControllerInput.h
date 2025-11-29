/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_INPUT_CONTROLLER_INPUT_H
#define COMS_INPUT_CONTROLLER_INPUT_H

#include "../stdlib/Types.h"

// @question Why are we even using this, shouldn't CONTROLLER_BUTTON_SIZE be sufficient
#define MAX_CONTROLLER_KEYS 32

enum ControllerButton {
    CONTROLLER_BUTTON_STICK_LEFT_BUTTON,
    CONTROLLER_BUTTON_STICK_LEFT_HORIZONTAL,
    CONTROLLER_BUTTON_STICK_LEFT_VERTICAL,

    CONTROLLER_BUTTON_STICK_RIGHT_BUTTON,
    CONTROLLER_BUTTON_STICK_RIGHT_HORIZONTAL,
    CONTROLLER_BUTTON_STICK_RIGHT_VERTICAL,

    // @question Shouldn't this be shoulder 1 - 3? front to back
    // this way we don't have to care about what a trigger is and isn't
    CONTROLLER_BUTTON_SHOULDER_LEFT_TRIGGER,
    CONTROLLER_BUTTON_SHOULDER_LEFT_BUTTON,

    CONTROLLER_BUTTON_SHOULDER_RIGHT_TRIGGER,
    CONTROLLER_BUTTON_SHOULDER_RIGHT_BUTTON,

    CONTROLLER_BUTTON_X,
    CONTROLLER_BUTTON_C,
    CONTROLLER_BUTTON_T,
    CONTROLLER_BUTTON_S,

    // @question Couldn't there be a second dpad?
    CONTROLLER_BUTTON_DPAD_LEFT,
    CONTROLLER_BUTTON_DPAD_RIGHT,
    CONTROLLER_BUTTON_DPAD_UP,
    CONTROLLER_BUTTON_DPAD_DOWN,

    // @question Do we need a sliders? (e.g. throttle / thrust lever)
    // @question Do we need a touchpad (x/y coordinate). probably 2 touchpads, one per thumb
    // @question Do we need turnable knobs
    // @question Do we need flippable switches (or more general multiple states)

    CONTROLLER_BUTTON_OTHER_0,
    CONTROLLER_BUTTON_OTHER_1,
    CONTROLLER_BUTTON_OTHER_2,
    CONTROLLER_BUTTON_OTHER_3,
    CONTROLLER_BUTTON_OTHER_4,
    CONTROLLER_BUTTON_OTHER_5,
    CONTROLLER_BUTTON_OTHER_6,
    CONTROLLER_BUTTON_OTHER_7,

    CONTROLLER_BUTTON_SIZE,
};

struct ControllerInput {
    // @todo should probably include controller_id for xinput and LPDIRECTINPUTDEVICE8 for directinput
    // @question Is int8 even enough? I thought some are int16, maybe even f32
    int8 button[MAX_CONTROLLER_KEYS];

    // @performance is a bit field better? uint64 should be plenty
    bool is_analog[MAX_CONTROLLER_KEYS]; // = uses deadzone

    int16 gyro_x;
    int16 gyro_y;
    int16 gyro_z;

    // @question do we need axis acceleration
};

#endif