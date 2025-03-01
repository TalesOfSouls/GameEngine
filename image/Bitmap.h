/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef TOS_IMAGE_BITMAP_H
#define TOS_IMAGE_BITMAP_H

#include <stdio.h>
#include <string.h>

#include "../stdlib/Types.h"
#include "../utils/Utils.h"
#include "../utils/EndianUtils.h"
#include "Image.h"

// See: https://en.wikipedia.org/wiki/BMP_file_format
// IMPORTANT: Remember that we are not using packing for the headers
//      Because of that the struct size is different from the actual header size in the file
//      This means we have to manually asign the data to the headers

#define BITMAP_HEADER_SIZE 14
struct BitmapHeader {
    byte identifier[2]; // 2 bytes - bitmap identifier
    uint32 size;     // 4 bytes - size in bytes
    byte app_data[4];   // 4 bytes - generated by the image software
    uint32 offset;   // 4 bytes - defines starting address for pixels
};

#define DIB_BITMAP_TYPE_BITMAPCOREHEADER 12
struct DIB_BITMAPCOREHEADER {
    uint32 size;
    uint16 width;
    uint16 height;
    uint16 color_planes;
    uint16 bits_per_pixel;
};

#define DIB_BITMAP_TYPE_OS21XBITMAPHEADER DIB_BITMAP_TYPE_BITMAPCOREHEADER
#define DIB_OS21XBITMAPHEADER DIB_BITMAPCOREHEADER

#define DIB_BITMAP_TYPE_BITMAPINFOHEADER 40
struct DIB_BITMAPINFOHEADER {
    uint32 size;
    uint32 width;
    uint32 height;
    uint16 color_planes;
    uint16 bits_per_pixel;
    uint32 compression_method;
    uint32 raw_image_size;
    int32 horizontal_ppm;
    int32 vertical_ppm;
    uint32 color_palette;
    uint32 important_colors;
};

#define DIB_BITMAP_TYPE_OS22XBITMAPHEADER 64
// OR BITMAPINFOHEADER2
struct DIB_OS22XBITMAPHEADER {
    uint32 size;
    uint32 width;
    uint32 height;
    uint16 color_planes;
    uint16 bits_per_pixel;
    uint32 compression_method;
    uint32 raw_image_size;
    int32 horizontal_ppm;
    int32 vertical_ppm;
    uint32 color_palette;
    uint32 important_colors;

    uint16 units;
    uint16 padding;
    uint16 bit_direction;
    uint16 halftoning_algorithm;
    int32 halftoning_parameter_1;
    int32 halftoning_parameter_2;
    int32 color_encoding;
    int32 application_identifier;
};

#define DIB_BITMAPINFOHEADER_COMPRESSION_BI_RGB 0x0000
#define DIB_BITMAPINFOHEADER_COMPRESSION_BI_RLE8 0x0001
#define DIB_BITMAPINFOHEADER_COMPRESSION_BI_RLE4 0x0002
#define DIB_BITMAPINFOHEADER_COMPRESSION_BI_BITFIELDS 0x0003
#define DIB_BITMAPINFOHEADER_COMPRESSION_BI_JPEG 0x0004
#define DIB_BITMAPINFOHEADER_COMPRESSION_BI_PNG 0x0005
#define DIB_BITMAPINFOHEADER_COMPRESSION_BI_ALPHABITFIELDS 0x0006
#define DIB_BITMAPINFOHEADER_COMPRESSION_BI_CMYK 0x000B
#define DIB_BITMAPINFOHEADER_COMPRESSION_BI_CMYKRLE8 0x000C
#define DIB_BITMAPINFOHEADER_COMPRESSION_BI_CMYKRLE4 0x000D

#define DIB_BITMAPINFOHEADER_HALFTONING_NONE 0
#define DIB_BITMAPINFOHEADER_HALFTONING_ERROR_DIFFUSION 1
#define DIB_BITMAPINFOHEADER_HALFTONING_PANDA 2
#define DIB_BITMAPINFOHEADER_HALFTONING_SUPER_CIRCLE 3

#define DIB_BITMAP_TYPE_BITMAPV2INFOHEADER 52
struct DIB_BITMAPV2INFOHEADER {

};

#define DIB_BITMAP_TYPE_BITMAPV3INFOHEADER 56
struct DIB_BITMAPV3INFOHEADER {

};

struct TOS_CIEXYZ {
    int32 ciexyzX;
    int32 ciexyzY;
    int32 ciexyzZ;
};

struct TOS_CIEXYZTRIPLE {
    TOS_CIEXYZ ciexyzRed;
    TOS_CIEXYZ ciexyzGreen;
    TOS_CIEXYZ ciexyzBlue;
};

#define DIB_BITMAP_TYPE_BITMAPV4HEADER 108
struct DIB_BITMAPV4HEADER {
    int32 size;
    uint32 width;
    uint32 height;
    uint16 color_planes;
    uint16 bits_per_pixel;
    int32 compression_method;
    int32 raw_image_size;
    int32 horizontal_ppm;
    int32 vertical_ppm;
    uint32 color_palette;
    uint32 important_colors;

