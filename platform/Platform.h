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
    #define OMS_MAX(a, b) max_branchless(a, b)
    #define OMS_MIN(a, b) min_branchless(a, b)
    #define OMS_CLAMP(val, low, high) clamp_branchless(val, low, high)

    inline
    f32 oms_max(f32 a, f32 b) NO_EXCEPT
    {
        return max_branchless_general(a, b);  // whatever your f32 version is
    }

    inline
    f64 oms_max(f64 a, f64 b) NO_EXCEPT
    {
        return max_branchless_general(a, b);
    }

    inline
    f32 oms_min(f32 a, f32 b) NO_EXCEPT
    {
        return min_branchless_general(a, b);
    }

    inline
    f64 oms_min(f64 a, f64 b) NO_EXCEPT
    {
        return min_branchless_general(a, b);
    }

    inline
    f32 oms_clamp(f32 v, f32 lo, f32 hi) NO_EXCEPT
    {
        return clamp_branchless_general(v, lo, hi);
    }

    inline
    f64 oms_clamp(f64 v, f64 lo, f64 hi) NO_EXCEPT
    {
        return clamp_branchless_general(v, lo, hi);
    }

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