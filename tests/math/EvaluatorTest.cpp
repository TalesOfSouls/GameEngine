#include "../TestFramework.h"
#include "../../math/Evaluator.h"

static void test_evaluator_evaluate() {
    char expr[64];
    memcpy(expr, "(2 * 4 - 1 + 3) / 2", sizeof("(2 * 4 - 1 + 3) / 2"));

    ASSERT_EQUALS(evaluator_evaluate(expr), 5);
    ASSERT_NOT_EQUALS(evaluator_evaluate(expr), 1);
}

static void test_evaluator_evaluate_variables() {
    char expr[64];
    memcpy(expr, "(x * 4 + y + 3) / 2", sizeof("(x * 4 + y + 3) / 2"));

    const EvaluatorVariable variables[2] = {
        {"x", 2},
        {"y", -1}
    };

    ASSERT_EQUALS(evaluator_evaluate(expr, 2, (const EvaluatorVariable *) &variables), 5);
}

static void test_evaluator_evaluate_function() {
    char expr[64];

    memcpy(expr, "1 + min(2, 3)", sizeof("1 + min(2, 3)"));
    ASSERT_EQUALS(evaluator_evaluate(expr), 3);

    memcpy(expr, "1 + max(2, 3)", sizeof("1 + max(2, 3)"));
    ASSERT_EQUALS(evaluator_evaluate(expr), 4);

    const EvaluatorVariable variables[1] = {
        {"x", 2}
    };
    memcpy(expr, "x + max(x * 2, 3 * x) / x", sizeof("x + max(x * 2, 3 * x) / x"));
    ASSERT_EQUALS(evaluator_evaluate(expr, 1, (const EvaluatorVariable *) &variables), 5);
}

#ifdef UBER_TEST
    #ifdef main
        #undef main
    #endif
    #define main MathEvaluatorTest
#endif

int main() {
    TEST_INIT(10);

    TEST_RUN(test_evaluator_evaluate);
    TEST_RUN(test_evaluator_evaluate_variables);
    TEST_RUN(test_evaluator_evaluate_function);

    TEST_FINALIZE();

    return 0;
}
