/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_STDLIB_HELPER_H
#define COMS_STDLIB_HELPER_H

#include "../compiler/CompilerUtils.h"

// Counts the elements in an array IFF its size is defined at compile time
#define ARRAY_COUNT(a) ((a) == NULL ? 0 : (sizeof(a) / sizeof((a)[0])))

// Gets the size of a struct member
#define MEMBER_SIZEOF(type, member) (sizeof(((type *)0)->member))

template <typename T, size_t N>
CONSTEXPR int32_t array_count_helper(const T (&)[N]) {
    return static_cast<int32_t>(N);
}

#define ARRAY_COUNT_MEMBER(type, member) array_count_helper(((type*)0)->member)
// This doesn't always result in CONSTEXPR
//#define ARRAY_COUNT_MEMBER(type, member) (sizeof(((type *) 0)->member) / sizeof(((type *) 0)->member[0]))

// Limits
#define OMS_MAX_BRANCHED(a, b) ((a) > (b) ? (a) : (b))
#define OMS_MIN_BRANCHED(a, b) ((a) > (b) ? (b) : (a))
#define OMS_MAX_BRANCHLESS(a, b) ((a) ^ (((a) ^ (b)) & -((a) < (b))))
#define OMS_MIN_BRANCHLESS(a, b) ((b) ^ (((a) ^ (b)) & -((a) < (b))))
#define OMS_CLAMP_BRANCHED(val, low, high) ((val) < (low) ? (low) : ((val) > (high) ? (high) : (val)))
#define OMS_CLAMP_BRANCHLESS(val, low, high) \
    ((val) ^ (((val) ^ (low)) & -(int32)((val) < (low))) ^ \
    ((((val) ^ ((val) ^ (low)) & -(int32)((val) < (low)))) ^ (high)) & -(int32)((((val) ^ ((val) ^ (low)) & -(int32)((val) < (low)))) < (high)))

// Abs
#define OMS_ABS(a) ((a) > 0 ? (a) : -(a))
#define OMS_ABS_INT8(a) (((a) ^ ((a) >> 7)) - ((a) >> 7))
#define OMS_ABS_INT16(a) (((a) ^ ((a) >> 15)) - ((a) >> 15))
#define OMS_ABS_INT32(a) (((a) ^ ((a) >> 31)) - ((a) >> 31))
#define OMS_ABS_INT64(a) (((a) ^ ((a) >> 63)) - ((a) >> 63))

template <typename T>
inline T max_branched(T a, T b) NO_EXCEPT
{
    return (a > b) ? a : b;
}

template <typename T>
inline T min_branched(T a, T b) NO_EXCEPT
{
    return (a > b) ? b : a;
}

// WARNING: May overflow for ints
template <typename T>
inline T max_branchless_general(T a, T b) NO_EXCEPT
{
    return a + (b - a) * (b > a);
}

// Only allowed for int types
template <typename T>
inline T max_branchless(T a, T b) NO_EXCEPT
{
    return (T)(a ^ (((a ^ b) & -(T)((a < b)))));
}

// WARNING: May overflow for ints
template <typename T>
inline T min_branchless_general(T a, T b) NO_EXCEPT
{
    return a + (b - a) * (b < a);
}

// Only allowed for int types
template <typename T>
inline T min_branchless(T a, T b) NO_EXCEPT
{
    return (T)(b ^ (((a ^ b) & -(T)((a < b)))));
}

template <typename T>
inline T clamp_branched(T val, T low, T high) NO_EXCEPT
{
    return (val < low) ? low : ((val > high) ? high : val);
}

// WARNING: May overflow for ints
template <typename T>
inline T clamp_branchless_general(T v, T lo, T hi) NO_EXCEPT
{
    T t = v + (hi - v) * (v > hi);

    return lo + (t - lo) * (t > lo);
}

// Only allowed for int types
template <typename T>
inline T clamp_branchless(T val, T low, T high) NO_EXCEPT
{
    T t1 = (T)(val ^ ((val ^ low) & -(T)((val < low))));
    return (T)(t1 ^ ((t1 ^ high) & -(T)((t1 > high))));
}

template <typename T>
inline T abs(T a) NO_EXCEPT
{
    return (a > (T)0) ? a : (T)(-a);
}

// For floats the high bit is still defining the sign
// But we need to reinterpret it as int to mask the sign
inline
f32 OMS_ABS_F32(f32 a) NO_EXCEPT
{
    union { f32 f; uint32 i; } u;
    u.f = a;
    u.i &= 0x7FFFFFFF;
    return u.f;
}

inline
f64 OMS_ABS_F64(f64 a) NO_EXCEPT
{
    union { f64 f; uint64 i; } u;
    u.f = a;
    u.i &= 0x7FFFFFFFFFFFFFFF;
    return u.f;
}

