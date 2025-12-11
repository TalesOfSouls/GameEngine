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

/**
 * @question Consider to make the helper functions below compiler dependent
 *          A lot of the code gets optimized in different ways by different compilers
 *          This is why very often standard functions like abs, ceil are very performant compared to userland functions.
 *          The compiler replaces them with the correct ASM instructions which unfortunately is not possible with all compilers (e.g. MSVC)
 */

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
template <typename T> FORCE_INLINE
CONSTEXPR T max_branched(T a, T b) {
    return (a > b) ? a : b;
}

template <typename T> FORCE_INLINE
CONSTEXPR T min_branched(T a, T b) {
    return (a > b) ? b : a;
}

template <typename T> FORCE_INLINE
CONSTEXPR T clamp_branched(T val, T low, T high) {
    return (val < low) ? low : ((val > high) ? high : val);
}

// The branchless versions only work for int types
template <typename T> FORCE_INLINE
CONSTEXPR T max_branchless(T a, T b) {
    T mask = T(0) - T(a < b);
    return a ^ ((a ^ b) & mask);
}

template <typename T> FORCE_INLINE
CONSTEXPR T min_branchless(T a, T b) {
    T mask = T(0) - T(a < b);
    return b ^ ((a ^ b) & mask);
}

template <typename T> FORCE_INLINE
CONSTEXPR T clamp_branchless(T val, T low, T high) {
    T maskLow  = T(0) - T(val < low);
    T temp = val ^ ((val ^ low) & maskLow);

    T maskHigh = T(0) - T(temp < high);
    T result = high ^ ((temp ^ high) & maskHigh);

    return result;
}

// WARNING: May overflow for ints
template <typename T>
inline T max_branchless_general(T a, T b) NO_EXCEPT
{
    return a + (b - a) * (b > a);
}

// WARNING: May overflow for ints
template <typename T>
inline T min_branchless_general(T a, T b) NO_EXCEPT
{
    return a + (b - a) * (b < a);
}

// WARNING: May overflow for ints
template <typename T>
inline T clamp_branchless_general(T v, T lo, T hi) NO_EXCEPT
{
    T t = v + (hi - v) * (v > hi);

    return lo + (t - lo) * (t > lo);
}

// Abs
FORCE_INLINE
int8 oms_abs(int8 a) NO_EXCEPT
{ return (a ^ (a >> 7)) - (a >> 7); }

FORCE_INLINE
int16 oms_abs(int16 a) NO_EXCEPT
{ return (a ^ (a >> 15)) - (a >> 15); }

FORCE_INLINE
int32 oms_abs(int32 a) NO_EXCEPT
{ return (a ^ (a >> 31)) - (a >> 31); }

FORCE_INLINE
int64 oms_abs(int64 a) NO_EXCEPT
{ return (a ^ (a >> 63)) - (a >> 63); }

// For floats the high bit is still defining the sign
// But we need to reinterpret it as int to mask the sign
// WARNING: This is only faster on msvc
FORCE_INLINE
f32 oms_abs(f32 a) NO_EXCEPT
{
    union { f32 f; uint32 i; } u;
    u.f = a;
    u.i &= 0x7FFFFFFF;
    return u.f;
}

FORCE_INLINE
f64 oms_abs(f64 a) NO_EXCEPT
{
    union { f64 f; uint64 i; } u;
    u.f = a;
    u.i &= 0x7FFFFFFFFFFFFFFF;
    return u.f;
}

// GCC seems to heavily optimize this making the above functions redundant but MSVC doesn't
/*
FORCE_INLINE template <typename T>
T oms_abs(T a) NO_EXCEPT {
    return (a > (T)0) ? a : (T)(-a);
}
*/

// Rounding
FORCE_INLINE CONSTEXPR
int32 oms_round_positive(f32 x) NO_EXCEPT {
    return (int32) (x + 0.5f);
}

FORCE_INLINE CONSTEXPR
int64 oms_round_positive(f64 x) NO_EXCEPT {
    return (int64) (x + 0.5);
}

FORCE_INLINE CONSTEXPR
f32 oms_round(f32 x) NO_EXCEPT {
    return (x >= 0.0f)
        ? (f32)((int32)(x + 0.5f))
        : (f32)((int32)(x - 0.5f));
}

