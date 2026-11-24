/**
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 *
 * @performance Check SIMD implementation
 */
#pragma once
#ifndef COMS_COMPRESSION_LZ4_H
#define COMS_COMPRESSION_LZ4_H

#include "../stdlib/Stdlib.h"

#define LZ4_MIN_MATCH 4
#define LZ4_WINDOW_SIZE 65535
#define LZ4_MAX_MATCH_LEN (LZ4_MIN_MATCH + 15 + 255)

CONSTEXPR static inline
int32 lz4_find_longest_match(
    const byte* __restrict in,
    int32 current,
    int32 length,
    int32* __restrict match_offset
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

CONSTEXPR static inline
void lz4_write_length(byte* __restrict out, uint32* __restrict out_pos, int32 length) NO_EXCEPT
{
    while (length >= 255) {
        out[(*out_pos)++] = 255;
        length -= 255;
    }
    out[(*out_pos)++] = (byte) length;
}

uint32 lz4_encode(const byte* __restrict in, size_t in_length, byte* __restrict out) NO_EXCEPT
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

uint32 lz4_decode(const byte* __restrict in, size_t in_length, byte* __restrict out) NO_EXCEPT
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

// Fast LZ4 block compression (same block format as lz4_encode/lz4_decode)
////////////////////////////////////////
// lz4_encode searches the whole window for every position (very slow for larger inputs).
// lz4_encode_fast uses a hash table of the last position of every 4 byte sequence (like the reference "fast" mode),
// the output is a valid LZ4 block: the last 5 bytes are always literals, no match starts in the last 12 bytes.

#define LZ4_FAST_HASH_BITS 12
#define LZ4_FAST_HASH_SIZE (1 << LZ4_FAST_HASH_BITS)
#define LZ4_FAST_LAST_LITERALS 5
#define LZ4_FAST_MATCH_LIMIT 12

/**
 * Worst case size of a compressed block (incompressible input)
 *
 * @param size_t length Input length
 *
 * @return size_t
 */
CONSTEXPR FORCE_INLINE
size_t lz4_encode_bound(size_t length) NO_EXCEPT
{
    return length + length / 255 + 16;
}

CONSTEXPR FORCE_INLINE
uint32 lz4_fast_hash(uint32 v) NO_EXCEPT
{
    return (v * 2654435761U) >> (32 - LZ4_FAST_HASH_BITS);
}

FORCE_INLINE
uint32 lz4_fast_read32(const byte* p) NO_EXCEPT
{
    uint32 v;
    memcpy(&v, p, sizeof(v));

    return v;
}

/**
 * Writes one sequence (literals + optional match)
 *
 * @return uint32 New output position, 0xFFFFFFFF if the output is too small
 */
static inline
uint32 lz4_fast_sequence(
    const byte* __restrict literals, uint32 literal_length,
    uint32 match_offset, uint32 match_length,
    byte* __restrict out, uint32 out_pos, uint32 out_capacity
) NO_EXCEPT
{
    // Token + max. length bytes + literals + offset
    const uint32 worst = 1 + literal_length / 255 + 1 + literal_length + 2 + match_length / 255 + 1;
    if (out_pos + worst > out_capacity) {
        return 0xFFFFFFFF;
    }

    const uint32 encoded_match = match_length ? match_length - LZ4_MIN_MATCH : 0;

    byte token = (byte) ((literal_length < 15 ? literal_length : 15) << 4);
    token |= (byte) (encoded_match < 15 ? encoded_match : 15);
    out[out_pos++] = token;

    if (literal_length >= 15) {
        uint32 rest = literal_length - 15;
        while (rest >= 255) {
            out[out_pos++] = 255;
            rest -= 255;
        }
        out[out_pos++] = (byte) rest;
    }

    memcpy(out + out_pos, literals, literal_length);
    out_pos += literal_length;

    if (!match_length) {
        return out_pos;
    }

    out[out_pos++] = (byte) (match_offset & 0xFF);
    out[out_pos++] = (byte) ((match_offset >> 8) & 0xFF);

    if (encoded_match >= 15) {
        uint32 rest = encoded_match - 15;
        while (rest >= 255) {
            out[out_pos++] = 255;
            rest -= 255;
        }
        out[out_pos++] = (byte) rest;
    }

    return out_pos;
}

/**
 * Compresses a block (hash table based, fast)
 *
 * @param const byte*   in              Input
 * @param uint32        in_length       Input length
 * @param byte*         out             Output
 * @param uint32        out_capacity    Output size (lz4_encode_bound() never fails)
 * @param uint32*       table           Scratch hash table with LZ4_FAST_HASH_SIZE entries (contents don't matter)
 *
 * @return uint32 Compressed size, 0 if the output is too small (or in_length is 0)
 */
inline
uint32 lz4_encode_fast(
    const byte* __restrict in, uint32 in_length,
    byte* __restrict out, uint32 out_capacity,
    uint32* __restrict table
) NO_EXCEPT
{
    if (!in_length) {
        return 0;
    }

    // Position + 1, 0 = empty
    memset(table, 0, sizeof(uint32) * LZ4_FAST_HASH_SIZE);

    uint32 out_pos = 0;
    uint32 anchor = 0;
    uint32 pos = 0;

    const uint32 match_end = in_length > LZ4_FAST_MATCH_LIMIT ? in_length - LZ4_FAST_MATCH_LIMIT : 0;

    while (pos < match_end) {
        const uint32 seq = lz4_fast_read32(in + pos);
        const uint32 h = lz4_fast_hash(seq);
        const uint32 candidate = table[h];
        table[h] = pos + 1;

        if (!candidate
            || pos - (candidate - 1) > LZ4_WINDOW_SIZE
            || lz4_fast_read32(in + candidate - 1) != seq
        ) {
            ++pos;
            continue;
        }

        const uint32 ref = candidate - 1;

        // Extend the match, the last LZ4_FAST_LAST_LITERALS bytes stay literals
        const uint32 limit = in_length - LZ4_FAST_LAST_LITERALS;
        uint32 length = LZ4_MIN_MATCH;
        while (pos + length < limit && in[ref + length] == in[pos + length]) {
            ++length;
        }

        out_pos = lz4_fast_sequence(in + anchor, pos - anchor, pos - ref, length, out, out_pos, out_capacity);
        if (out_pos == 0xFFFFFFFF) {
            return 0;
        }

        // Index some positions inside of the match for better ratios
        const uint32 next = pos + length;
        if (next >= 2 && next - 2 < match_end) {
            table[lz4_fast_hash(lz4_fast_read32(in + next - 2))] = next - 2 + 1;
        }

        pos = next;
        anchor = pos;
    }

    // Last literals
    out_pos = lz4_fast_sequence(in + anchor, in_length - anchor, 0, 0, out, out_pos, out_capacity);

    return out_pos == 0xFFFFFFFF ? 0 : out_pos;
}

/**
 * Decompresses a block with bounds checks (corrupted input never reads/writes out of bounds)
 *
 * @param const byte*   in              Compressed block
 * @param uint32        in_length       Compressed size
 * @param byte*         out             Output
 * @param uint32        out_capacity    Output size
 *
 * @return int32 Decompressed size, -1 on corrupted input or if the output is too small
 */
inline
int32 lz4_decode_safe(
    const byte* __restrict in, uint32 in_length,
    byte* __restrict out, uint32 out_capacity
) NO_EXCEPT
{
    uint32 in_pos = 0;
    uint32 out_pos = 0;

    while (in_pos < in_length) {
        const byte token = in[in_pos++];

        uint32 literal_length = (token >> 4) & 0x0F;
        if (literal_length == 15) {
            byte b;
            do {
                if (in_pos >= in_length) {
                    return -1;
                }

                b = in[in_pos++];
                literal_length += b;
            } while (b == 255);
        }

        if (literal_length > in_length - in_pos || literal_length > out_capacity - out_pos) {
            return -1;
        }

        memcpy(out + out_pos, in + in_pos, literal_length);
        out_pos += literal_length;
        in_pos += literal_length;

        // The last sequence only has literals
        if (in_pos >= in_length) {
            break;
        }

        if (in_pos + 2 > in_length) {
            return -1;
        }

        const uint32 match_offset = (uint32) in[in_pos] | ((uint32) in[in_pos + 1] << 8);
        in_pos += 2;

        uint32 match_length = token & 0x0F;
        if (match_length == 15) {
            byte b;
            do {
                if (in_pos >= in_length) {
                    return -1;
                }

                b = in[in_pos++];
                match_length += b;
            } while (b == 255);
        }
        match_length += LZ4_MIN_MATCH;

        if (match_offset == 0 || match_offset > out_pos || match_length > out_capacity - out_pos) {
            return -1;
        }

        // Byte copy, the ranges may overlap (e.g. runs)
        const uint32 match_pos = out_pos - match_offset;
        for (uint32 i = 0; i < match_length; ++i) {
            out[out_pos + i] = out[match_pos + i];
        }

        out_pos += match_length;
    }

    return (int32) out_pos;
}

#endif
