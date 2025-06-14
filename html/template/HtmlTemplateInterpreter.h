/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_HTML_TEMPLATE_INTERPRETER_H
#define COMS_HTML_TEMPLATE_INTERPRETER_H

#include "HtmlTemplateContext.h"
#include "HtmlTemplateParser.h"
#include "../../utils/StringUtils.h"

#define MAX_VARIABLES 1000

enum HtmlTemplateValueType : byte {
    VALUE_BOOL,
    VALUE_INTEGER64,
    VALUE_FLOAT64,
    VALUE_STRING
};

struct HtmlTemplateValue {
    HtmlTemplateValueType type;
    int32 value_length;
    union {
        bool boolValue;
        int64 int64Value;
        f64 f64Value;
        const char* ptrValue;
    };
};

enum HtmlTemplateVariableType : byte {
    VAR_BOOL,
    VAR_INTEGER64,
    VAR_FLOAT64,
    VAR_STRING
};

struct HtmlTemplateVariable {
    char name[32];
    HtmlTemplateVariableType type;
    int32 value_length;
    union {
        bool boolValue;
        int64 int64Value;
        f64 f64Value;
        const char* ptrValue;
    };
};

HtmlTemplateVariable symbol_table[MAX_VARIABLES];
int32 symbol_table_size = 0;

HtmlTemplateValue html_template_variable_get(const char* name, HtmlTemplateContextStack*) {
    for (int32 i = 0; i < symbol_table_size; i++) {
        if (str_compare(symbol_table[i].name, name) == 0) {
            HtmlTemplateValue result;
            switch (symbol_table[i].type) {
                case VAR_BOOL: {
                    result.type = VALUE_BOOL;
                    result.boolValue = symbol_table[i].boolValue;
                } break;
                case VAR_INTEGER64: {
                    result.type = VALUE_INTEGER64;
                    result.int64Value = symbol_table[i].int64Value;
                } break;
                case VAR_FLOAT64: {
                    result.type = VALUE_FLOAT64;
                    result.f64Value = symbol_table[i].f64Value;
                } break;
                case VAR_STRING: {
                    result.type = VALUE_STRING;
                    result.ptrValue = symbol_table[i].ptrValue;
                    result.value_length = symbol_table[i].value_length;
                } break;
                default: {
                    UNREACHABLE();
                }
            }

            return result;
        }
    }
    printf("Error: Variable %s not found\n", name);
    exit(1);
}

void html_template_variable_set(const char* name, HtmlTemplateValue value, HtmlTemplateContextStack*) {
    for (int32 i = 0; i < symbol_table_size; i++) {
        if (str_compare(symbol_table[i].name, name) == 0) {
            switch (value.type) {
                case VALUE_BOOL: {
                    symbol_table[i].type = VAR_BOOL;
                    symbol_table[i].boolValue = value.boolValue;
                } break;
                case VALUE_INTEGER64: {
                    symbol_table[i].type = VAR_INTEGER64;
                    symbol_table[i].int64Value = value.int64Value;
                } break;
                case VALUE_FLOAT64: {
                    symbol_table[i].type = VAR_FLOAT64;
                    symbol_table[i].f64Value = value.f64Value;
                } break;
                case VALUE_STRING: {
                    symbol_table[i].type = VAR_STRING;
                    symbol_table[i].ptrValue = value.ptrValue;
                    symbol_table[i].value_length = value.value_length;
                } break;
                default:
                    UNREACHABLE();
            }
            return;
        }
    }

    // If the variable doesn't exist, add it to the symbol table
    if (symbol_table_size < MAX_VARIABLES) {
        str_copy_short(symbol_table[symbol_table_size].name, name);
        switch (value.type) {
            case VALUE_BOOL: {
                symbol_table[symbol_table_size].type = VAR_BOOL;
                symbol_table[symbol_table_size].boolValue = value.boolValue;
            } break;
            case VALUE_INTEGER64: {
                symbol_table[symbol_table_size].type = VAR_INTEGER64;
                symbol_table[symbol_table_size].int64Value = value.int64Value;
            } break;
            case VALUE_FLOAT64: {
                symbol_table[symbol_table_size].type = VAR_FLOAT64;
                symbol_table[symbol_table_size].f64Value = value.f64Value;
            } break;
            case VALUE_STRING: {
                symbol_table[symbol_table_size].type = VAR_STRING;
                symbol_table[symbol_table_size].ptrValue = value.ptrValue;
                symbol_table[symbol_table_size].value_length = value.value_length;
            } break;
            default:
                UNREACHABLE();
        }

        ++symbol_table_size;
    } else {
        printf("Error: Symbol table overflow\n");
        exit(1);
    }
}

