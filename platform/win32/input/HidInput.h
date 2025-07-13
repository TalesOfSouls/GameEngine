/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_PLATFORM_WIN32_INPUT_HID_H
#define COMS_PLATFORM_WIN32_INPUT_HID_H

#include <windows.h>
#include <setupapi.h>
#include <hidsdi.h>
#include <stdio.h>
#include <stdlib.h>

#include "../../../input/Input.h"
#include "../../../input/ControllerType.h"
#include "../../../memory/RingMemory.h"
#include "controller/DualShock4.h"

#pragma comment(lib, "hid.lib")
#pragma comment(lib, "setupapi.lib")

void hid_init_controllers(Input* __restrict states, RingMemory* ring) {

    // Get the GUID for HID devices
    GUID hid_guid;
    HidD_GetHidGuid(&hid_guid);

    // Get a handle to the device information set
    HDEVINFO device_info_set = SetupDiGetClassDevs(&hid_guid, NULL, NULL, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
    if (device_info_set == INVALID_HANDLE_VALUE) {
        return;
    }

    SP_DEVICE_INTERFACE_DATA device_interface_data;
    device_interface_data.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

    DWORD device_index = 0;
    int32 controller_found = 0;

    while (SetupDiEnumDeviceInterfaces(device_info_set, NULL, &hid_guid, device_index, &device_interface_data)) {
        ++device_index;
        DWORD required_size = 0;

        // First call to get required buffer size for device detail data
        SetupDiGetDeviceInterfaceDetail(device_info_set, &device_interface_data, NULL, 0, &required_size, NULL);
        PSP_DEVICE_INTERFACE_DETAIL_DATA device_detail_data = (PSP_DEVICE_INTERFACE_DETAIL_DATA) ring_get_memory(ring, required_size, 8);
        device_detail_data->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);

        // Get device interface detail
        if (!SetupDiGetDeviceInterfaceDetail(device_info_set, &device_interface_data, device_detail_data, required_size, NULL, NULL)) {
            continue;
        }

        // Open the device
        HANDLE device_handle = CreateFile(
            device_detail_data->DevicePath,
            GENERIC_READ | GENERIC_WRITE,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            NULL,
            OPEN_EXISTING,
            FILE_FLAG_OVERLAPPED,
            NULL
        );

        if (device_handle == INVALID_HANDLE_VALUE) {
            continue;
        }

        // Check if the device is a gamepad or joystick by reading its attributes
        HIDD_ATTRIBUTES attributes;
        attributes.Size = sizeof(HIDD_ATTRIBUTES);
        if (!HidD_GetAttributes(device_handle, &attributes)) {
            continue;
        }

        // Get the preparsed data to check usage page and usage ID
        PHIDP_PREPARSED_DATA preparsed_data;
        if (!HidD_GetPreparsedData(device_handle, &preparsed_data)) {
            continue;
        }

        HIDP_CAPS caps;
        if (HidP_GetCaps(preparsed_data, &caps) == HIDP_STATUS_SUCCESS) {
            HidD_FreePreparsedData(preparsed_data);
            continue;
        }

        // Check for game controllers (Generic Desktop Page, Gamepad or Joystick Usage)
        if (caps.UsagePage == 0x01 && caps.Usage == 0x05) {
            if (states[controller_found].handle_controller != NULL) {
                ++controller_found;
            }

            states[controller_found].handle_controller = device_handle;
            // @bug This is not always true, how to check?
            states[controller_found].connection_type = INPUT_CONNECTION_TYPE_USB;

            if (attributes.VendorID == 0x054C
                && (attributes.ProductID == 0x05C4 || attributes.ProductID == 0x09CC)
            ) {
                states[controller_found].controller_type = CONTROLLER_TYPE_DUALSHOCK4;
            } else if (attributes.VendorID == 0x054C
                && (attributes.ProductID == 0x0CE6 || attributes.ProductID == 0x0DF2)
            ) {
                states[controller_found].controller_type = CONTROLLER_TYPE_DUALSENSE;
            } else if (attributes.VendorID == 0x045E && attributes.ProductID == 0x02E0) {
                states[controller_found].controller_type = CONTROLLER_TYPE_XBOX_360;
            } else if (attributes.VendorID == 0x045E && attributes.ProductID == 0x02FF) {
                states[controller_found].controller_type = CONTROLLER_TYPE_XBOX_ONE;
            } else if (attributes.VendorID == 0x045E && attributes.ProductID == 0x028E) {
                states[controller_found].controller_type = CONTROLLER_TYPE_XBOX_S;
            } else {
                states[controller_found].controller_type = CONTROLLER_TYPE_OTHER;
            }
        } else if (caps.UsagePage == 0x01 && caps.Usage == 0x04) {
            if (states[controller_found].handle_controller != NULL) {
                ++controller_found;
            }

            states[controller_found].handle_controller = device_handle;
            // @bug This is not always true, how to check?
            states[controller_found].connection_type = INPUT_CONNECTION_TYPE_USB;
            states[controller_found].controller_type = CONTROLLER_TYPE_OTHER;
        } else {
            CloseHandle(device_handle);
        }

        HidD_FreePreparsedData(preparsed_data);
    }

    SetupDiDestroyDeviceInfoList(device_info_set);
}

uint32 hid_device_poll(Input* state, uint64 time) {
    UCHAR buffer[128];
    DWORD bytes_read;

    if (!ReadFile(state->handle_controller, buffer, sizeof(buffer), &bytes_read, NULL)) {
        return 0;
    }

    ControllerInput controller = {};
    switch(state->controller_type) {
        case CONTROLLER_TYPE_DUALSHOCK4: {
            input_map_dualshock4(&controller, state->connection_type, buffer);
        } break;
        default: {
        };
    }
    input_set_controller_state(state, &controller, time);

    state->general_states |= INPUT_STATE_GENERAL_BUTTON_CHANGE;
    state->time_last_input_check = time;

    return 0;
}

#endif