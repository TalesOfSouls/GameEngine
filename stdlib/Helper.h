/**
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_STDLIB_HELPER_H
#define COMS_STDLIB_HELPER_H

#include "Defines.h"
#include "Types.h"
#include "../compiler/CompilerUtils.h"

// Counts the elements in an array IFF its size is defined at compile time
#define ARRAY_COUNT(a) ((a) == NULL ? 0 : (sizeof(a) / sizeof((a)[0])))

// Gets the size of a struct member
#define MEMBER_SIZEOF(type, member) (sizeof(((type *)0)->member))
#define MEMBER_ARRAY_COUNT(type, member) ARRAY_COUNT(((type *)0)->member)

// Older c++ version don't allow member variable assignment using ".name = value" style
// Why use it at all? Well because it helps spotting bugs if the order changes in a struct
#if CPP_VERSION >= 17
    #define SMN(name) .name =
#else
    #define SMN(name)
#endif

#define MEMORY_OFFSET(high, low) (size_t) ((uintptr_t) (high) - (uintptr_t) (low))
#define MEMORY_ELEMENT_ZERO(ptr) memset(ptr, 0, sizeof(*ptr))

template <typename T, size_t N>
CONSTEXPR int32_t array_count_helper(const T (&)[N]) {
    return static_cast<int32_t>(N);
}

#define ARRAY_COUNT_MEMBER(type, member) array_count_helper(((type*)0)->member)

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
FORCE_INLINE CONSTEXPR T align_up(T x, size_t align) NO_EXCEPT
{ return (T) (((x) + ((align) - 1)) & ~((align) - 1)); }

template <typename T>
FORCE_INLINE CONSTEXPR T align_down(T x, size_t align) NO_EXCEPT
{ return (T)((x) & ~((align) - 1)); }

#define OMS_IS_ALIGNED(x, align) (((x) & ((align) - 1)) == 0)

// Uses value for bit instead of position
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

// Uses index/position to set
#define OMS_BIT_SET(flag, pos) ((flag) | (1 << (pos)))
#define OMS_BIT_IS_SET(flag, pos) ((flag) & (1 << (pos)))

// Works on arrays of where the element size is sizeof(size_t) = usually 8 bytes
#define OMS_BITARRAY_SET(flags, pos) ((flags)[OMS_BIT_WORD_INDEX(pos)] | ((size_t)1 << OMS_BIT_INDEX(pos)))
#define OMS_BITARRAY_CLEAR(flags, pos) ((flags)[OMS_BIT_WORD_INDEX(pos)] & ~((size_t)1 << OMS_BIT_INDEX(pos)))
#define OMS_BITARRAY_REMOVE(flags, pos) OMS_BITARRAY_CLEAR(flags, pos)
#define OMS_BITARRAY_DELETE(flags, pos) OMS_BITARRAY_CLEAR(flags, pos)
#define OMS_BITARRAY_TOGGLE(flags, pos) ((flags)[OMS_BIT_WORD_INDEX(pos)] ^ ((size_t)1 << OMS_BIT_INDEX(pos)))
#define OMS_BITARRAY_FLIP(flags, pos) OMS_BITARRAY_TOGGLE(flags, pos)
#define OMS_BITARRAY_CHECK(flags, pos) (((flags)[OMS_BIT_WORD_INDEX(pos)] >> OMS_BIT_INDEX(pos)) & (size_t)1)
#define OMS_BITARRAY_SET_TO(flags, pos, value) {                                                              \
        if (value)                                                    \
            (flags)[OMS_BIT_WORD_INDEX(pos)] |=                        \
                ((size_t)1 << OMS_BIT_INDEX(pos));                     \
        else                                                          \
            (flags)[OMS_BIT_WORD_INDEX(pos)] &=                        \
                ~((size_t)1 << OMS_BIT_INDEX(pos));                    \
    }

// Usually these specific implementation R2L or L2R are only used if you are handling a specific format
// e.g. file format or algorithm (e.g. Huffman encoding)
// If you are only doing bit manipulation of your own format or data that only exists during runtime
// feel free to use the OMS_BIT and OMS_FLAG versions defined in Types.h

// Left to right (big endian)
// "bits" refers to the bits of the data type (e.g. 8, 16, 32, 64)
#define IS_BIT_SET_L2R(num, pos, bits) ((bool) ((num) & (OMS_UINT_ONE << ((bits - 1) - (pos)))))
#define BIT_SET_L2R(num, pos, bits) ((num) | (OMS_UINT_ONE << ((bits - 1) - (pos))))

#define IS_BIT_SET_64_L2R(num, pos, bits) ((bool) ((num) & (1ULL << ((bits - 1) - (pos)))))
#define BIT_SET_64_L2R(num, pos, bits) ((num) | (1ULL << ((bits - 1) - (pos))))
#define BIT_UNSET_64_L2R(num, pos, bits) ((num) & ~(1ULL << ((bits - 1) - (pos))))
#define BIT_FLIP_64_L2R(num, pos, bits) ((num) ^ (1ULL << ((bits - 1) - (pos))))
#define BIT_SET_TO_64_L2R(num, pos, x, bits) (((num) & ~(1ULL << ((bits) - 1 - (pos)))) | (((uint64_t)(x) & 1ULL) << ((bits) - 1 - (pos))))

#define IS_BIT_SET_32_L2R(num, pos, bits) ((bool) ((num) & (1 << ((bits - 1) - (pos)))))
#define BIT_SET_32_L2R(num, pos, bits) ((num) | (1U << ((bits - 1) - (pos))))
#define BIT_UNSET_L2R(num, pos, bits) ((num) & ~(1U << ((bits - 1) - (pos))))
#define BIT_FLIP_L2R(num, pos, bits) ((num) ^ (1U << ((bits - 1) - (pos))))
#define BIT_SET_TO_L2R(num, pos, x, bits) (((num) & ~(1U << ((bits) - 1 - (pos)))) | (((uint32_t)(x) & 1U) << ((bits) - 1 - (pos))))

#define BITS_GET_8_L2R(num, pos, to_read) (((num) >> (8 - (pos) - (to_read))) & ((1U << (to_read)) - 1))
#define BITS_GET_16_L2R(num, pos, to_read) (((num) >> (16 - (pos) - (to_read))) & ((1U << (to_read)) - 1))
#define BITS_GET_32_L2R(num, pos, to_read) (((num) >> (32 - (pos) - (to_read))) & ((1U << (to_read)) - 1))
#define BITS_GET_64_L2R(num, pos, to_read) (((num) >> (64ULL - (pos) - (to_read))) & ((1ULL << (to_read)) - 1))

// Merges an array of bytes as an int value (16bit, 32bit, 64bit)
// The implementation is endian aware
#if _WIN32 || __LITTLE_ENDIAN__
    #define BYTES_MERGE_2_L2R(arr) (((arr)[0] << 8) | (arr)[1])
    #define BYTES_MERGE_4_L2R(arr) (((arr)[0] << 24) | ((arr)[1] << 16) | ((arr)[2] << 8) | (arr)[3])
    #define BYTES_MERGE_8_L2R(arr) (((uint64_t)(arr)[0] << 56) | ((uint64_t)(arr)[1] << 48) | ((uint64_t)(arr)[2] << 40) | ((uint64_t)(arr)[3] << 32) | ((uint64_t)(arr)[4] << 24) | ((uint64_t)(arr)[5] << 16) | ((uint64_t)(arr)[6] << 8)  | ((uint64_t)(arr)[7]))
#else
    #define BYTES_MERGE_2_L2R(arr) (*(const uint16_t *)(arr))
    #define BYTES_MERGE_4_L2R(arr) (*(const uint32_t *)(arr))
    #define BYTES_MERGE_8_L2R(arr) (*(const uint64_t *)(arr))
#endif

// The R2L version is basically the same as: OMS_FLAG_ and OMS_BIT_ defined in Types
// Right to left (little endian)
#define IS_BIT_SET_R2L(num, pos) ((bool) ((num) & (OMS_UINT_ONE << (pos))))
#define BIT_SET_R2L(num, pos) ((num) | (OMS_UINT_ONE << (pos)))

#define IS_BIT_SET_64_R2L(num, pos) ((bool) ((num) & (1ULL << (pos))))
#define BIT_SET_64_R2L(num, pos) ((num) | (1ULL << (pos)))
#define BIT_UNSET_64_R2L(num, pos) ((num) & ~(1ULL << (pos)))
#define BIT_FLIP_64_R2L(num, pos) ((num) ^ (1ULL << (pos)))
#define BIT_SET_TO_64_R2L(num, pos, x) (((num) & ~(1ULL << (pos))) | ((uint64_t)(x) << (pos)))

#define IS_BIT_SET_32_R2L(num, pos) ((bool) ((num) & (1 << (pos))))
#define BIT_SET_32_R2L(num, pos) ((num) | (1U << (pos)))
#define BIT_UNSET_R2L(num, pos) ((num) & ~(1U << (pos)))
#define BIT_FLIP_R2L(num, pos) ((num) ^ (1U << (pos)))
#define BIT_SET_TO_R2L(num, pos, x) (((num) & ~(1U << (pos))) | ((uint32_t)(x) << (pos)))

#define BITS_GET_8_R2L(num, pos, to_read) (((num) >> (pos)) & ((1U << (to_read)) - 1))
#define BITS_GET_16_R2L(num, pos, to_read) (((num) >> (pos)) & ((1U << (to_read)) - 1))
#define BITS_GET_32_R2L(num, pos, to_read) (((num) >> (pos)) & ((1U << (to_read)) - 1))
#define BITS_GET_64_R2L(num, pos, to_read) (((num) >> (pos)) & ((1ULL << (to_read)) - 1))

// Merges an array of bytes as an int value (16bit, 32bit, 64bit)
// The implementation is endian aware
#if _WIN32 || __LITTLE_ENDIAN__
    #define BYTES_MERGE_2_R2L(arr) (*(const uint16_t *)(arr))
    #define BYTES_MERGE_4_R2L(arr) (*(const uint32_t *)(arr))
    #define BYTES_MERGE_8_R2L(arr) (*(const uint64_t *)(arr))
#else
    #define BYTES_MERGE_2_R2L(arr) (((arr)[1] << 8) | (arr)[0])
    #define BYTES_MERGE_4_R2L(arr) (((arr)[3] << 24) | ((arr)[2] << 16) | ((arr)[1] << 8) | (arr)[0])
    #define BYTES_MERGE_8_R2L(arr) (((uint64_t)(arr)[7] << 56) | ((uint64_t)(arr)[6] << 48) | ((uint64_t)(arr)[5] << 40) | ((uint64_t)(arr)[4] << 32) | ((uint64_t)(arr)[3] << 24) | ((uint64_t)(arr)[2] << 16) | ((uint64_t)(arr)[1] << 8)  | ((uint64_t)(arr)[0]))
#endif

#define OMS_HAS_ALPHA(color) (color & 0xFF)

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
template<typename To, typename From>
static inline To bitcast(From src)
{
    IF_CONSTEXPR(sizeof(To) == sizeof(From)) {
        union {
            From src;
            To dst;
        } u;

        u.src = src;
        return u.dst;
    } else {
        To dst = {0};
        memcpy(&dst, &src, sizeof(From) > sizeof(To) ? sizeof(To) : sizeof(From));

        return dst;
    }
}
#define BITCAST(x, new_type) bitcast<new_type>(x)

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

FORCE_INLINE CONSTEXPR
bool is_little_endian() NO_EXCEPT
{
    uint32 num = 1;
    return ((int32) (*(char *) & num)) == 1;
}

FORCE_INLINE CONSTEXPR_DOGSHIT
uint16 endian_swap(uint16 val) NO_EXCEPT
{
    //return ((val << 8) | (val >> 8));
    return SWAP_ENDIAN_16(val);
}

FORCE_INLINE CONSTEXPR_DOGSHIT
int16 endian_swap(int16 val) NO_EXCEPT
{
    //return (int16) ((val << 8) | (val >> 8));
    return SWAP_ENDIAN_16(val);
}

FORCE_INLINE CONSTEXPR_DOGSHIT
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

FORCE_INLINE CONSTEXPR_DOGSHIT
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

FORCE_INLINE CONSTEXPR_DOGSHIT
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

FORCE_INLINE CONSTEXPR_DOGSHIT
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

FORCE_INLINE CONSTEXPR_DOGSHIT
f32 endian_swap(f32 val) NO_EXCEPT
{
    return (f32) BITCAST(endian_swap(BITCAST(val, uint32)), f32);
}

FORCE_INLINE CONSTEXPR_DOGSHIT
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

// Used to hash strings used in macros (e.g. __FILE__)
CONSTEXPR
int macro_fnv1a_32(const char* str)
{
    uint32_t hash = 2166136261u;

    while (*str) {
        hash ^= (uint8_t) (*str++);
        hash *= 16777619u;
    }

    return (int) hash;
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

// Used to check if two types are the same
template<typename T, typename U>
struct is_same
{
    enum { value = 0 };
};

template<typename T>
struct is_same<T, T>
{
    enum { value = 1 };
};

#endif