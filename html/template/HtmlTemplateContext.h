/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_HTML_TEMPLATE_CONTEXT_H
#define COMS_HTML_TEMPLATE_CONTEXT_H

#include "../../stdlib/Types.h"
#include "../../utils/TestUtils.h"

struct HtmlTemplateContext {
    int32 scope_level;
    int32 loop_nesting_level;
};

#define MAX_STACK_SIZE 2400

struct HtmlTemplateContextStack {
    HtmlTemplateContext stack[MAX_STACK_SIZE];
    int32 top;
};

void pushContext(HtmlTemplateContextStack* stack, HtmlTemplateContext context) {
    ASSERT_SIMPLE(stack->top < MAX_STACK_SIZE);
    stack->stack[stack->top++] = context;
}

HtmlTemplateContext popContext(HtmlTemplateContextStack* stack) {
    ASSERT_SIMPLE(stack->top > 0);
    return stack->stack[--stack->top];
}

HtmlTemplateContext peekContext(HtmlTemplateContextStack* stack) {
    ASSERT_SIMPLE(stack->top > 0);
    return stack->stack[stack->top - 1];
}

#endif