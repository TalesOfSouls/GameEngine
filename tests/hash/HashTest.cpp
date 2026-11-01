#include "../TestFramework.h"
#include "../../utils/StringUtils.h"
#include "../../hash/Crc.h"
#include "../../hash/GeneralHash.h"
#include "../../hash/Sha1.h"
#include "../../hash/Sha256.h"
#include "../../hash/Hmac.h"

/**
 * Hash tests (@see hash/)
 *
 * The cryptographic hashes are verified against the official/reference test vectors
 * (FIPS 180-1, FIPS 180-4, RFC 4231) and the general purpose hashes are verified
 * with property based checks (determinism, sensitivity, distribution).
 */

static void hash_to_hex(const byte* digest, int32 length, char* out) {
    static const char hex[] = "0123456789abcdef";
    for (int32 i = 0; i < length; ++i) {
        out[i * 2] = hex[digest[i] >> 4];
        out[i * 2 + 1] = hex[digest[i] & 0x0F];
    }

    out[length * 2] = '\0';
}

static void test_crc32_software() {
    // Standard CRC-32 (ISO-HDLC) check value
    const char* const check = "123456789";
    const uint32 expected = 0xCBF43926;

    TEST_EQUALS(crc32_software_u8((const byte *) check, 9), expected);
    TEST_EQUALS(crc32_software_u8_table((const uint8 *) check, 9), expected);

    // Empty input
    TEST_EQUALS(crc32_software_u8((const byte *) "", 0), 0x00000000);
    TEST_EQUALS(crc32_software_u8_table((const uint8 *) "", 0), 0x00000000);

    // Both implementations have to agree for arbitrary data
    const byte data[] = "The quick brown fox jumps over the lazy dog";
    TEST_EQUALS(
        crc32_software_u8(data, sizeof(data) - 1),
        crc32_software_u8_table(data, (uint32) (sizeof(data) - 1))
    );

    // Single bit change -> different checksum
    byte modified[sizeof(data)];
    memcpy(modified, data, sizeof(data));
    modified[0] ^= 0x01;
    TEST_NOT_EQUALS(crc32_software_u8(data, sizeof(data) - 1), crc32_software_u8(modified, sizeof(data) - 1));

    // The lookup table is the well known CRC-32 table
    TEST_EQUALS(crc_table[0], 0x00000000);
    TEST_EQUALS(crc_table[1], 0x77073096);
    TEST_EQUALS(crc_table[255], 0x2D02EF8D);

    // A generated table has to be identical to the static table
    uint32 generated[256];
    crc32_table_fill(generated);
    TEST_MEMORY_EQUALS(generated, crc_table, sizeof(crc_table));
}

// crc32_intrin_* uses the CRC32C instruction (Castagnoli), so it is NOT interchangeable
// with crc32_software_* (CRC-32). The different word sizes however have to agree.
static void test_crc32_intrin_consistency() {
    const char* const check = "123456789";

    // CRC-32C reference check value (RFC 3720)
    TEST_EQUALS(crc32_intrin_u8((const byte *) check, 9), 0xE3069283);

    // ... and it is a different polynomial than the software CRC-32
    TEST_NOT_EQUALS(crc32_intrin_u8((const byte *) check, 9), crc32_software_u8((const byte *) check, 9));

    const byte data[] = "12345678901234567890123456789012"; // 32 bytes

    const uint32 crc_u8 = crc32_intrin_u8(data, sizeof(data) - 1);
    const uint32 crc_u16 = crc32_intrin_u16((const uint16 *) data, (sizeof(data) - 1) / 2);
    const uint32 crc_u32 = crc32_intrin_u32((const uint32 *) data, (sizeof(data) - 1) / 4);
    const uint32 crc_u64 = crc32_intrin_u64((const uint64 *) data, (sizeof(data) - 1) / 8);

    TEST_EQUALS(crc_u8, crc_u16);
    TEST_EQUALS(crc_u8, crc_u32);
    TEST_EQUALS(crc_u8, crc_u64);

    // Deterministic
    TEST_EQUALS(crc32_intrin_u8(data, sizeof(data) - 1), crc_u8);

    // Different data -> different checksum
    TEST_NOT_EQUALS(crc_u8, crc32_intrin_u8(data, sizeof(data) - 2));
}

static void test_hash_downscale_64() {
    // Murmur3 fmix64 is deterministic and avalanche
    TEST_EQUALS(hash_downscale_64(0), hash_downscale_64(0));
    TEST_EQUALS(hash_downscale_64(1234), hash_downscale_64(1234));

    TEST_NOT_EQUALS(hash_downscale_64(0), hash_downscale_64(1));
    TEST_NOT_EQUALS(hash_downscale_64(1), hash_downscale_64(2));
    TEST_NOT_EQUALS(hash_downscale_64(0), hash_downscale_64(1ULL << 32));

    // hash_int64 is the identity of the pointer/value
    int32 value = 42;
    TEST_EQUALS(hash_int64(&value), (uint64) (uintptr_t) &value);
}