    int32 bV4RedMask;
    int32 bV4GreenMask;
    int32 bV4BlueMask;
    int32 bV4AlphaMask;
    int32 bV4CSType;
    TOS_CIEXYZTRIPLE bV4Endpoints;
    int32 bV4GammaRed;
    int32 bV4GammaGreen;
    int32 bV4GammaBlue;
};

#define DIB_BITMAP_TYPE_BITMAPV5HEADER 124
struct DIB_BITMAPV5HEADER {
    int32 size;
    uint32 width;
    uint32 height;
    uint16 color_planes;
    uint16 bits_per_pixel;
    int32 compression_method;
    int32 raw_image_size;
    int32 horizontal_ppm;
    int32 vertical_ppm;
    uint32 color_palette;
    uint32 important_colors;

    int32 bV5RedMask;
    int32 bV5GreenMask;
    int32 bV5BlueMask;
    int32 bV5AlphaMask;
    int32 bV5CSType;
    TOS_CIEXYZTRIPLE bV5Endpoints;
    int32 bV5GammaRed;
    int32 bV5GammaGreen;
    int32 bV5GammaBlue;
    int32 bV5Intent;
    int32 bV5ProfileData;
    int32 bV5ProfileSize;
    int32 bV5Reserved;
};

struct Bitmap {
    BitmapHeader header;
    uint32 dib_header_type;
    DIB_BITMAPINFOHEADER dib_header; // Despite the different header types we use this one for simplicity
    uint32* extra_bit_mask; // 3-4 = 12-16 bytes
    byte color_table_size;
    byte* color_table;

    // Structure:
    //      1. pixels stored in rows
    //      2. rows are padded in multiples of 4 bytes
    //      3. rows start from the bottom (unless the height is negative)
    //      4. pixel data is stored in ABGR (graphics libraries usually need BGRA or RGBA)
    byte* pixels; // WARNING: This is not the owner of the data. The owner is the FileBody

    uint32 size;
    byte* data; // WARNING: This is not the owner of the data. The owner is the FileBody
};

void generate_default_bitmap_references(const FileBody* file, Bitmap* bitmap) noexcept
{
    bitmap->size = (uint32) file->size;
    bitmap->data = file->content;

    if (bitmap->size < BITMAP_HEADER_SIZE) {
        // This shouldn't happen
        return;
    }

    // Fill header
    bitmap->header.identifier[0] = *(file->content + 0);
    bitmap->header.identifier[1] = *(file->content + 1);

    bitmap->header.size        = SWAP_ENDIAN_LITTLE(*((uint32 *) (file->content + 2)));
    bitmap->header.app_data[0] = *(file->content + 6);
    bitmap->header.app_data[1] = *(file->content + 7);
    bitmap->header.app_data[2] = *(file->content + 8);
    bitmap->header.app_data[3] = *(file->content + 9);
    bitmap->header.offset      = SWAP_ENDIAN_LITTLE(*((uint32 *) (file->content + 10)));

    byte* dib_header_offset = file->content + BITMAP_HEADER_SIZE;
    bitmap->dib_header_type  = SWAP_ENDIAN_LITTLE(*((uint32 *) dib_header_offset));

    byte* color_table_offset = file->content + BITMAP_HEADER_SIZE + bitmap->dib_header_type;

    // Fill DIB header
    switch (bitmap->dib_header_type) {
        case DIB_BITMAP_TYPE_BITMAPCOREHEADER: {
                bitmap->dib_header.size           = SWAP_ENDIAN_LITTLE(*((uint32 *) (dib_header_offset)));
                bitmap->dib_header.width          = SWAP_ENDIAN_LITTLE(*((uint16 *) (dib_header_offset + 4)));
                bitmap->dib_header.height         = SWAP_ENDIAN_LITTLE(*((uint16 *) (dib_header_offset + 6)));
                bitmap->dib_header.color_planes   = SWAP_ENDIAN_LITTLE(*((uint16 *) (dib_header_offset + 8)));
                bitmap->dib_header.bits_per_pixel = SWAP_ENDIAN_LITTLE(*((uint16 *) (dib_header_offset + 10)));
                bitmap->dib_header.color_palette  = 1U << bitmap->dib_header.bits_per_pixel;
            } break;
        case DIB_BITMAP_TYPE_BITMAPV5HEADER:
        case DIB_BITMAP_TYPE_BITMAPV4HEADER:
        case DIB_BITMAP_TYPE_BITMAPV3INFOHEADER:
        case DIB_BITMAP_TYPE_BITMAPV2INFOHEADER:
        case DIB_BITMAP_TYPE_OS22XBITMAPHEADER:
        case DIB_BITMAP_TYPE_BITMAPINFOHEADER: {
                bitmap->dib_header.size               = SWAP_ENDIAN_LITTLE(*((uint32 *) (dib_header_offset)));
                bitmap->dib_header.width              = SWAP_ENDIAN_LITTLE(*((int32 *) (dib_header_offset + 4)));
                bitmap->dib_header.height             = SWAP_ENDIAN_LITTLE(*((int32 *) (dib_header_offset + 8)));
                bitmap->dib_header.color_planes       = SWAP_ENDIAN_LITTLE(*((uint16 *) (dib_header_offset + 12)));
                bitmap->dib_header.bits_per_pixel     = SWAP_ENDIAN_LITTLE(*((uint16 *) (dib_header_offset + 14)));
                bitmap->dib_header.compression_method = SWAP_ENDIAN_LITTLE(*((uint32 *) (dib_header_offset + 16)));
                bitmap->dib_header.raw_image_size     = SWAP_ENDIAN_LITTLE(*((uint32 *) (dib_header_offset + 20)));
                bitmap->dib_header.horizontal_ppm     = SWAP_ENDIAN_LITTLE(*((uint32 *) (dib_header_offset + 24)));
                bitmap->dib_header.vertical_ppm       = SWAP_ENDIAN_LITTLE(*((uint32 *) (dib_header_offset + 28)));
                bitmap->dib_header.color_palette      = SWAP_ENDIAN_LITTLE(*((uint32 *) (dib_header_offset + 32)));
                bitmap->dib_header.important_colors   = SWAP_ENDIAN_LITTLE(*((uint32 *) (dib_header_offset + 36)));

                if (bitmap->dib_header.compression_method == DIB_BITMAPINFOHEADER_COMPRESSION_BI_BITFIELDS) {
                    // 12 bytes
                    bitmap->extra_bit_mask = (uint32 *) (dib_header_offset + DIB_BITMAP_TYPE_BITMAPINFOHEADER);
                    color_table_offset += 12;
                } else if (bitmap->dib_header.compression_method == DIB_BITMAPINFOHEADER_COMPRESSION_BI_ALPHABITFIELDS) {
                    // 16 bytes
                    bitmap->extra_bit_mask = (uint32 *) (dib_header_offset + DIB_BITMAP_TYPE_BITMAPINFOHEADER);
                    color_table_offset += 16;
                }
            } break;
        default: {

        }
    }

    // Fill other
    bitmap->color_table = color_table_offset;
    bitmap->pixels      = (byte *) (file->content + bitmap->header.offset);
}

