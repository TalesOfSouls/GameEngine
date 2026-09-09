/**
 * @copyright Jingga
 * @license   OMS License 2.0
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

static
void image_flip_vertical_scalar(Image* const image) NO_EXCEPT
{
    constexpr size_t TEMP_SIZE = 8 * KILOBYTE;
    alignas(size_t) byte temp[TEMP_SIZE];

    const size_t stride = image->width * (image->image_settings & IMAGE_SETTING_CHANNEL_COUNT);

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
}

#if defined(__SSE4_2__)
static
void image_flip_vertical_sse(Image* const image)
{
    const int32 pixel_size = (image->image_settings & IMAGE_SETTING_CHANNEL_COUNT);
    const size_t stride = image->width * pixel_size;

    for (uint32 y = 0; y < image->height / 2; ++y) {
        byte* top = image->pixels + y * stride;
        byte* bottom = image->pixels + (image->height - 1 - y) * stride;

        size_t i = 0;
        for (; i + 16 <= stride; i += 16) {
            __m128i t = _mm_load_si128((const __m128i*)(top + i));
            __m128i b = _mm_load_si128((const __m128i*)(bottom + i));
            _mm_store_si128((__m128i*)(top + i), b);
            _mm_store_si128((__m128i*)(bottom + i), t);
        }

        if (pixel_size == 4) {
            for (; i < stride; i += pixel_size) {
                uint32* t = (uint32*)(top + i);
                uint32* b = (uint32*)(bottom + i);
                uint32 tmp = *t;
                *t = *b;
                *b = tmp;
            }
        } else {
            for (; i < stride; ++i) {
                byte t = top[i];
                top[i] = bottom[i];
                bottom[i] = t;
            }
        }
    }
}
#endif

#if defined(__AVX2__)
static
void image_flip_vertical_avx2(Image* const image)
{
    const int32 pixel_size = (image->image_settings & IMAGE_SETTING_CHANNEL_COUNT);
    const size_t stride = image->width * pixel_size;

    for (uint32 y = 0; y < image->height / 2; ++y) {
        byte* top = image->pixels + y * stride;
        byte* bottom = image->pixels + (image->height - 1 - y) * stride;

        size_t i = 0;
        for (; i + 32 <= stride; i += 32) {
            __m256i t = _mm256_load_si256((const __m256i*)(top + i));
            __m256i b = _mm256_load_si256((const __m256i*)(bottom + i));
            _mm256_store_si256((__m256i*)(top + i), b);
            _mm256_store_si256((__m256i*)(bottom + i), t);
        }

        if (pixel_size == 4) {
            for (; i < stride; i += pixel_size) {
                uint32* t = (uint32*)(top + i);
                uint32* b = (uint32*)(bottom + i);
                uint32 tmp = *t;
                *t = *b;
                *b = tmp;
            }
        } else {
            for (; i < stride; ++i) {
                byte t = top[i];
                top[i] = bottom[i];
                bottom[i] = t;
            }
        }
    }
}
#endif

#if defined(__AVX512F__)
static
void image_flip_vertical_avx512(Image* const image)
{
    const int32 pixel_size = (image->image_settings & IMAGE_SETTING_CHANNEL_COUNT);
    const size_t stride = image->width * pixel_size;

    for (uint32 y = 0; y < image->height / 2; ++y) {
        byte* top = image->pixels + y * stride;
        byte* bottom = image->pixels + (image->height - 1 - y) * stride;

        size_t i = 0;
        for (; i + 64 <= stride; i += 64) {
            __m512i t = _mm512_load_si512((const void*)(top + i));
            __m512i b = _mm512_load_si512((const void*)(bottom + i));
            _mm512_store_si512((void*)(top + i), b);
            _mm512_store_si512((void*)(bottom + i), t);
        }

        if (pixel_size == 4) {
            for (; i < stride; i += pixel_size) {
                uint32* t = (uint32*)(top + i);
                uint32* b = (uint32*)(bottom + i);
                uint32 tmp = *t;
                *t = *b;
                *b = tmp;
            }
        } else {
            for (; i < stride; ++i) {
                byte t = top[i];
                top[i] = bottom[i];
                bottom[i] = t;
            }
        }
    }
}
#endif

#if defined(__ARM_NEON)
static
void image_flip_vertical_neon(Image* const image)
{
    const int32 pixel_size = (image->image_settings & IMAGE_SETTING_CHANNEL_COUNT);
    const size_t stride = image->width * pixel_size;

    for (uint32 y = 0; y < image->height / 2; ++y) {
        byte* top = image->pixels + y * stride;
        byte* bottom = image->pixels + (image->height - 1 - y) * stride;

        size_t i = 0;
        for (; i + 16 <= stride; i += 16) {
            uint8x16_t t = vld1q_u8(top + i);
            uint8x16_t b = vld1q_u8(bottom + i);
            vst1q_u8(top + i, b);
            vst1q_u8(bottom + i, t);
        }

        for (; i < stride; i += pixel_size) {
            uint32* t = (uint32*)(top + i);
            uint32* b = (uint32*)(bottom + i);
            uint32 tmp = *t;
            *t = *b;
            *b = tmp;
        }
    }
}
#endif

#if defined(__ARM_FEATURE_SVE)
static
void image_flip_vertical_sve(Image* const image, int32 steps = 8)
{
    const int32 pixel_size = (image->image_settings & IMAGE_SETTING_CHANNEL_COUNT);
    const size_t stride = image->width * pixel_size;

    for (uint32 y = 0; y < image->height / 2; ++y) {
        byte* top = image->pixels + y * stride;
        byte* bottom = image->pixels + (image->height - 1 - y) * stride;

        size_t i = 0;
        const size_t vl = steps * pixel_size;
        for (; i < stride; i += vl) {
            svbool_t pg = svwhilelt_b8(i, stride);
            svuint8_t t = svld1_u8(pg, top + i);
            svuint8_t b = svld1_u8(pg, bottom + i);
            svst1_u8(pg, top + i, b);
            svst1_u8(pg, bottom + i, t);
        }
    }
}
#endif

void image_flip_vertical(Image* image, int32 steps = 8) NO_EXCEPT
{
    #if defined(__ARM_FEATURE_SVE)
        if (steps >= 4) {
            image_flip_vertical_sve(image, steps);
            steps = 0;
        }
    #elif defined(__ARM_NEON)
        if (steps >= 4) {
            image_flip_vertical_neon(image);
            steps = 0;
        }
    #else
        #if defined(__AVX512F__)
            if (steps >= 8) {
                image_flip_vertical_avx2(image);
                steps = 0;
            }
        #endif

        #if defined(__AVX2__)
            if (steps >= 8) {
                image_flip_vertical_avx2(image);
                steps = 0;
            }
        #endif

        #if defined(__SSE4_2__)
            if (steps == 4) {
                image_flip_vertical_sse(image);
                steps = 0;
            }
        #endif
    #endif

    if (steps > 0) {
        image_flip_vertical_scalar(image);
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
    memcpy(
        image->pixels,
        pos,
        image_size = (image_pixel_size_from_type(image->image_settings) * image->pixel_count)
    );
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
    memcpy(
        pos,
        image->pixels,
        image_size = (image_pixel_size_from_type(image->image_settings) * image->pixel_count)
    );
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