static void test_general_hash_strings() {
    // Deterministic
    TEST_EQUALS(hash_djb2("test"), hash_djb2("test"));
    TEST_EQUALS(hash_sdbm((const byte *) "test"), hash_sdbm((const byte *) "test"));
    TEST_EQUALS(hash_lose_lose((const byte *) "test"), hash_lose_lose((const byte *) "test"));
    TEST_EQUALS(hash_polynomial_rolling("test"), hash_polynomial_rolling("test"));
    TEST_EQUALS(hash_fnv1a("test"), hash_fnv1a("test"));
    TEST_EQUALS(hash_oat("test"), hash_oat("test"));
    TEST_EQUALS(hash_ejb("test"), hash_ejb("test"));

    // Different strings -> different hashes
    const char* const keys[] = {"test", "Test", "tEst", "teSt", "tesT", "tes", "test1", "1test", ""};
    for (int32 i = 0; i < (int32) (sizeof(keys) / sizeof(keys[0])); ++i) {
        for (int32 j = i + 1; j < (int32) (sizeof(keys) / sizeof(keys[0])); ++j) {
            // hash_lose_lose/hash_ejb are weak but still distinguish these keys
            TEST_NOT_EQUALS(hash_djb2(keys[i]), hash_djb2(keys[j]));
            TEST_NOT_EQUALS(hash_fnv1a(keys[i]), hash_fnv1a(keys[j]));
            TEST_NOT_EQUALS(hash_oat(keys[i]), hash_oat(keys[j]));
            TEST_NOT_EQUALS(hash_sdbm((const byte *) keys[i]), hash_sdbm((const byte *) keys[j]));
        }
    }

    // The void* overload has to behave like the string version
    TEST_EQUALS(hash_djb2((const void *) "abc"), hash_djb2("abc"));
}

static void test_general_hash_seeded() {
    // The seeded variants differ from the unseeded ones and are deterministic
    TEST_NOT_EQUALS(hash_djb2_seeded("test", 7), hash_djb2("test"));
    TEST_EQUALS(hash_djb2_seeded("test", 7), hash_djb2_seeded("test", 7));
    TEST_NOT_EQUALS(hash_djb2_seeded("test", 7), hash_djb2_seeded("test", 8));

    TEST_NOT_EQUALS(hash_fnv1a_seeded("test", 7), hash_fnv1a("test"));
    TEST_EQUALS(hash_fnv1a_seeded("test", 7), hash_fnv1a_seeded("test", 7));

    TEST_NOT_EQUALS(hash_sdbm_seeded("test", 7), hash_sdbm((const byte *) "test"));
    TEST_EQUALS(hash_sdbm_seeded("test", 7), hash_sdbm_seeded("test", 7));

    TEST_EQUALS(hash_lose_lose_seeded("test", 3), hash_lose_lose_seeded("test", 3));
    TEST_EQUALS(hash_polynomial_rolling_seeded("test", 3), hash_polynomial_rolling_seeded("test", 3));
    TEST_EQUALS(hash_oat_seeded("test", 3), hash_oat_seeded("test", 3));
    TEST_EQUALS(hash_ejb_seeded("test", 3), hash_ejb_seeded("test", 3));
}

static void test_general_hash_distribution() {
    // 1000 distinct keys should give (almost) 1000 distinct hashes
    char keys[1000][16];
    uint32 hashes[1000];

    for (int32 i = 0; i < 1000; ++i) {
        sprintf_fast(keys[i], (int32) sizeof(keys[i]), "key_%d", i);
        hashes[i] = hash_murmur3_32((const byte *) keys[i], strlen(keys[i]));
    }

    int32 unique = 0;
    for (int32 i = 0; i < 1000; ++i) {
        bool seen = false;
        for (int32 j = 0; j < i; ++j) {
            seen = seen || hashes[i] == hashes[j];
        }

        unique += !seen;
    }

    TEST_GREATER_THAN(unique, 990);

    // 1000 * 16 bit range -> the buckets should be reasonably filled
    int32 buckets[16] = {0};
    for (int32 i = 0; i < 1000; ++i) {
        ++buckets[hashes[i] & 15];
    }

    for (int32 i = 0; i < 16; ++i) {
        TEST_GREATER_THAN(buckets[i], 20);
        TEST_LESSER_THAN(buckets[i], 130);
    }
}

