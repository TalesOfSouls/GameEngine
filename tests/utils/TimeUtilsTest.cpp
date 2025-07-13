#include <string.h>
#include "../TestFramework.h"
#include "../../utils/TimeUtils.h"

static void test_time_index() {
    ASSERT_GREATER_THAN(time_index(), 0.0f);
}

static void test_system_time() {
    ASSERT_GREATER_THAN(system_time(), 0.0f);
}

static void test_time_mu() {
    ASSERT_GREATER_THAN(time_mu(), 0.0f);
}

#ifdef UBER_TEST
    #ifdef main
        #undef main
    #endif
    #define main UtilsTimeUtilsTest
#endif

int main() {
    TEST_INIT(10);

    TEST_RUN(test_time_index);
    TEST_RUN(test_system_time);
    TEST_RUN(test_time_mu);

    TEST_FINALIZE();

    return 0;
}
