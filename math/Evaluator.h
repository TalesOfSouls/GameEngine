/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_MATH_EVALUATOR_H
#define COMS_MATH_EVALUATOR_H

#include "../stdlib/Types.h"
#include "../utils/StringUtils.h"
#include "../utils/TestUtils.h"
#include "../compiler/CompilerUtils.h"
#include <string.h>
#include <math.h>

#define EVALUATOR_MAX_STACK_SIZE 16

// Stack for operators
struct EvaluatorOperatorStack {
    char items[EVALUATOR_MAX_STACK_SIZE];
    int32 top;
};

// Stack for values
struct EvaluatorValueStack {
    f32 items[EVALUATOR_MAX_STACK_SIZE];
    int32 top;
};

struct EvaluatorVariable {
    char name[8];
    f32 value;
};

// Stack operations
void evaluator_push_operator(EvaluatorOperatorStack* stack, char op) {
    if (stack->top >= EVALUATOR_MAX_STACK_SIZE - 1) {
        return;
    }

    stack->items[++stack->top] = op;
}

char evaluator_pop_operator(EvaluatorOperatorStack* stack) {
    ASSERT_SIMPLE(stack->top >= 0);

    return stack->items[stack->top--];
}

char evaluator_peek_operator(EvaluatorOperatorStack* stack) {
    if (stack->top < 0) {
        return '\0';
    }

    return stack->items[stack->top];
}

void evaluator_push_value(EvaluatorValueStack* stack, f32 value) {
    if (stack->top >= EVALUATOR_MAX_STACK_SIZE - 1) {
        return;
    }

    stack->items[++stack->top] = value;
}

f32 evaluator_pop_value(EvaluatorValueStack* stack) {
    ASSERT_SIMPLE(stack->top >= 0);

    return stack->items[stack->top--];
}

// Precedence of operators
int32 evaluator_precedence(char op) {
    switch (op) {
        case '+':
        case '-':
            return 1;
        case '*':
        case '/':
            return 2;
        case 'u': // Unary minus
            return 3;
        default:
            return 0;
    }
}

// Apply an operator to one or two values
f32 evaluator_apply_operator(char op, f32 a, f32 b) {
    switch (op) {
        case '+':
            return a + b;
        case '-':
            return a - b;
        case '*':
            return a * b;
        case '/':
            return a / b;
        case 'u': // Unary minus
            return -a;
        default: {
            UNREACHABLE();
        }
    }
}

f32 evaluator_evaluate_function(const char* name, const char* args);

// Shunting-yard algorithm to evaluate the expression
f32 evaluator_evaluate_expression(const char* expr) {
    EvaluatorOperatorStack operators = { .items = {}, .top = -1 };
    EvaluatorValueStack values = { .items = {}, .top = -1 };

    const char* ptr = expr;
    while (*ptr) {
        if (str_is_num(*ptr) || *ptr == '.') {
            // Parse number
            const char* end;
            f32 value = str_to_float(ptr, &end);
            evaluator_push_value(&values, value);
            ptr = end;
        } else if (str_is_alpha(*ptr)) {
            // Parse function
            const char* start = ptr;
            while (str_is_alphanum(*ptr)) {
                ++ptr;
            }

            ASSERT_SIMPLE(*ptr == '(');

            ++ptr;
            const char* args_start = ptr;
            int32 depth = 1;
            while (*ptr && depth > 0) {
                if (*ptr == '(') {
                    ++depth;
                } else if (*ptr == ')') {
                    --depth;
                }

                ++ptr;
            }

            ASSERT_SIMPLE(depth == 0);

            char name[32];
            memcpy(name, start, ptr - start);
            name[ptr - start] = '\0';

            char args[128];
            memcpy(args, args_start, ptr - args_start - 1);
            args[ptr - args_start - 1] = '\0';

            f32 result = evaluator_evaluate_function(name, args);
            evaluator_push_value(&values, result);
        } else if (*ptr == '(') {
            // Open parenthesis
            evaluator_push_operator(&operators, *ptr);
            ++ptr;
        } else if (*ptr == ')') {
            // Close parenthesis
            while (evaluator_peek_operator(&operators) != '(') {
                char op = evaluator_pop_operator(&operators);
                f32 b = evaluator_pop_value(&values);
                if (op == 'u') {
                    evaluator_push_value(&values, evaluator_apply_operator(op, b, 0));
                } else {
                    f32 a = evaluator_pop_value(&values);
                    evaluator_push_value(&values, evaluator_apply_operator(op, a, b));
                }
            }

            evaluator_pop_operator(&operators); // Remove '('
            ++ptr;
        } else if (str_find("+-*/", *ptr)) {
            // Operator
            char op = *ptr;
            // Check if the operator is unary
            if (op == '-' && (ptr == expr || *(ptr - 1) == '(' || str_find("+-*/", *(ptr - 1)))) {
                op = 'u'; // Unary minus
            }

            while (evaluator_precedence(evaluator_peek_operator(&operators)) >= evaluator_precedence(op)) {
                char top_op = evaluator_pop_operator(&operators);
                f32 b = evaluator_pop_value(&values);
                if (top_op == 'u') {
                    evaluator_push_value(&values, evaluator_apply_operator(top_op, b, 0));
                } else {
                    f32 a = evaluator_pop_value(&values);
                    evaluator_push_value(&values, evaluator_apply_operator(top_op, a, b));
                }
            }

            evaluator_push_operator(&operators, op);
            ++ptr;
        } else {
            UNREACHABLE();
        }
    }

    // Process remaining operators
    while (operators.top >= 0) {
        char op = evaluator_pop_operator(&operators);
        f32 b = evaluator_pop_value(&values);
        if (op == 'u') {
            evaluator_push_value(&values, evaluator_apply_operator(op, b, 0));
        } else {
            f32 a = evaluator_pop_value(&values);
            evaluator_push_value(&values, evaluator_apply_operator(op, a, b));
        }
    }

    return evaluator_pop_value(&values);
}

