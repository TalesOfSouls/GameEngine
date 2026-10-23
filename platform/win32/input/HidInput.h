/**
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 *
 * HID controllers
 *
 * The controllers are opened directly and read asynchronously (overlapped).
 * Every tick hid_controller_poll() collects all reports that arrived since the last tick and maps the latest one.
 * This is polling based -> controllers that constantly send reports (gyro, timestamps) don't flood any event queue.
 *
 * Connects/disconnects:
 *      startup:    hid_init_controllers() enumerates all present hid devices
 *      connect:    hid_controller_add() with the device path (e.g. from WM_DEVICECHANGE)
 *      disconnect: device path from WM_DEVICECHANGE or a failed read in hid_controller_poll()
 *
 * WARNING: The setup functions (hid_init_controllers, hid_controller_add) require hid_init() + setupapi_init()
 *          in the same module (dynamic linking). The poll/close functions only use kernel32.
 */
#pragma once
#ifndef COMS_PLATFORM_WIN32_INPUT_HID_H
#define COMS_PLATFORM_WIN32_INPUT_HID_H

#include <windows.h>
#include <setupapi.h>
#include <guiddef.h>
#include <hidsdi.h>

#include "../../../input/Input.cpp"
#include "../../../input/ControllerType.h"
#include "InputDevice.h"
#include "ControllerReport.h"

#include "../libs/hid.h"
#include "../libs/setupapi.h"

#define HID_USAGE_ID_MOUSE 0x02
#define HID_USAGE_ID_KEYBOARD 0x06
#define HID_USAGE_ID_JOYSTICK 0x04
#define HID_USAGE_ID_GAMEPAD 0x05

/**
 * Closes the hid controller handles
 *
 * The device id is kept -> a reconnect returns to the same input
 * The keys are NOT released here (@see input_controller_disconnect)
 */
inline
void hid_controller_close(ControllerDevice* const controller) NO_EXCEPT
{
    if (controller->handle && controller->handle != INVALID_HANDLE_VALUE) {
        if (controller->read_pending) {
            // We must wait for the cancellation, otherwise the read could still write into read_buffer
            DWORD bytes;
            CancelIoEx(controller->handle, &controller->overlapped);
            GetOverlappedResult(controller->handle, &controller->overlapped, &bytes, TRUE);
        }

        CloseHandle(controller->handle);
    }

    if (controller->overlapped.hEvent) {
        CloseHandle(controller->overlapped.hEvent);
    }

    memset(&controller->overlapped, 0, sizeof(controller->overlapped));
    controller->handle = NULL;
    controller->read_pending = false;
    controller->has_report = false;
    controller->connection_type = INPUT_CONNECTION_TYPE_NONE;
}

/**
 * Opens a hid device and assigns it to an input if it is a controller (gamepad/joystick)
 *
 * Safe to call for any hid device and for already connected controllers (e.g. duplicate notifications)
 *
 * @param path                Device interface path
 * @param preferred_types     InputType flags per input (@see input_state_assign), may be NULL
 * @param skip_xinput_devices Ignore XInput compatible devices (they are handled by XInput)
 *
 * @return Input index the controller was assigned to (-1 = not a controller, already connected or no free input)
 */
