/**
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_AUTH_AUTH_TOKEN_H
#define COMS_AUTH_AUTH_TOKEN_H

#include "../stdlib/Stdlib.h"
#include "../encoding/Hex.h"
#include "../hash/Hmac.h"
#include "../utils/ConstantTime.h"

/**
 * Authentication token
 *
 * The auth server issues a token after a successful login. The token is handed
 * to the other services (game, chat, ...) which can verify it without a
 * database lookup and without trusting the network:
 *
 *      token = payload || HMAC-SHA256(secret, payload)
 *
 * The payload contains the account, the character (if already selected), the
 * server and the session id. Because the signature is created with a secret
 * shared between the servers, a service can validate the token on its own.
 * The session id additionally allows the auth server to revoke a token before
 * it expires (stateful check through the internal server interface).
 *
 * The payload is serialized explicitly in little endian byte order. Never use
 * the struct memory directly since that would depend on the compiler layout.
 *
 * SECURITY:
 *  - The secret must be at least 32 random bytes and never leave the server
 *    environment (config file with restricted permissions).
 *  - Tokens are bearer credentials. Always use them over an encrypted
 *    transport (DTLS/TLS) or a trusted internal network.
 *  - Expiry is only a fallback. Revocation is done through the session id.
 */

#define AUTH_TOKEN_VERSION 1

#define AUTH_TOKEN_PAYLOAD_SIZE 40
#define AUTH_TOKEN_SIGNATURE_SIZE 32
#define AUTH_TOKEN_SIZE (AUTH_TOKEN_PAYLOAD_SIZE + AUTH_TOKEN_SIGNATURE_SIZE)
#define AUTH_TOKEN_HEX_LENGTH (AUTH_TOKEN_SIZE * 2)

// The token allows the clock of the verifying service to be slightly off
#define AUTH_TOKEN_CLOCK_SKEW_SECONDS 30

// Token flags
#define AUTH_TOKEN_FLAG_CHARACTER_BOUND (1U << 0) // The token is bound to a character
#define AUTH_TOKEN_FLAG_GAME_READY (1U << 1)      // The token can be used to join a game server

// Service mask, describes which services accept the token
#define AUTH_SERVICE_AUTH (1U << 0)
#define AUTH_SERVICE_GAME (1U << 1)
#define AUTH_SERVICE_CHAT (1U << 2)
#define AUTH_SERVICE_ALL 0xFFFFU

struct AuthToken {
    uint8 version;
    uint8 flags;
    uint16 service_mask;

    uint32 account_id;

    // 0 if the token is not bound to a character yet
    uint32 character_id;

    // 0 if the token is not bound to a server yet
    uint32 server_id;

    // Session identifier, also used for revocation
    uint64 session_id;

    uint32 issued_at;  // unix seconds
    uint32 expires_at; // unix seconds

    // Random value which makes every token unique even within the same second
    uint32 nonce;

    uint32 reserved;

    byte signature[AUTH_TOKEN_SIGNATURE_SIZE];
};

/**
 * Write the payload of a token in a canonical (little endian) form
 *
 * @param const AuthToken* token Token
 * @param byte*            out   Output buffer (AUTH_TOKEN_PAYLOAD_SIZE bytes)
 */
inline
void auth_token_serialize_payload(const AuthToken* const __restrict token, byte* const __restrict out) NO_EXCEPT
{
    out[0] = token->version;
    out[1] = token->flags;
    out[2] = (byte) (token->service_mask);
    out[3] = (byte) (token->service_mask >> 8);

    out[4] = (byte) (token->account_id);
    out[5] = (byte) (token->account_id >> 8);
    out[6] = (byte) (token->account_id >> 16);
    out[7] = (byte) (token->account_id >> 24);

    out[8] = (byte) (token->character_id);
    out[9] = (byte) (token->character_id >> 8);
    out[10] = (byte) (token->character_id >> 16);
    out[11] = (byte) (token->character_id >> 24);

    out[12] = (byte) (token->server_id);
    out[13] = (byte) (token->server_id >> 8);
    out[14] = (byte) (token->server_id >> 16);
    out[15] = (byte) (token->server_id >> 24);

    for (int32 i = 0; i < 8; ++i) {
        out[16 + i] = (byte) (token->session_id >> (i * 8));
    }

    out[24] = (byte) (token->issued_at);
    out[25] = (byte) (token->issued_at >> 8);
    out[26] = (byte) (token->issued_at >> 16);
    out[27] = (byte) (token->issued_at >> 24);

    out[28] = (byte) (token->expires_at);
    out[29] = (byte) (token->expires_at >> 8);
    out[30] = (byte) (token->expires_at >> 16);
    out[31] = (byte) (token->expires_at >> 24);

    out[32] = (byte) (token->nonce);
    out[33] = (byte) (token->nonce >> 8);
    out[34] = (byte) (token->nonce >> 16);
    out[35] = (byte) (token->nonce >> 24);

    out[36] = (byte) (token->reserved);
    out[37] = (byte) (token->reserved >> 8);
    out[38] = (byte) (token->reserved >> 16);
    out[39] = (byte) (token->reserved >> 24);
}

