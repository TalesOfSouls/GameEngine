/**
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_AUTH_BCRYPT_H
#define COMS_AUTH_BCRYPT_H

#include "../stdlib/Stdlib.h"
#include "../encryption/Blowfish.h"
#include "../utils/ConstantTime.h"

/**
 * bcrypt password hashing
 *
 * Implementation of the bcrypt password hash as used by PHP's
 * password_hash()/password_verify() with PASSWORD_DEFAULT (PASSWORD_BCRYPT).
 * Existing hashes from the website database can therefore be verified without
 * changing the web stack.
 *
 * Supported hash prefixes are $2a$, $2b$ and $2y$.
 *      $2y$ is what PHP emits (correct 8 bit handling)
 *      $2b$ is the current OpenBSD default
 *      $2a$ is the historic prefix (accepted, treated like $2b$)
 *      $2x$ is rejected since it contains the old sign extension bug
 *
 * The format is: $2y$CC$<22 chars salt><31 chars hash>
 *      CC          cost exponent, the key schedule runs 2^CC times
 *      salt        16 bytes, bcrypt base64
 *      hash        23 bytes, bcrypt base64
 *
 * WARNING: bcrypt is intentionally slow (about 60-100 ms at cost 10).
 *          Never call it on a latency critical thread and rate limit logins.
 */

#define COMS_BCRYPT_HASH_LENGTH 60
#define COMS_BCRYPT_SALT_LENGTH 16
#define COMS_BCRYPT_HASH_BYTES 23
#define COMS_BCRYPT_SALT_CHARS 22
#define COMS_BCRYPT_HASH_CHARS 31

// OpenBSD clamps the key (password + terminating null) to this size
#define COMS_BCRYPT_MAX_KEY_LENGTH 72

#define COMS_BCRYPT_MIN_COST 4
#define COMS_BCRYPT_MAX_COST 31

// Costs above this value are rejected during verification since a malicious
// hash could otherwise be used to block a worker thread for minutes.
#define COMS_BCRYPT_MAX_VERIFY_COST 16

#define COMS_BCRYPT_DEFAULT_COST 10

/**
 * bcrypt uses its own base64 alphabet ('.' and '/' instead of '+' and '-')
 * and no padding.
 */
CONSTEXPR const char COMS_BCRYPT_BASE64_CHARS[64] = {
    '.', '/', 'A', 'B', 'C', 'D', 'E', 'F',
    'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N',
    'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V',
    'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd',
    'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l',
    'm', 'n', 'o', 'p', 'q', 'r', 's', 't',
    'u', 'v', 'w', 'x', 'y', 'z', '0', '1',
    '2', '3', '4', '5', '6', '7', '8', '9',
};

CONSTEXPR const int8 COMS_BCRYPT_BASE64_LOOKUP[128] = {
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  0,  1,
    54, 55, 56, 57, 58, 59, 60, 61, 62, 63, -1, -1, -1, -1, -1, -1,
    -1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15, 16,
    17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, -1, -1, -1, -1, -1,
    -1, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42,
    43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, -1, -1, -1, -1, -1,
};

struct BcryptHash {
    // Character after "$2" ('a', 'b' or 'y')
    byte version;

    // Cost exponent
    byte cost;

    byte salt[COMS_BCRYPT_SALT_LENGTH];

    // The raw hash (not the base64 representation)
    byte hash[COMS_BCRYPT_HASH_BYTES];
};

/**
 * Encode bytes with the bcrypt base64 alphabet (no padding)
 *
 * @param char*       out    Destination (must hold 2 * length + 2 bytes)
 * @param const byte* data   Data to encode
 * @param int32       length Amount of bytes
 *
 * @return int32 Amount of characters written
 */
