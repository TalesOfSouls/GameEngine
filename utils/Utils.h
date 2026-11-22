/**
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_UTILS_H
#define COMS_UTILS_H

#include "../stdlib/Stdlib.h"
#include "../utils/StringUtils.h"

#ifdef __linux__
    #include <unistd.h>
#endif

#if defined(__aarch64__) && defined(__ARM_FEATURE_SVE)
    #include "../architecture/arm/sve/utils/Utils.h"
#elif defined(__aarch64__) && defined(__ARM_NEON)
    #include "../architecture/arm/neon/utils/Utils.h"
#else
    // Without any SIMD macros (e.g. msvc on ARM64 doesn't define __ARM_NEON) the x86 version is plain scalar code
    #include "../architecture/x86/simd/utils/Utils.h"
#endif

FORCE_INLINE
bool is_equal(const byte* __restrict region1, const byte* __restrict region2, uint64 size) NO_EXCEPT
{
    return memcmp(region1, region2, size) == 0;
}

inline
void str_output(const char* __restrict str, ...) NO_EXCEPT
{
    char buffer[1024];
    if (str_find(str, '%')) {
        #if defined(NO_STDLIB) && NO_STDLIB
            // Without the standard library we cannot format a va_list
        #else
            va_list args;
            va_start(args, str);
            vsnprintf(buffer, sizeof(buffer), str, args);
            va_end(args);

            str = buffer;
        #endif
    }

    #ifdef _WIN32
        HANDLE hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
        WriteFile(hStdout, str, (DWORD) str_length(str), NULL, NULL);
    #else
        write(STDOUT_FILENO, str, str_length(str));
    #endif
}

inline
void swap_memory(void* __restrict a, void* __restrict b, size_t size) NO_EXCEPT
{
    byte* p = (byte*) a;
    byte* q = (byte*) b;

    // Swap in chunks of size_t
    while (size >= sizeof(size_t)) {
        size_t tmp = *(size_t *) p;
        *(size_t *) p = *(size_t *) q;
        *(size_t *) q = tmp;

        p += sizeof(size_t);
        q += sizeof(size_t);
        size -= sizeof(size_t);
    }

    // Swap remaining bytes
    while (size > 0) {
        OMS_SWAP(byte, *p, *q);

        ++p;
        ++q;
        --size;
    }
}

#endif