/**
 * Read a token payload in canonical (little endian) form
 *
 * @param const byte* in    Payload (AUTH_TOKEN_PAYLOAD_SIZE bytes)
 * @param AuthToken*  token Token to fill
 */
inline
void auth_token_deserialize_payload(const byte* const __restrict in, AuthToken* const __restrict token) NO_EXCEPT
{
    token->version = in[0];
    token->flags = in[1];
    token->service_mask = (uint16) (in[2] | (in[3] << 8));

    token->account_id = (uint32) in[4]
        | ((uint32) in[5] << 8)
        | ((uint32) in[6] << 16)
        | ((uint32) in[7] << 24);

    token->character_id = (uint32) in[8]
        | ((uint32) in[9] << 8)
        | ((uint32) in[10] << 16)
        | ((uint32) in[11] << 24);

    token->server_id = (uint32) in[12]
        | ((uint32) in[13] << 8)
        | ((uint32) in[14] << 16)
        | ((uint32) in[15] << 24);

    token->session_id = 0;
    for (int32 i = 0; i < 8; ++i) {
        token->session_id |= ((uint64) in[16 + i]) << (i * 8);
    }

    token->issued_at = (uint32) in[24]
        | ((uint32) in[25] << 8)
        | ((uint32) in[26] << 16)
        | ((uint32) in[27] << 24);

    token->expires_at = (uint32) in[28]
        | ((uint32) in[29] << 8)
        | ((uint32) in[30] << 16)
        | ((uint32) in[31] << 24);

    token->nonce = (uint32) in[32]
        | ((uint32) in[33] << 8)
        | ((uint32) in[34] << 16)
        | ((uint32) in[35] << 24);

    token->reserved = (uint32) in[36]
        | ((uint32) in[37] << 8)
        | ((uint32) in[38] << 16)
        | ((uint32) in[39] << 24);
}

/**
 * Sign a token
 *
 * @param AuthToken*  token      Token to sign
 * @param const byte* secret     Signing secret
 * @param size_t      secret_size Secret size in bytes
 *
 * @return bool True on success
 */
inline
bool auth_token_sign(
    AuthToken* const __restrict token,
    const byte* const __restrict secret,
    size_t secret_size
) NO_EXCEPT
{
    if (!token || !secret || secret_size < 16) {
        return false;
    }

    byte payload[AUTH_TOKEN_PAYLOAD_SIZE];
    auth_token_serialize_payload(token, payload);

    hmac_sha256(secret, secret_size, payload, sizeof(payload), token->signature);

    ct_memzero(payload, sizeof(payload));

    return true;
}

/**
 * Verify the signature of a token
 *
 * @param const AuthToken* token      Token
 * @param const byte*      secret     Signing secret
 * @param size_t           secret_size Secret size in bytes
 *
 * @return bool True if the signature is valid
 */
inline
bool auth_token_verify(
    const AuthToken* const __restrict token,
    const byte* const __restrict secret,
    size_t secret_size
) NO_EXCEPT
{
    if (!token || !secret || secret_size < 16) {
        return false;
    }

    byte payload[AUTH_TOKEN_PAYLOAD_SIZE];
    auth_token_serialize_payload(token, payload);

    const bool valid = hmac_sha256_verify(
        secret, secret_size,
        payload, sizeof(payload),
        token->signature
    );

    ct_memzero(payload, sizeof(payload));

    return valid;
}

