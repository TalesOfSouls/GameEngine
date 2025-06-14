/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_COMPRESSION_LZP_H
#define COMS_COMPRESSION_LZP_H

#include <stdio.h>
#include <string.h>

#include "../stdlib/Types.h"

uint32 lzp_encode(const byte* in, size_t length, byte* out) noexcept
{
    byte buf[9];
    byte table[1 << 16] = {0};

    uint16 hash = 0;
    int32 i, j;
    byte mask, c;
    uint32 in_pos = 0, out_pos = 0;

    while(true) {
        j = 1;
        mask = 0;
        for (i = 0; i < 8; ++i) {
            if (in_pos == length) {
                break;
            }

            c = in[in_pos++];
            if (c == table[hash]) {
                mask |= 1 << i;
            } else {
                table[hash] = c;
                buf[j++] = c;
            }

            hash = (hash << 4) ^ c;
        }

        if (i > 0) {
            buf[0] = mask;
            for (i = 0; i < j; ++i) {
                out[out_pos++] = buf[i];
            }
        }

        if (in_pos == length) {
            break;
        }
    }

    return out_pos;
}

uint32 lzp_decode(const byte* in, size_t length, byte* out) noexcept
{
    byte buf[8];
    byte table[1 << 16] = {0};

    uint16 hash = 0;
    int32 i, j;
    byte mask, c;
    uint32 in_pos = 0, out_pos = 0;

    while (true) {
        j = 0;
        if (in_pos == length) {
            break;
        }

        mask = in[in_pos++];
        for (i = 0; i < 8; ++i) {
            if ((mask & (1 << i)) != 0) {
                c = table[hash];
            } else {
                if (in_pos == length) {
                    break;
                }

                c = in[in_pos++];
                table[hash] = c;
            }

            buf[j++] = c;

            hash = (hash << 4) ^ c;
        }

        for (i = 0; i < j; ++i) {
            out[out_pos++] = buf[i];
        }
    }

    return out_pos;
}

int32 find_longest_match(char *window, int32 window_start, char *buffer, int32 buffer_size, int32 *match_position) noexcept
{
    int32 best_length = 0;
    int32 best_offset = 0;

    for (int32 i = window_start; i < 4096 && i < buffer_size; ++i) {
        int32 length = 0;

        while (length < 18
            && i + length < 4096
            && buffer[length] == window[i + length]
        ) {
            ++length;
        }

        if (length > best_length) {
            best_length = length;
            best_offset = i;
        }
    }

    *match_position = best_offset;

    return best_length;
}

uint32 lzp3_encode(const byte* in, size_t length, byte* out) noexcept
{
    char window[4096] = {0};
    int32 window_start = 0;

    int32 out_size = 0;

    size_t i = 0;
    while (i < length) {
        int32 match_position = 0;
        int32 match_length = find_longest_match(
            window,
            window_start,
            (char *) &in[i], (int32) (length - i),
            &match_position
        );

        if (match_length > 2) {
            out[out_size++] = 0xFF;
            out[out_size++] = match_position & 0xFF;
            out[out_size++] = match_length & 0xFF;

            i += match_length;
        } else {
            out[out_size++] = in[i];
            ++i;
        }

        int32 shift_length = match_length > 0 ? match_length : 1;
        memmove(window, window + shift_length, 4096 - shift_length);
        memcpy(window + (4096 - shift_length), &in[i - shift_length], shift_length);
        window_start = (window_start + shift_length) >= 4096 ? 0 : window_start + shift_length;
    }

    return out_size;
}

uint32 lzp3_decode(const byte* in, size_t length, byte* out) noexcept
{
    char window[4096] = {0};
    int32 window_start = 0;

    int32 out_size = 0;

    size_t i = 0;
    while (i < length) {
        if (in[i] == 0xFF) {
            int32 match_position = in[i + 1];
            int32 match_length = in[i + 2];

            for (int32 j = 0; j < match_length; j++) {
                out[out_size++] = window[MODULO_2(match_position + j, 4096)];
            }

            memmove(window, window + match_length, 4096 - match_length);
            memcpy(window + (4096 - match_length), &out[out_size - match_length], match_length);
            window_start = (window_start + match_length) >= 4096 ? 0 : window_start + match_length;

            i += 3;
        } else {
            out[out_size++] = in[i];

            memmove(window, window + 1, 4096 - 1);
            window[4096 - 1] = in[i];
            window_start = (window_start + 1) >= 4096 ? 0 : window_start + 1;

            ++i;
        }
    }

    return out_size;
}

#endif