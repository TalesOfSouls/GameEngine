/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef TOS_IMAGE_TGA_H
#define TOS_IMAGE_TGA_H

#include <string.h>
#include "../stdlib/Types.h"
#include "../utils/Utils.h"
#include "../utils/EndianUtils.h"
#include "Image.h"

// See: https://en.wikipedia.org/wiki/Truevision_TGA
// IMPORTANT: Remember that we are not using packing for the headers
//      Because of that the struct size is different from the actual header size in the file
//      This means we have to manually asign the data to the headers

// Packed header size
#define TGA_HEADER_SIZE 18
struct TgaHeader {
    byte id_length;
    byte color_map_type;
    byte image_type;
    uint16 color_map_index;
    uint16 color_map_length;
    uint16 color_map_bits;
    uint16 x_origin;
    uint16 y_origin;
    uint16 width;
    uint16 height;
    byte bits_per_pixel;
    byte alpha_bits_per_pixel;
    byte horizonal_ordering;
    byte vertical_ordering;
};

struct Tga {
    TgaHeader header;

    byte* pixels;

    uint32 size;
    byte* data;
};

void generate_default_tga_references(const file_body* file, Tga* tga)
{
    tga->header.id_length = file->content[0];
    tga->header.color_map_type = file->content[1];
    tga->header.image_type = file->content[2];
    tga->header.color_map_index = SWAP_ENDIAN_LITTLE(*((uint16 *) (file->content + 3)));
    tga->header.color_map_length = SWAP_ENDIAN_LITTLE(*((uint16 *) (file->content + 5)));
    tga->header.color_map_bits = file->content[7];
    tga->header.width = SWAP_ENDIAN_LITTLE(*((uint16 *) (file->content + 12)));
    tga->header.height = SWAP_ENDIAN_LITTLE(*((uint16 *) (file->content + 14)));
    tga->header.bits_per_pixel = file->content[16];
    tga->header.alpha_bits_per_pixel = file->content[17] & 0x07;
    tga->header.horizonal_ordering = file->content[17] & (1 << 4); // 0 = left-to-right
    tga->header.vertical_ordering = (file->content[17] & (1 << 5)) == 0 ? 1 : 0; // 0 = top-to-bottom

    tga->pixels = file->content + TGA_HEADER_SIZE
        + tga->header.id_length // can be 0
        + tga->header.color_map_length * (tga->header.color_map_bits / 8); // can be 0
}

void generate_tga_image(const file_body* src_data, Image* image)
{
    Tga src = {};
    generate_default_tga_references(src_data, &src);

    image->width = src.header.width;
    image->height = src.header.height;
    image->length = image->width * image->height;

    // @todo also handle bottom-top/top-bottom order here
    uint32 pixel_bytes = src.header.bits_per_pixel / 8;
    if (image->order_pixels == IMAGE_PIXEL_ORDER_BGRA) {
        memcpy((void *) image->pixels, src.pixels, image->length * pixel_bytes);

        return;
    }

    byte alpha_offset = pixel_bytes == 3 ? 0 : 1;
    uint32 pixel_rgb_bytes = pixel_bytes - alpha_offset;

    uint32 row_pos1;
    uint32 row_pos2;

    for (uint32 y = 0; y < src.header.height; ++y) {
        for (uint32 x = 0; x < src.header.width; ++x) {
            row_pos1 = y * image->width * pixel_bytes;
            row_pos2 = src.header.vertical_ordering == 0
                ? y * image->width * pixel_bytes
                : (image->height - y - 1) * image->width * pixel_bytes;

            for (uint32 i = 0; i < pixel_rgb_bytes; ++i) {
                image->pixels[row_pos1 + x * pixel_bytes + i] = src.pixels[row_pos2 + x * pixel_bytes + pixel_rgb_bytes - i];
            }

            // Add alpha channel at end
            if (alpha_offset > 0) {
                image->pixels[row_pos1 + x * pixel_bytes + 3] = src.pixels[row_pos2 + x * pixel_bytes + pixel_bytes + 3];
            }
        }
    }
}

#endif