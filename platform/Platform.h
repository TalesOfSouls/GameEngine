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
#include "../stdlib/Helper.h"

#if PLAYSTATION || XBOX
#elif NINTENDO_SWITCH
#else
    // Depending on the platform we may want to use different versions
    // On some platforms the other version may be faster
    #define OMS_MAX(a, b) OMS_MAX_BRANCHLESS(a, b)
    #define OMS_MIN(a, b) OMS_MIN_BRANCHLESS(a, b)
    #define OMS_CLAMP(val, low, high) OMS_CLAMP_BRANCHLESS(val, low, high)

    template <typename T>
    inline T oms_max(T a, T b) NO_EXCEPT
{
        return max_branchless(a, b);
    }

    template <typename T>
    inline T oms_min(T a, T b) NO_EXCEPT
{
        return min_branchless(a, b);
    }

    template <typename T>
    inline T oms_clamp(T val, T low, T high) NO_EXCEPT
{
        return clamp_branchless(val, low, high);
    }
#endif

#endif