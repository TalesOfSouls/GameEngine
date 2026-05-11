/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_COMPRESSION_LZ4_H
#define COMS_COMPRESSION_LZ4_H

#include "../stdlib/Stdlib.h"

#define LZ4_MIN_MATCH 4
#define LZ4_WINDOW_SIZE 65535

static inline
int32 lz4_find_longest_match(
    const byte* in,
    int32 current,
    int32 length,
    int32* match_offset
) NO_EXCEPT
{
    int32 best_length = 0;
    int32 best_offset = 0;

    const int32 window_start = current > LZ4_WINDOW_SIZE
            ? current - LZ4_WINDOW_SIZE
            : 0;

    for (int32 i = window_start; i < current; ++i) {
        int32 match_length = 0;

        while ((current + match_length) < length
            && in[i + match_length] == in[current + match_length]
            && match_length < 255 + 15 + LZ4_MIN_MATCH
        ) {
            ++match_length;
        }

        if (match_length >= LZ4_MIN_MATCH
            && match_length > best_length
        ) {
            best_length = match_length;
            best_offset = current - i;
        }
    }

    *match_offset = best_offset;

    return best_length;
}

uint32 lz4_encode(const byte* in, size_t length, byte* out) NO_EXCEPT
{
    uint32 in_pos = 0;
    uint32 out_pos = 0;

    while (in_pos < length) {
        int32 match_offset = 0;

        const int32 match_length = lz4_find_longest_match(
            in,
            (int32) in_pos,
            (int32) length,
            &match_offset
        );

        if (match_length < LZ4_MIN_MATCH) {
            const uint32 literal_start = in_pos;
            uint32 literal_length = 1;

            ++in_pos;

            while (in_pos < length) {
                int32 next_offset = 0;

                const int32 next_match = lz4_find_longest_match(
                    in,
                    (int32) in_pos,
                    (int32) length,
                    &next_offset
                );

                if (next_match >= LZ4_MIN_MATCH
                    || literal_length >= 15
                ) {
                    break;
                }

                ++literal_length;
                ++in_pos;
            }

            out[out_pos++] = (byte) (literal_length << 4);
            // @performance memmove?
            for (uint32 i = 0; i < literal_length; ++i) {
                out[out_pos++] = in[literal_start + i];
            }

            continue;
        }

        byte token = 0;

        int32 literal_length = 0;
        const int32 encoded_match_length = match_length - LZ4_MIN_MATCH;

        token |= (literal_length & 0x0F) << 4;
        token |= encoded_match_length > 15
            ? 15
            : encoded_match_length;

        out[out_pos++] = token;

        out[out_pos++] = (byte) (match_offset & 0xFF);
        out[out_pos++] = (byte) ((match_offset >> 8) & 0xFF);

        if (encoded_match_length >= 15) {
            out[out_pos++] = (byte) (encoded_match_length - 15);
        }

        in_pos += match_length;
    }

    return out_pos;
}

uint32 lz4_decode(const byte* in, size_t length, byte* out) NO_EXCEPT
{
    uint32 in_pos = 0;
    uint32 out_pos = 0;

    while (in_pos < length) {
        const byte token = in[in_pos++];
        const int32 literal_length = (token >> 4) & 0x0F;

        memcpy(out + out_pos, in + in_pos, literal_length);
        out_pos += literal_length;
        in_pos += literal_length;

        if (in_pos >= length) {
            break;
        }

        const int32 match_offset =
            in[in_pos]
            | (in[in_pos + 1] << 8);

        in_pos += 2;

        int32 match_length = token & 0x0F;
        if (match_length == 15) {
            match_length += in[in_pos++];
        }

        match_length += LZ4_MIN_MATCH;

        const uint32 match_pos = out_pos - match_offset;
        // @performance Isn't this a memmove?
        for (int32 i = 0; i < match_length; ++i) {
            out[out_pos++] = out[match_pos + i];
        }
    }

    return out_pos;
}

#endif