/**
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_ENCRYPTION_X25519_H
#define COMS_ENCRYPTION_X25519_H

#include "../stdlib/Stdlib.h"
#include "../utils/ConstantTime.h"

/**
 * X25519 Diffie-Hellman (RFC 7748)
 *
 * Used for the key exchange of the network handshake (network/NetConnection.h).
 * Both sides create a key pair, exchange the public keys and compute the same
 * shared secret with x25519_shared().
 *
 * The field arithmetic is the well known TweetNaCl implementation (16 limbs with
 * 16 bits each in int64). It's slower than ref10 but small and easy to audit and
 * only runs once per handshake. Constant time (no secret dependent branches or
 * memory accesses).
 */

#define X25519_KEY_SIZE 32

typedef int64 x25519_fe[16];

static inline
void x25519_carry(x25519_fe o) NO_EXCEPT
{
    for (int32 i = 0; i < 16; ++i) {
        o[i] += ((int64) 1 << 16);
        const int64 c = o[i] >> 16;

        // Limb 15 wraps around to limb 0 with factor 38 (2^256 = 38 mod p)
        if (i < 15) {
            o[i + 1] += c - 1;
        } else {
            o[0] += 38 * (c - 1);
        }

        o[i] -= c * ((int64) 1 << 16);
    }
}

/**
 * Swaps p and q if b == 1 (constant time)
 */
static inline
void x25519_select(x25519_fe p, x25519_fe q, int64 b) NO_EXCEPT
{
    const int64 c = ~(b - 1);
    for (int32 i = 0; i < 16; ++i) {
        const int64 t = c & (p[i] ^ q[i]);
        p[i] ^= t;
        q[i] ^= t;
    }
}

static inline
void x25519_pack(byte* const o, const x25519_fe n) NO_EXCEPT
{
    x25519_fe t;
    x25519_fe m;

    for (int32 i = 0; i < 16; ++i) {
        t[i] = n[i];
    }

    x25519_carry(t);
    x25519_carry(t);
    x25519_carry(t);

    // Subtract p twice (constant time) to get the canonical representation
    for (int32 j = 0; j < 2; ++j) {
        m[0] = t[0] - 0xffed;
        for (int32 i = 1; i < 15; ++i) {
            m[i] = t[i] - 0xffff - ((m[i - 1] >> 16) & 1);
            m[i - 1] &= 0xffff;
        }

        m[15] = t[15] - 0x7fff - ((m[14] >> 16) & 1);
        const int64 b = (m[15] >> 16) & 1;
        m[14] &= 0xffff;

        x25519_select(t, m, 1 - b);
    }

    for (int32 i = 0; i < 16; ++i) {
        o[2 * i] = (byte) (t[i] & 0xff);
        o[2 * i + 1] = (byte) (t[i] >> 8);
    }
}

static inline
void x25519_unpack(x25519_fe o, const byte* const n) NO_EXCEPT
{
    for (int32 i = 0; i < 16; ++i) {
        o[i] = n[2 * i] + ((int64) n[2 * i + 1] << 8);
    }

    // The most significant bit of the u coordinate is ignored (RFC 7748 5)
    o[15] &= 0x7fff;
}

static inline
void x25519_add(x25519_fe o, const x25519_fe a, const x25519_fe b) NO_EXCEPT
{
    for (int32 i = 0; i < 16; ++i) {
        o[i] = a[i] + b[i];
    }
}

static inline
void x25519_sub(x25519_fe o, const x25519_fe a, const x25519_fe b) NO_EXCEPT
{
    for (int32 i = 0; i < 16; ++i) {
        o[i] = a[i] - b[i];
    }
}

static inline
void x25519_mul(x25519_fe o, const x25519_fe a, const x25519_fe b) NO_EXCEPT
{
    int64 t[31];
    for (int32 i = 0; i < 31; ++i) {
        t[i] = 0;
    }

    for (int32 i = 0; i < 16; ++i) {
        for (int32 j = 0; j < 16; ++j) {
            t[i + j] += a[i] * b[j];
        }
    }

    for (int32 i = 0; i < 15; ++i) {
        t[i] += 38 * t[i + 16];
    }

    for (int32 i = 0; i < 16; ++i) {
        o[i] = t[i];
    }

    x25519_carry(o);
    x25519_carry(o);
}

