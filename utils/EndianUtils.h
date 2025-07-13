/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_UTILS_ENDIAN_H
#define COMS_UTILS_ENDIAN_H

#include "../stdlib/Types.h"

#define SWAP_ENDIAN_16(val) ((((val) << 8) | ((val) >> 8)))
#define SWAP_ENDIAN_32(val) (((val) << 24) | (((val) & 0xFF00) << 8) | (((val) >> 8) & 0xFF00) | ((val) >> 24))
#define SWAP_ENDIAN_64(val) (((val) << 56) | (((val) & 0x000000000000FF00ULL) << 40) | (((val) & 0x0000000000FF0000ULL) << 24) | (((val) & 0x00000000FF000000ULL) << 8) | (((val) & 0x000000FF00000000ULL) >> 8) | (((val) & 0x0000FF0000000000ULL) >> 24) | (((val) & 0x00FF000000000000ULL) >> 40) | ((val) >> 56))

// Automatically perform endian swap if necessary
// If we are on little endian (e.g. Win32) we swap big endian data but not little endian
#if _WIN32 || __LITTLE_ENDIAN__
    #define SWAP_ENDIAN_LITTLE(val) (val)
    #define SWAP_ENDIAN_BIG(val) endian_swap(val)
#else
    #define SWAP_ENDIAN_LITTLE(val) endian_swap(val)
    #define SWAP_ENDIAN_BIG(val) (val)
#endif

inline
bool is_little_endian() noexcept
{
    uint32 num = 1;
    return ((int32) (*(char *) & num)) == 1;
}

inline
uint16 endian_swap(uint16 val) noexcept
{
    return ((val << 8) | (val >> 8));
}

inline
int16 endian_swap(int16 val) noexcept
{
    return (int16) ((val << 8) | (val >> 8));
}

inline
uint32 endian_swap(uint32 val) noexcept
{
    return ((val << 24)
        | ((val & 0xFF00) << 8)
        | ((val >> 8) & 0xFF00)
        | (val >> 24));
}

inline
int32 endian_swap(int32 val) noexcept
{
    return (int32) ((val << 24)
        | ((val & 0xFF00) << 8)
        | ((val >> 8) & 0xFF00)
        | (val >> 24));
}

inline
uint64 endian_swap(uint64 val) noexcept
{
    return ((val << 56)
        | ((val & 0x000000000000FF00ULL) << 40)
        | ((val & 0x0000000000FF0000ULL) << 24)
        | ((val & 0x00000000FF000000ULL) << 8)
        | ((val & 0x000000FF00000000ULL) >> 8)
        | ((val & 0x0000FF0000000000ULL) >> 24)
        | ((val & 0x00FF000000000000ULL) >> 40)
        | (val >> 56));
}

inline
int64 endian_swap(int64 val) noexcept
{
    return (int64) ((val << 56)
        | ((val & 0x000000000000FF00ULL) << 40)
        | ((val & 0x0000000000FF0000ULL) << 24)
        | ((val & 0x00000000FF000000ULL) << 8)
        | ((val & 0x000000FF00000000ULL) >> 8)
        | ((val & 0x0000FF0000000000ULL) >> 24)
        | ((val & 0x00FF000000000000ULL) >> 40)
        | (val >> 56));
}

inline
f32 endian_swap(f32 val) noexcept
{
    return (f32) endian_swap(val);
}

inline
f64 endian_swap(f64 val) noexcept
{
    return (f64) endian_swap(val);
}

#endif