/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_STDLIB_INTERNAL_H
#define COMS_STDLIB_INTERNAL_H

#include "Types.h"
#include "../compiler/CompilerUtils.h"

#define OMS_HAS_ZERO(x) (((x) - ((size_t)-1 / 0xFF)) & ~(x) & (((size_t)-1 / 0xFF) * (0xFF / 2 + 1)))
#define OMS_HAS_CHAR(x, c) (OMS_HAS_ZERO((x) ^ (((size_t)-1 / 0xFF) * (c))))

FORCE_INLINE
bool __internal_has_zero(char c) NO_EXCEPT {
    return (c - ((size_t)-1 / 0xFF)) & ~c & (((size_t)-1 / 0xFF) * (0xFF / 2 + 1));
}

#if WCHAR_MAX <= 0xFFFF
    // 2-byte wchar_t
    #define OMS_HAS_ZERO_WCHAR(x) ((((x) - 0x0001000100010001ULL) & ~(x) & 0x8000800080008000ULL) != 0)

    FORCE_INLINE
    bool __internal_has_zero(wchar_t c) NO_EXCEPT {
        return ((c - 0x0001000100010001ULL) & ~c & 0x8000800080008000ULL) != 0;
    }
#else
    // 4-byte wchar_t
    #define OMS_HAS_ZERO_WCHAR(x) ((((x) - 0x0000000100000001ULL) & ~(x) & 0x8000000080000000ULL) != 0)

    FORCE_INLINE
    bool __internal_has_zero(wchar_t c) NO_EXCEPT {
        return ((c - 0x0000000100000001ULL) & ~c & 0x8000000080000000ULL) != 0;
    }
#endif

inline
void __internal_memmove(void* dst, const void* src, size_t len)
{
    unsigned char* d = (unsigned char*)dst;
    const unsigned char* s = (const unsigned char*)src;

    if (d == s || len == 0) {
        return;
    }

    if (d < s) {
        // Forward copy

        // Align destination
        while (len && ((uintptr_t)d & (sizeof(uintptr_t) - 1))) {
            *d++ = *s++;
            --len;
        }

        // Word copy
        uintptr_t* dw = (uintptr_t*)d;
        const uintptr_t* sw = (const uintptr_t*)s;

        while (len >= sizeof(uintptr_t)) {
            *dw++ = *sw++;
            len -= sizeof(uintptr_t);
        }

        // Handle end
        d = (unsigned char*)dw;
        s = (const unsigned char*)sw;
        while (len--) {
            *d++ = *s++;
        }
    } else {
        // Backward copy

        d += len;
        s += len;

        // Align destination end
        while (len && ((uintptr_t)d & (sizeof(uintptr_t) - 1))) {
            --d;
            --s;
            *d = *s;
            --len;
        }

        // Word copy
        uintptr_t* dw = (uintptr_t*)d;
        const uintptr_t* sw = (const uintptr_t*)s;

        while (len >= sizeof(uintptr_t)) {
            --dw;
            --sw;
            *dw = *sw;
            len -= sizeof(uintptr_t);
        }

        // Handle end
        d = (unsigned char*)dw;
        s = (const unsigned char*)sw;
        while (len--) {
            --d;
            --s;
            *d = *s;
        }
    }
}

inline
void __internal_memcpy(void* dst, const void* src, size_t len)
{
    unsigned char* d = (unsigned char*)dst;
    const unsigned char* s = (const unsigned char*)src;

    // Align destination
    while (len && ((uintptr_t)d & (sizeof(uintptr_t) - 1))) {
        *d++ = *s++;
        --len;
    }

    // Word copy
    uintptr_t* dw = (uintptr_t*)d;
    const uintptr_t* sw = (const uintptr_t*)s;

    while (len >= sizeof(uintptr_t)) {
        *dw++ = *sw++;
        len -= sizeof(uintptr_t);
    }

    // Handle end
    d = (unsigned char*)dw;
    s = (const unsigned char*)sw;

    while (len--) {
        *d++ = *s++;
    }
}

