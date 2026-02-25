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

#include "Defines.h"
#include "Types.h"
#include "../compiler/CompilerUtils.h"

/**
 * @question Consider to pull out some of the helper sections (e.g. create MathHelper.h, TemplateHelper)
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

// Math operations
// Only useful if n is a variable BUT you as programmer know the form of the value
#define OMS_POW2_I64(n) (1ULL << (n))
#define OMS_POW2_I32(n) (1U << (n))
#define OMS_DIV2_I64(n) ((n) >> 1ULL)
#define OMS_DIV2_I32(n) ((n) >> 1U)
#define OMS_MUL2_I64(n) ((n) << 1ULL)
#define OMS_MUL2_I32(n) ((n) << 1U)
#define OMS_IS_POW2(n) ((n & (n - 1)) == 0)

// Zero and comparison
#define OMS_IS_ZERO_F32(x) (abs(x) < OMS_EPSILON_F32)
#define OMS_IS_ZERO_F64(x) (abs(x) < OMS_EPSILON_F64)
#define OMS_FEQUAL_F32(a, b) (abs((a) - (b)) < OMS_EPSILON_F32)
#define OMS_FEQUAL_F64(a, b) (abs((a) - (b)) < OMS_EPSILON_F64)

// Bitwise utilities
#define OMS_SIGN_32(x) (1 | ((x) >> 31 << 1))
#define OMS_SIGN_64(x) (1LL | ((x) >> 63 << 1))

template <typename T>
FORCE_INLINE T align_up(T x, size_t align) NO_EXCEPT
{ return (T) (((x) + ((align) - 1)) & ~((align) - 1)); }

template <typename T>
FORCE_INLINE T align_down(T x, size_t align) NO_EXCEPT
{ return (T)((x) & ~((align) - 1)); }

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
#define OMS_WRAPPED_INCREMENT(value, end) ++value; if (value >= (end)) UNLIKELY value = 0
#define OMS_WRAPPED_DECREMENT(value, end) --value; if (value < 0) UNLIKELY value = (end) - 1

#define OMS_WRAPPED_INC_SE(value, start, end) ++value; if (value >= (end)) UNLIKELY value = (start)
#define OMS_WRAPPED_DEC_SE(value, start, end) --value; if (value < (start)) UNLIKELY value = (end) - 1

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
    int _i = start;                      \
    while (_i++ < end) {

#define iterator_end \
        ++obj;       \
    }}               \

/*
#define SWAP_ENDIAN_16(val) ((((val) << 8) | ((val) >> 8)))
#define SWAP_ENDIAN_32(val) (((val) << 24) | (((val) & 0xFF00) << 8) | (((val) >> 8) & 0xFF00) | ((val) >> 24))
#define SWAP_ENDIAN_64(val) (((val) << 56) | (((val) & 0x000000000000FF00ULL) << 40) | (((val) & 0x0000000000FF0000ULL) << 24) | (((val) & 0x00000000FF000000ULL) << 8) | (((val) & 0x000000FF00000000ULL) >> 8) | (((val) & 0x0000FF0000000000ULL) >> 24) | (((val) & 0x00FF000000000000ULL) >> 40) | ((val) >> 56))
*/

// Automatically perform endian swap if necessary
// If we are on little endian (e.g. Win32) we swap big endian data but not little endian
#if _WIN32 || __LITTLE_ENDIAN__
    #define SWAP_ENDIAN_LITTLE(val) (val)
    #define SWAP_ENDIAN_BIG(val) endian_swap(val)

    #define SWAP_ENDIAN_LITTLE_SELF(val) ((void) 0)
    #define SWAP_ENDIAN_BIG_SELF(val) val = endian_swap(val)
#else
    #define SWAP_ENDIAN_LITTLE(val) endian_swap(val)
    #define SWAP_ENDIAN_BIG(val) (val)

    #define SWAP_ENDIAN_LITTLE_SELF(val) val = endian_swap(val)
    #define SWAP_ENDIAN_BIG_SELF(val) ((void) 0)
#endif

FORCE_INLINE
bool is_little_endian() NO_EXCEPT
{
    uint32 num = 1;
    return ((int32) (*(char *) & num)) == 1;
}

FORCE_INLINE
uint16 endian_swap(uint16 val) NO_EXCEPT
{
    //return ((val << 8) | (val >> 8));
    return SWAP_ENDIAN_16(val);
}

FORCE_INLINE
int16 endian_swap(int16 val) NO_EXCEPT
{
    //return (int16) ((val << 8) | (val >> 8));
    return SWAP_ENDIAN_16(val);
}

FORCE_INLINE
uint32 endian_swap(uint32 val) NO_EXCEPT
{
    /*
    return ((val << 24)
        | ((val & 0xFF00) << 8)
        | ((val >> 8) & 0xFF00)
        | (val >> 24));
    */
   return SWAP_ENDIAN_32(val);
}

FORCE_INLINE
int32 endian_swap(int32 val) NO_EXCEPT
{
    /*
    return (int32) ((val << 24)
        | ((val & 0xFF00) << 8)
        | ((val >> 8) & 0xFF00)
        | (val >> 24));
    */
    return SWAP_ENDIAN_32(val);
}