int32 hid_controller_add(
    Input* const __restrict states, int32 count,
    const wchar_t* const __restrict path,
    const byte* const __restrict preferred_types = NULL,
    bool skip_xinput_devices = false
) NO_EXCEPT
{
    // XInput compatible devices have "IG_" in their hid path
    // Their hid reports are limited (e.g. both triggers share one axis) -> XInput is the better api
    if (skip_xinput_devices && input_device_path_contains(path, L"ig_")) {
        return -1;
    }

    const uint64 device_id = input_device_path_hash(path);
    if (!device_id) {
        return -1;
    }

    // Already connected
    for (int32 i = 0; i < count; ++i) {
        if (states[i].controller.connection_type && states[i].controller.device_id == device_id) {
            return -1;
        }
    }

    // We first only query the device information (no access rights)
    // This also works for devices the system opened exclusively (e.g. keyboards, mice)
    HANDLE query_handle = CreateFileW(
        path,
        0,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        0,
        NULL
    );

    if (query_handle == INVALID_HANDLE_VALUE) {
        return -1;
    }

    HIDD_ATTRIBUTES attributes;
    attributes.Size = sizeof(HIDD_ATTRIBUTES);

    HIDP_CAPS caps = {};
    bool is_valid = HID_HidD_GetAttributes(query_handle, &attributes);

    PHIDP_PREPARSED_DATA preparsed_data;
    if (is_valid && HID_HidD_GetPreparsedData(query_handle, &preparsed_data)) {
        is_valid = HID_HidP_GetCaps(preparsed_data, &caps) == HIDP_STATUS_SUCCESS;
        HID_HidD_FreePreparsedData(preparsed_data);
    } else {
        is_valid = false;
    }

    CloseHandle(query_handle);

    if (!is_valid
        || caps.UsagePage != 0x01
        || (caps.Usage != HID_USAGE_ID_GAMEPAD && caps.Usage != HID_USAGE_ID_JOYSTICK)
    ) {
        return -1;
    }

    if (caps.InputReportByteLength == 0 || caps.InputReportByteLength > INPUT_CONTROLLER_REPORT_SIZE) {
        ASSERT_TRUE(caps.InputReportByteLength <= INPUT_CONTROLLER_REPORT_SIZE);
        LOG_1(
            "[WARNING] HID controller report too large: %d bytes",
            {DATA_TYPE_UINT16, &caps.InputReportByteLength}
        );

        return -1;
    }

    // Now we open the device for reading
    HANDLE device_handle = CreateFileW(
        path,
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        FILE_FLAG_OVERLAPPED,
        NULL
    );

    if (device_handle == INVALID_HANDLE_VALUE) {
        // Some devices don't allow write access
        device_handle = CreateFileW(
            path,
            GENERIC_READ,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            NULL,
            OPEN_EXISTING,
            FILE_FLAG_OVERLAPPED,
            NULL
        );

        if (device_handle == INVALID_HANDLE_VALUE) {
            LOG_1("[WARNING] Couldn't open HID controller");
            return -1;
        }
    }

    // Manual reset event, required for the async reads
    HANDLE read_event = CreateEventW(NULL, TRUE, FALSE, NULL);
    if (!read_event) {
        CloseHandle(device_handle);
        return -1;
    }

    // Same controller as before -> same input
    int32 index = input_controller_find_by_device_id(states, count, device_id);
    if (index < 0 || states[index].controller.connection_type) {
        index = input_state_assign(states, count, INPUT_DEVICE_KIND_CONTROLLER, preferred_types);
    }

    if (index < 0) {
        LOG_1("[INFO] No free input for HID controller");

        CloseHandle(read_event);
        CloseHandle(device_handle);

        return -1;
    }

    Input* const input = &states[index];
    ControllerDevice* const controller = &input->controller;

    controller->api = CONTROLLER_INPUT_TYPE_HID;
    controller->device_id = device_id;
    controller->handle = device_handle;
    controller->report_size = caps.InputReportByteLength;
    controller->has_report = false;
    controller->read_pending = false;
    memset(&controller->overlapped, 0, sizeof(controller->overlapped));
    controller->overlapped.hEvent = read_event;
    controller->connection_type = input_device_path_connection(path);

    input->controller_type = input_controller_type_get(attributes.VendorID, attributes.ProductID);
    input->device_types |= INPUT_TYPE_CONTROLLER;
    input_state_connection_update(input);

    LOG_1(
        "[INFO] HID controller connected to input %d (type %d)",
        {DATA_TYPE_INT32, &index}, {DATA_TYPE_INT32, &input->controller_type}
    );

    return index;
}

/**
 * Finds the input of a connected hid controller by its device path
 *
 * @return Input index (-1 = not a connected hid controller)
 */
inline
int32 hid_controller_find_by_path(
    const Input* const __restrict states, int32 count,
    const wchar_t* const __restrict path
) NO_EXCEPT
{
    const uint64 device_id = input_device_path_hash(path);
    if (!device_id) {
        return -1;
    }

    for (int32 i = 0; i < count; ++i) {
        if (states[i].controller.connection_type
            && states[i].controller.api == CONTROLLER_INPUT_TYPE_HID
            && states[i].controller.device_id == device_id
        ) {
            return i;
        }
    }

    return -1;
}