inline
void __internal_memset(void* dst, int value, size_t len)
{
    unsigned char* d = (unsigned char*)dst;
    unsigned char v = (unsigned char)value;

    // Align destination
    while (len && ((uintptr_t)d & (sizeof(uintptr_t) - 1))) {
        *d++ = v;
        --len;
    }

    // Build repeated word
    uintptr_t word = v;
    word |= word << 8;
    word |= word << 16;
    #if UINTPTR_MAX > 0xFFFFFFFF
        word |= word << 32;
    #endif

    // Word fill
    uintptr_t* dw = (uintptr_t*)d;
    while (len >= sizeof(uintptr_t)) {
        *dw++ = word;
        len -= sizeof(uintptr_t);
    }

    // Handle end
    d = (unsigned char*)dw;
    while (len--) {
        *d++ = v;
    }
}

inline CONSTEXPR
size_t __internal_strlen(const char* const str)
{
    const char* ptr = str;

    while (((uintptr_t) ptr % sizeof(size_t)) != 0) {
        if (*ptr == '\0') {
            return ptr - str;
        }

        ++ptr;
    }

    const size_t* longword_ptr = (const size_t *) ptr;

    while (true) {
        const size_t v = *longword_ptr;

        if (OMS_HAS_ZERO(v)) {
            const char* cp = (const char *)longword_ptr;
            for (size_t i = 0; i < sizeof(size_t); ++i) {
                if (cp[i] == '\0') {
                    return cp + i - str;
                }
            }
        }

        ++longword_ptr;
    }
}

inline CONSTEXPR
size_t __internal_wcslen(const wchar_t* const str)
{
    const wchar_t* ptr = str;

    while (((uintptr_t)ptr % sizeof(size_t)) != 0) {
        if (*ptr == L'\0') {
            return ptr - str;
        }

        ++ptr;
    }

    const size_t* longword_ptr = (const size_t*) ptr;

    while (true) {
        const size_t v = *longword_ptr;

        if (OMS_HAS_ZERO_WCHAR(v)) {
            const wchar_t* cp = (const wchar_t*) longword_ptr;
            for (size_t i = 0; i < sizeof(size_t) / sizeof(wchar_t); ++i) {
                if (cp[i] == L'\0') {
                    return cp + i - str;
                }
            }
        }

        ++longword_ptr;
    }
}

inline
const char* __internal_strcpy(char* __restrict dest, const char* __restrict src)
{
    const char* start = dest;

    // Align src pointer
    while ((uintptr_t)src % sizeof(size_t) != 0) {
        const char c = *src++;
        *dest++ = c;
        if (c == '\0') {
            return NULL;
        }
    }

    const size_t* wptr = (const size_t *) src;
    size_t* dptr = (size_t *)dest;

    while (true) {
        const size_t v = *wptr;
        if (OMS_HAS_ZERO(v)) {
            break; // null byte found
        }

        *dptr++ = v;
        wptr++;
    }

    // Copy remaining bytes
    src = (const char *) wptr;
    dest = (char *) dptr;
    while (true) {
        const char c = *src++;
        *dest++ = c;
        if (c == '\0') {
            break;
        }
    }

    return start;
}

inline
const wchar_t* __internal_wcscpy(
    wchar_t* __restrict dest,
    const wchar_t* __restrict src
)
{
    const wchar_t* start = dest;

    // Align src pointer
    while ((uintptr_t)src % sizeof(size_t) != 0) {
        const wchar_t c = *src++;
        *dest++ = c;
        if (c == L'\0') {
            return NULL;
        }
    }

    const size_t* wptr = (const size_t*)src;
    size_t* dptr = (size_t*)dest;

    // Bulk copy size_t blocks
    while (true) {
        const size_t v = *wptr;
        if (OMS_HAS_ZERO_WCHAR(v)) {
            break;
        }

        *dptr++ = v;
        ++wptr;
    }

    // Copy remaining bytes
    src = (const wchar_t*)wptr;
    dest = (wchar_t*)dptr;
    while (true) {
        const wchar_t c = *src++;
        *dest++ = c;
        if (c == L'\0') {
            break;
        }
    }

    return start;
}