inline
int32 bcrypt_base64_encode(char* const __restrict out, const byte* const __restrict data, int32 length) NO_EXCEPT
{
    int32 source = 0;
    int32 target = 0;

    while (source < length) {
        uint32 c1 = data[source++];

        out[target++] = COMS_BCRYPT_BASE64_CHARS[(c1 >> 2) & 0x3F];
        c1 = (c1 & 0x03) << 4;

        if (source >= length) {
            out[target++] = COMS_BCRYPT_BASE64_CHARS[c1 & 0x3F];
            break;
        }

        const uint32 c2 = data[source++];
        c1 |= (c2 >> 4) & 0x0F;
        out[target++] = COMS_BCRYPT_BASE64_CHARS[c1 & 0x3F];

        c1 = (c2 & 0x0F) << 2;

        if (source >= length) {
            out[target++] = COMS_BCRYPT_BASE64_CHARS[c1 & 0x3F];
            break;
        }

        const uint32 c3 = data[source++];
        c1 |= (c3 >> 6) & 0x03;
        out[target++] = COMS_BCRYPT_BASE64_CHARS[c1 & 0x3F];
        out[target++] = COMS_BCRYPT_BASE64_CHARS[c3 & 0x3F];
    }

    out[target] = '\0';

    return target;
}

/**
 * Decode a bcrypt base64 string
 *
 * @param byte*       out    Destination buffer
 * @param int32       length Amount of characters to decode
 * @param const char* in     Encoded characters
 *
 * @return int32 Amount of bytes written or -1 on invalid input
 */
inline
int32 bcrypt_base64_decode(
    byte* const __restrict out,
    int32 length,
    const char* const __restrict in
) NO_EXCEPT
{
    uint32 buffer = 0;
    int32 bits = 0;
    int32 target = 0;

    for (int32 i = 0; i < length; ++i) {
        const byte index = (byte) in[i];

        if (index >= 128) {
            return -1;
        }

        const int8 value = COMS_BCRYPT_BASE64_LOOKUP[index];
        if (value < 0) {
            return -1;
        }

        buffer = (buffer << 6) | (uint32) value;
        bits += 6;

        if (bits >= 8) {
            bits -= 8;
            out[target++] = (byte) ((buffer >> bits) & 0xFF);
        }
    }

    return target;
}

/**
 * Parse a bcrypt hash string
 *
 * @param const char* hash   Hash string
 * @param BcryptHash* parsed Parsed hash
 *
 * @return bool True if the hash is valid
 */
inline
bool bcrypt_parse_hash(const char* const __restrict hash, BcryptHash* const __restrict parsed) NO_EXCEPT
{
    if (!hash || !parsed || strlen(hash) != COMS_BCRYPT_HASH_LENGTH) {
        return false;
    }

    if (hash[0] != '$' || hash[1] != '2' || hash[3] != '$' || hash[6] != '$') {
        return false;
    }

    if (hash[2] != 'a' && hash[2] != 'b' && hash[2] != 'y') {
        return false;
    }

    if (hash[4] < '0' || hash[4] > '9' || hash[5] < '0' || hash[5] > '9') {
        return false;
    }

    const int32 cost = (hash[4] - '0') * 10 + (hash[5] - '0');
    if (cost < COMS_BCRYPT_MIN_COST || cost > COMS_BCRYPT_MAX_COST) {
        return false;
    }

    if (bcrypt_base64_decode(parsed->salt, COMS_BCRYPT_SALT_CHARS, hash + 7) != COMS_BCRYPT_SALT_LENGTH) {
        return false;
    }

    if (bcrypt_base64_decode(parsed->hash, COMS_BCRYPT_HASH_CHARS, hash + 7 + COMS_BCRYPT_SALT_CHARS) != COMS_BCRYPT_HASH_BYTES) {
        return false;
    }

    parsed->version = (byte) hash[2];
    parsed->cost = (byte) cost;

    return true;
}

/**
 * Check if a hash needs to be replaced with a hash using a higher cost
 *
 * @param const char* hash          Existing hash
 * @param int32       desired_cost  Cost the application currently uses
 *
 * @return bool True if a rehash is recommended or the hash is invalid
 */
