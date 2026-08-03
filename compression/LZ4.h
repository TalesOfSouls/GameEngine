/**
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_COMPRESSION_LZ4_H
#define COMS_COMPRESSION_LZ4_H

#include "../stdlib/Stdlib.h"

#define LZ4_MIN_MATCH 4
#define LZ4_WINDOW_SIZE 65535
#define LZ4_MAX_MATCH_LEN (LZ4_MIN_MATCH + 15 + 255)

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
            && match_length < LZ4_MAX_MATCH_LEN
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

static inline
void lz4_write_length(byte* out, uint32* out_pos, int32 length) NO_EXCEPT
{
    while (length >= 255) {
        out[(*out_pos)++] = 255;
        length -= 255;
    }
    out[(*out_pos)++] = (byte) length;
}

uint32 lz4_encode(const byte* in, size_t in_length, byte* out) NO_EXCEPT
{
    uint32 in_pos = 0;
    uint32 out_pos = 0;

    while (in_pos < in_length) {
        const uint32 literal_start = in_pos;

        int32 match_offset = 0;
        int32 match_length = lz4_find_longest_match(
            in, (int32) in_pos, (int32) in_length, &match_offset
        );

        while (match_length < LZ4_MIN_MATCH && in_pos < in_length) {
            ++in_pos;

            if (in_pos >= in_length) {
                match_length = 0;
                break;
            }

            match_length = lz4_find_longest_match(
                in, (int32) in_pos, (int32) in_length, &match_offset
            );
        }

        const uint32 literal_length = in_pos - literal_start;
        const int32 has_match = match_length >= LZ4_MIN_MATCH;
        const int32 encoded_match_length = has_match ? match_length - LZ4_MIN_MATCH : 0;

        byte token = 0;
        token |= (byte) ((literal_length < 15 ? literal_length : 15) << 4);
        token |= (byte) (has_match ? (encoded_match_length < 15 ? encoded_match_length : 15) : 0);
        out[out_pos++] = token;

        if (literal_length >= 15) {
            lz4_write_length(out, &out_pos, (int32) literal_length - 15);
        }

        memcpy(out + out_pos, in + literal_start, literal_length);
        out_pos += literal_length;

        if (has_match) {
            out[out_pos++] = (byte) (match_offset & 0xFF);
            out[out_pos++] = (byte) ((match_offset >> 8) & 0xFF);

            if (encoded_match_length >= 15) {
                lz4_write_length(out, &out_pos, encoded_match_length - 15);
            }

            in_pos += match_length;
        }
    }

    return out_pos;
}

uint32 lz4_decode(const byte* in, size_t in_length, byte* out) NO_EXCEPT
{
    uint32 in_pos = 0;
    uint32 out_pos = 0;

    while (in_pos < in_length) {
        const byte token = in[in_pos++];

        uint32 literal_length = (token >> 4) & 0x0F;
        if (literal_length == 15) {
            byte b;
            do {
                b = in[in_pos++];
                literal_length += b;
            } while (b == 255);
        }

        memcpy(out + out_pos, in + in_pos, literal_length);
        out_pos += literal_length;
        in_pos += literal_length;

        if (in_pos >= in_length) {
            break;
        }

        const int32 match_offset = in[in_pos] | (in[in_pos + 1] << 8);
        in_pos += 2;

        uint32 match_length = token & 0x0F;
        if (match_length == 15) {
            byte b;
            do {
                b = in[in_pos++];
                match_length += b;
            } while (b == 255);
        }
        match_length += LZ4_MIN_MATCH;

        const uint32 match_pos = out_pos - match_offset;

        if ((uint32) match_offset >= match_length) {
            memcpy(out + out_pos, out + match_pos, match_length);
            out_pos += match_length;
        } else {
            for (uint32 i = 0; i < match_length; ++i) {
                out[out_pos++] = out[match_pos + i];
            }
        }
    }

    return out_pos;
}

#endif