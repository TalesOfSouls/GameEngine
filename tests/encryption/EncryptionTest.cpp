#include "../TestFramework.h"
#include "../../encoding/Hex.h"
#include "../../utils/StringUtils.h"
#include "../../hash/Sha256.h"
#include "../../hash/Hkdf.h"
#include "../../encryption/ChaCha20Poly1305.h"
#include "../../encryption/X25519.h"

/**
 * Encryption tests
 *
 * ChaCha20, Poly1305 and the AEAD are verified against RFC 8439, X25519 against
 * RFC 7748 and HKDF against RFC 5869. The additional AEAD length cases (block
 * boundaries of ChaCha20 and Poly1305) were generated with a reference
 * implementation (python cryptography / OpenSSL).
 */

static int32 encryption_hex_decode(const char* hex, byte* out) {
    return hex_decode(hex, str_length(hex), out);
}

static const char ENCRYPTION_SUNSCREEN[] =
    "Ladies and Gentlemen of the class of '99: If I could offer you only one tip for the future, sunscreen would be it.";

static void test_encryption_chacha20() {
    // RFC 8439 2.4.2
    byte key[32];
    for (int32 i = 0; i < 32; ++i) {
        key[i] = (byte) i;
    }

    byte nonce[12];
    encryption_hex_decode("000000000000004a00000000", nonce);

    byte expected[sizeof(ENCRYPTION_SUNSCREEN)];
    const int32 length = encryption_hex_decode(
        "6e2e359a2568f98041ba0728dd0d6981e97e7aec1d4360c20a27afccfd9fae0bf91b65c5524733ab8f593dabcd62b357"
        "1639d624e65152ab8f530c359f0861d807ca0dbf500d6a6156a38e088a22b65e52bc514d16ccf806818ce91ab7793736"
        "5af90bbf74a35be6b40b8eedf2785e42874d",
        expected
    );

    TEST_EQUALS(length, (int32) sizeof(ENCRYPTION_SUNSCREEN) - 1);

    byte cipher[sizeof(ENCRYPTION_SUNSCREEN)];
    chacha20_xor(key, 1, nonce, (const byte *) ENCRYPTION_SUNSCREEN, cipher, length);
    TEST_MEMORY_EQUALS(cipher, expected, length);

    // Decryption is the same operation (in place)
    chacha20_xor(key, 1, nonce, cipher, cipher, length);
    TEST_MEMORY_EQUALS(cipher, ENCRYPTION_SUNSCREEN, length);
}

static void test_encryption_poly1305() {
    // RFC 8439 2.5.2
    byte key[32];
    encryption_hex_decode("85d6be7857556d337f4452fe42d506a80103808afb0db2fd4abff6af4149f51b", key);

    byte expected[16];
    encryption_hex_decode("a8061dc1305136c6c22b8baf0c0127a9", expected);

    const char message[] = "Cryptographic Forum Research Group";

    byte tag[16];
    poly1305(key, (const byte *) message, sizeof(message) - 1, tag);
    TEST_MEMORY_EQUALS(tag, expected, 16);

    // Streaming with odd chunk sizes gives the same tag
    Poly1305Context ctx;
    poly1305_init(&ctx, key);
    poly1305_update(&ctx, (const byte *) message, 3);
    poly1305_update(&ctx, (const byte *) message + 3, 17);
    poly1305_update(&ctx, (const byte *) message + 20, sizeof(message) - 1 - 20);
    poly1305_final(&ctx, tag);
    TEST_MEMORY_EQUALS(tag, expected, 16);
}

