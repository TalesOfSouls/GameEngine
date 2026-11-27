/**
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_ENCRYPTION_CHACHA20_POLY1305_H
#define COMS_ENCRYPTION_CHACHA20_POLY1305_H

#include "../stdlib/Stdlib.h"
#include "../utils/ConstantTime.h"

/**
 * ChaCha20-Poly1305 AEAD (RFC 8439)
 *
 * Authenticated encryption used by the network envelope (network/packet/NetPacket.h).
 * The associated data (the packet header) is authenticated but not encrypted,
 * the payload is encrypted and authenticated.
 *
 * A (key, nonce) pair must never be used for two different messages. The network
 * layer derives the nonce from the connection id and the packet sequence and
 * uses a separate key per direction.
 *
 * All words are assembled from bytes explicitly -> endian independent.
 * Poly1305 uses 26 bit limbs so all products fit into uint64 (no 128 bit integers,
 * works with MSVC).
 */

#define CHACHA20_KEY_SIZE 32
#define CHACHA20_NONCE_SIZE 12
#define CHACHA20_BLOCK_SIZE 64
#define POLY1305_KEY_SIZE 32
#define POLY1305_TAG_SIZE 16

#define AEAD_CHACHA20_POLY1305_KEY_SIZE CHACHA20_KEY_SIZE
#define AEAD_CHACHA20_POLY1305_NONCE_SIZE CHACHA20_NONCE_SIZE
#define AEAD_CHACHA20_POLY1305_TAG_SIZE POLY1305_TAG_SIZE

FORCE_INLINE
uint32 chacha_load32_le(const byte* const p) NO_EXCEPT
{
    return (uint32) p[0] | ((uint32) p[1] << 8) | ((uint32) p[2] << 16) | ((uint32) p[3] << 24);
}

FORCE_INLINE
void chacha_store32_le(byte* const p, uint32 v) NO_EXCEPT
{
    p[0] = (byte) v;
    p[1] = (byte) (v >> 8);
    p[2] = (byte) (v >> 16);
    p[3] = (byte) (v >> 24);
}

FORCE_INLINE
uint32 chacha_rotl32(uint32 v, int32 c) NO_EXCEPT
{
    return (v << c) | (v >> (32 - c));
}

#define CHACHA_QUARTER_ROUND(a, b, c, d)                  \
    a += b; d ^= a; d = chacha_rotl32(d, 16);            \
    c += d; b ^= c; b = chacha_rotl32(b, 12);            \
    a += b; d ^= a; d = chacha_rotl32(d, 8);             \
    c += d; b ^= c; b = chacha_rotl32(b, 7)

/**
 * Creates one 64 byte key stream block (RFC 8439 2.3)
 *
 * @param const byte*   key     Key (32 bytes)
 * @param uint32        counter Block counter
 * @param const byte*   nonce   Nonce (12 bytes)
 * @param byte*         out     Key stream block (64 bytes)
 */
inline
void chacha20_block(
    const byte* const __restrict key,
    uint32 counter,
    const byte* const __restrict nonce,
    byte* const __restrict out
) NO_EXCEPT
{
    uint32 state[16];
    state[0] = 0x61707865;
    state[1] = 0x3320646e;
    state[2] = 0x79622d32;
    state[3] = 0x6b206574;

    for (int32 i = 0; i < 8; ++i) {
        state[4 + i] = chacha_load32_le(key + i * 4);
    }

    state[12] = counter;
    state[13] = chacha_load32_le(nonce);
    state[14] = chacha_load32_le(nonce + 4);
    state[15] = chacha_load32_le(nonce + 8);

    uint32 x[16];
    memcpy(x, state, sizeof(x));

    for (int32 i = 0; i < 10; ++i) {
        // Column rounds
        CHACHA_QUARTER_ROUND(x[0], x[4], x[8], x[12]);
        CHACHA_QUARTER_ROUND(x[1], x[5], x[9], x[13]);
        CHACHA_QUARTER_ROUND(x[2], x[6], x[10], x[14]);
        CHACHA_QUARTER_ROUND(x[3], x[7], x[11], x[15]);

        // Diagonal rounds
        CHACHA_QUARTER_ROUND(x[0], x[5], x[10], x[15]);
        CHACHA_QUARTER_ROUND(x[1], x[6], x[11], x[12]);
        CHACHA_QUARTER_ROUND(x[2], x[7], x[8], x[13]);
        CHACHA_QUARTER_ROUND(x[3], x[4], x[9], x[14]);
    }

    for (int32 i = 0; i < 16; ++i) {
        chacha_store32_le(out + i * 4, x[i] + state[i]);
    }

    ct_memzero(x, sizeof(x));
    ct_memzero(state, sizeof(state));
}