void image_bmp_generate(const FileBody* src_data, Image* image) noexcept
{
    // @performance We are generating the struct and then filling the data.
    //      There is some assignment/copy overhead
    Bitmap src = {};
    generate_default_bitmap_references(src_data, &src);

    image->width = src.dib_header.width;
    image->height = src.dib_header.height;
    image->pixel_count = image->width * image->height;

    // rows are 4 bytes multiples in length
    uint32 width = ROUND_TO_NEAREST(src.dib_header.width, 4);

    uint32 pixel_bytes = src.dib_header.bits_per_pixel / 8;
    byte alpha_offset = pixel_bytes > 3;

    image->image_settings |= (image->image_settings & IMAGE_SETTING_CHANNEL_COUNT) == 0
        ? pixel_bytes
        : image->image_settings & IMAGE_SETTING_CHANNEL_COUNT;

    if ((image->image_settings & IMAGE_SETTING_PIXEL_BGR)
        && (image->image_settings & IMAGE_SETTING_BOTTOM_TO_TOP)
    ) {
        // @bug This doesn't consider the situation where we want alpha as a setting but the img doesn't have it
        // @bug This also copies possible padding which will corrupt the image
        memcpy((void *) image->pixels, src.pixels, image->pixel_count * pixel_bytes);

        return;
    }

    uint32 pixel_rgb_bytes = pixel_bytes - alpha_offset;
    uint32 width_pixel_bytes = width * pixel_bytes;

    for (uint32 y = 0; y < src.dib_header.height; ++y) {
        uint32 row_pos1 = y * width_pixel_bytes;
        uint32 row_pos2 = image->image_settings & IMAGE_SETTING_BOTTOM_TO_TOP
            ? y * width_pixel_bytes
            : (src.dib_header.height - y - 1) * width_pixel_bytes;

        for (uint32 x = 0; x < width; ++x) {
            if (x >= image->width) {
                // Bitmaps may have padding at the end of the row
                // We don't care about that
                continue;
            }

            // Invert byte order
            if (image->image_settings & IMAGE_SETTING_PIXEL_BGR) {
                for (uint32 i = 0; i < pixel_rgb_bytes; ++i) {
                    image->pixels[row_pos1 + x * pixel_bytes + i] = src.pixels[row_pos2 + x * pixel_bytes + i];
                }
            } else {
                for (uint32 i = 0; i < pixel_rgb_bytes; ++i) {
                    image->pixels[row_pos1 + x * pixel_bytes + i] = src.pixels[row_pos2 + x * pixel_bytes + pixel_rgb_bytes - i];
                }
            }

            // Add alpha channel at end of every RGB value (either the image already contains it or we have to add it manually)
            if (alpha_offset > 0) {
                image->pixels[row_pos1 + x * pixel_bytes + 3] = src.pixels[row_pos2 + x * pixel_bytes + pixel_bytes + 3];
            } else if ((image->image_settings & IMAGE_SETTING_CHANNEL_COUNT) == 4) {
                image->pixels[row_pos1 + x * pixel_bytes + 3] = 0xFF;
            }
        }
    }
}

#endif