static inline
void x25519_square(x25519_fe o, const x25519_fe a) NO_EXCEPT
{
    x25519_mul(o, a, a);
}

/**
 * o = i^(p - 2) = 1 / i
 */
static inline
void x25519_invert(x25519_fe o, const x25519_fe i) NO_EXCEPT
{
    x25519_fe c;
    for (int32 a = 0; a < 16; ++a) {
        c[a] = i[a];
    }

    for (int32 a = 253; a >= 0; --a) {
        x25519_square(c, c);
        if (a != 2 && a != 4) {
            x25519_mul(c, c, i);
        }
    }

    for (int32 a = 0; a < 16; ++a) {
        o[a] = c[a];
    }
}

/**
 * Scalar multiplication on Curve25519 (Montgomery ladder)
 *
 * @param byte*       out     Result u coordinate (32 bytes)
 * @param const byte* scalar  Scalar (32 bytes, clamped internally)
 * @param const byte* point   u coordinate (32 bytes)
 */
inline
void x25519(byte* const out, const byte* const scalar, const byte* const point) NO_EXCEPT
{
    static const x25519_fe a24 = {0xDB41, 1};

    byte z[32];
    for (int32 i = 0; i < 31; ++i) {
        z[i] = scalar[i];
    }

    // Clamping (RFC 7748 5)
    z[31] = (byte) ((scalar[31] & 127) | 64);
    z[0] &= 248;

    x25519_fe x;
    x25519_unpack(x, point);

    x25519_fe a = {0};
    x25519_fe b;
    x25519_fe c = {0};
    x25519_fe d = {0};
    x25519_fe e;
    x25519_fe f;

    for (int32 i = 0; i < 16; ++i) {
        b[i] = x[i];
    }

    a[0] = 1;
    d[0] = 1;

    for (int32 i = 254; i >= 0; --i) {
        const int64 r = (z[i >> 3] >> (i & 7)) & 1;

        x25519_select(a, b, r);
        x25519_select(c, d, r);

        x25519_add(e, a, c);
        x25519_sub(a, a, c);
        x25519_add(c, b, d);
        x25519_sub(b, b, d);
        x25519_square(d, e);
        x25519_square(f, a);
        x25519_mul(a, c, a);
        x25519_mul(c, b, e);
        x25519_add(e, a, c);
        x25519_sub(a, a, c);
        x25519_square(b, a);
        x25519_sub(c, d, f);
        x25519_mul(a, c, a24);
        x25519_add(a, a, d);
        x25519_mul(c, c, a);
        x25519_mul(a, d, f);
        x25519_mul(d, b, x);
        x25519_square(b, e);

        x25519_select(a, b, r);
        x25519_select(c, d, r);
    }

    x25519_invert(c, c);
    x25519_mul(a, a, c);
    x25519_pack(out, a);

    ct_memzero(z, sizeof(z));
    ct_memzero(a, sizeof(a));
    ct_memzero(b, sizeof(b));
    ct_memzero(c, sizeof(c));
    ct_memzero(d, sizeof(d));
    ct_memzero(e, sizeof(e));
    ct_memzero(f, sizeof(f));
}

/**
 * Computes the public key of a private key
 *
 * @param byte*       public_key  Output public key (32 bytes)
 * @param const byte* private_key Private key (32 random bytes)
 */
inline
void x25519_public_key(byte* const public_key, const byte* const private_key) NO_EXCEPT
{
    static const byte base_point[32] = {9};
    x25519(public_key, private_key, base_point);
}

/**
 * Computes the shared secret
 *
 * Rejects low order public keys (the result would be all zero and known to an attacker).
 *
 * @param byte*       shared      Output shared secret (32 bytes)
 * @param const byte* private_key Own private key
 * @param const byte* public_key  Public key of the other side
 *
 * @return bool False if the public key of the other side is invalid
 */
inline
bool x25519_shared(byte* const shared, const byte* const private_key, const byte* const public_key) NO_EXCEPT
{
    x25519(shared, private_key, public_key);

    byte acc = 0;
    for (int32 i = 0; i < X25519_KEY_SIZE; ++i) {
        acc |= shared[i];
    }

    return acc != 0;
}

#endif
