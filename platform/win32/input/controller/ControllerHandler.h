/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_PLATFORM_WIN32_INPUT_CONTROLLER_HANDLER_H
#define COMS_PLATFORM_WIN32_INPUT_CONTROLLER_HANDLER_H

enum ControllerHandlerType {
    CONTROLLER_HANDLER_TYPE_AUTO, // Automatically picks a handler based on the device info and hard coded preferences
    CONTROLLER_HANDLER_TYPE_XINPUT,
    CONTROLLER_HANDLER_TYPE_DIRECTINPUT,
    CONTROLLER_HANDLER_TYPE_RAWINPUT_DS,
    CONTROLLER_HANDLER_TYPE_RAWINPUT_DS4,
    CONTROLLER_HANDLER_TYPE_RAWINPUT_XBOXS,
};

#endif