inline
bool bcrypt_needs_rehash(const char* const __restrict hash, int32 desired_cost) NO_EXCEPT
{
    BcryptHash parsed;

    if (!bcrypt_parse_hash(hash, &parsed)) {
        return true;
    }

    return parsed.cost < desired_cost;
}

/**
 * Hash a password with bcrypt and return the raw hash bytes
 *
 * The raw 23 bytes are the actual bcrypt output before the base64 encoding.
 * They are used as a symmetric key for the login challenge-response, so the
 * client and the server can prove knowledge of the password without sending
 * the password (or the hash string) over the network.
 *
 * @param const char* password        Password (not null terminated)
 * @param int32       password_length Amount of password bytes
 * @param int32       cost            Cost exponent (COMS_BCRYPT_MIN_COST - COMS_BCRYPT_MAX_COST)
 * @param const byte* salt            16 byte salt
 * @param byte*       out             Output buffer (COMS_BCRYPT_HASH_BYTES bytes)
 *
 * @return bool True on success
 */
inline
bool bcrypt_hash_raw(
    const char* const __restrict password, int32 password_length,
    int32 cost,
    const byte* const __restrict salt,
    byte* const __restrict out
) NO_EXCEPT
{
    if (!password || password_length < 0 || !salt || !out) {
        return false;
    }

    if (cost < COMS_BCRYPT_MIN_COST || cost > COMS_BCRYPT_MAX_COST) {
        return false;
    }

    // bcrypt includes the terminating null in the key and truncates to 72 bytes
    byte key[COMS_BCRYPT_MAX_KEY_LENGTH];
    int32 key_length = password_length + 1;
    if (key_length > COMS_BCRYPT_MAX_KEY_LENGTH) {
        key_length = COMS_BCRYPT_MAX_KEY_LENGTH;
    }

    memcpy(key, password, (size_t) (key_length - 1));
    key[key_length - 1] = 0;

    BlowfishContext ctx;
    blowfish_init_state(&ctx);
    blowfish_expand_state(&ctx, salt, COMS_BCRYPT_SALT_LENGTH, key, key_length);

    for (uint64 i = 0; i < (1ULL << cost); ++i) {
        blowfish_expand_key(&ctx, key, key_length);
        blowfish_expand_key(&ctx, salt, COMS_BCRYPT_SALT_LENGTH);
    }

    // "OrpheanBeholderScryDoubt" as six big endian 32 bit words
    static const byte magic[24] = {
        'O', 'r', 'p', 'h', 'e', 'a', 'n', 'B',
        'e', 'h', 'o', 'l', 'd', 'e', 'r', 'S',
        'c', 'r', 'y', 'D', 'o', 'u', 'b', 't',
    };

    uint32 data[6];
    int32 index = 0;

    for (int32 i = 0; i < 6; ++i) {
        data[i] = blowfish_stream2word(magic, sizeof(magic), &index);
    }

    for (int32 i = 0; i < 64; ++i) {
        for (int32 j = 0; j < 3; ++j) {
            blowfish_encipher(&ctx, &data[j * 2], &data[j * 2 + 1]);
        }
    }

    // bcrypt hashes are 23 bytes (COMS_BCRYPT_HASH_BYTES).
    // The final word only contributes three bytes
    for (int32 i = 0; i < 6; ++i) {
        out[i * 4] = (byte) (data[i] >> 24);

        if (i * 4 + 1 < COMS_BCRYPT_HASH_BYTES) {
            out[i * 4 + 1] = (byte) (data[i] >> 16);
        }

        if (i * 4 + 2 < COMS_BCRYPT_HASH_BYTES) {
            out[i * 4 + 2] = (byte) (data[i] >> 8);
        }

        if (i * 4 + 3 < COMS_BCRYPT_HASH_BYTES) {
            out[i * 4 + 3] = (byte) (data[i]);
        }
    }

    ct_memzero(key, sizeof(key));
    ct_memzero(&ctx, sizeof(ctx));

    return true;
}

