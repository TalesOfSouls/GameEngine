/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_HTML_TEMPLATE_PARSER_H
#define COMS_HTML_TEMPLATE_PARSER_H

#include "HtmlTemplateLexer.h"
#include "HtmlTemplateContext.h"

enum HtmlTemplateNodeType : byte {
    NODE_BINOP,
    NODE_PTR,
    NODE_IDENTIFIER,
    NODE_RAW,
    NODE_BOOL,
    NODE_INTEGER64,
    NODE_FLOAT64,
    NODE_ASSIGN, // =
    NODE_INCREMENT, // ++
    NODE_DECREMENT, // --
    NODE_ADD_ASSIGN, // +=
    NODE_SUBTRACT_ASSIGN, // -=
    NODE_MULTIPLY_ASSIGN, // *=
    NODE_DIVIDE_ASSIGN, // /=
    NODE_EQUALS, // ==
    NODE_UNEQUAL, // !=
    NODE_GREATER, // >
    NODE_GREATER_EQUAL, // >=
    NODE_LESSER, // <
    NODE_LESSER_EQUAL, // <=
    NODE_PLUS,
    NODE_MINUS,
    NODE_MULTIPLY,
    NODE_DIVIDE,
    NODE_COLON, // :
    NODE_QUESTION, // ?
    NODE_EXCLAMATION, // !
    NODE_CODE_START, // <?
    NODE_CODE_END, // ?>
    NODE_LBRACK, // [
    NODE_RBRACK, // ]
    NODE_LPAREN, // (
    NODE_RPAREN, // )
    NODE_LBRACE, // {
    NODE_RBRACE, // }
    NODE_SEMICOLON, // ;
    NODE_STRING, // "..."
    NODE_WHILE,
    NODE_ENDWHILE,
    NODE_IF,
    NODE_ENDIF,
    NODE_ELSEIF,
    NODE_ELSE,
    NODE_FOR,
    NODE_ENDFOR,
    NODE_FOREACH,
    NODE_ENDFOREACH,
};

struct HtmlTemplateASTNode {
    HtmlTemplateASTNode* left;
    HtmlTemplateASTNode* right;

    HtmlTemplateNodeType type;
    uint32 value_length;
    union {
        bool boolValue;
        int64 int64Value;
        f64 f64Value;
        const char* ptrValue;
    };
};

HtmlTemplateASTNode* html_template_node_create(HtmlTemplateNodeType type, const HtmlTemplateToken* token, byte** memory) {
    *memory = (byte *) ROUND_TO_NEAREST((uintptr_t) *memory, 32);
    HtmlTemplateASTNode* node = (HtmlTemplateASTNode *) *memory;
    *memory = (byte *) ROUND_TO_NEAREST((uintptr_t) (*memory + sizeof(HtmlTemplateASTNode)), 32);

    node->type = type;
    node->left = NULL;
    node->right = NULL;
    node->value_length = token->length;

    // @question instead of handling the parsing below, why not handle it here for known types such as int, float, bool, string
    node->ptrValue = token->value;

    return node;
}

HtmlTemplateASTNode* html_template_factor_parse(const char**, HtmlTemplateToken*, HtmlTemplateContextFlag, byte**);
HtmlTemplateASTNode* html_template_term_parse(const char** input, HtmlTemplateToken* token_current, HtmlTemplateContextFlag context_flag, byte** memory) {
    // @bug This is wrong
    HtmlTemplateASTNode* left = html_template_factor_parse(input, token_current, context_flag, memory);
    while (token_current->type == TOKEN_MULTIPLY || token_current->type == TOKEN_DIVIDE) {
        HtmlTemplateToken* old = token_current;
        *token_current = html_template_token_next(input, context_flag);
        HtmlTemplateASTNode* right = html_template_factor_parse(input, token_current, context_flag, memory);
        // @bug This is wrong
        left = html_template_node_create(NODE_BINOP, old, memory);
        left->left = left;
        left->right = right;
    }

    return left;
}