inline void
__internal_strcat(char* __restrict dst, const char* __restrict src)
{
    dst += __internal_strlen(dst);
    __internal_strcpy(dst, src);
}

inline void
__internal_wcscat(wchar_t* __restrict dst, const wchar_t* __restrict src)
{
    dst += __internal_wcslen(dst);
    __internal_wcscpy(dst, src);
}

inline
char* __internal_strstr(const char* __restrict haystack, const char* __restrict needle)
{
    const char first = needle[0];
    const size_t needle_len = __internal_strlen(needle);
    const char* ptr = haystack;

    while (*ptr != '\0') {
        // Align pointer for size_t access
        while ((uintptr_t) ptr % sizeof(size_t) != 0 && *ptr != '\0') {
            if (*ptr == first) {
                break;
            }

            ++ptr;
        }

        if (*ptr == '\0') {
            break;
        }

        // Word-wise scanning
        const size_t* wptr = (const size_t *) ptr;
        const size_t first_mask = ((size_t) -1 / 0xFF) * first;

        while (true) {
            const size_t v = *wptr;
            if (((v - ((size_t) - 1 / 0xFF)) & ~v & (((size_t) - 1 / 0xFF) * 0x80))
                || (((v ^ first_mask) - ((size_t) - 1 / 0xFF)) & ~(v ^ first_mask) & (((size_t) - 1 / 0xFF) * 0x80))
            ) {
                break;
            }

            ++wptr;
        }

        ptr = (const char *) wptr;

        const char* p1 = ptr;
        size_t i = 0;
        while (i < needle_len && p1[i] != '\0' && p1[i] == needle[i]) {
            ++i;
        }

        if (i == needle_len) {
            return (char *) ptr;
        }

        ++ptr;
    }

    return NULL;
}

char* __internal_strchr(const char* str, char needle) NO_EXCEPT
{
    const byte target = (const byte) needle;

    // Process byte-by-byte until alignment is achieved
    for (; (uintptr_t) str % sizeof(size_t) != 0; ++str) {
        if (*str == target) {
            return (char *) str;
        }

        if (*str == '\0') {
            return NULL;
        }
    }

    // Broadcast the target character to all bytes of a word
    size_t target_word = target;
    for (size_t i = 1; i < sizeof(size_t); ++i) {
        target_word |= target_word << 8;
    }

    const size_t* word_ptr = (const size_t *) str;
    while (true) {
        size_t word = *word_ptr++;
        if (OMS_HAS_CHAR(word, target)) {
            const char* byte_ptr = (const char *) (word_ptr - 1);
            for (size_t i = 0; i < sizeof(size_t); ++i) {
                if (byte_ptr[i] == target) {
                    return (char *) (byte_ptr + i);
                }
            }
        }

        if (OMS_HAS_ZERO(word)) {
            const char* byte_ptr = (const char *) (word_ptr - 1);
            for (size_t i = 0; i < sizeof(size_t); ++i) {
                if (byte_ptr[i] == '\0') {
                    return NULL;
                }
            }
        }
    }
}

inline
wchar_t* __internal_wcsstr(
    const wchar_t* __restrict haystack,
    const wchar_t* __restrict needle
)
{
    const wchar_t first = needle[0];
    const size_t needle_len = __internal_wcslen(needle);
    const wchar_t* ptr = haystack;

    if (needle_len == 0) {
        return NULL;
    }

    constexpr size_t W = sizeof(wchar_t);
    constexpr size_t S = sizeof(size_t);
    constexpr size_t N = S / W;

    size_t first_mask = 0;
    for (size_t i = 0; i < N; ++i) {
        first_mask |= (size_t)first << (i * 8 * W);
    }

    while (*ptr != L'\0') {
        // Align pointer
        while ((uintptr_t)ptr % S != 0 && *ptr != L'\0') {
            if (*ptr == first) {
                break;
            }
            ++ptr;
        }

        if (*ptr == L'\0') {
            break;
        }

        // Word-wise scan
        const size_t* wptr = (const size_t*)ptr;

        while (true) {
            const size_t v = *wptr;
            if (OMS_HAS_ZERO_WCHAR(v) || OMS_HAS_ZERO_WCHAR(v ^ first_mask)) {
                break;
            }
            ++wptr;
        }

        ptr = (const wchar_t*)wptr;
        for (size_t i = 0; i < N; ++i) {
            if (ptr[i] == L'\0') {
                return NULL;
            }
            if (ptr[i] == first) {
                ptr += i;
                break;
            }
        }

        const wchar_t* p1 = ptr;
        size_t i = 0;
        while (i < needle_len && p1[i] != L'\0' && p1[i] == needle[i]) {
            ++i;
        }

        if (i == needle_len) {
            return (wchar_t *) ptr;
        }

        ++ptr;
    }

    return NULL;
}