// Rounding
#define OMS_ROUND_POSITIVE_32(x) ((int32)((x) + 0.5f))
#define OMS_ROUND_POSITIVE_64(x) ((int64)((x) + 0.5f))
#define OMS_ROUND(x) (((x) >= 0) ? (f32)((int32)((x) + 0.5f)) : (f32)((int32)((x) - 0.5f)))

#define CEIL_DIV(a, b) (((a) + (b) - 1) / (b))
#define OMS_CEIL_32(x) ((x) == (int32)(x) ? (int32)(x) : ((x) > 0 ? (int32)(x) + 1 : (int32)(x)))
#define OMS_CEIL_16(x) ((x) == (int16)(x) ? (int16)(x) : ((x) > 0 ? (int16)(x) + 1 : (int16)(x)))
#define OMS_CEIL_8(x) ((x) == (int8)(x) ? (int8)(x) : ((x) > 0 ? (int8)(x) + 1 : (int8)(x)))
#define FLOORF(x) ((float)((int32)(x) - ((x) < 0.0f && (x) != (int32)(x))))

template <typename T>
inline T ceil_div(T a, T b) NO_EXCEPT
{ return (a + b - 1) / b; }

template <typename T, typename F>
inline T ceil(F x) NO_EXCEPT
{
    T xi = (T)x;
    if (x == (F)xi) {
        return xi;
    }

    return (x > (F) 0) ? (T) (xi + 1) : xi;
}

template <typename F>
inline F floorf(F x) NO_EXCEPT
{
    int32 xi = (int32)x;
    return (F)(xi - ((x < (F)0.0 && x != (F)xi) ? 1 : 0));
}

// Fast integer division and floor, IFF the divisor **is positive**
// (= (int) floorf((float)a/(float)b))
// This is required because -7 / 3 = -2 with normal int division, but we want -3
// However, 7 / 3 = 2 is what we would expect
#define IFLOORI_POS_DIV_32(a, b) (((a) - (((b) - 1) & ((a) >> 31))) / (b))
#define IFLOORI_POS_DIV_64(a, b) (((a) - (((b) - 1) & ((a) >> 63))) / (b))

// Trig
#define OMS_DEG2RAD(angle) ((angle) * OMS_PI_F32 / 180.0f)
#define OMS_RAD2DEG(angle) ((angle) * 180.0f / OMS_PI_F32)

// -pi / pi
#define OMS_NORMALIZE_RAD(angle) ((angle) - OMS_TAU_F32 * FLOORF(((angle) + OMS_PI_F32) / OMS_TAU_F32))
// 0 / 360
#define OMS_NORMALIZE_DEG(angle) ((angle) - 360.0f * FLOORF((angle) / 360.0f))

// Zero and comparison
#define OMS_EPSILON_F32 1.19209290e-07f
#define OMS_EPSILON_F64 2.2204460492503131e-16

#define OMS_IS_ZERO_F32(x) (OMS_ABS(x) < OMS_EPSILON_F32)
#define OMS_IS_ZERO_F64(x) (OMS_ABS(x) < OMS_EPSILON_F64)
#define OMS_FEQUAL_F32(a, b) (OMS_ABS((a) - (b)) < OMS_EPSILON_F32)
#define OMS_FEQUAL_F64(a, b) (OMS_ABS((a) - (b)) < OMS_EPSILON_F64)

#define OMS_HAS_ZERO(x) (((x) - ((size_t)-1 / 0xFF)) & ~(x) & (((size_t)-1 / 0xFF) * (0xFF / 2 + 1)))
#define OMS_HAS_CHAR(x, c) (OMS_HAS_ZERO((x) ^ (((size_t)-1 / 0xFF) * (c))))

// Math operations
// Only useful if n is a variable BUT you as programmer know the form of the value
#define OMS_POW2_I64(n) (1ULL << (n))
#define OMS_POW2_I32(n) (1U << (n))
#define OMS_DIV2_I64(n) ((n) >> 1ULL)
#define OMS_DIV2_I32(n) ((n) >> 1U)
#define OMS_MUL2_I64(n) ((n) << 1ULL)
#define OMS_MUL2_I32(n) ((n) << 1U)

// Bitwise utilities
#define OMS_SIGN_32(x) (1 | ((x) >> 31 << 1))
#define OMS_SIGN_64(x) (1LL | ((x) >> 63 << 1))
#define OMS_IS_POW2(x) (((x) & ((x) - 1)) == 0)
#define OMS_ALIGN_UP(x, align) (((x) + ((align) - 1)) & ~((align) - 1))
#define OMS_ALIGN_DOWN(x, align) ((x) & ~((align) - 1))
#define OMS_IS_ALIGNED(x, align) (((x) & ((align) - 1)) == 0)

