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

#if __linux__
    #include <linux/limits.h>
    #define PATH_MAX_LENGTH PATH_MAX
#elif _WIN32
    #include <stddef.h>
    #define PATH_MAX_LENGTH MAX_PATH
#else
    #define PATH_MAX_LENGTH 255
#endif

#ifdef DEBUG
    #define NO_EXCEPT
#else
    #define NO_EXCEPT noexcept
#endif

#ifndef ASSUMED_CACHE_LINE_SIZE
    // In some places it is not really feasible to use runtime cache line size
    // By using this macro we can at least define a semi sensible value,
    // which we can modify during compilation for different platforms
    #define ASSUMED_CACHE_LINE_SIZE 64
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
#define MAX_UINT8 0xFF
#define MAX_UINT16 0xFFFF
#define MAX_UINT32 0xFFFFFFFF
#define MAX_UINT64 0xFFFFFFFFFFFFFFFFULL

#define MAX_CHAR 0x7F
#define MAX_INT8 0x7F
#define MAX_INT16 0x7FFF
#define MAX_INT32 0x7FFFFFFF
#define MAX_INT64 0x7FFFFFFFFFFFFFFF

#define MIN_CHAR 0x80
#define MIN_INT8 0x80
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

#define FLOAT32_SIGN_MASK 0x80000000
#define FLOAT64_SIGN_MASK 0x8000000000000000

#define OMS_EPSILON_F32 1.19209290e-07f
#define OMS_EPSILON_F64 2.2204460492503131e-16

/**
 * Some implementations should support 32 bit and 64 bit.
 * For this reason we need types that are universal.
 * And who knows, maybe we will get to 128 bits as default integer sizes.
 */
#if !defined(_WIN64) && !__x86_64__ && !__ppc64__
    #define OMS_UINT_MAX 0xFFFFFFFFU
    #define OMS_UINT_ONE 1U

    #define ENV_64 1
#else
    #define OMS_UINT_MAX 0xFFFFFFFFFFFFFFFFULL
    #define OMS_UINT_ONE 1ULL

    #define ENV_32 1
#endif

// This is of course CPU dependent but we need a reasonable default value
#define CACHE_LINE_SIZE 64
#define CACHE_PAGE_SIZE 4096
#define CACHE_L1_SIZE 32768
#define CACHE_L2_SIZE 262144
#define CACHE_L3_SIZE 4194304

// Compiler macros/flags defaults
#ifndef CPP_VERSION
    #define CPP_VERSION 25
#endif

#endif