FORCE_INLINE
uint64 endian_swap(uint64 val) NO_EXCEPT
{
    /*
    return ((val << 56)
        | ((val & 0x000000000000FF00ULL) << 40)
        | ((val & 0x0000000000FF0000ULL) << 24)
        | ((val & 0x00000000FF000000ULL) << 8)
        | ((val & 0x000000FF00000000ULL) >> 8)
        | ((val & 0x0000FF0000000000ULL) >> 24)
        | ((val & 0x00FF000000000000ULL) >> 40)
        | (val >> 56));
    */
    return SWAP_ENDIAN_64(val);
}

FORCE_INLINE
int64 endian_swap(int64 val) NO_EXCEPT
{
    /*
    return (int64) ((val << 56)
        | ((val & 0x000000000000FF00ULL) << 40)
        | ((val & 0x0000000000FF0000ULL) << 24)
        | ((val & 0x00000000FF000000ULL) << 8)
        | ((val & 0x000000FF00000000ULL) >> 8)
        | ((val & 0x0000FF0000000000ULL) >> 24)
        | ((val & 0x00FF000000000000ULL) >> 40)
        | (val >> 56));
    */
    return SWAP_ENDIAN_64(val);
}

FORCE_INLINE
f32 endian_swap(f32 val) NO_EXCEPT
{
    return (f32) BITCAST(endian_swap(BITCAST(val, uint32)), f32);
}

FORCE_INLINE
f64 endian_swap(f64 val) NO_EXCEPT
{
    return (f64) BITCAST(endian_swap(BITCAST(val, uint64)), f64);
}

/**
 * The following read and write functions are often used for writing data directly to/from a byte buffer.
 * Such a byte buffer is often directly stored in files that use binary formats
 * You might think that you could just do reads/writes like this *((int32 *) buffer) = int_value
 * However, that is undefined if the buffer at that position isn't correctly aligned and only memcpy handles that correctly
 * If we are sure that the buffer is aligned we could of course use the above mentioned method which should be faster
 */
FORCE_INLINE
byte* write_le(byte* p, uint32 v) NO_EXCEPT
{
    SWAP_ENDIAN_LITTLE_SELF(v);
    memcpy(p, &v, sizeof(v));

    return p + sizeof(v);
}

FORCE_INLINE
byte* write_le(byte* p, int32 v) NO_EXCEPT
{
    return write_le(p, (uint32)v);
}

FORCE_INLINE
byte* write_le(byte* p, uint64 v) NO_EXCEPT
{
    SWAP_ENDIAN_LITTLE_SELF(v);
    memcpy(p, &v, sizeof(v));

    return p + sizeof(v);
}

FORCE_INLINE
byte* write_le(byte* p, int64 v) NO_EXCEPT
{
    return write_le(p, (uint64)v);
}

FORCE_INLINE
byte* write_le(byte* p, f32 v) NO_EXCEPT
{
    uint32 bits;
    memcpy(&bits, &v, sizeof(bits));

    return write_le(p, bits);
}

FORCE_INLINE
byte* write_le(byte* p, f64 v) NO_EXCEPT
{
    uint64 bits;
    memcpy(&bits, &v, sizeof(bits));

    return write_le(p, bits);
}

FORCE_INLINE
const byte* read_le(const byte* __restrict p, uint32* __restrict out) NO_EXCEPT
{
    // @question Why do I even need v? Just use out directly
    uint32 v;
    memcpy(&v, p, sizeof(v));
    *out = SWAP_ENDIAN_LITTLE(v);

    return p + sizeof(v);
}

FORCE_INLINE
const byte* read_le(const byte* __restrict p, int32* __restrict out) NO_EXCEPT
{
    uint32 v;
    p = read_le(p, &v);
    *out = (int32)v;

    return p;
}

FORCE_INLINE
const byte* read_le(const byte* __restrict p, uint64* __restrict out) NO_EXCEPT
{
    uint64 v;
    memcpy(&v, p, sizeof(v));
    *out = SWAP_ENDIAN_LITTLE(v);

    return p + sizeof(v);
}

FORCE_INLINE
const byte* read_le(const byte* __restrict p, int64* __restrict out) NO_EXCEPT
{
    uint64 v;
    p = read_le(p, &v);
    *out = (int64)v;

    return p;
}

FORCE_INLINE
const byte* read_le(const byte* __restrict p, f32* __restrict out) NO_EXCEPT
{
    uint32 bits;
    p = read_le(p, &bits);
    memcpy(out, &bits, sizeof(bits));

    return p;
}

FORCE_INLINE
const byte* read_le(const byte* __restrict p, f64* __restrict out) NO_EXCEPT
{
    uint64 bits;
    p = read_le(p, &bits);
    memcpy(out, &bits, sizeof(bits));

    return p;
}

template <bool B, typename T = void>
struct enable_if { };

template <typename T>
struct enable_if<true, T> { using type = T; };

template <bool B, typename T = void>
using enable_if_t = typename enable_if<B, T>::type;

template <typename From, typename To>
struct is_convertible {
private:
    static char test(To);
    static int  test(...);
    static From make();
public:
    static constexpr bool value = sizeof(test(make())) == sizeof(char);
};

template <typename From, typename To>
constexpr bool is_convertible_v = is_convertible<From, To>::value;

#endif