/**
 * Check version and time validity of a signed token
 *
 * The signature itself is not checked here, use auth_token_verify() for that.
 *
 * @param const AuthToken* token Token
 * @param uint64           now   Current unix time in seconds
 *
 * @return bool True if the token can be used at the given time
 */
inline
bool auth_token_is_valid(const AuthToken* const __restrict token, uint64 now) NO_EXCEPT
{
    if (!token || token->version != AUTH_TOKEN_VERSION || token->session_id == 0) {
        return false;
    }

    if (token->issued_at > now + AUTH_TOKEN_CLOCK_SKEW_SECONDS) {
        return false;
    }

    return token->expires_at + AUTH_TOKEN_CLOCK_SKEW_SECONDS >= now;
}

/**
 * Serialize a complete token (payload + signature)
 *
 * @param const AuthToken* token Token
 * @param byte*            out   Output buffer (AUTH_TOKEN_SIZE bytes)
 *
 * @return int32 Amount of bytes written
 */
inline
int32 auth_token_to_bytes(const AuthToken* const __restrict token, byte* const __restrict out) NO_EXCEPT
{
    auth_token_serialize_payload(token, out);
    memcpy(out + AUTH_TOKEN_PAYLOAD_SIZE, token->signature, AUTH_TOKEN_SIGNATURE_SIZE);

    return AUTH_TOKEN_SIZE;
}

/**
 * Deserialize and verify a complete token
 *
 * @param const byte*  data      Serialized token
 * @param size_t       size      Size of the serialized token
 * @param const byte*  secret    Signing secret
 * @param size_t       secret_size Secret size in bytes
 * @param AuthToken*   token     Output token
 *
 * @return bool True if the token is valid
 */
inline
bool auth_token_from_bytes(
    const byte* const __restrict data,
    size_t size,
    const byte* const __restrict secret,
    size_t secret_size,
    AuthToken* const __restrict token
) NO_EXCEPT
{
    if (!data || !token || size != AUTH_TOKEN_SIZE) {
        return false;
    }

    auth_token_deserialize_payload(data, token);
    memcpy(token->signature, data + AUTH_TOKEN_PAYLOAD_SIZE, AUTH_TOKEN_SIGNATURE_SIZE);

    return auth_token_verify(token, secret, secret_size);
}

/**
 * Serialize a token as hex string
 *
 * @param const AuthToken* token      Token
 * @param char*            out        Output buffer (AUTH_TOKEN_HEX_LENGTH + 1 bytes)
 * @param size_t           out_size   Size of the output buffer
 *
 * @return bool True on success
 */
inline
bool auth_token_to_hex(
    const AuthToken* const __restrict token,
    char* const __restrict out,
    size_t out_size
) NO_EXCEPT
{
    if (!token || !out || out_size < AUTH_TOKEN_HEX_LENGTH + 1) {
        return false;
    }

    byte raw[AUTH_TOKEN_SIZE];
    auth_token_to_bytes(token, raw);
    hex_encode(raw, sizeof(raw), out);

    ct_memzero(raw, sizeof(raw));

    return true;
}

/**
 * Parse and verify a hex encoded token
 *
 * @param const char* hex         Hex token (AUTH_TOKEN_HEX_LENGTH characters)
 * @param const byte* secret      Signing secret
 * @param size_t      secret_size Secret size in bytes
 * @param AuthToken*  token       Output token
 *
 * @return bool True if the token is valid
 */
inline
bool auth_token_from_hex(
    const char* const __restrict hex,
    const byte* const __restrict secret,
    size_t secret_size,
    AuthToken* const __restrict token
) NO_EXCEPT
{
    if (!hex || !token || strlen(hex) != AUTH_TOKEN_HEX_LENGTH) {
        return false;
    }

    byte raw[AUTH_TOKEN_SIZE];
    if (hex_decode(hex, AUTH_TOKEN_HEX_LENGTH, raw) != AUTH_TOKEN_SIZE) {
        return false;
    }

    const bool valid = auth_token_from_bytes(raw, sizeof(raw), secret, secret_size, token);
    ct_memzero(raw, sizeof(raw));

    return valid;
}

/**
 * Clear a token so a stolen memory dump cannot reuse it
 *
 * @param AuthToken* token Token to clear
 */
inline
void auth_token_clear(AuthToken* const token) NO_EXCEPT
{
    if (token) {
        ct_memzero(token, sizeof(*token));
    }
}

#endif