HtmlTemplateASTNode* html_template_expression_parse(const char** input, HtmlTemplateToken* token_current, HtmlTemplateContextFlag context_flag, byte** memory) {
    HtmlTemplateASTNode* left = html_template_term_parse(input, token_current, context_flag, memory);
    while (token_current->type == TOKEN_PLUS || token_current->type == TOKEN_MINUS) {
        HtmlTemplateToken* old = token_current;
        *token_current = html_template_token_next(input, context_flag);
        HtmlTemplateASTNode* right = html_template_term_parse(input, token_current, context_flag, memory);
        left = html_template_node_create(NODE_BINOP, old, memory);
        left->left = left;
        left->right = right;
    }

    return left;
}

HtmlTemplateASTNode* html_template_factor_parse(const char** input, HtmlTemplateToken* token_current, HtmlTemplateContextFlag context_flag, byte** memory) {
    // @performance Consider to order the token types in a smart way and perform range check here
    // @todo use switch
    if (token_current->type == TOKEN_INTEGER64) {
        HtmlTemplateASTNode* node = html_template_node_create(NODE_INTEGER64, token_current, memory);
        *token_current = html_template_token_next(input, context_flag);

        // @todo parse string representation of int

        return node;
    } else if (token_current->type == TOKEN_FLOAT64) {
        HtmlTemplateASTNode* node = html_template_node_create(NODE_FLOAT64, token_current, memory);
        *token_current = html_template_token_next(input, context_flag);

        // @todo parse string representation of float

        return node;
    } else if (token_current->type == TOKEN_STRING) {
        HtmlTemplateASTNode* node = html_template_node_create(NODE_STRING, token_current, memory);
        *token_current = html_template_token_next(input, context_flag);
        return node;
    } else if (token_current->type == TOKEN_IDENTIFIER) {
        HtmlTemplateASTNode* node = html_template_node_create(NODE_IDENTIFIER, token_current, memory);
        *token_current = html_template_token_next(input, context_flag);
        return node;
    } else if (token_current->type == TOKEN_LPAREN) {
        *token_current = html_template_token_next(input, context_flag); // Consume '('
        HtmlTemplateASTNode* node = html_template_expression_parse(input, token_current, context_flag, memory);
        *token_current = html_template_token_next(input, context_flag); // Consume ')'
        return node;
    } else if (token_current->type == TOKEN_LBRACK) {
        *token_current = html_template_token_next(input, context_flag); // Consume '['
        HtmlTemplateASTNode* node = html_template_expression_parse(input, token_current, context_flag, memory);
        *token_current = html_template_token_next(input, context_flag); // Consume ']'
        return node;
    } else if (token_current->type == TOKEN_LBRACE) {
        *token_current = html_template_token_next(input, context_flag); // Consume '{'
        HtmlTemplateASTNode* node = html_template_expression_parse(input, token_current, context_flag, memory);
        *token_current = html_template_token_next(input, context_flag); // Consume '}'
        return node;
    }

    ASSERT_SIMPLE(false);

    return NULL;
}

HtmlTemplateASTNode* html_template_statement_parse(const char**, HtmlTemplateToken*, HtmlTemplateContextStack*, HtmlTemplateContextFlag, byte**);
HtmlTemplateASTNode* html_template_assignment_parse(const char** input, HtmlTemplateToken* token_current, HtmlTemplateContextFlag context_flag, byte** memory) {
    /*HtmlTemplateASTNode* left =*/ html_template_node_create(NODE_IDENTIFIER, token_current, memory);
    *token_current = html_template_token_next(input, context_flag); // Consume identifier
    *token_current = html_template_token_next(input, context_flag); // Consume '='
    /*HtmlTemplateASTNode* right =*/ html_template_expression_parse(input, token_current, context_flag, memory);
    *token_current = html_template_token_next(input, context_flag); // Consume ';'

    return html_template_node_create(NODE_ASSIGN, {}, memory);
}

HtmlTemplateASTNode* html_template_parse_if(const char** input, HtmlTemplateToken* token_current, HtmlTemplateContextStack* context_stack, HtmlTemplateContextFlag context_flag, byte** memory) {
    HtmlTemplateContext newContext = peekContext(context_stack);
    ++newContext.scope_level;
    pushContext(context_stack, newContext);

    *token_current = html_template_token_next(input, context_flag); // Consume 'if'
    *token_current = html_template_token_next(input, context_flag); // Consume '('
    HtmlTemplateASTNode* condition = html_template_expression_parse(input, token_current, context_flag, memory);
    *token_current = html_template_token_next(input, context_flag); // Consume ')'
    *token_current = html_template_token_next(input, context_flag); // Consume '{'
    HtmlTemplateASTNode* body = html_template_statement_parse(input, token_current, context_stack, context_flag, memory);
    *token_current = html_template_token_next(input, context_flag); // Consume '}'

    popContext(context_stack);

    HtmlTemplateASTNode* ifNode = html_template_node_create(NODE_IF, {}, memory);
    ifNode->left = condition; // Condition
    ifNode->right = body;     // Body

    return ifNode;
}

