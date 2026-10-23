/**
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_PLATFORM_WIN32_INPUT_CONTROLLER_TYPE_H
#define COMS_PLATFORM_WIN32_INPUT_CONTROLLER_TYPE_H

#include "../stdlib/Stdlib.h"

enum ControllerType {
    CONTROLLER_TYPE_NONE,
    CONTROLLER_TYPE_DUALSHOCK4,
    CONTROLLER_TYPE_DUALSENSE,
    CONTROLLER_TYPE_XBOX_360,
    CONTROLLER_TYPE_XBOX_ONE,
    CONTROLLER_TYPE_XBOX_S,
    CONTROLLER_TYPE_XBOX_GENERIC, // Used for xinput
    CONTROLLER_TYPE_OTHER,
};

// Api used to read the controllers
enum ControllerInputType : byte {
    // Async HID reads, polled once per tick (default)
    // Supports controllers with a mapping (e.g. DualShock4, DualSense)
    CONTROLLER_INPUT_TYPE_HID,

    // Supports all XInput compatible controllers (e.g. Xbox controllers), max 4 controllers
    CONTROLLER_INPUT_TYPE_XINPUT,

    // Raw input (WM_INPUT), only the latest report per controller and tick is mapped
    // WARNING: Many controllers constantly send reports (gyro, timestamps) -> floods the raw input buffer
    CONTROLLER_INPUT_TYPE_RAWINPUT,

    CONTROLLER_INPUT_TYPE_SIZE,
};

#endif