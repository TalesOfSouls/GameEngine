/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_UTILS_TIME_UTILS_H
#define COMS_UTILS_TIME_UTILS_H

#if _WIN32
    #include "../platform/win32/TimeUtils.h"
#else
    #include "../platform/linux/TimeUtils.h"
#endif

#endif