#define OMS_FLAG_SET(flags, bit) ((flags) | (bit))
#define OMS_FLAG_CLEAR(flags, bit) ((flags) & ~(bit))
#define OMS_FLAG_REMOVE(flags, bit) OMS_FLAG_CLEAR(flags, bit)
#define OMS_FLAG_DELETE(flags, bit) OMS_FLAG_CLEAR(flags, bit)
#define OMS_FLAG_TOGGLE(flags, bit) ((flags) ^ (bit))
#define OMS_FLAG_FLIP(flags, bit) OMS_FLAG_TOGGLE(flags, bit)
#define OMS_FLAG_CHECK(flags, bit) ((flags) & (bit))
#define OMS_FLAG_IS_SET(flags, bit) OMS_FLAG_CHECK(flags, bit)

#define OMS_BIT_WORD_INDEX(pos) ((pos) / (8 * sizeof(size_t)))
#define OMS_BIT_INDEX(pos) ((pos) & ((8 * sizeof(size_t)) - 1))
#define OMS_BIT_SET(flags, pos) ((flags)[OMS_BIT_WORD_INDEX(pos)] | ((size_t)1 << OMS_BIT_INDEX(pos)))
#define OMS_BIT_CLEAR(flags, pos) ((flags)[OMS_BIT_WORD_INDEX(pos)] & ~((size_t)1 << OMS_BIT_INDEX(pos)))
#define OMS_BIT_REMOVE(flags, pos) OMS_BIT_CLEAR(flags, pos)
#define OMS_BIT_DELETE(flags, pos) OMS_BIT_CLEAR(flags, pos)
#define OMS_BIT_TOGGLE(flags, pos) ((flags)[OMS_BIT_WORD_INDEX(pos)] ^ ((size_t)1 << OMS_BIT_INDEX(pos)))
#define OMS_BIT_FLIP(flags, pos) OMS_BIT_TOGGLE(flags, pos)
#define OMS_BIT_CHECK(flags, pos) (((flags)[OMS_BIT_WORD_INDEX(pos)] >> OMS_BIT_INDEX(pos)) & (size_t)1)
#define OMS_BIT_IS_SET(flags, pos) OMS_BIT_CHECK(flags, pos)

// This is the same as using % but for sufficiently large wrapping this is faster
// WARNING: if wrap is a power of 2 don't use this but use the & operator
//          I recommend to use this macro if wrap >= 1,000
#define OMS_WRAPPED_INCREMENT(value, wrap) ++value; if (value >= wrap) UNLIKELY value = 0
#define OMS_WRAPPED_DECREMENT(value, wrap) --value; if (value < 0) UNLIKELY value = wrap - 1

#define OMS_SWAP(type, a, b) type _oms_tmp = (a); (a) = (b); (b) = _oms_tmp

#define MEMSET_ZERO_32(ptr) (*(uint32 *)(ptr) = 0U)
#define MEMSET_ZERO_64(ptr) (*(uint64 *)(ptr) = 0ULL)
#define MEMSET_ZERO(ptr) (*(size_t *)(ptr) = 0)

// Casting between e.g. f32 and int32 without changing bits
#define BITCAST(x, new_type) bitcast_impl_##new_type(x)
#define DEFINE_BITCAST_FUNCTION(from_type, to_type) \
    static inline to_type bitcast_impl_##to_type(from_type src) { \
        union { from_type src; to_type dst; } u; \
        u.src = src; \
        return u.dst; \
    }

DEFINE_BITCAST_FUNCTION(f32, uint32)
DEFINE_BITCAST_FUNCTION(uint32, f32)
DEFINE_BITCAST_FUNCTION(f64, uint64)
DEFINE_BITCAST_FUNCTION(uint64, f64)
DEFINE_BITCAST_FUNCTION(f32, int32)
DEFINE_BITCAST_FUNCTION(int32, f32)
DEFINE_BITCAST_FUNCTION(f64, int64)
DEFINE_BITCAST_FUNCTION(int64, f64)

// Modulo function when b is a power of 2
#define MODULO_2(a, b) ((a) & (b - 1))

// Simple iterator implementation
#define iterator_start(start, end, obj) {   \
    int32 _i = start;                      \
    while (_i++ < end) {

#define iterator_end    \
        ++obj;       \
    }}                  \

// Adjusts the step size based on the memory alignment
inline
int32 intrin_validate_steps(const byte* mem, int32 steps) NO_EXCEPT
{
    if (steps >= 16 && ((uintptr_t) mem & 63) == 0) {
        return 16;
    } else if (steps >= 8 && ((uintptr_t) mem & 31) == 0) {
        return 8;
    } else if (steps >= 4 && ((uintptr_t) mem & 15) == 0) {
        return 4;
    } else {
        return 1;
    }
}

// Template helpers
template<typename A, typename B>
struct is_same {
    static const bool value = false;
};

template<typename A>
struct is_same<A, A> {
    static const bool value = true;
};

template<bool Condition, typename T = void>
struct enable_if {};

template<typename T>
struct enable_if<true, T> {
    using type = T;
};

#endif