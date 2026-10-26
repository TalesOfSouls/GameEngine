/**
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_AUTH_SESSION_REGISTRY_H
#define COMS_AUTH_SESSION_REGISTRY_H

#include "../stdlib/Stdlib.h"
#include "../hash/GeneralHash.h"
#include "AuthToken.h"

/**
 * In-memory authentication session registry
 *
 * Tokens are stateless, so the auth server needs a place to remember which
 * sessions are still alive. The registry is a fixed capacity hash table with
 * open addressing and round-robin eviction. It never allocates and is safe to
 * query from multiple threads as long as modifications are serialized by the
 * caller (the auth server does this with a spinlock).
 *
 * The registry also implements the "one session per account" policy: creating
 * a session for an account that already has one revokes the old session. This
 * prevents account sharing and makes a new login kick the old client.
 *
 * Sessions must be refreshed (touch) by the owning service, otherwise they
 * expire after the configured idle timeout.
 */

enum AuthSessionState : uint8 {
    AUTH_SESSION_STATE_FREE = 0,
    AUTH_SESSION_STATE_ACTIVE = 1,
    AUTH_SESSION_STATE_REVOKED = 2,
};

struct AuthSession {
    uint64 session_id;

    uint32 account_id;
    uint32 character_id;
    uint32 server_id;

    uint32 created_at;
    uint32 expires_at;
    uint32 last_seen;

    uint32 flags;
    uint16 service_mask;

    // AuthAccountRole of the account
    uint8 role;

    uint8 state;

    // Account name for logging and for services that don't want a DB lookup
    char account_name[40];

    // Client address (IPv6, IPv4 is stored as IPv4 mapped address)
    byte ip[16];

    // Character name (filled when the character is bound)
    char character_name[40];
};

struct AuthSessionRegistry {
    AuthSession* sessions;

    // Always a power of two
    uint32 capacity;
    uint32 mask;

    uint32 count;

    // Round robin position used for eviction
    uint32 next_evict;

    uint32 seed;
};

/**
 * Initialize a session registry
 *
 * @param AuthSessionRegistry* registry Registry
 * @param AuthSession*         sessions Session memory
 * @param uint32               capacity Amount of sessions (rounded down to a power of two)
 * @param uint32               seed     Hash seed
 */
inline
void session_registry_init(
    AuthSessionRegistry* const __restrict registry,
    AuthSession* const __restrict sessions,
    uint32 capacity,
    uint32 seed = 0
) NO_EXCEPT
{
    ASSERT_TRUE(registry);
    ASSERT_TRUE(sessions);
    ASSERT_TRUE(capacity >= 8);

    uint32 power_of_two = 8;
    while ((power_of_two << 1) <= capacity) {
        power_of_two <<= 1;
    }

    registry->sessions = sessions;
    registry->capacity = power_of_two;
    registry->mask = power_of_two - 1;
    registry->count = 0;
    registry->next_evict = 0;
    registry->seed = seed;

    memset(sessions, 0, sizeof(AuthSession) * power_of_two);
}

/**
 * Get the amount of active sessions
 *
 * @param const AuthSessionRegistry* registry Registry
 *
 * @return uint32 Active session count
 */
FORCE_INLINE
uint32 session_registry_active_count(const AuthSessionRegistry* const __restrict registry) NO_EXCEPT
{
    return registry->count;
}

/**
 * Find a session slot for a session id
 *
 * @param const AuthSessionRegistry* registry Registry
 * @param uint64                     session_id Session id
 *
 * @return AuthSession* Session or NULL
 */
inline
AuthSession* session_registry_find(
    const AuthSessionRegistry* const __restrict registry,
    uint64 session_id
) NO_EXCEPT
{
    if (!session_id) {
        return NULL;
    }

    const uint32 start = hash_downscale_64(session_id ^ registry->seed) & registry->mask;

    for (uint32 probe = 0; probe < registry->capacity; ++probe) {
        AuthSession* const session = &registry->sessions[(start + probe) & registry->mask];

        if (session->state == AUTH_SESSION_STATE_FREE) {
            return NULL;
        }

        if (session->state == AUTH_SESSION_STATE_ACTIVE && session->session_id == session_id) {
            return session;
        }
    }

    return NULL;
}

/**
 * Find the active session of an account
 *
 * WARNING: This is a linear scan and only intended for the login/session
 *          creation path. Do not call it for every packet.
 *
 * @param const AuthSessionRegistry* registry   Registry
 * @param uint32                     account_id Account id
 *
 * @return AuthSession* Session or NULL
 */
inline
AuthSession* session_registry_find_by_account(
    const AuthSessionRegistry* const __restrict registry,
    uint32 account_id
) NO_EXCEPT
{
    if (!account_id) {
        return NULL;
    }

    for (uint32 i = 0; i < registry->capacity; ++i) {
        AuthSession* const session = &registry->sessions[i];

        if (session->state == AUTH_SESSION_STATE_ACTIVE && session->account_id == account_id) {
            return session;
        }
    }

    return NULL;
}

/**
 * Find the active session of a character
 *
 * @param const AuthSessionRegistry* registry     Registry
 * @param uint32                     character_id Character id
 *
 * @return AuthSession* Session or NULL
 */