HtmlTemplateValue html_template_expression_eval(HtmlTemplateASTNode* node, HtmlTemplateContextStack* context_stack) {
    HtmlTemplateValue result;
    switch (node->type) {
        case NODE_INTEGER64:
            result.type = VALUE_INTEGER64;
            result.int64Value = node->int64Value;

            return result;
        case NODE_FLOAT64:
            result.type = VALUE_FLOAT64;
            result.f64Value = node->f64Value;

            return result;
        case NODE_IDENTIFIER:
            return html_template_variable_get(node->ptrValue, context_stack);
        case NODE_STRING:
            result.type = VALUE_STRING;
            result.ptrValue = node->ptrValue;
            result.value_length = node->value_length;

            return result;
        case NODE_BINOP: {
            HtmlTemplateValue leftValue = html_template_expression_eval(node->left, context_stack);
            HtmlTemplateValue rightValue = html_template_expression_eval(node->right, context_stack);
            if (leftValue.type == VALUE_INTEGER64 && rightValue.type == VALUE_INTEGER64) {
                result.type = VALUE_INTEGER64;
                if (str_compare(node->ptrValue, "+") == 0) {
                    result.int64Value = leftValue.int64Value + rightValue.int64Value;
                } else if (str_compare(node->ptrValue, "-") == 0) {
                    result.int64Value = leftValue.int64Value - rightValue.int64Value;
                } else if (str_compare(node->ptrValue, "*") == 0) {
                    result.int64Value = leftValue.int64Value * rightValue.int64Value;
                } else if (str_compare(node->ptrValue, "/") == 0) {
                    result.int64Value = leftValue.int64Value / rightValue.int64Value;
                } else {
                    exit(1);
                }

                return result;
            } else if (leftValue.type == VALUE_FLOAT64 || rightValue.type == VALUE_FLOAT64) {
                // Promote to double if either operand is a double
                result.type = VALUE_FLOAT64;
                double left = (leftValue.type == VALUE_FLOAT64) ? leftValue.f64Value : (f64)leftValue.int64Value;
                f64 right = (rightValue.type == VALUE_FLOAT64) ? rightValue.f64Value : (f64)rightValue.int64Value;
                if (str_compare(node->ptrValue, "+") == 0) {
                    result.f64Value = left + right;
                } else if (str_compare(node->ptrValue, "-") == 0) {
                    result.f64Value = left - right;
                } else if (str_compare(node->ptrValue, "*") == 0) {
                    result.f64Value = left * right;
                } else if (str_compare(node->ptrValue, "/") == 0) {
                    result.f64Value = left / right;
                } else {
                    exit(1);
                }

                return result;
            } else {
                printf("Error: Incompatible types for binary operation\n");
                exit(1);
            }
        }
        default:
            printf("Error: Unknown node type in expression\n");
            exit(1);
    }
}

bool html_template_condition_eval(HtmlTemplateASTNode *node, HtmlTemplateContextStack *context_stack) {
    if (node->type == NODE_BINOP) {
        HtmlTemplateValue leftValue = html_template_expression_eval(node->left, context_stack);
        HtmlTemplateValue rightValue = html_template_expression_eval(node->right, context_stack);

        if (leftValue.type == VALUE_INTEGER64 && rightValue.type == VALUE_INTEGER64) {
            // Integer comparison
            if (str_compare(node->ptrValue, ">") == 0) {
                return leftValue.int64Value > rightValue.int64Value;
            } else if (str_compare(node->ptrValue, "<") == 0) {
                return leftValue.int64Value < rightValue.int64Value;
            } else if (str_compare(node->ptrValue, "==") == 0) {
                return leftValue.int64Value == rightValue.int64Value;
            } else if (str_compare(node->ptrValue, "!=") == 0) {
                return leftValue.int64Value != rightValue.int64Value;
            } else if (str_compare(node->ptrValue, ">=") == 0) {
                return leftValue.int64Value >= rightValue.int64Value;
            } else if (str_compare(node->ptrValue, "<=") == 0) {
                return leftValue.int64Value <= rightValue.int64Value;
            } else {
                exit(1);
            }
        } else if (leftValue.type == VALUE_FLOAT64 || rightValue.type == VALUE_FLOAT64) {
            // Promote to double for comparison
            f64 left = (leftValue.type == VALUE_FLOAT64) ? leftValue.f64Value : (f64)leftValue.int64Value;
            f64 right = (rightValue.type == VALUE_FLOAT64) ? rightValue.f64Value : (f64)rightValue.int64Value;
            if (str_compare(node->ptrValue, ">") == 0) {
                return left > right;
            } else if (str_compare(node->ptrValue, "<") == 0) {
                return left < right;
            } else if (str_compare(node->ptrValue, "==") == 0) {
                return left == right;
            } else if (str_compare(node->ptrValue, "!=") == 0) {
                return left != right;
            } else if (str_compare(node->ptrValue, ">=") == 0) {
                return left >= right;
            } else if (str_compare(node->ptrValue, "<=") == 0) {
                return left <= right;
            } else {
                exit(1);
            }
        } else {
            printf("Error: Incompatible types for comparison\n");
            exit(1);
        }
    } else {
        printf("Error: Expected a binary operation in condition\n");
        exit(1);
    }
}

// @todo should take in a buffer for template output
int32 html_template_interpret(
    HtmlTemplateASTNode* node,
    char* buffer,
    int32 buffer_size,
    HtmlTemplateContextStack* context_stack
) {
    int32 out_length = 0;

    switch (node->type) {
        case NODE_RAW:
            // @performance If the entire file is raw we shouldn't have to copy the text over
            memcpy(buffer, node->ptrValue, node->value_length);
            out_length += node->value_length;

            if (node->right) {
                out_length += html_template_interpret(node->right, buffer + node->value_length, buffer_size - node->value_length, context_stack);
            }
            break;
        case NODE_ASSIGN:
            // Handle assignment
            break;
        case NODE_IF:
            // Handle if statement
            html_template_interpret(node->left, buffer, buffer_size, context_stack); // Condition
            html_template_interpret(node->right, buffer, buffer_size, context_stack); // Body
            break;
        case NODE_FOR:
            // Handle for loop
            html_template_interpret(node->left, buffer, buffer_size, context_stack); // Init
            while (html_template_condition_eval(node->right->left, context_stack)) { // Condition
                html_template_interpret(node->right->right, buffer, buffer_size, context_stack); // Body
                html_template_interpret(node->right->left->right, buffer, buffer_size, context_stack); // Update
            }
            break;
        default:
            break;
    }

    return out_length;
}

#endif