/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef TOS_IMAGE_C
#define TOS_IMAGE_C

#include "../utils/StringUtils.h"
#include "../memory/RingMemory.h"

#if _WIN32
    #include "../platform/win32/UtilsWin32.h"
#else
    #include "../platform/linux/UtilsLinux.h"
#endif

#include "Image.h"
#include "Tga.h"
#include "Bitmap.h"
#include "Png.h"

void image_from_file(RingMemory* ring, const char* path, Image* image)
{
    FileBody file;
    file_read(path, &file, ring);

    if (str_ends_with(path, ".png")) {
        image_png_generate(&file, image);
    } else if (str_ends_with(path, ".tga")) {
        image_tga_generate(&file, image);
    } else if (str_ends_with(path, ".bmp")) {
        image_bmp_generate(&file, image);
    }
}

#endif