inline CONSTEXPR
void __internal_strncpy(char* __restrict dest, const char* __restrict src, int32 length) NO_EXCEPT
{
    int32 copied = 0;

    // Align src pointer to size_t boundary
    while ((uintptr_t)src % sizeof(size_t) != 0 && copied < length - 1) {
        const char c = *src++;
        *dest++ = c;
        ++copied;

        if (c == '\0') {
            return;
        }
    }

    const size_t* wptr = (const size_t *) src;
    size_t* dptr = (size_t *) dest;

    while (copied + (int32) sizeof(size_t) < length) {
        const size_t v = *wptr;
        if (OMS_HAS_ZERO(v)) {
            break;
        }

        *dptr++ = v;
        ++wptr;
        copied += sizeof(size_t);
    }

    // Copy remaining bytes one by one
    src = (const char *) wptr;
    dest = (char *) dptr;
    while (copied < length - 1) {
        const char c = *src++;
        *dest++ = c;
        ++copied;

        if (c == '\0') {
            break;
        }
    }

    *dest = '\0';
}

inline CONSTEXPR
void __internal_wcsncpy(
    wchar_t* __restrict dest,
    const wchar_t* __restrict src,
    int32 length
) NO_EXCEPT
{
    int32 copied = 0;

    // Align src pointer to size_t boundary
    while ((uintptr_t)src % sizeof(size_t) != 0 && copied < length - 1) {
        const wchar_t c = *src++;
        *dest++ = c;
        ++copied;

        if (c == L'\0') {
            return;
        }
    }

    const size_t* wptr = (const size_t*)src;
    size_t* dptr = (size_t*)dest;

    // Bulk copy size_t blocks
    while (copied + (int32) sizeof(size_t) / sizeof(wchar_t) < length) {
        const size_t v = *wptr;
        if (OMS_HAS_ZERO_WCHAR(v)) {
            break;
        }

        *dptr++ = v;
        ++wptr;

        copied += sizeof(size_t) / sizeof(wchar_t);
    }

    // Tail copy wchar_t-by-wchar_t
    src = (const wchar_t*)wptr;
    dest = (wchar_t*)dptr;

    while (copied < length - 1) {
        const wchar_t c = *src++;
        *dest++ = c;
        ++copied;

        if (c == L'\0') {
            break;
        }
    }

    *dest = L'\0';
}

inline CONSTEXPR
int32 __internal_strcmp(const char* str1, const char* str2) NO_EXCEPT
{
    byte c1 INITIALIZER;
    byte c2 INITIALIZER;

    do {
        c1 = (byte) *str1++;
        c2 = (byte) *str2++;
    } while (c1 == c2 && c1 != '\0');

    return c1 - c2;
}

inline CONSTEXPR
int32 __internal_wcscmp(const wchar_t* str1, const wchar_t* str2) NO_EXCEPT
{
    wchar_t c1 INITIALIZER;
    wchar_t c2 INITIALIZER;

    do {
        c1 = *str1++;
        c2 = *str2++;
    } while (c1 == c2 && c1 != L'\0');

    return c1 - c2;
}