template <typename T>
FORCE_INLINE T ceil_div(T a, T b) NO_EXCEPT
{ return (a + b - 1) / b; }

template <typename T, typename F>
FORCE_INLINE T ceil(F x) NO_EXCEPT
{
    T xi = (T)x;
    if (x == (F)xi) {
        return xi;
    }

    return (x > (F) 0) ? (T) (xi + 1) : xi;
}

template <typename F>
FORCE_INLINE CONSTEXPR
F oms_floor(F x) NO_EXCEPT {
    /*
    int32 xi = (int32) x;
    bool adjust = (x < 0.0f) && (x != (f32)(xi));

    return (f32)(xi - (int32)(adjust));
    */

    int32 xi = (int32)x;
    return (F)(xi - ((x < (F)0.0 && x != (F)xi) ? 1 : 0));
}

// Fast integer division and floor, IFF the divisor **is positive**
// (= (int) floorf((float)a/(float)b))
// This is required because -7 / 3 = -2 with normal int division, but we want -3
// However, 7 / 3 = 2 is what we would expect
FORCE_INLINE
int32 floor_div(int32 a, int32 b) NO_EXCEPT
{ return (a - ((b - 1) & (a >> 31))) / b; }

FORCE_INLINE
int64 floor_div(int64 a, int64 b) NO_EXCEPT
{ return (a - ((b - 1) & (a >> 63))) / b; }

// Trig
FORCE_INLINE
f32 deg2rad(f32 angle) NO_EXCEPT
{ return angle * OMS_PI_F32 / 180.0f; }

FORCE_INLINE
f32 rad2deg(f32 angle) NO_EXCEPT
{ return angle * 180.0f / OMS_PI_F32; }

// -pi / pi
FORCE_INLINE
f32 normalize_rad(f32 angle) NO_EXCEPT {
    return angle - OMS_TAU_F32 * oms_floor((angle + OMS_PI_F32) / OMS_TAU_F32);
}

// 0 / 360
FORCE_INLINE
f32 normalize_deg(f32 angle) NO_EXCEPT {
    return angle - 360.0f * oms_floor(angle / 360.0f);
}

// Zero and comparison
#define OMS_EPSILON_F32 1.19209290e-07f
#define OMS_EPSILON_F64 2.2204460492503131e-16

#define OMS_IS_ZERO_F32(x) (oms_abs(x) < OMS_EPSILON_F32)
#define OMS_IS_ZERO_F64(x) (oms_abs(x) < OMS_EPSILON_F64)
#define OMS_FEQUAL_F32(a, b) (oms_abs((a) - (b)) < OMS_EPSILON_F32)
#define OMS_FEQUAL_F64(a, b) (oms_abs((a) - (b)) < OMS_EPSILON_F64)

#define OMS_HAS_ZERO(x) (((x) - ((size_t)-1 / 0xFF)) & ~(x) & (((size_t)-1 / 0xFF) * (0xFF / 2 + 1)))
#define OMS_HAS_CHAR(x, c) (OMS_HAS_ZERO((x) ^ (((size_t)-1 / 0xFF) * (c))))

FORCE_INLINE
bool has_zero(char c) NO_EXCEPT {
    return (c - ((size_t)-1 / 0xFF)) & ~c & (((size_t)-1 / 0xFF) * (0xFF / 2 + 1));
}

#if WCHAR_MAX <= 0xFFFF
    // 2-byte wchar_t
    #define OMS_HAS_ZERO_WCHAR(x) ((((x) - 0x0001000100010001ULL) & ~(x) & 0x8000800080008000ULL) != 0)

    FORCE_INLINE
    bool has_zero(wchar_t c) NO_EXCEPT {
        return ((c - 0x0001000100010001ULL) & ~c & 0x8000800080008000ULL) != 0;
    }
#else
    // 4-byte wchar_t
    #define OMS_HAS_ZERO_WCHAR(x) ((((x) - 0x0000000100000001ULL) & ~(x) & 0x8000000080000000ULL) != 0)

    FORCE_INLINE
    bool has_zero(wchar_t c) NO_EXCEPT {
        return ((c - 0x0000000100000001ULL) & ~c & 0x8000000080000000ULL) != 0;
    }
#endif


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