/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_IMAGE_C
#define COMS_IMAGE_C

#include "../utils/StringUtils.h"
#include "../memory/RingMemory.cpp"
#include "../system/FileUtils.cpp"

#include "Image.h"
#include "Tga.h"
#include "Bitmap.h"
#include "Png.h"

// Only loads the important image header data without having to parse the entire file
inline
void image_header_from_file(Image* __restrict image, const char* __restrict path) NO_EXCEPT
{
    byte buffer[1024];

    FileBody file;
    file.content = buffer;

    if (str_ends_with(path, ".png")) {
        file.size = 64;
        file_read(path, &file);
        image_header_png_generate(&file, image);
    } else if (str_ends_with(path, ".tga")) {
        file.size = 32;
        file_read(path, &file);
        image_header_tga_generate(&file, image);
    } else if (str_ends_with(path, ".bmp")) {
        file.size = 1024;
        file_read(path, &file);
        image_header_bmp_generate(&file, image);
    }
}

inline
void image_from_file(Image* __restrict image, const char* __restrict path, RingMemory* const __restrict ring) NO_EXCEPT
{
    FileBody file = {0};
    file_read(path, &file, ring);

    if (str_ends_with(path, ".png")) {
        //image_png_generate(&file, image, ring);
    } else if (str_ends_with(path, ".tga")) {
        image_tga_generate(&file, image);
    } else if (str_ends_with(path, ".bmp")) {
        image_bmp_generate(&file, image);
    }
}

/*
@performance This version is faster BUT requires more memory since we have to store the entire image twice in memory
inline
void image_flip_vertical(RingMemory* const __restrict ring, Image* __restrict image) NO_EXCEPT
{
    const uint32 stride = image->width * sizeof(uint32);
    byte* temp = memory_get(ring, image->pixel_count * sizeof(uint32), sizeof(size_t));
    memcpy(temp, image->pixels, image->pixel_count * sizeof(uint32));

    // Last row
    const byte* end = temp + image->pixel_count * sizeof(uint32) - image->width * sizeof(uint32);

    for (uint32 y = 0; y < image->height; ++y) {
        memcpy(image->pixels + y * stride, end - y * stride, stride);
    }

    image->image_settings ^= IMAGE_SETTING_BOTTOM_TO_TOP;
}
*/

void image_flip_vertical(Image* __restrict image) NO_EXCEPT
{
    constexpr size_t TEMP_SIZE = 8 * KILOBYTE;
    alignas(size_t) byte temp[TEMP_SIZE];

    const size_t stride = image->width * sizeof(uint32);

    for (uint32 y = 0; y < image->height / 2; ++y) {
        byte* top = image->pixels + y * stride;
        byte* bottom = image->pixels + (image->height - 1 - y) * stride;

        size_t remaining = stride;
        size_t offset = 0;

        while (remaining > 0) {
            const size_t chunk = (remaining > TEMP_SIZE) ? TEMP_SIZE : remaining;

            memcpy(temp, top + offset, chunk);
            memcpy(top + offset, bottom + offset, chunk);
            memcpy(bottom + offset, temp, chunk);

            offset += chunk;
            remaining -= chunk;
        }
    }

    image->image_settings ^= IMAGE_SETTING_BOTTOM_TO_TOP;
}

inline
int32 image_pixel_size_from_type(byte type) NO_EXCEPT
{
    const int32 channel_size = type & IMAGE_SETTING_CHANNEL_4_SIZE ? 4 : 1;
    const int32 channel_count = type & IMAGE_SETTING_CHANNEL_COUNT;

    return channel_size * channel_count;
}

inline
int32 image_data_size(const Image* image) NO_EXCEPT
{
    return image->pixel_count * image_pixel_size_from_type(image->image_settings)
        + sizeof(image->width) + sizeof(image->height)
        + sizeof(image->image_settings);
}

inline
uint32 image_header_from_data(const byte* __restrict data, Image* const __restrict image) NO_EXCEPT
{
    const byte* const start = data;

    data = read_le(data, &image->width);
    data = read_le(data, &image->height);

    image->pixel_count = image->width * image->height;

    image->image_settings = *data;
    data += sizeof(image->image_settings);

    return (int32) (data - start);
}

inline
uint32 image_from_data(const byte* __restrict data, Image* __restrict image) NO_EXCEPT
{
    LOG_3("Load image");
    const byte* pos = data;
    pos += image_header_from_data(data, image);

    int32 image_size;
    memcpy(image->pixels, pos, image_size = (image_pixel_size_from_type(image->image_settings) * image->pixel_count));
    pos += image_size;

    LOG_3("Loaded image");

    return (int32) (pos - data);
}

inline
uint32 image_header_to_data(const Image* __restrict image, byte* __restrict data) NO_EXCEPT
{
    const byte* const start = data;

    data = write_le(data, image->width);
    data = write_le(data, image->height);

    *data = image->image_settings;
    data += sizeof(image->image_settings);

    return (int32) (data - start);
}

inline
uint32 image_to_data(const Image* __restrict image, byte* __restrict data) NO_EXCEPT
{
    byte* pos = data;
    pos += image_header_to_data(image, data);

    int32 image_size;
    memcpy(pos, image->pixels, image_size = (image_pixel_size_from_type(image->image_settings) * image->pixel_count));
    pos += image_size;

    return (int32) (pos - data);
}

void image_blit(const Image* src, const Image* dst, int offset_x, int offset_y) {
    for (uint32 y = 0; y < src->height; ++y) {
        const uint32 dst_y = y + offset_y;
        if (dst_y >= dst->height) {
            continue;
        }

        for (uint32 x = 0; x < src->width; ++x) {
            const uint32 dst_x = x + offset_x;
            if (dst_x >= dst->width) {
                continue;
            }

            const int dst_index = (dst_y * dst->width + dst_x) * 4;
            const int src_index = (y * src->width + x) * 4;

            byte* d = &dst->pixels[dst_index];
            const byte* s = &src->pixels[src_index];

            // Simple alpha blending
            float alpha = s[3] / 255.0f;

            d[0] = (byte)(s[0] * alpha + d[0] * (1 - alpha));
            d[1] = (byte)(s[1] * alpha + d[1] * (1 - alpha));
            d[2] = (byte)(s[2] * alpha + d[2] * (1 - alpha));
            d[3] = (byte)(255 * (alpha + (d[3] / 255.0f) * (1 - alpha)));
        }
    }
}

#endif