/**
 * Adds all connected hid controllers
 *
 * @return Amount of controllers added
 */
int32 hid_init_controllers(
    Input* const __restrict states, int32 count,
    const byte* const __restrict preferred_types = NULL,
    bool skip_xinput_devices = false
) NO_EXCEPT
{
    GUID hid_guid;
    HID_HidD_GetHidGuid(&hid_guid);

    HDEVINFO device_info_set = SETUPAPI_SetupDiGetClassDevsW(
        &hid_guid, NULL, NULL,
        DIGCF_PRESENT | DIGCF_DEVICEINTERFACE
    );
    if (device_info_set == INVALID_HANDLE_VALUE) {
        return 0;
    }

    SP_DEVICE_INTERFACE_DATA device_interface_data;
    device_interface_data.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

    // Device paths are rather short (~150 characters)
    alignas(8) byte detail_buffer[2048];
    PSP_DEVICE_INTERFACE_DETAIL_DATA_W device_detail_data = (PSP_DEVICE_INTERFACE_DETAIL_DATA_W) detail_buffer;

    int32 controller_found = 0;
    for (DWORD device_index = 0;
        SETUPAPI_SetupDiEnumDeviceInterfaces(device_info_set, NULL, &hid_guid, device_index, &device_interface_data);
        ++device_index
    ) {
        DWORD required_size = 0;
        SETUPAPI_SetupDiGetDeviceInterfaceDetailW(
            device_info_set,
            &device_interface_data,
            NULL,
            0,
            &required_size,
            NULL
        );

        if (required_size == 0 || required_size > sizeof(detail_buffer)) {
            continue;
        }

        device_detail_data->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W);
        if (!SETUPAPI_SetupDiGetDeviceInterfaceDetailW(
            device_info_set,
            &device_interface_data,
            device_detail_data,
            required_size,
            NULL, NULL)
        ) {
            continue;
        }

        if (hid_controller_add(
            states, count,
            device_detail_data->DevicePath,
            preferred_types,
            skip_xinput_devices) >= 0
        ) {
            ++controller_found;
        }
    }

    SETUPAPI_SetupDiDestroyDeviceInfoList(device_info_set);

    return controller_found;
}

/**
 * Collects all reports that arrived since the last poll and maps the latest one
 * Never blocks
 *
 * The latest report is also mapped if no new report arrived
 * -> stick values stay active every tick (deltas are reset every tick @see input_frame_begin)
 *
 * @return false = device disconnected (the caller must disconnect the controller)
 */
inline
bool hid_controller_poll(Input* const input, uint64 time) NO_EXCEPT
{
    ControllerDevice* const controller = &input->controller;

    // The hid driver buffers up to 32 reports (default), we only care about the latest one
    for (int32 i = 0; i < 64; ++i) {
        if (!controller->read_pending) {
            if (!ReadFile(
                    controller->handle,
                    controller->read_buffer,
                    controller->report_size,
                    NULL,
                    &controller->overlapped
                )
                && GetLastError() != ERROR_IO_PENDING
            ) {
                // e.g. ERROR_DEVICE_NOT_CONNECTED
                return false;
            }

            controller->read_pending = true;
        }

        DWORD bytes_read = 0;
        if (!GetOverlappedResult(controller->handle, &controller->overlapped, &bytes_read, FALSE)) {
            if (GetLastError() == ERROR_IO_INCOMPLETE) {
                // No new report
                break;
            }

            controller->read_pending = false;

            return false;
        }

        controller->read_pending = false;

        if (bytes_read) {
            memcpy(
                controller->report,
                controller->read_buffer,
                OMS_MIN((uint32) bytes_read, (uint32) INPUT_CONTROLLER_REPORT_SIZE)
            );
            controller->has_report = true;
        }
    }

    if (controller->has_report) {
        input_controller_report_handle(input, controller->report, time);
    }

    return true;
}

#endif