/**
 * Encrypts/decrypts (same operation) with the ChaCha20 key stream (RFC 8439 2.4)
 *
 * in and out may be identical (in place).
 *
 * @param const byte*   key     Key (32 bytes)
 * @param uint32        counter Initial block counter
 * @param const byte*   nonce   Nonce (12 bytes)
 * @param const byte*   in      Input
 * @param byte*         out     Output
 * @param size_t        length  Amount of bytes
 */
inline
void chacha20_xor(
    const byte* const key,
    uint32 counter,
    const byte* const nonce,
    const byte* const in,
    byte* const out,
    size_t length
) NO_EXCEPT
{
    byte block[CHACHA20_BLOCK_SIZE];

    size_t pos = 0;
    while (pos < length) {
        chacha20_block(key, counter++, nonce, block);

        const size_t chunk = length - pos < CHACHA20_BLOCK_SIZE ? length - pos : CHACHA20_BLOCK_SIZE;
        for (size_t i = 0; i < chunk; ++i) {
            out[pos + i] = (byte) (in[pos + i] ^ block[i]);
        }

        pos += chunk;
    }

    ct_memzero(block, sizeof(block));
}

/**
 * Poly1305 one-time authenticator (RFC 8439 2.5)
 */
struct Poly1305Context {
    uint32 r[5];
    uint32 h[5];
    uint32 pad[4];

    byte buffer[16];
    uint32 buffer_used;
};

inline
void poly1305_init(Poly1305Context* const __restrict ctx, const byte* const __restrict key) NO_EXCEPT
{
    // r is clamped (RFC 8439 2.5.1)
    ctx->r[0] = (chacha_load32_le(key + 0)) & 0x3ffffff;
    ctx->r[1] = (chacha_load32_le(key + 3) >> 2) & 0x3ffff03;
    ctx->r[2] = (chacha_load32_le(key + 6) >> 4) & 0x3ffc0ff;
    ctx->r[3] = (chacha_load32_le(key + 9) >> 6) & 0x3f03fff;
    ctx->r[4] = (chacha_load32_le(key + 12) >> 8) & 0x00fffff;

    ctx->h[0] = 0;
    ctx->h[1] = 0;
    ctx->h[2] = 0;
    ctx->h[3] = 0;
    ctx->h[4] = 0;

    ctx->pad[0] = chacha_load32_le(key + 16);
    ctx->pad[1] = chacha_load32_le(key + 20);
    ctx->pad[2] = chacha_load32_le(key + 24);
    ctx->pad[3] = chacha_load32_le(key + 28);

    ctx->buffer_used = 0;
}

/**
 * Processes full 16 byte blocks
 *
 * @param uint32 hibit 1 << 24 for normal blocks, 0 for the padded final block
 */
static inline
void poly1305_blocks(
    Poly1305Context* const __restrict ctx,
    const byte* __restrict data,
    size_t length,
    uint32 hibit
) NO_EXCEPT
{
    const uint32 r0 = ctx->r[0];
    const uint32 r1 = ctx->r[1];
    const uint32 r2 = ctx->r[2];
    const uint32 r3 = ctx->r[3];
    const uint32 r4 = ctx->r[4];

    const uint32 s1 = r1 * 5;
    const uint32 s2 = r2 * 5;
    const uint32 s3 = r3 * 5;
    const uint32 s4 = r4 * 5;

    uint32 h0 = ctx->h[0];
    uint32 h1 = ctx->h[1];
    uint32 h2 = ctx->h[2];
    uint32 h3 = ctx->h[3];
    uint32 h4 = ctx->h[4];

    while (length >= 16) {
        // h += m
        h0 += (chacha_load32_le(data + 0)) & 0x3ffffff;
        h1 += (chacha_load32_le(data + 3) >> 2) & 0x3ffffff;
        h2 += (chacha_load32_le(data + 6) >> 4) & 0x3ffffff;
        h3 += (chacha_load32_le(data + 9) >> 6) & 0x3ffffff;
        h4 += (chacha_load32_le(data + 12) >> 8) | hibit;

        // h *= r
        const uint64 d0 = (uint64) h0 * r0 + (uint64) h1 * s4 + (uint64) h2 * s3 + (uint64) h3 * s2 + (uint64) h4 * s1;
        uint64 d1 = (uint64) h0 * r1 + (uint64) h1 * r0 + (uint64) h2 * s4 + (uint64) h3 * s3 + (uint64) h4 * s2;
        uint64 d2 = (uint64) h0 * r2 + (uint64) h1 * r1 + (uint64) h2 * r0 + (uint64) h3 * s4 + (uint64) h4 * s3;
        uint64 d3 = (uint64) h0 * r3 + (uint64) h1 * r2 + (uint64) h2 * r1 + (uint64) h3 * r0 + (uint64) h4 * s4;
        uint64 d4 = (uint64) h0 * r4 + (uint64) h1 * r3 + (uint64) h2 * r2 + (uint64) h3 * r1 + (uint64) h4 * r0;

        // Partial reduction mod 2^130 - 5
        uint32 c = (uint32) (d0 >> 26);
        h0 = (uint32) d0 & 0x3ffffff;
        d1 += c;
        c = (uint32) (d1 >> 26);
        h1 = (uint32) d1 & 0x3ffffff;
        d2 += c;
        c = (uint32) (d2 >> 26);
        h2 = (uint32) d2 & 0x3ffffff;
        d3 += c;
        c = (uint32) (d3 >> 26);
        h3 = (uint32) d3 & 0x3ffffff;
        d4 += c;
        c = (uint32) (d4 >> 26);
        h4 = (uint32) d4 & 0x3ffffff;
        h0 += c * 5;
        c = h0 >> 26;
        h0 &= 0x3ffffff;
        h1 += c;

        data += 16;
        length -= 16;
    }

    ctx->h[0] = h0;
    ctx->h[1] = h1;
    ctx->h[2] = h2;
    ctx->h[3] = h3;
    ctx->h[4] = h4;
}

