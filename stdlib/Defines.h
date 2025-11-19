/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_STDLIB_DEFINES_H
#define COMS_STDLIB_DEFINES_H

#if _WIN32
    // @question Do I really need <windows.h> here or could I go lower?
    #include <windows.h>
    typedef SSIZE_T ssize_t;
#elif __linux__
    #include <linux/limits.h>
    #define MAX_PATH PATH_MAX
#endif

#ifdef DEBUG
    #define NO_EXCEPT
#else
    #define NO_EXCEPT noexcept
#endif

// PI
#define OMS_PI_F32 3.14159265358979323846f
#define OMS_PI_OVER_TWO_F32 (OMS_PI_F32 / 2.0f)
#define OMS_PI_OVER_FOUR_F32 (OMS_PI_F32 / 4.0f)
#define OMS_TWO_PI_F32 (2.0f * OMS_PI_F32)
#define OMS_TAU_F32 OMS_TWO_PI_F32
#define OMS_INV_SQRT_2_F32 0.70710678118654752440f

#define OMS_PI_F64 3.14159265358979323846
#define OMS_PI_OVER_TWO_F64 (OMS_PI_F64 / 2.0)
#define OMS_PI_OVER_FOUR_F64 (OMS_PI_F64 / 4.0)
#define OMS_TWO_PI_F64 (2.0 * OMS_PI_F64)
#define OMS_TAU_F64 OMS_TWO_PI_F64
#define OMS_INV_SQRT_2_F64 0.70710678118654752440

#define FLOAT_CAST_EPS 0.001953125f

#define SQRT_2F 1.4142135623730950488016887242097f

#define KILOBYTE 1024
#define MEGABYTE 1048576
#define GIGABYTE 1073741824ULL

#define MAX_BYTE 0xFF
#define MAX_UINT16 0xFFFF
#define MAX_UINT32 0xFFFFFFFF
#define MAX_UINT64 0xFFFFFFFFFFFFFFFFULL

#define MAX_CHAR 0x7F
#define MAX_INT16 0x7FFF
#define MAX_INT32 0x7FFFFFFF
#define MAX_INT64 0x7FFFFFFFFFFFFFFF

#define MIN_CHAR 0x80
#define MIN_INT16 0x8000
#define MIN_INT32 0x80000000
#define MIN_INT64 0x8000000000000000LL

#define MIN_MILLI 60000
#define SEC_MILLI 1000
#define MIN_MICRO 60000000
#define SEC_MICRO 1000000
#define MILLI_MICRO 1000

#define MHZ 1000000
#define GHZ 1000000000ULL

#define HALF_FLOAT_SIGN_MASK 0x8000
#define HALF_FLOAT_EXP_MASK 0x7C00
#define HALF_FLOAT_FRAC_MASK 0x03FF

#define HALF_FLOAT_EXP_SHIFT 10
#define HALF_FLOAT_EXP_BIAS 15

#define FLOAT32_SIGN_MASK 0x80000000
#define FLOAT32_EXP_MASK 0x7F800000
#define FLOAT32_FRAC_MASK 0x007FFFFF

#define FLOAT32_EXP_SHIFT 23
#define FLOAT32_EXP_BIAS 127

#endif