/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_UTILS_H
#define COMS_UTILS_H

#include "../stdlib/Stdlib.h"
#include "../utils/StringUtils.h"
#include "../compiler/CompilerUtils.h"

#ifdef __linux__
    #include <unistd.h>
#endif

#ifdef __aarch64__
    #ifdef __aarch64___NEON
        #include "../architecture/arm/neon/utils/Utils.h"
    #else
        #include "../architecture/arm/sve/utils/Utils.h"
    #endif
#else
    #include "../architecture/x86/simd/utils/Utils.h"
#endif

struct FileBody {
    // doesn't include null termination (same as strlen)
    size_t size;

    // If size is defined you also must allocate memory for content when reading from a file
    // Otherwise the API expects a ring memory it can use for reserving memory
    // Of course this means that the content is only temporarily available and will be overwritten any time
    // If you allocate content, make sure to allocate +1 since we always add \0 at the end even in binary
    byte* content;
};

FORCE_INLINE
bool is_equal(const byte* __restrict region1, const byte* __restrict region2, uint64 size) NO_EXCEPT
{
    return memcmp(region1, region2, size) == 0;
}

inline
void str_output(const char* __restrict str, ...) NO_EXCEPT
{
    char buffer[1024];
    if (strchr(str, '%')) {
        va_list args;
        va_start(args, str);
        sprintf_fast(buffer, 1024, str, args);
        va_end(args);

        str = buffer;
    }

    #ifdef _WIN32
        HANDLE hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
        WriteFile(hStdout, str, (DWORD) strlen(str), NULL, NULL);
    #else
        write(STDOUT_FILENO, str, strlen(str));
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