// Evaluate built-in functions
f32 evaluator_evaluate_function(const char* name, const char* args) {
    if (str_compare(name, "min") == 0) {
        const char* comma = str_find(args, ',');
        if (!comma) {
            return 0.0; // Invalid function call (min requires at least two arguments)
        }

        // Split the arguments into two expressions
        char arg1[64], arg2[64];
        memcpy(arg1, args, comma - args);
        arg1[comma - args] = '\0';
        str_copy_short(arg2, comma + 1);

        // Recursively evaluate the arguments
        f32 val1 = evaluator_evaluate_expression(arg1);
        f32 val2 = evaluator_evaluate_expression(arg2);

        return (val1 < val2) ? val1 : val2; // Return the minimum value
    } else if (str_compare(name, "max") == 0) {
        const char* comma = str_find(args, ',');
        if (!comma) {
            return 0.0; // Invalid function call (max requires at least two arguments)
        }

        // Split the arguments into two expressions
        char arg1[64], arg2[64];
        memcpy(arg1, args, comma - args);
        arg1[comma - args] = '\0';
        str_copy_short(arg2, comma + 1);

        // Recursively evaluate the arguments
        f32 val1 = evaluator_evaluate_expression(arg1);
        f32 val2 = evaluator_evaluate_expression(arg2);

        return (val1 > val2) ? val1 : val2; // Return the maximum value
    } else if (str_compare(name, "sqrt") == 0) {
        // Evaluate the single argument
        f32 val = evaluator_evaluate_expression(args);
        return sqrtf(val); // Return the square root
    } else if (str_compare(name, "abs") == 0) {
        // Evaluate the single argument
        f32 val = evaluator_evaluate_expression(args);
        return OMS_ABS(val); // Return the absolute value
    }

    // Handle unknown functions (you can add more functions as needed)
    return 0.0;
}

f32 evaluator_evaluate(const char* expr, int32 variable_count = 0, const EvaluatorVariable* variables = NULL) {
    ASSERT_SIMPLE(str_length(expr) > 0);

    // Handle variables
    char expr_internal[1024];
    char* dest = expr_internal;

    if (variable_count) {
        while (*expr != '\0') {
            // Skip none-alpha values
            while (!str_is_alpha(*expr) && *expr != '\0') {
                if (*expr != ' ') {
                    *dest++ = *expr;
                }

                ++expr;
            }

            if (*expr == '\0') {
                break;
            }

            // Potential variable name
            bool found_variable = false;
            for (int32 i = 0; i < variable_count; ++i) {
                size_t len = str_length(variables[i].name);

                // Check if string is variable (must be followed by none-alpha value = string mustn't continue)
                if (str_compare(expr, variables[i].name, len) == 0
                    && !str_is_alphanum(expr[len]) && expr[len] != '('
                ) {
                    // Replace variable with value
                    dest += float_to_str(variables[i].value, dest, 4);
                    expr += len;
                    found_variable = true;
                    break;
                }
            }

            if (!found_variable) {
                // If no variable is found we must skip the entire alpha string,
                // otherwise a substring may match a variable name later on
                while (str_is_alphanum(*expr)) {
                    *dest++ = *expr++;
                }
            }
        }
    }

    // Copy remaining / or entire string over to internal expression
    // We also ignore all whitespaces here for simplified handling later on
    while (*expr != '\0') {
        if (*expr != ' ') {
            *dest++ = *expr;
        }

        ++expr;
    }

    *dest = '\0';

    return evaluator_evaluate_expression(expr_internal);
}

#endif