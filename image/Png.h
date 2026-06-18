/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 *
 * png: https://www.w3.org/TR/2003/REC-PNG-20031110/
 * png: https://www.w3.org/TR/PNG-Chunks.html
 * zlib: https://www.ietf.org/rfc/rfc1950.txt
 * deflate: https://www.ietf.org/rfc/rfc1951.txt
 */
#ifndef COMS_IMAGE_PNG_H
#define COMS_IMAGE_PNG_H

#include <string.h>
#include "../stdlib/Stdlib.h"
#include "../utils/BitUtils.h"
#include "Image.h"

// Packed header size
#define PNG_HEADER_SIZE 8

struct PngHeader {
    uint8 signature[8];
};

/*
The following table describes the chunk layout.
Please note that we do NOT support most of this

Critical chunks (order is defined):

    Name  Multiple  Ordering constraints
    IHDR    No      Must be first
    PLTE    No      Before IDAT (optional)
    IDAT    Yes     Multiple IDATs must be consecutive
    IEND    No      Must be last

Ancillary chunks (order is not defined):

    Name  Multiple  Ordering constraints
    cHRM    No      Before PLTE and IDAT
    gAMA    No      Before PLTE and IDAT
    iCCP    No      Before PLTE and IDAT
    sBIT    No      Before PLTE and IDAT
    sRGB    No      Before PLTE and IDAT
    bKGD    No      After PLTE, before IDAT
    hIST    No      After PLTE, before IDAT
    tRNS    No      After PLTE, before IDAT
    pHYs    No      Before IDAT
    sPLT    Yes     Before IDAT
    tIME    No      None
    iTXt    Yes     None
    tEXt    Yes     None
    zTXt    Yes     None
*/
#define PNG_CHUNK_SIZE_MIN 12

struct PngChunk {
    uint32 length;
    uint32 type;
    // +data here, can be 0
    uint32 crc;
};

// Special chunk
#define PNG_IHDR_SIZE 25
struct PngIHDR {
    uint32 length;
    uint32 type;
    uint32 width;
    uint32 height;
    uint8 bit_depth;
    uint8 color_type;
    uint8 compression;
    uint8 filter;
    uint8 interlace;
    uint32 crc;
};

struct PngIDATHeader {
    uint8 zlib_method_flag;
    uint8 add_flag;
};

struct Png {
    PngHeader header;
    PngIHDR ihdr;

    // Encoded pixel data
    uint8* pixels; // WARNING: This is not the owner of the data. The owner is the FileBody

    uint32 size;
    uint8* data; // WARNING: This is not the owner of the data. The owner is the FileBody
};

void generate_default_png_references(const FileBody* file, Png* png)
{
    png->size = (uint32) file->size;
    png->data = file->content;

    if (png->size < PNG_IHDR_SIZE + PNG_HEADER_SIZE) {
        // This shouldn't happen
        ASSERT_THROW();
        return;
    }

    // The first chunk MUST be IHDR -> we handle it here
    ASSERT_TRUE_CONST(PNG_HEADER_SIZE + PNG_IHDR_SIZE == 33);
    memcpy(png, file->content, PNG_HEADER_SIZE + PNG_IHDR_SIZE);

    SWAP_ENDIAN_BIG_SELF(png->ihdr.length);
    SWAP_ENDIAN_BIG_SELF(png->ihdr.type);
    SWAP_ENDIAN_BIG_SELF(png->ihdr.width);
    SWAP_ENDIAN_BIG_SELF(png->ihdr.height);
    SWAP_ENDIAN_BIG_SELF(png->ihdr.crc);
}

bool image_header_png_generate(const FileBody* src_data, Image* image)
{
    // @performance We are generating the struct and then filling the data.
    //      There is some asignment/copy overhead
    Png src = {0};
    generate_default_png_references(src_data, &src);

    // @todo Support color_type == 3
    if (src.ihdr.bit_depth != 8
        || (src.ihdr.color_type != 6 && src.ihdr.color_type != 2)
        || src.ihdr.compression != 0
        || src.ihdr.filter != 0
        || src.ihdr.interlace != 0
    ) {
        // We don't support this type of png (see comment below)
        ASSERT_THROW();

        /*
        Color   Allowed     Interpretation
        Type    Bit Depths

        0       1,2,4,8,16  Each pixel is a grayscale sample.
        2       8,16        Each pixel is an R,G,B triple.
        3       1,2,4,8     Each pixel is a palette index, a PLTE chunk must appear.
        4       8,16        Each pixel is a grayscale sample, followed by an alpha sample.
        6       8,16        Each pixel is an R,G,B triple, followed by an alpha sample.
        */

        return false;
    }

    image->width = src.ihdr.width;
    image->height = src.ihdr.height;
    image->pixel_count = image->width * image->height;

    uint32 bytes_per_pixel;
    if (src.ihdr.color_type == 6) {
        bytes_per_pixel = 4;
    } else if (src.ihdr.color_type == 2) {
        bytes_per_pixel = 3;
    } else if (src.ihdr.color_type == 3) {
        bytes_per_pixel = 1;
    } else {
        return false;
    }

    image->image_settings |= bytes_per_pixel;

    return true;
}

#endif