static void test_encryption_aead_rfc() {
    // RFC 8439 2.8.2
    byte key[32];
    for (int32 i = 0; i < 32; ++i) {
        key[i] = (byte) (0x80 + i);
    }

    byte nonce[12];
    encryption_hex_decode("070000004041424344454647", nonce);

    byte aad[12];
    encryption_hex_decode("50515253c0c1c2c3c4c5c6c7", aad);

    byte expected_cipher[sizeof(ENCRYPTION_SUNSCREEN)];
    const int32 length = encryption_hex_decode(
        "d31a8d34648e60db7b86afbc53ef7ec2a4aded51296e08fea9e2b5a736ee62d63dbea45e8ca9671282fafb69da92728b"
        "1a71de0a9e060b2905d6a5b67ecd3b3692ddbd7f2d778b8c9803aee328091b58fab324e4fad675945585808b4831d7bc"
        "3ff4def08e4b7a9de576d26586cec64b6116",
        expected_cipher
    );

    byte expected_tag[16];
    encryption_hex_decode("1ae10b594f09e26a7e902ecbd0600691", expected_tag);

    byte cipher[sizeof(ENCRYPTION_SUNSCREEN)];
    byte tag[16];
    aead_chacha20_poly1305_seal(key, nonce, aad, sizeof(aad), (const byte *) ENCRYPTION_SUNSCREEN, length, cipher, tag);

    TEST_MEMORY_EQUALS(cipher, expected_cipher, length);
    TEST_MEMORY_EQUALS(tag, expected_tag, 16);

    byte plain[sizeof(ENCRYPTION_SUNSCREEN)];
    TEST_TRUE(aead_chacha20_poly1305_open(key, nonce, aad, sizeof(aad), cipher, length, tag, plain));
    TEST_MEMORY_EQUALS(plain, ENCRYPTION_SUNSCREEN, length);

    // Any modification is detected and the output is not written
    memset(plain, 0xAA, sizeof(plain));

    cipher[5] ^= 0x01;
    TEST_FALSE(aead_chacha20_poly1305_open(key, nonce, aad, sizeof(aad), cipher, length, tag, plain));
    TEST_EQUALS(plain[0], 0xAA);
    cipher[5] ^= 0x01;

    aad[0] ^= 0x01;
    TEST_FALSE(aead_chacha20_poly1305_open(key, nonce, aad, sizeof(aad), cipher, length, tag, plain));
    aad[0] ^= 0x01;

    tag[15] ^= 0x80;
    TEST_FALSE(aead_chacha20_poly1305_open(key, nonce, aad, sizeof(aad), cipher, length, tag, plain));
    tag[15] ^= 0x80;

    nonce[11] ^= 0x01;
    TEST_FALSE(aead_chacha20_poly1305_open(key, nonce, aad, sizeof(aad), cipher, length, tag, plain));
    nonce[11] ^= 0x01;

    TEST_TRUE(aead_chacha20_poly1305_open(key, nonce, aad, sizeof(aad), cipher, length, tag, plain));
}

static void test_encryption_aead_lengths() {
    struct {
        int32 length;
        int32 aad_length;
        const char* tag;
        const char* cipher_sha256;
    } cases[] = {
        {0, 0, "a0784d7a4716f3feb4f64e7f4b39bf04", "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"},
        {1, 0, "aeb2d772ded361e89a5fd667aeadecd2", "35af2d15ebde4d67b29569ec19749e813c76f02286afc5a2a9871c2bfbdaab32"},
        {15, 3, "42da23a71f6cee5dbec129e5741418d5", "4239432e8a5ddae245f364ee3485b00eb130ac444e4eb38b0241d630cc8576b4"},
        {16, 16, "d590275731768f4da09b7dd7ceca2824", "c34d63c90d47b89d0c196c69132b622bec4767534a4a3c664b9a697fb5bc0e79"},
        {17, 1, "8a12daf6ea3ee419ef6937a1a52fb399", "35abda89265008d8eb3dc4bf0094736a4d70e566e771a6efd1a25fdba23efc47"},
        {63, 12, "2c9ebdb4cd9d7a9ad06f731125d965a1", "71d68d80796c08a4361bb62f14ce280f3b7dd30ef02c422bf0ffc44c63b7ff7c"},
        {64, 24, "6d5ce6317d3651c45594d51832879c1e", "fc9d41b8ab18827dbc73fb456c9aff932e6adcd26fe7784c8d568ac22b0e1735"},
        {65, 0, "bc8b16a1e7a2058117375d659bf2f32c", "a54dc012c4090324ad3df670e89c027c07059952213a7ae375ca3b88f849c4ed"},
        {255, 24, "477c45793b98c4b73366b80444a5c2dc", "83796d44f8b47517286f564626b34cba68882fedf9076dcb41d85d1f6c569534"},
        {1360, 24, "78fca9d0b21648f280deba5a4991aa81", "a6972e844f0550e08f0bc64280c1bdc0230dc9b8b84cbcb37f1d435298dc0229"},
    };

    byte key[32];
    for (int32 i = 0; i < 32; ++i) {
        key[i] = (byte) (0x80 + i);
    }

    byte nonce[12];
    encryption_hex_decode("070000004041424344454647", nonce);

    for (int32 c = 0; c < (int32) ARRAY_COUNT(cases); ++c) {
        byte plain[1360];
        byte aad[24];
        byte cipher[1360];
        byte tag[16];

        for (int32 i = 0; i < cases[c].length; ++i) {
            plain[i] = (byte) ((i * 7 + cases[c].length) & 255);
        }

        for (int32 i = 0; i < cases[c].aad_length; ++i) {
            aad[i] = (byte) ((i * 13 + 1) & 255);
        }

        aead_chacha20_poly1305_seal(key, nonce, aad, cases[c].aad_length, plain, cases[c].length, cipher, tag);

        byte expected_tag[16];
        encryption_hex_decode(cases[c].tag, expected_tag);
        TEST_MEMORY_EQUALS(tag, expected_tag, 16);

        byte digest[32];
        byte expected_digest[32];
        sha256_hash(cipher, cases[c].length, digest);
        encryption_hex_decode(cases[c].cipher_sha256, expected_digest);
        TEST_MEMORY_EQUALS(digest, expected_digest, 32);

        byte decrypted[1360];
        TEST_TRUE(aead_chacha20_poly1305_open(key, nonce, aad, cases[c].aad_length, cipher, cases[c].length, tag, decrypted));
        TEST_MEMORY_EQUALS(decrypted, plain, cases[c].length);
    }
}

