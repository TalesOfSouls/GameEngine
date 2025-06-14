/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_SYSTEM_ALLOCATOR_H
#define COMS_SYSTEM_ALLOCATOR_H

#if _WIN32
    #include "../platform/win32/Allocator.h"
#elif __linux__
    #include "../platform/linux/Allocator.h"
#endif

#endif