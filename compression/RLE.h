/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_COMPRESSION_RLE_H
#define COMS_COMPRESSION_RLE_H

#include <stdio.h>
#include <string.h>

#include "../stdlib/Types.h"
#include "../utils/StringUtils.h"

// max out length = length * 2 + 1
uint64 rle_encode(const char* in, size_t length, char* out) noexcept
{
    uint64 count;
    uint64 j = 0;

    for (uint64 i = 0; i < length; ++i) {
        count = 1;
        while (i + 1 < length && in[i] == in[i + 1]) {
            ++count;
            ++i;
        }

        out[j++] = in[i];

        j += int_to_str(count, &out[j], NULL);
    }

    out[j] = '\0';

    return j;
}

uint64 rle_decode(const char* in, size_t length, char* out) noexcept
{
    uint64 j = 0;

    for (int64 i = 0; i < length; ++i) {
        char current_char = in[i];
        ++i;

        int32 count = 0;
        while (i < length && in[i] >= '0' && in[i] <= '9') {
            count = count * 10 + (in[i] - '0');
            ++i;
        }
        --i;

        for (int32 k = 0; k < count; ++k) {
            out[j++] = current_char;
        }
    }

    out[j] = '\0';

    return j;
}

#endif