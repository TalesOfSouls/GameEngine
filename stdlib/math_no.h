/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_STDLIB_MATH_NO_H
#define COMS_STDLIB_MATH_NO_H

#if defined(__GNUC__) || defined(__clang__)
    // Functions provided by the compiler even if "no" stdlib is available
    // In reality THIS is just the stdlib
    #define fabsf __builtin_fabsf
    #define sin __builtin_sin
    #define cos __builtin_cos
    #define tan __builtin_tan
    #define asin __builtin_asin
    #define acos __builtin_acos
    #define atan __builtin_atan
    #define sqrt __builtin_sqrt
    #define exp __builtin_exp
    #define pow __builtin_pow
    #define log __builtin_log
    #define roundf __builtin_roundf
    #define ceil __builtin_ceil
    #define floorf __builtin_floorf
#else
    #include "math_internal.h"

    // These are user space stdlib functions
    #define fabsf __internal_abs

    // @todo implement more accurate solutions instead of using the _approx ones
    #define sin sin_approx
    #define cos cos_approx
    #define tan tan_approx
    #define asin asin_approx
    #define acos acos_approx
    #define atan atan_approx
    #define sqrt sqrt_approx
    #define exp exp_approx
    #define pow pow_approx
    #define log log_approx

    #define roundf __internal_round
    #define ceil __internal_ceil
    #define floorf __internal_floorf
#endif

#endif