static void test_encryption_x25519() {
    // RFC 7748 6.1
    byte alice_private[32];
    byte bob_private[32];
    byte expected_alice_public[32];
    byte expected_bob_public[32];
    byte expected_shared[32];

    encryption_hex_decode("77076d0a7318a57d3c16c17251b26645df4c2f87ebc0992ab177fba51db92c2a", alice_private);
    encryption_hex_decode("5dab087e624a8a4b79e17f8b83800ee66f3bb1292618b6fd1c2f8b27ff88e0eb", bob_private);
    encryption_hex_decode("8520f0098930a754748b7ddcb43ef75a0dbf3a0d26381af4eba4a98eaa9b4e6a", expected_alice_public);
    encryption_hex_decode("de9edb7d7b7dc1b4d35b61c2ece435373f8343c85b78674dadfc7e146f882b4f", expected_bob_public);
    encryption_hex_decode("4a5d9d5ba4ce2de1728e3bf480350f25e07e21c947d19e3376f09b3c1e161742", expected_shared);

    byte alice_public[32];
    byte bob_public[32];
    x25519_public_key(alice_public, alice_private);
    x25519_public_key(bob_public, bob_private);

    TEST_MEMORY_EQUALS(alice_public, expected_alice_public, 32);
    TEST_MEMORY_EQUALS(bob_public, expected_bob_public, 32);

    byte shared_alice[32];
    byte shared_bob[32];
    TEST_TRUE(x25519_shared(shared_alice, alice_private, bob_public));
    TEST_TRUE(x25519_shared(shared_bob, bob_private, alice_public));

    TEST_MEMORY_EQUALS(shared_alice, expected_shared, 32);
    TEST_MEMORY_EQUALS(shared_bob, expected_shared, 32);

    // Low order point (0) -> all zero result is rejected
    byte zero_point[32] = {0};
    byte shared[32];
    TEST_FALSE(x25519_shared(shared, alice_private, zero_point));
}

static void test_encryption_x25519_iterated() {
    // RFC 7748 5.2: k = u = 9, after one iteration
    byte k[32] = {9};
    byte u[32] = {9};
    byte result[32];

    x25519(result, k, u);

    byte expected[32];
    encryption_hex_decode("422c8e7a6227d7bca1350b3e2bb7279f7897b87bb6854b783c60e80311ae3079", expected);
    TEST_MEMORY_EQUALS(result, expected, 32);

    // After 1000 iterations
    for (int32 i = 1; i < 1000; ++i) {
        memcpy(u, k, 32);
        memcpy(k, result, 32);
        x25519(result, k, u);
    }

    encryption_hex_decode("684cf59ba83309552800ef566f2f4d3c1c3887c49360e3875f2eb94d99532c51", expected);
    TEST_MEMORY_EQUALS(result, expected, 32);
}

static void test_encryption_hkdf() {
    // RFC 5869 A.1
    byte ikm[22];
    memset(ikm, 0x0b, sizeof(ikm));

    byte salt[13];
    for (int32 i = 0; i < 13; ++i) {
        salt[i] = (byte) i;
    }

    byte info[10];
    for (int32 i = 0; i < 10; ++i) {
        info[i] = (byte) (0xf0 + i);
    }

    byte expected[42];
    encryption_hex_decode("3cb25f25faacd57a90434f64d0362f2a2d2d0a90cf1a5a4c5db02d56ecc4c5bf34007208d5b887185865", expected);

    byte okm[42];
    TEST_TRUE(hkdf_sha256(salt, sizeof(salt), ikm, sizeof(ikm), info, sizeof(info), okm, sizeof(okm)));
    TEST_MEMORY_EQUALS(okm, expected, sizeof(okm));

    // Too much output is rejected
    byte large[8];
    byte prk[32] = {0};
    TEST_FALSE(hkdf_sha256_expand(prk, NULL, 0, large, 255 * 32 + 1));
}

#ifdef UBER_TEST
    #ifdef main
        #undef main
    #endif
    #define main EncryptionEncryptionTest
#endif

int main() {
    TEST_INIT(128);

    TEST_RUN(test_encryption_chacha20);
    TEST_RUN(test_encryption_poly1305);
    TEST_RUN(test_encryption_aead_rfc);
    TEST_RUN(test_encryption_aead_lengths);
    TEST_RUN(test_encryption_x25519);
    TEST_RUN(test_encryption_x25519_iterated);
    TEST_RUN(test_encryption_hkdf);

    TEST_FINALIZE();

    return 0;
}