inline
void poly1305_update(Poly1305Context* const __restrict ctx, const byte* __restrict data, size_t length) NO_EXCEPT
{
    if (ctx->buffer_used) {
        const size_t want = 16 - ctx->buffer_used;
        const size_t take = length < want ? length : want;

        memcpy(ctx->buffer + ctx->buffer_used, data, take);
        ctx->buffer_used += (uint32) take;
        data += take;
        length -= take;

        if (ctx->buffer_used < 16) {
            return;
        }

        poly1305_blocks(ctx, ctx->buffer, 16, 1U << 24);
        ctx->buffer_used = 0;
    }

    if (length >= 16) {
        const size_t full = length & ~(size_t) 15;
        poly1305_blocks(ctx, data, full, 1U << 24);
        data += full;
        length -= full;
    }

    if (length) {
        memcpy(ctx->buffer, data, length);
        ctx->buffer_used = (uint32) length;
    }
}

inline
void poly1305_final(Poly1305Context* const __restrict ctx, byte* const __restrict tag) NO_EXCEPT
{
    // Final partial block: append 1 and pad with zeros (the 1 replaces the hibit)
    if (ctx->buffer_used) {
        ctx->buffer[ctx->buffer_used] = 1;
        for (uint32 i = ctx->buffer_used + 1; i < 16; ++i) {
            ctx->buffer[i] = 0;
        }

        poly1305_blocks(ctx, ctx->buffer, 16, 0);
    }

    uint32 h0 = ctx->h[0];
    uint32 h1 = ctx->h[1];
    uint32 h2 = ctx->h[2];
    uint32 h3 = ctx->h[3];
    uint32 h4 = ctx->h[4];

    // Full carry
    uint32 c = h1 >> 26;
    h1 &= 0x3ffffff;
    h2 += c;
    c = h2 >> 26;
    h2 &= 0x3ffffff;
    h3 += c;
    c = h3 >> 26;
    h3 &= 0x3ffffff;
    h4 += c;
    c = h4 >> 26;
    h4 &= 0x3ffffff;
    h0 += c * 5;
    c = h0 >> 26;
    h0 &= 0x3ffffff;
    h1 += c;

    // g = h + -p = h - (2^130 - 5)
    uint32 g0 = h0 + 5;
    c = g0 >> 26;
    g0 &= 0x3ffffff;
    uint32 g1 = h1 + c;
    c = g1 >> 26;
    g1 &= 0x3ffffff;
    uint32 g2 = h2 + c;
    c = g2 >> 26;
    g2 &= 0x3ffffff;
    uint32 g3 = h3 + c;
    c = g3 >> 26;
    g3 &= 0x3ffffff;
    const uint32 g4 = h4 + c - (1U << 26);

    // Select h if h < p, g otherwise (constant time)
    uint32 mask = (g4 >> 31) - 1;
    g0 &= mask;
    g1 &= mask;
    g2 &= mask;
    g3 &= mask;
    const uint32 g4m = g4 & mask;
    mask = ~mask;
    h0 = (h0 & mask) | g0;
    h1 = (h1 & mask) | g1;
    h2 = (h2 & mask) | g2;
    h3 = (h3 & mask) | g3;
    h4 = (h4 & mask) | g4m;

    // h = h % 2^128
    h0 = (h0) | (h1 << 26);
    h1 = (h1 >> 6) | (h2 << 20);
    h2 = (h2 >> 12) | (h3 << 14);
    h3 = (h3 >> 18) | (h4 << 8);

    // tag = (h + pad) % 2^128
    uint64 f = (uint64) h0 + ctx->pad[0];
    h0 = (uint32) f;
    f = (uint64) h1 + ctx->pad[1] + (f >> 32);
    h1 = (uint32) f;
    f = (uint64) h2 + ctx->pad[2] + (f >> 32);
    h2 = (uint32) f;
    f = (uint64) h3 + ctx->pad[3] + (f >> 32);
    h3 = (uint32) f;

    chacha_store32_le(tag + 0, h0);
    chacha_store32_le(tag + 4, h1);
    chacha_store32_le(tag + 8, h2);
    chacha_store32_le(tag + 12, h3);

    ct_memzero(ctx, sizeof(*ctx));
}

