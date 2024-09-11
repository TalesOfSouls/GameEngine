/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef TOS_OBJECT_MATERIAL_H
#define TOS_OBJECT_MATERIAL_H

#include "../stdlib/Types.h"
#include "../memory/RingMemory.h"

#if _WIN32
    #include "../platform/win32/UtilsWin32.h"
#else
    #include "../platform/linux/UtilsLinux.h"
#endif

struct Material {

};

void material_from_file_txt(
    RingMemory*,
    const char*,
    Material*
)
{

}

void material_from_file()
{

}

void material_to_file()
{

}

#endif