HtmlTemplateASTNode* html_template_parse_for(const char** input, HtmlTemplateToken* token_current, HtmlTemplateContextStack* context_stack, HtmlTemplateContextFlag context_flag, byte** memory) {
    HtmlTemplateContext newContext = peekContext(context_stack);
    ++newContext.scope_level;
    ++newContext.loop_nesting_level;
    pushContext(context_stack, newContext);

    *token_current = html_template_token_next(input, context_flag); // Consume 'for'
    *token_current = html_template_token_next(input, context_flag); // Consume '('
    HtmlTemplateASTNode* init = html_template_assignment_parse(input, token_current, context_flag, memory);
    HtmlTemplateASTNode* condition = html_template_expression_parse(input, token_current, context_flag, memory);
    *token_current = html_template_token_next(input, context_flag); // Consume ';'
    HtmlTemplateASTNode* update = html_template_assignment_parse(input, token_current, context_flag, memory);
    *token_current = html_template_token_next(input, context_flag); // Consume ')'
    *token_current = html_template_token_next(input, context_flag); // Consume '{'
    HtmlTemplateASTNode* body = html_template_statement_parse(input, token_current, context_stack, context_flag, memory);
    *token_current = html_template_token_next(input, context_flag); // Consume '}'

    popContext(context_stack);

    HtmlTemplateASTNode* forNode = html_template_node_create(NODE_FOR, {}, memory);
    forNode->left = init;       // Initialization
    forNode->right = html_template_node_create(NODE_BINOP, {}, memory);
    forNode->right->left = condition; // Condition
    forNode->right->right = update;   // Update
    forNode->right->right->right = body; // Body

    return forNode;
}

HtmlTemplateASTNode* html_template_html_parse(
    const char** input,
    HtmlTemplateToken* token_current,
    HtmlTemplateContextStack* context_stack,
    HtmlTemplateContextFlag context_flag,
    byte** memory
) {
    HtmlTemplateASTNode* html = html_template_node_create(NODE_RAW, token_current, memory);

    *token_current = html_template_token_next(input, context_flag); // Consume html
    if (token_current->type != TOKEN_EOF) {
        html->right = html_template_statement_parse(input, token_current, context_stack, context_flag, memory);
    }

    return html;
}

HtmlTemplateASTNode* html_template_code_parse(
    const char** input,
    HtmlTemplateToken* token_current,
    HtmlTemplateContextStack* context_stack,
    HtmlTemplateContextFlag context_flag,
    byte** memory
) {
    HtmlTemplateASTNode* code = html_template_node_create(NODE_RAW, token_current, memory);

    *token_current = html_template_token_next(input, context_flag); // Consume code
    if (token_current->type != TOKEN_EOF) {
        code->right = html_template_statement_parse(input, token_current, context_stack, context_flag, memory);
    }

    return code;
}

HtmlTemplateASTNode* html_template_statement_parse(
    const char** input,
    HtmlTemplateToken* token_current,
    HtmlTemplateContextStack* context_stack,
    HtmlTemplateContextFlag context_flag,
    byte** memory
) {
    switch(token_current->type) {
        case TOKEN_HTML: {
            return html_template_html_parse(input, token_current, context_stack, context_flag, memory);
        };
        case TOKEN_ASSIGN: {
            return html_template_assignment_parse(input, token_current, context_flag, memory);
        };
        case TOKEN_IF: {
            return html_template_parse_if(input, token_current, context_stack, context_flag, memory);
        };
        case TOKEN_FOR: {
            return html_template_parse_for(input, token_current, context_stack, context_flag, memory);
        };
        case TOKEN_CODE_START: {
            return html_template_code_parse(input, token_current, context_stack, context_flag, memory);
        };
        default: UNREACHABLE();
    }
}

#endif