CONSTEXPR
int32 __internal_strncmp(const char* str1, const char* str2, size_t n) NO_EXCEPT
{
    byte c1 = '\0';
    byte c2 = '\0';

    if (n >= 4) {
        size_t n4 = n >> 2;

        do {
            c1 = (byte) *str1++;
            c2 = (byte) *str2++;

            if (c1 == '\0' || c1 != c2) {
                return c1 - c2;
            }

            c1 = (byte) *str1++;
            c2 = (byte) *str2++;

            if (c1 == '\0' || c1 != c2) {
                return c1 - c2;
            }

            c1 = (byte) *str1++;
            c2 = (byte) *str2++;

            if (c1 == '\0' || c1 != c2) {
                return c1 - c2;
            }

            c1 = (byte) *str1++;
            c2 = (byte) *str2++;

            if (c1 == '\0' || c1 != c2) {
                return c1 - c2;
            }
        } while (--n4 > 0);

        n &= 3;
    }

    while (n > 0) {
        c1 = (byte) *str1++;
        c2 = (byte) *str2++;

        if (c1 == '\0' || c1 != c2) {
            return c1 - c2;
        }

        --n;
    }

    return c1 - c2;
}

static CONSTEXPR const unsigned char TO_LOWER_TABLE[256] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F,
    0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F,
    0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F,
    0x40, 'a',  'b',  'c',  'd',  'e',  'f',  'g',  'h',  'i',  'j',  'k',  'l',  'm',  'n',  'o',
    'p',  'q',  'r',  's',  't',  'u',  'v',  'w',  'x',  'y',  'z',  0x5B, 0x5C, 0x5D, 0x5E, 0x5F,
    0x60, 'a',  'b',  'c',  'd',  'e',  'f',  'g',  'h',  'i',  'j',  'k',  'l',  'm',  'n',  'o',
    'p',  'q',  'r',  's',  't',  'u',  'v',  'w',  'x',  'y',  'z',  0x7B, 0x7C, 0x7D, 0x7E, 0x7F,
    0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8A, 0x8B, 0x8C, 0x8D, 0x8E, 0x8F,
    0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9A, 0x9B, 0x9C, 0x9D, 0x9E, 0x9F,
    0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xAB, 0xAC, 0xAD, 0xAE, 0xAF,
    0xB0, 0xB1, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB7, 0xB8, 0xB9, 0xBA, 0xBB, 0xBC, 0xBD, 0xBE, 0xBF,
    0xC0, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xCB, 0xCC, 0xCD, 0xCE, 0xCF,
    0xD0, 0xD1, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xDA, 0xDB, 0xDC, 0xDD, 0xDE, 0xDF,
    0xE0, 0xE1, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9, 0xEA, 0xEB, 0xEC, 0xED, 0xEE, 0xEF,
    0xF0, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF
};

static CONSTEXPR const unsigned char TO_UPPER_TABLE[256] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F,
    0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F,
    0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F,
    0x40, 'A',  'B',  'C',  'D',  'E',  'F',  'G',  'H',  'I',  'J',  'K',  'L',  'M',  'N',  'O',
    'P',  'Q',  'R',  'S',  'T',  'U',  'V',  'W',  'X',  'Y',  'Z',  0x5B, 0x5C, 0x5D, 0x5E, 0x5F,
    0x60, 'A',  'B',  'C',  'D',  'E',  'F',  'G',  'H',  'I',  'J',  'K',  'L',  'M',  'N',  'O',
    'P',  'Q',  'R',  'S',  'T',  'U',  'V',  'W',  'X',  'Y',  'Z',  0x7B, 0x7C, 0x7D, 0x7E, 0x7F,
    0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8A, 0x8B, 0x8C, 0x8D, 0x8E, 0x8F,
    0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9A, 0x9B, 0x9C, 0x9D, 0x9E, 0x9F,
    0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xAB, 0xAC, 0xAD, 0xAE, 0xAF,
    0xB0, 0xB1, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB7, 0xB8, 0xB9, 0xBA, 0xBB, 0xBC, 0xBD, 0xBE, 0xBF,
    0xC0, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xCB, 0xCC, 0xCD, 0xCE, 0xCF,
    0xD0, 0xD1, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xDA, 0xDB, 0xDC, 0xDD, 0xDE, 0xDF,
    0xE0, 0xE1, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9, 0xEA, 0xEB, 0xEC, 0xED, 0xEE, 0xEF,
    0xF0, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF
};


