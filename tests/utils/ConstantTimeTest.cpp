#include "../TestFramework.h"
#include "../../utils/ConstantTime.h"

/**
 * Tests for the constant time helpers in ConstantTime.h
 *
 * The functions must not early exit on the first difference and must behave
 * exactly like their normal counterparts for equal/unequal inputs.
 */

static void test_constant_time_memcmp_equal()
{
    const byte first[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07};
    const byte second[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07};

    TEST_TRUE(ct_memcmp(first, second, sizeof(first)));
    TEST_TRUE(ct_memcmp(first, first, sizeof(first)));
    TEST_TRUE(ct_memcmp(first, second, 1));
    TEST_TRUE(ct_memcmp(first, second, 0));

    TEST_TRUE(ct_memcmp(first, second, 7));
}

static void test_constant_time_memcmp_diff()
{
    const byte first[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07};

    byte diff_first[8];
    memcpy(diff_first, first, sizeof(first));
    diff_first[0] ^= 0x01;
    TEST_FALSE(ct_memcmp(first, diff_first, sizeof(first)));

    byte diff_middle[8];
    memcpy(diff_middle, first, sizeof(first));
    diff_middle[4] ^= 0x80;
    TEST_FALSE(ct_memcmp(first, diff_middle, sizeof(first)));

    byte diff_last[8];
    memcpy(diff_last, first, sizeof(first));
    diff_last[7] ^= 0xFF;
    TEST_FALSE(ct_memcmp(first, diff_last, sizeof(first)));

    // A difference outside of the compared range is ignored
    TEST_TRUE(ct_memcmp(first, diff_last, 7));
    TEST_TRUE(ct_memcmp(first, diff_middle, 4));
}

static void test_constant_time_strcmp()
{
    TEST_TRUE(ct_strcmp("secret", 6, "secret", 6));
    TEST_TRUE(ct_strcmp("", 0, "", 0));
    TEST_FALSE(ct_strcmp("secret", 6, "secreT", 6));
    TEST_FALSE(ct_strcmp("secret", 6, "secret ", 7));
    TEST_FALSE(ct_strcmp("secret", 6, "secre", 5));
    TEST_FALSE(ct_strcmp("a", 1, "", 0));

    char buffer_a[8] = "token";
    char buffer_b[8] = "token";
    TEST_TRUE(ct_strcmp(buffer_a, 5, buffer_b, 5));

    buffer_b[2] = 'K';
    TEST_FALSE(ct_strcmp(buffer_a, 5, buffer_b, 5));
}

static void test_constant_time_memzero()
{
    byte data[8];
    memset(data, 0xAA, sizeof(data));

    ct_memzero(data + 1, 5);

    TEST_EQUALS(data[0], 0xAA);
    TEST_EQUALS(data[6], 0xAA);
    TEST_EQUALS(data[7], 0xAA);

    const byte zeroes[5] = {0};
    TEST_MEMORY_EQUALS(data + 1, zeroes, 5);

    byte untouched[2] = {0xAA, 0xBB};
    ct_memzero(untouched, 0);
    TEST_EQUALS(untouched[0], 0xAA);
    TEST_EQUALS(untouched[1], 0xBB);

    byte full[4] = {0xFF, 0xFF, 0xFF, 0xFF};
    ct_memzero(full, sizeof(full));
    const byte full_zeroes[4] = {0};
    TEST_MEMORY_EQUALS(full, full_zeroes, sizeof(full));
}

static void test_constant_time_select()
{
    TEST_EQUALS(ct_select(0xFFFFFFFFu, 0x1234u, 0x5678u), 0x1234);
    TEST_EQUALS(ct_select(0x00000000u, 0x1234u, 0x5678u), 0x5678);

    TEST_EQUALS(ct_select(0x0F0F0F0Fu, 0xFFFFFFFFu, 0x00000000u), 0x0F0F0F0F);

    const uint32 mask = 0x00FF00FFu;
    const uint32 a = 0xAAAAAAAAu;
    const uint32 b = 0x55555555u;
    TEST_EQUALS(ct_select(mask, a, b), (a & mask) | (b & ~mask));

    TEST_EQUALS(ct_select(0xFFFFFFFFu, 0, 0), 0);
    TEST_EQUALS(ct_select(0x00000000u, 0, 0), 0);
}

static void test_constant_time_mask()
{
    TEST_EQUALS(ct_mask(true), 0xFFFFFFFFu);
    TEST_EQUALS(ct_mask(false), 0x00000000u);
    TEST_EQUALS(ct_mask(true), (uint32) -1);
    TEST_EQUALS(ct_mask(false), 0u);

    // ct_mask combined with ct_select must select without a branch
    TEST_EQUALS(ct_select(ct_mask(true), 0xDEADBEEFu, 0x00000000u), 0xDEADBEEF);
    TEST_EQUALS(ct_select(ct_mask(false), 0xDEADBEEFu, 0x00000000u), 0x00000000);
}

#ifdef UBER_TEST
    #ifdef main
        #undef main
    #endif
    #define main UtilsConstantTimeTest
#endif

int main() {
    TEST_INIT(64);

    TEST_RUN(test_constant_time_memcmp_equal);
    TEST_RUN(test_constant_time_memcmp_diff);
    TEST_RUN(test_constant_time_strcmp);
    TEST_RUN(test_constant_time_memzero);
    TEST_RUN(test_constant_time_select);
    TEST_RUN(test_constant_time_mask);

    TEST_FINALIZE();

    return 0;
}
