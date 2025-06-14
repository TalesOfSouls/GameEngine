/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_PLATFORM_WIN32_INPUT_CONTROLLER_TYPE_H
#define COMS_PLATFORM_WIN32_INPUT_CONTROLLER_TYPE_H

#include "../stdlib/Types.h"

enum ControllerType {
    CONTROLLER_TYPE_NONE,
    CONTROLLER_TYPE_DUALSHOCK4,
    CONTROLLER_TYPE_DUALSENSE,
    CONTROLLER_TYPE_XBOX_360,
    CONTROLLER_TYPE_XBOX_ONE,
    CONTROLLER_TYPE_XBOX_S,
    CONTROLLER_TYPE_OTHER,
};

#endif