inline
AuthSession* session_registry_find_by_character(
    const AuthSessionRegistry* const __restrict registry,
    uint32 character_id
) NO_EXCEPT
{
    if (!character_id) {
        return NULL;
    }

    for (uint32 i = 0; i < registry->capacity; ++i) {
        AuthSession* const session = &registry->sessions[i];

        if (session->state == AUTH_SESSION_STATE_ACTIVE && session->character_id == character_id) {
            return session;
        }
    }

    return NULL;
}

/**
 * Revoke a session
 *
 * @param AuthSessionRegistry* registry   Registry
 * @param uint64               session_id Session id
 *
 * @return bool True if an active session was revoked
 */
inline
bool session_registry_revoke(
    AuthSessionRegistry* const __restrict registry,
    uint64 session_id
) NO_EXCEPT
{
    AuthSession* const session = session_registry_find(registry, session_id);
    if (!session) {
        return false;
    }

    session->state = AUTH_SESSION_STATE_REVOKED;

    if (registry->count) {
        --registry->count;
    }

    return true;
}

/**
 * Create a session
 *
 * @param AuthSessionRegistry* registry     Registry
 * @param uint64               session_id   Random session id (must not be 0)
 * @param uint32               account_id   Account id
 * @param uint32               now          Current unix time in seconds
 * @param uint32               lifetime     Session lifetime in seconds
 *
 * @return AuthSession* Created session or NULL if the registry is unusable
 */
inline
AuthSession* session_registry_create(
    AuthSessionRegistry* const __restrict registry,
    uint64 session_id,
    uint32 account_id,
    uint32 now,
    uint32 lifetime
) NO_EXCEPT
{
    if (!session_id || !account_id || !lifetime) {
        return NULL;
    }

    // One session per account: kick the old client
    AuthSession* const existing = session_registry_find_by_account(registry, account_id);
    if (existing) {
        session_registry_revoke(registry, existing->session_id);
    }

    const uint32 start = hash_downscale_64(session_id ^ registry->seed) & registry->mask;
    AuthSession* session = NULL;

    for (uint32 probe = 0; probe < registry->capacity; ++probe) {
        AuthSession* const candidate = &registry->sessions[(start + probe) & registry->mask];

        if (candidate->state != AUTH_SESSION_STATE_ACTIVE) {
            session = candidate;
            break;
        }
    }

    if (!session) {
        // Table is full: evict the next slot round-robin
        session = &registry->sessions[registry->next_evict & registry->mask];
        registry->next_evict = (registry->next_evict + 1) & registry->mask;

        if (session->state == AUTH_SESSION_STATE_ACTIVE && registry->count) {
            --registry->count;
        }
    }

    memset(session, 0, sizeof(*session));

    session->session_id = session_id;
    session->account_id = account_id;
    session->created_at = now;
    session->expires_at = now + lifetime;
    session->last_seen = now;
    session->service_mask = AUTH_SERVICE_ALL;
    session->state = AUTH_SESSION_STATE_ACTIVE;

    ++registry->count;

    return session;
}

/**
 * Refresh a session (sliding expiration)
 *
 * @param AuthSessionRegistry* registry   Registry
 * @param uint64               session_id Session id
 * @param uint32               now        Current unix time in seconds
 * @param uint32               lifetime   New lifetime in seconds (0 keeps the old expiry)
 *
 * @return bool True if the session is active
 */
inline
bool session_registry_touch(
    AuthSessionRegistry* const __restrict registry,
    uint64 session_id,
    uint32 now,
    uint32 lifetime = 0
) NO_EXCEPT
{
    AuthSession* const session = session_registry_find(registry, session_id);
    if (!session) {
        return false;
    }

    session->last_seen = now;

    if (lifetime) {
        session->expires_at = now + lifetime;
    }

    return true;
}

/**
 * Remove all expired sessions
 *
 * @param AuthSessionRegistry* registry Registry
 * @param uint32               now      Current unix time in seconds
 *
 * @return int32 Amount of expired sessions
 */
inline
int32 session_registry_expire(
    AuthSessionRegistry* const __restrict registry,
    uint32 now
) NO_EXCEPT
{
    int32 expired = 0;

    for (uint32 i = 0; i < registry->capacity; ++i) {
        AuthSession* const session = &registry->sessions[i];

        if (session->state != AUTH_SESSION_STATE_ACTIVE) {
            continue;
        }

        if (session->expires_at <= now) {
            session->state = AUTH_SESSION_STATE_REVOKED;
            ++expired;

            if (registry->count) {
                --registry->count;
            }
        }
    }

    return expired;
}

/**
 * Copy the account name into a session
 *
 * @param AuthSession* session Session
 * @param const char*  name    Account name
 * @param int32        length  Name length
 */
inline
void session_set_account_name(
    AuthSession* const __restrict session,
    const char* const __restrict name,
    int32 length
) NO_EXCEPT
{
    const int32 copy_length = OMS_MIN(length, (int32) sizeof(session->account_name) - 1);

    memcpy(session->account_name, name, (size_t) copy_length);
    session->account_name[copy_length] = '\0';
}

/**
 * Copy the character name into a session
 *
 * @param AuthSession* session Session
 * @param const char*  name    Character name
 * @param int32        length  Name length
 */
inline
void session_set_character_name(
    AuthSession* const __restrict session,
    const char* const __restrict name,
    int32 length
) NO_EXCEPT
{
    const int32 copy_length = OMS_MIN(length, (int32) sizeof(session->character_name) - 1);

    memcpy(session->character_name, name, (size_t) copy_length);
    session->character_name[copy_length] = '\0';
}

#endif