FORCE_INLINE CONSTEXPR
char __internal_toupper(char c) NO_EXCEPT
{
    return c - 32 * (c >= 'a' && c <= 'z');
}

FORCE_INLINE
void __internal__toupper(char* str) NO_EXCEPT
{
    while (*str != '\0') {
        *str -= 32 * (*str >= 'a' && *str <= 'z');
        ++str;
    }
}

FORCE_INLINE CONSTEXPR
char __internal_tolower(char c) NO_EXCEPT
{
    return c + 32 * (c >= 'A' && c <= 'Z');
}

inline
void __internal_tolower(char* str) NO_EXCEPT
{
    while (*str != '\0') {
        *str += 32 * (*str >= 'A' && *str <= 'Z');
        ++str;
    }
}

static CONSTEXPR const bool STR_IS_NUM_LOOKUP_TABLE[] = {
    false, false, false, false, false, false, false, false, // 0-7
    false, false, false, false, false, false, false, false, // 8-15
    false, false, false, false, false, false, false, false, // 16-23
    false, false, false, false, false, false, false, false, // 24-31
    false, false, false, false, false, false, false, false, // 32-39
    false, false, false, false, false, false, false, false, // 40-47
    true,  true,  true,  true,  true,  true,  true,  true,  // 48-55 ('0'-'7')
    true,  true,  false, false, false, false, false, false, // 56-63 ('8'-'9', others)
    false, true,  false, false, false, false, false, false, // 64-71 ('A'-'G')
    false, false, false, false, false, false, false, false, // 72-79 ('H'-'O')
    false, false, false, false, false, false, false, false, // 80-87 ('P'-'W')
    false, false, false, false, false, false, false, false, // 88-95 ('X'-'Z', others)
    false, false, false, false, false, false, false, false, // 96-103 ('a'-'g')
    false, false, false, false, false, false, false, false, // 104-111 ('h'-'o')
    false, false, false, false, false, false, false, false, // 112-119 ('p'-'w')
    false, false, false, false, false, false, false, false, // 120-127 ('x'-'z', others)
    false, false, false, false, false, false, false, false, // 128-135
    false, false, false, false, false, false, false, false, // 136-143
    false, false, false, false, false, false, false, false, // 144-151
    false, false, false, false, false, false, false, false, // 152-159
    false, false, false, false, false, false, false, false, // 160-167
    false, false, false, false, false, false, false, false, // 168-175
    false, false, false, false, false, false, false, false, // 176-183
    false, false, false, false, false, false, false, false, // 184-191
    false, false, false, false, false, false, false, false, // 192-199
    false, false, false, false, false, false, false, false, // 200-207
    false, false, false, false, false, false, false, false, // 208-215
    false, false, false, false, false, false, false, false, // 216-223
    false, false, false, false, false, false, false, false, // 224-231
    false, false, false, false, false, false, false, false, // 232-239
    false, false, false, false, false, false, false, false, // 240-247
    false, false, false, false, false, false, false, false  // 248-255
};