/**
 * Poly1305 in one call
 *
 * @param const byte* key    One-time key (32 bytes)
 * @param const byte* data   Message
 * @param size_t      length Message size
 * @param byte*       tag    Output tag (16 bytes)
 */
inline
void poly1305(const byte* const key, const byte* const data, size_t length, byte* const tag) NO_EXCEPT
{
    Poly1305Context ctx;
    poly1305_init(&ctx, key);
    poly1305_update(&ctx, data, length);
    poly1305_final(&ctx, tag);
}

/**
 * Computes the AEAD tag over aad and ciphertext (RFC 8439 2.8)
 */
static inline
void aead_chacha20_poly1305_tag(
    const byte* const key,
    const byte* const nonce,
    const byte* const aad, size_t aad_length,
    const byte* const cipher, size_t cipher_length,
    byte* const tag
) NO_EXCEPT
{
    // One-time Poly1305 key = first 32 bytes of the key stream block 0
    byte block[CHACHA20_BLOCK_SIZE];
    chacha20_block(key, 0, nonce, block);

    Poly1305Context ctx;
    poly1305_init(&ctx, block);
    ct_memzero(block, sizeof(block));

    static const byte zeros[16] = {0};

    if (aad_length) {
        poly1305_update(&ctx, aad, aad_length);
    }

    if (aad_length & 15) {
        poly1305_update(&ctx, zeros, 16 - (aad_length & 15));
    }

    if (cipher_length) {
        poly1305_update(&ctx, cipher, cipher_length);
    }

    if (cipher_length & 15) {
        poly1305_update(&ctx, zeros, 16 - (cipher_length & 15));
    }

    byte lengths[16];
    for (int32 i = 0; i < 8; ++i) {
        lengths[i] = (byte) ((uint64) aad_length >> (i * 8));
        lengths[8 + i] = (byte) ((uint64) cipher_length >> (i * 8));
    }

    poly1305_update(&ctx, lengths, sizeof(lengths));
    poly1305_final(&ctx, tag);
}

/**
 * Encrypts and authenticates a message
 *
 * plain and cipher may be identical (in place).
 *
 * @param const byte* key           Key (32 bytes)
 * @param const byte* nonce         Nonce (12 bytes), never reuse with the same key
 * @param const byte* aad           Associated data (authenticated, not encrypted)
 * @param size_t      aad_length    Associated data size
 * @param const byte* plain         Plaintext
 * @param size_t      length        Plaintext size
 * @param byte*       cipher        Output ciphertext (length bytes)
 * @param byte*       tag           Output tag (16 bytes)
 */
inline
void aead_chacha20_poly1305_seal(
    const byte* const key,
    const byte* const nonce,
    const byte* const aad, size_t aad_length,
    const byte* const plain, size_t length,
    byte* const cipher,
    byte* const tag
) NO_EXCEPT
{
    if (length) {
        chacha20_xor(key, 1, nonce, plain, cipher, length);
    }

    aead_chacha20_poly1305_tag(key, nonce, aad, aad_length, cipher, length, tag);
}

/**
 * Verifies and decrypts a message
 *
 * The plaintext is only written if the tag is valid. cipher and plain may be identical.
 *
 * @param const byte* key           Key (32 bytes)
 * @param const byte* nonce         Nonce (12 bytes)
 * @param const byte* aad           Associated data
 * @param size_t      aad_length    Associated data size
 * @param const byte* cipher        Ciphertext
 * @param size_t      length        Ciphertext size
 * @param const byte* tag           Received tag (16 bytes)
 * @param byte*       plain         Output plaintext (length bytes)
 *
 * @return bool True if the message is authentic
 */
inline
bool aead_chacha20_poly1305_open(
    const byte* const key,
    const byte* const nonce,
    const byte* const aad, size_t aad_length,
    const byte* const cipher, size_t length,
    const byte* const tag,
    byte* const plain
) NO_EXCEPT
{
    byte expected[POLY1305_TAG_SIZE];
    aead_chacha20_poly1305_tag(key, nonce, aad, aad_length, cipher, length, expected);

    const bool valid = ct_memcmp(expected, tag, sizeof(expected));
    ct_memzero(expected, sizeof(expected));

    if (!valid) {
        return false;
    }

    if (length) {
        chacha20_xor(key, 1, nonce, cipher, plain, length);
    }

    return true;
}

#endif
