/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_PLATFORM_WIN32_INPUT_CONNECTION_TYPE_H
#define COMS_PLATFORM_WIN32_INPUT_CONNECTION_TYPE_H

// Important, since some connection protocols differ between for example USB and Bluetooth (see Playstation DualSense)
enum InputConnectionType {
    INPUT_CONNECTION_TYPE_NONE,
    INPUT_CONNECTION_TYPE_USB,
    INPUT_CONNECTION_TYPE_BLUETOOTH,
    INPUT_CONNECTION_TYPE_WIRELESS,
};


#endif