static void test_hash_murmur3() {
    // Every length (including the tail handling 0-15 bytes) has to work and differ
    const char* const text = "0123456789ABCDEF";
    uint32 previous = 0xFFFFFFFF;
    for (int32 length = 0; length <= 16; ++length) {
        const uint32 hash = hash_murmur3_32((const byte *) text, length);
        TEST_NOT_EQUALS(hash, previous);
        previous = hash;
    }

    // Seed changes the result
    TEST_NOT_EQUALS(
        hash_murmur3_32((const byte *) "test", 4, 0),
        hash_murmur3_32((const byte *) "test", 4, 1)
    );

    // Same input -> same hash
    TEST_EQUALS(
        hash_murmur3_32((const byte *) "test", 4, 123),
        hash_murmur3_32((const byte *) "test", 4, 123)
    );

    // 64 bit version
    TEST_EQUALS(hash_murmur3_64("test", 4), hash_murmur3_64("test", 4));
    TEST_NOT_EQUALS(hash_murmur3_64("test", 4), hash_murmur3_64("test", 5));
}

static void test_sha1_vectors() {
    byte digest[20];
    char hex[41];

    sha1_hash((const byte *) "", 0, digest);
    hash_to_hex(digest, 20, hex);
    TEST_EQUALS(strcmp(hex, "da39a3ee5e6b4b0d3255bfef95601890afd80709"), 0);

    sha1_hash((const byte *) "abc", 3, digest);
    hash_to_hex(digest, 20, hex);
    TEST_EQUALS(strcmp(hex, "a9993e364706816aba3e25717850c26c9cd0d89d"), 0);

    const char* const long_message = "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq";
    sha1_hash((const byte *) long_message, strlen(long_message), digest);
    hash_to_hex(digest, 20, hex);
    TEST_EQUALS(strcmp(hex, "84983e441c3bd26ebaae4aa1f95129e5e54670f1"), 0);
}

static void test_sha1_streaming() {
    const char* const message = "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq";
    const size_t length = strlen(message);

    byte one_shot[20];
    sha1_hash((const byte *) message, length, one_shot);

    // Streaming in many small chunks has to give the same digest
    Sha1Context ctx;
    sha1_init(&ctx);
    for (size_t i = 0; i < length; i += 7) {
        const size_t chunk = OMS_MIN((size_t) 7, length - i);
        sha1_update(&ctx, (const byte *) message + i, chunk, 8);
    }

    byte streamed[20];
    sha1_final(&ctx, streamed, 8);

    TEST_MEMORY_EQUALS(streamed, one_shot, 20);

    // A modified message gives a different digest
    byte modified[20];
    sha1_hash((const byte *) "abd", 3, modified);
    byte original[20];
    sha1_hash((const byte *) "abc", 3, original);
    TEST_TRUE(memcmp(modified, original, 20) != 0);
}

static void test_sha256_vectors() {
    byte digest[32];
    char hex[65];

    sha256_hash((const byte *) "", 0, digest);
    hash_to_hex(digest, 32, hex);
    TEST_EQUALS(strcmp(hex, "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"), 0);

    sha256_hash((const byte *) "abc", 3, digest);
    hash_to_hex(digest, 32, hex);
    TEST_EQUALS(strcmp(hex, "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad"), 0);

    const char* const long_message = "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq";
    sha256_hash((const byte *) long_message, strlen(long_message), digest);
    hash_to_hex(digest, 32, hex);
    TEST_EQUALS(strcmp(hex, "248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1"), 0);
}

static void test_sha256_streaming() {
    const char* const message = "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq";
    const size_t length = strlen(message);

    byte one_shot[32];
    sha256_hash((const byte *) message, length, one_shot);

    Sha256Context ctx;
    sha256_init(&ctx);
    for (size_t i = 0; i < length; i += 5) {
        const size_t chunk = OMS_MIN((size_t) 5, length - i);
        sha256_update(&ctx, (const byte *) message + i, chunk);
    }

    byte streamed[32];
    sha256_final(&ctx, streamed);

    TEST_MEMORY_EQUALS(streamed, one_shot, 32);

    // The helper functions match the round/sigma definitions
    TEST_EQUALS(sha256_rotr(0x80000000, 1), 0x40000000);
    TEST_EQUALS(sha256_rotr(0x00000001, 1), 0x80000000);
    // ch(x, y, z) = (x & y) ^ (~x & z), maj(x, y, z) = (x & y) ^ (x & z) ^ (y & z)
    TEST_EQUALS(sha256_ch(0xFFFFFFFF, 0x0F0F0F0F, 0x00FF00FF), 0x0F0F0F0F);
    TEST_EQUALS(sha256_ch(0x00000000, 0x0F0F0F0F, 0x00FF00FF), 0x00FF00FF);
    TEST_EQUALS(sha256_maj(0xFF00FF00, 0x0FF00FF0, 0x00FFFFFF), 0x0FF0FFF0);
}

