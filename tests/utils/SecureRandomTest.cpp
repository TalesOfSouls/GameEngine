#include "../TestFramework.h"
#include "../../utils/SecureRandom.h"

// BCryptGenRandom lives in bcrypt.lib; the single test build script does not
// link it for us, so request it from the translation unit instead
#if defined(_MSC_VER)
    #pragma comment(lib, "bcrypt.lib")
#endif

/**
 * Tests for SecureRandom.h
 *
 * The output is by definition not deterministic, so only success/failure,
 * buffer boundaries and non-constant/non-zero output are asserted.
 */

static void test_secure_random_bytes_invalid()
{
    byte buffer[16] = {};

    TEST_FALSE(secure_random_bytes(NULL, sizeof(buffer)));
    TEST_FALSE(secure_random_bytes(buffer, 0));
    TEST_FALSE(secure_random_bytes(NULL, 0));

    TEST_FALSE(secure_random_u32(NULL));
    TEST_FALSE(secure_random_u64(NULL));
}

static void test_secure_random_bytes_success()
{
    byte buffer[32] = {};
    TEST_TRUE(secure_random_bytes(buffer, sizeof(buffer)));

    byte single = 0xAA;
    TEST_TRUE(secure_random_bytes(&single, 1));

    byte pair[2] = {};
    TEST_TRUE(secure_random_bytes(pair, sizeof(pair)));

    byte large[256] = {};
    TEST_TRUE(secure_random_bytes(large, sizeof(large)));
    TEST_TRUE(secure_random_bytes(large + 128, 128));
}

static void test_secure_random_u32()
{
    uint32 values[8] = {};
    bool all_ok = true;

    for (int32 i = 0; i < 8; ++i) {
        all_ok = all_ok && secure_random_u32(&values[i]);
    }
    TEST_TRUE(all_ok);

    bool all_same = true;
    for (int32 i = 1; i < 8; ++i) {
        all_same = all_same && (values[i] == values[0]);
    }
    TEST_FALSE(all_same);
}

static void test_secure_random_u64()
{
    uint64 values[8] = {};
    bool all_ok = true;

    for (int32 i = 0; i < 8; ++i) {
        all_ok = all_ok && secure_random_u64(&values[i]);
    }
    TEST_TRUE(all_ok);

    bool all_same = true;
    for (int32 i = 1; i < 8; ++i) {
        all_same = all_same && (values[i] == values[0]);
    }
    TEST_FALSE(all_same);
}

static void test_secure_random_non_constant()
{
    byte first[32] = {};
    byte second[32] = {};

    TEST_TRUE(secure_random_bytes(first, sizeof(first)));
    TEST_TRUE(secure_random_bytes(second, sizeof(second)));

    TEST_TRUE(memcmp(first, second, sizeof(first)) != 0);

    bool all_zero = true;
    bool all_same = true;

    for (int32 i = 0; i < 32; ++i) {
        all_zero = all_zero && (first[i] == 0);
        all_same = all_same && (first[i] == first[0]);
    }

    TEST_FALSE(all_zero);
    TEST_FALSE(all_same);
}

static void test_secure_random_overwrites_buffer()
{
    byte buffer[64];
    memset(buffer, 0xAA, sizeof(buffer));

    TEST_TRUE(secure_random_bytes(buffer, sizeof(buffer)));

    bool changed = false;
    for (int32 i = 0; i < 64; ++i) {
        if (buffer[i] != 0xAA) {
            changed = true;
            break;
        }
    }
    TEST_TRUE(changed);
}

#ifdef UBER_TEST
    #ifdef main
        #undef main
    #endif
    #define main UtilsSecureRandomTest
#endif

int main() {
    TEST_INIT(64);

    TEST_RUN(test_secure_random_bytes_invalid);
    TEST_RUN(test_secure_random_bytes_success);
    TEST_RUN(test_secure_random_u32);
    TEST_RUN(test_secure_random_u64);
    TEST_RUN(test_secure_random_non_constant);
    TEST_RUN(test_secure_random_overwrites_buffer);

    TEST_FINALIZE();

    return 0;
}
