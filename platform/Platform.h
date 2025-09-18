/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_PLATFORM
#define COMS_PLATFORM

#include "../stdlib/Types.h"

#if PLAYSTATION || XBOX
#elif NINTENDO_SWITCH
#else
    // Depending on the platform we may want to use different versions
    // On some platforms the other version may be faster
    #define OMS_MAX(a, b) OMS_MAX_BRANCHLESS(a, b)
    #define OMS_MIN(a, b) OMS_MIN_BRANCHLESS(a, b)
    #define OMS_CLAMP(val, low, high) OMS_CLAMP_BRANCHLESS(val, low, high)
#endif

#endif