static CONSTEXPR const bool STR_IS_ALPHA_LOOKUP_TABLE[] = {
    false, false, false, false, false, false, false, false, // 0-7
    false, false, false, false, false, false, false, false, // 8-15
    false, false, false, false, false, false, false, false, // 16-23
    false, false, false, false, false, false, false, false, // 24-31
    false, false, false, false, false, false, false, false, // 32-39
    false, false, false, false, false, false, false, false, // 40-47
    false, false, false, false, false, false, false, false, // 48-55 ('0'-'7')
    false, false, false, false, false, false, false, false, // 56-63 ('8'-'9', others)
    false, true,  true,  true,  true,  true,  true,  true,  // 64-71 ('A'-'G')
    true,  true,  true,  true,  true,  true,  true,  true,  // 72-79 ('H'-'O')
    true,  true,  true,  true,  true,  true,  true,  true,  // 80-87 ('P'-'W')
    true,  true,  true, false, false, false, false, false,  // 88-95 ('X'-'Z', others)
    false, true,  true,  true,  true,  true,  true,  true,  // 96-103 ('a'-'g')
    true,  true,  true,  true,  true,  true,  true,  true,  // 104-111 ('h'-'o')
    true,  true,  true,  true,  true,  true,  true,  true,  // 112-119 ('p'-'w')
    true,  true,  true, false, false, false, false, false,  // 120-127 ('x'-'z', others)
    false, false, false, false, false, false, false, false, // 128-135
    false, false, false, false, false, false, false, false, // 136-143
    false, false, false, false, false, false, false, false, // 144-151
    false, false, false, false, false, false, false, false, // 152-159
    false, false, false, false, false, false, false, false, // 160-167
    false, false, false, false, false, false, false, false, // 168-175
    false, false, false, false, false, false, false, false, // 176-183
    false, false, false, false, false, false, false, false, // 184-191
    false, false, false, false, false, false, false, false, // 192-199
    false, false, false, false, false, false, false, false, // 200-207
    false, false, false, false, false, false, false, false, // 208-215
    false, false, false, false, false, false, false, false, // 216-223
    false, false, false, false, false, false, false, false, // 224-231
    false, false, false, false, false, false, false, false, // 232-239
    false, false, false, false, false, false, false, false, // 240-247
    false, false, false, false, false, false, false, false  // 248-255
};

static CONSTEXPR const bool STR_IS_ALPHANUM_LOOKUP_TABLE[] = {
    false, false, false, false, false, false, false, false, // 0-7
    false, false, false, false, false, false, false, false, // 8-15
    false, false, false, false, false, false, false, false, // 16-23
    false, false, false, false, false, false, false, false, // 24-31
    false, false, false, false, false, false, false, false, // 32-39
    false, false, false, false, false, false, false, false, // 40-47
    true,  true,  true,  true,  true,  true,  true,  true,  // 48-55 ('0'-'7')
    true,  true, false, false, false, false, false, false,  // 56-63 ('8'-'9', others)
    false, true,  true,  true,  true,  true,  true,  true,  // 64-71 ('A'-'G')
    true,  true,  true,  true,  true,  true,  true,  true,  // 72-79 ('H'-'O')
    true,  true,  true,  true,  true,  true,  true,  true,  // 80-87 ('P'-'W')
    true,  true,  true, false, false, false, false, false,  // 88-95 ('X'-'Z', others)
    false, true,  true,  true,  true,  true,  true,  true,  // 96-103 ('a'-'g')
    true,  true,  true,  true,  true,  true,  true,  true,  // 104-111 ('h'-'o')
    true,  true,  true,  true,  true,  true,  true,  true,  // 112-119 ('p'-'w')
    true,  true,  true, false, false, false, false, false,  // 120-127 ('x'-'z', others)
    false, false, false, false, false, false, false, false, // 128-135
    false, false, false, false, false, false, false, false, // 136-143
    false, false, false, false, false, false, false, false, // 144-151
    false, false, false, false, false, false, false, false, // 152-159
    false, false, false, false, false, false, false, false, // 160-167
    false, false, false, false, false, false, false, false, // 168-175
    false, false, false, false, false, false, false, false, // 176-183
    false, false, false, false, false, false, false, false, // 184-191
    false, false, false, false, false, false, false, false, // 192-199
    false, false, false, false, false, false, false, false, // 200-207
    false, false, false, false, false, false, false, false, // 208-215
    false, false, false, false, false, false, false, false, // 216-223
    false, false, false, false, false, false, false, false, // 224-231
    false, false, false, false, false, false, false, false, // 232-239
    false, false, false, false, false, false, false, false, // 240-247
    false, false, false, false, false, false, false, false  // 248-255
};

FORCE_INLINE CONSTEXPR
bool isdigit(char str) NO_EXCEPT
{
    return STR_IS_NUM_LOOKUP_TABLE[(byte) str];
}

FORCE_INLINE CONSTEXPR
bool isalpha(char str) NO_EXCEPT
{
    return STR_IS_ALPHANUM_LOOKUP_TABLE[(byte) str];
}

FORCE_INLINE CONSTEXPR
bool isalnum(char str) NO_EXCEPT
{
    return STR_IS_ALPHA_LOOKUP_TABLE[(byte) str];
}

#endif