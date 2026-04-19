/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_IMAGE_H
#define COMS_IMAGE_H

#include "../stdlib/Stdlib.h"

enum ImageSettingType : uint8 {
    IMAGE_SETTING_PIXEL_BGR = 0b10000000, // 0 = rgba, 1 = bgra
    IMAGE_SETTING_BOTTOM_TO_TOP = 0b01000000, // 0 = ttb, 1 = btt
    IMAGE_SETTING_COLOR_MODE_SRGB = 0b00100000, // 0 = rgb, 1 = srgb
    IMAGE_SETTING_CHANNEL_4_SIZE = 0b00010000, // 0 = 1 byte, 1 = 4 byte (usually float)
    IMAGE_SETTING_CHANNEL_COUNT = 0b00001111
};

// This struct also functions as a setting on how to load the image data
//      has_alpha is defined it forces an alpha channel even for bitmaps
//      order_pixels defines how the pixels should be ordered
//      order_rows defines how the rows should be ordered
struct Image {
    // We could reduce the size to uint16 but probably need 4 bytes anyways for follow up calculations
    uint32 width;
    uint32 height;
    uint32 pixel_count;

    // ImageSettingType doesn't define the setting but where the setting can be found
    // To get the actual setting simply image_settings & IMAGE_SETTING_CHANNEL_COUNT
    byte image_settings;

    byte* pixels; // owner of data
};

#endif