static void test_hmac_sha256_vectors() {
    byte digest[HMAC_SHA256_DIGEST_SIZE];
    char hex[HMAC_SHA256_DIGEST_SIZE * 2 + 1];

    // RFC 4231 test case 1
    byte key1[20];
    memset(key1, 0x0b, sizeof(key1));
    hmac_sha256(key1, sizeof(key1), "Hi There", 8, digest);
    hash_to_hex(digest, HMAC_SHA256_DIGEST_SIZE, hex);
    TEST_EQUALS(strcmp(hex, "b0344c61d8db38535ca8afceaf0bf12b881dc200c9833da726e9376c2e32cff7"), 0);

    // RFC 4231 test case 2
    const char* const key2 = "Jefe";
    const char* const data2 = "what do ya want for nothing?";
    hmac_sha256((const byte *) key2, strlen(key2), data2, strlen(data2), digest);
    hash_to_hex(digest, HMAC_SHA256_DIGEST_SIZE, hex);
    TEST_EQUALS(strcmp(hex, "5bdcc146bf60754e6a042426089575c75a003f089d2739839dec58b964ec3843"), 0);

    // RFC 4231 test case 3
    byte key3[20];
    byte data3[50];
    memset(key3, 0xaa, sizeof(key3));
    memset(data3, 0xdd, sizeof(data3));
    hmac_sha256(key3, sizeof(key3), data3, sizeof(data3), digest);
    hash_to_hex(digest, HMAC_SHA256_DIGEST_SIZE, hex);
    TEST_EQUALS(strcmp(hex, "773ea91e36800e46854db8ebd09181a72959098b3ef8c122d9635514ced565fe"), 0);
}

static void test_hmac_sha256_streaming_and_verify() {
    const byte* const key = (const byte *) "super secret key";
    const size_t key_size = strlen((const char *) key);
    const char* const data = "header.payload";

    byte one_shot[HMAC_SHA256_DIGEST_SIZE];
    hmac_sha256(key, key_size, data, strlen(data), one_shot);

    // Streaming (header + payload, like the API description suggests)
    HmacSha256Context ctx;
    hmac_sha256_init(&ctx, key, key_size);
    hmac_sha256_update(&ctx, "header.", 7);
    hmac_sha256_update(&ctx, "payload", 7);
    byte streamed[HMAC_SHA256_DIGEST_SIZE];
    hmac_sha256_final(&ctx, streamed);

    TEST_MEMORY_EQUALS(streamed, one_shot, HMAC_SHA256_DIGEST_SIZE);

    // Verification
    TEST_TRUE(hmac_sha256_verify(key, key_size, data, strlen(data), one_shot));

    byte tampered[HMAC_SHA256_DIGEST_SIZE];
    memcpy(tampered, one_shot, sizeof(tampered));
    tampered[0] ^= 0x80;
    TEST_FALSE(hmac_sha256_verify(key, key_size, data, strlen(data), tampered));

    // A different key must not verify
    const byte* const wrong_key = (const byte *) "super secret keY";
    TEST_FALSE(hmac_sha256_verify(wrong_key, key_size, data, strlen(data), one_shot));

    // A key longer than the block size is hashed first (RFC 2104), must not crash
    byte long_key[HMAC_SHA256_BLOCK_SIZE * 3];
    memset(long_key, 0x5A, sizeof(long_key));
    byte long_digest[HMAC_SHA256_DIGEST_SIZE];
    hmac_sha256(long_key, sizeof(long_key), data, strlen(data), long_digest);
    TEST_TRUE(memcmp(long_digest, one_shot, sizeof(long_digest)) != 0);
}

#ifdef UBER_TEST
    #ifdef main
        #undef main
    #endif
    #define main HashUtilsTest
#endif

int main() {
    TEST_INIT(128);

    TEST_RUN(test_crc32_software);
    TEST_RUN(test_crc32_intrin_consistency);
    TEST_RUN(test_hash_downscale_64);
    TEST_RUN(test_general_hash_strings);
    TEST_RUN(test_general_hash_seeded);
    TEST_RUN(test_general_hash_distribution);
    TEST_RUN(test_hash_murmur3);
    TEST_RUN(test_sha1_vectors);
    TEST_RUN(test_sha1_streaming);
    TEST_RUN(test_sha256_vectors);
    TEST_RUN(test_sha256_streaming);
    TEST_RUN(test_hmac_sha256_vectors);
    TEST_RUN(test_hmac_sha256_streaming_and_verify);

    TEST_FINALIZE();

    return 0;
}