/**
 * Hash a password with bcrypt
 *
 * @param const char* password        Password (not null terminated)
 * @param int32       password_length Amount of password bytes
 * @param int32       cost            Cost exponent (COMS_BCRYPT_MIN_COST - COMS_BCRYPT_MAX_COST)
 * @param const byte* salt            16 byte salt
 * @param char*       out             Output buffer (COMS_BCRYPT_HASH_LENGTH + 1 bytes)
 *
 * @return bool True on success
 */
inline
bool bcrypt_hash_password(
    const char* const __restrict password, int32 password_length,
    int32 cost,
    const byte* const __restrict salt,
    char* const __restrict out
) NO_EXCEPT
{
    if (!out) {
        return false;
    }

    byte raw[COMS_BCRYPT_HASH_BYTES];
    if (!bcrypt_hash_raw(password, password_length, cost, salt, raw)) {
        return false;
    }

    char encoded_hash[COMS_BCRYPT_HASH_CHARS + 1];
    char encoded_salt[COMS_BCRYPT_SALT_CHARS + 1];

    bcrypt_base64_encode(encoded_salt, salt, COMS_BCRYPT_SALT_LENGTH);
    bcrypt_base64_encode(encoded_hash, raw, COMS_BCRYPT_HASH_BYTES);

    out[0] = '$';
    out[1] = '2';
    out[2] = 'y';
    out[3] = '$';
    out[4] = (char) ('0' + cost / 10);
    out[5] = (char) ('0' + cost % 10);
    out[6] = '$';

    memcpy(out + 7, encoded_salt, COMS_BCRYPT_SALT_CHARS);
    memcpy(out + 7 + COMS_BCRYPT_SALT_CHARS, encoded_hash, COMS_BCRYPT_HASH_CHARS);
    out[COMS_BCRYPT_HASH_LENGTH] = '\0';

    ct_memzero(raw, sizeof(raw));

    return true;
}

/**
 * Verify a password against a bcrypt hash
 *
 * @param const char* password        Password (not null terminated)
 * @param int32       password_length Amount of password bytes
 * @param const char* hash            bcrypt hash
 *
 * @return bool True if the password matches
 */
inline
bool bcrypt_verify(
    const char* const __restrict password,
    int32 password_length,
    const char* const __restrict hash
) NO_EXCEPT
{
    BcryptHash parsed;

    if (!bcrypt_parse_hash(hash, &parsed)) {
        return false;
    }

    // A malicious hash must not be able to block a worker thread for minutes
    if (parsed.cost > COMS_BCRYPT_MAX_VERIFY_COST) {
        return false;
    }

    char candidate[COMS_BCRYPT_HASH_LENGTH + 1];
    if (!bcrypt_hash_password(password, password_length, parsed.cost, parsed.salt, candidate)) {
        return false;
    }

    // Compare only the hash part, the prefix/salt are equal by construction
    const bool valid = ct_memcmp(
        candidate + 7 + COMS_BCRYPT_SALT_CHARS,
        hash + 7 + COMS_BCRYPT_SALT_CHARS,
        COMS_BCRYPT_HASH_CHARS
    );

    ct_memzero(candidate, sizeof(candidate));

    return valid;
}

/**
 * Verify a null terminated password against a bcrypt hash
 *
 * @param const char* password Password
 * @param const char* hash     bcrypt hash
 *
 * @return bool True if the password matches
 */
inline
bool bcrypt_verify(const char* const __restrict password, const char* const __restrict hash) NO_EXCEPT
{
    return bcrypt_verify(password, (int32) strlen(password), hash);
}

/**
 * Get the cost of a bcrypt hash
 *
 * @param const char* hash bcrypt hash
 *
 * @return int32 Cost or 0 if the hash is invalid
 */
inline
int32 bcrypt_cost(const char* const __restrict hash) NO_EXCEPT
{
    BcryptHash parsed;

    return bcrypt_parse_hash(hash, &parsed) ? parsed.cost : 0;
}

#endif
