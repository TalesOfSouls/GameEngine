/**
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 *
 * Helpers to identify input devices (shared by raw input, hid and xinput)
 */
#pragma once
#ifndef COMS_PLATFORM_WIN32_INPUT_DEVICE_H
#define COMS_PLATFORM_WIN32_INPUT_DEVICE_H

#include <windows.h>

#include "../../../stdlib/Stdlib.h"
#include "../../../input/ControllerType.h"
#include "../../../input/InputConnectionType.h"

FORCE_INLINE
wchar_t input_device_char_lower(wchar_t c) NO_EXCEPT
{
    return (c >= L'A' && c <= L'Z') ? (wchar_t) (c - L'A' + L'a') : c;
}

/**
 * Case insensitive hash of a device path
 *
 * The same device interface may be reported with different casing
 * (e.g. SetupDi = lower case, WM_DEVICECHANGE = upper case)
 *
 * @return 0 = invalid path
 */
inline
uint64 input_device_path_hash(const wchar_t* path) NO_EXCEPT
{
    if (!path || !*path) {
        return 0;
    }

    // FNV-1a
    uint64 hash = 0xCBF29CE484222325ULL;
    while (*path) {
        hash ^= (uint64) input_device_char_lower(*path++);
        hash *= 0x100000001B3ULL;
    }

    return hash ? hash : 1;
}

// Case insensitive sub string search (needle MUST be lower case)
inline
bool input_device_path_contains(const wchar_t* path, const wchar_t* needle) NO_EXCEPT
{
    if (!path || !needle) {
        return false;
    }

    for (; *path; ++path) {
        const wchar_t* p = path;
        const wchar_t* n = needle;
        while (*p && *n && input_device_char_lower(*p) == *n) {
            ++p;
            ++n;
        }

        if (!*n) {
            return true;
        }
    }

    return false;
}

// Bluetooth devices have the bluetooth hid service guid or the bluetooth enumerator in their path
inline
InputConnectionType input_device_path_connection(const wchar_t* path) NO_EXCEPT
{
    if (input_device_path_contains(path, L"00001124-0000-1000-8000-00805f9b34fb")
        || input_device_path_contains(path, L"bthenum")
        || input_device_path_contains(path, L"bthledevice")
    ) {
        return INPUT_CONNECTION_TYPE_BLUETOOTH;
    }

    return INPUT_CONNECTION_TYPE_USB;
}

#define INPUT_CONTROLLER_TYPE_DUALSHOCK4

inline
ControllerType input_controller_type_get(uint32 vendor_id, uint32 product_id) NO_EXCEPT
{
    if (vendor_id == 0x054C) {
        // Sony
        switch (product_id) {
            case 0x05C4: // DualShock 4 (v1)
            case 0x09CC: // DualShock 4 (v2)
            case 0x0BA0: // DualShock 4 wireless adapter
                return CONTROLLER_TYPE_DUALSHOCK4;
            case 0x0CE6: // DualSense
            case 0x0DF2: // DualSense Edge
                return CONTROLLER_TYPE_DUALSENSE;
            default:
                return CONTROLLER_TYPE_OTHER;
        }
    } else if (vendor_id == 0x045E) {
        // Microsoft
        switch (product_id) {
            case 0x02E0: return CONTROLLER_TYPE_XBOX_360;
            case 0x02FF: return CONTROLLER_TYPE_XBOX_ONE;
            case 0x028E: // @question Isn't this the xbox 360 controller?
            case 0x0B12:
            case 0x0B13:
                return CONTROLLER_TYPE_XBOX_S;
            default:
                return CONTROLLER_TYPE_OTHER;
        }
    }

    return CONTROLLER_TYPE_OTHER;
}

#endif
