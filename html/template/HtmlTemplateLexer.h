/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_HTML_TEMPLATE_LEXER_H
#define COMS_HTML_TEMPLATE_LEXER_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "../../stdlib/Types.h"
#include "../../utils/StringUtils.h"

enum HtmlTemplateTokenType : byte {
    TOKEN_HTML,
    TOKEN_BOOL,
    TOKEN_INTEGER64,
    TOKEN_FLOAT64,
    TOKEN_IDENTIFIER,
    TOKEN_ASSIGN, // =
    TOKEN_INCREMENT, // ++
    TOKEN_DECREMENT, // --
    TOKEN_ADD_ASSIGN, // +=
    TOKEN_SUBTRACT_ASSIGN, // -=
    TOKEN_MULTIPLY_ASSIGN, // *=
    TOKEN_DIVIDE_ASSIGN, // /=
    TOKEN_EQUALS, // ==
    TOKEN_UNEQUAL, // !=
    TOKEN_GREATER, // >
    TOKEN_GREATER_EQUAL, // >=
    TOKEN_LESSER, // <
    TOKEN_LESSER_EQUAL, // <=
    TOKEN_PLUS,
    TOKEN_MINUS,
    TOKEN_MULTIPLY,
    TOKEN_DIVIDE,
    TOKEN_COLON, // :
    TOKEN_QUESTION, // ?
    TOKEN_EXCLAMATION, // !
    TOKEN_CODE_START, // <?
    TOKEN_CODE_END, // ?>
    TOKEN_LBRACK, // [
    TOKEN_RBRACK, // ]
    TOKEN_LPAREN, // (
    TOKEN_RPAREN, // )
    TOKEN_LBRACE, // {
    TOKEN_RBRACE, // }
    TOKEN_SEMICOLON, // ;
    TOKEN_STRING, // "..."
    TOKEN_WHILE,
    TOKEN_ENDWHILE,
    TOKEN_IF,
    TOKEN_ENDIF,
    TOKEN_ELSEIF,
    TOKEN_ELSE,
    TOKEN_FOR,
    TOKEN_ENDFOR,
    TOKEN_FOREACH,
    TOKEN_ENDFOREACH,
    TOKEN_EOF,
    TOKEN_UNKNOWN
};

#define HTML_TEMPLATE_TOKEN_VALUE_SIZE 127

struct HtmlTemplateToken {
    HtmlTemplateTokenType type;

    // Normally we would actually store the value instead of a pointer
    // However, since html templates can have long sections without any template code and only html
    // We would rather use pointers compared to huge arrays for value storage
    const char* value;
    size_t length;
};

enum HtmlTemplateContextFlag {
    HTML_TEMPLATE_CONTEXT_FLAG_HTML,
    HTML_TEMPLATE_CONTEXT_FLAG_TEMPLATE,
};

HtmlTemplateToken html_template_token_next(const char** input, HtmlTemplateContextFlag context_flag) {
    str_skip_empty(input);

    HtmlTemplateToken token = { TOKEN_HTML, *input, 0 };
    if (**input == '\0') {
        token.type = TOKEN_EOF;
    } else if (context_flag == HTML_TEMPLATE_CONTEXT_FLAG_HTML) {
        // We might be in html mode or template mode
        // Depending on the mode we must handle chars differently
        if ((**input == '<' && (*input)[1] == '?')) {
            token.type = TOKEN_CODE_START;
            token.length = 2;

            *input += 2;
        } else {
            token.type = TOKEN_HTML;

            // Move input along until EOF or template code is found
            while (**input != '\0' && (**input != '<' || (*input)[1] != '?')) {
                ++(*input);
                ++token.length;
            }
        }
    } else if (**input == '"') {
        ++(*input);
        token.type = TOKEN_STRING;
        token.value = *input;

        while (**input != '"' && **input != '\0') {
            ++(*input);
            ++token.length;
        }

        if (**input == '"') {
            ++(*input);
        }
    } else if (str_is_num(**input)) {
        token.type = TOKEN_INTEGER64;

        while (str_is_num(**input) || **input == '.') {
            if (**input == '.') {
                token.type = TOKEN_FLOAT64;
            }

            ++(*input);
            ++token.length;
        }
    } else if (str_is_alpha(**input)) {
        while (str_is_alphanum(**input)) {
            ++(*input);
            ++token.length;
        }

        if (str_compare(token.value, "if") == 0) {
            token.type = TOKEN_IF;
        } else if (str_compare(token.value, "endif") == 0) {
            token.type = TOKEN_ENDIF;
        } else if (str_compare(token.value, "elseif") == 0) {
            token.type = TOKEN_ELSEIF;
        } else if (str_compare(token.value, "else") == 0) {
            token.type = TOKEN_ELSE;
        } else if (str_compare(token.value, "for") == 0) {
            token.type = TOKEN_FOR;
        } else if (str_compare(token.value, "endfor") == 0) {
            token.type = TOKEN_ENDFOR;
        } else if (str_compare(token.value, "while") == 0) {
            token.type = TOKEN_WHILE;
        } else if (str_compare(token.value, "endwhile") == 0) {
            token.type = TOKEN_ENDWHILE;
        } else if (str_compare(token.value, "foreach") == 0) {
            token.type = TOKEN_FOREACH;
        } else if (str_compare(token.value, "endforeach") == 0) {
            token.type = TOKEN_ENDFOREACH;
        } else {
            token.type = TOKEN_IDENTIFIER;
        }
    } else {
        token.length = 1;

        switch (**input) {
            case '=': {
                if (*input[1] == '=') {
                    token.type = TOKEN_EQUALS;
                    token.length = 2;
                    ++(*input);
                } else {
                    token.type = TOKEN_ASSIGN;
                }
            } break;
            case '!': {
                if (*input[1] == '=') {
                    token.type = TOKEN_UNEQUAL;
                    token.length = 2;
                    ++(*input);
                } else {
                    token.type = TOKEN_EXCLAMATION;
                }
            } break;
            case ':': token.type = TOKEN_COLON; break;
            case '?': {
                if (*input[1] == '>') {
                    token.type = TOKEN_CODE_END;
                    token.length = 2;
                    ++(*input);
                } else {
                    token.type = TOKEN_QUESTION;
                }
            } break;
            case '>': {
                if (*input[1] == '=') {
                    token.type = TOKEN_GREATER_EQUAL;
                    token.length = 2;
                    ++(*input);
                } else {
                    token.type = TOKEN_GREATER;
                }
            } break;
            case '<': {
                if (*input[1] == '=') {
                    token.type = TOKEN_LESSER_EQUAL;
                    token.length = 2;
                    ++(*input);
                } else {
                    token.type = TOKEN_LESSER;
                }
            } break;
            case '+': {
                if (*input[1] == '=') {
                    token.type = TOKEN_ADD_ASSIGN;
                    token.length = 2;
                    ++(*input);
                } else if (*input[1] == '+') {
                    token.type = TOKEN_INCREMENT;
                    token.length = 2;
                    ++(*input);
                } else {
                    token.type = TOKEN_PLUS;
                }
            } break;
            case '-': {
                if (*input[1] == '=') {
                    token.type = TOKEN_SUBTRACT_ASSIGN;
                    token.length = 2;
                    ++(*input);
                } else if (*input[1] == '-') {
                    token.type = TOKEN_DECREMENT;
                    token.length = 2;
                    ++(*input);
                } else {
                    token.type = TOKEN_MINUS;
                }
            } break;
            case '*': {
                if (*input[1] == '=') {
                    token.type = TOKEN_MULTIPLY_ASSIGN;
                    token.length = 2;
                    ++(*input);
                } else {
                    token.type = TOKEN_MULTIPLY;
                }
             } break;
            case '/': {
                if (*input[1] == '=') {
                    token.type = TOKEN_DIVIDE_ASSIGN;
                    token.length = 2;
                    ++(*input);
                } else {
                    token.type = TOKEN_DIVIDE;
                }
             } break;
            case '[': token.type = TOKEN_LBRACK; break;
            case ']': token.type = TOKEN_RBRACK; break;
            case '(': token.type = TOKEN_LPAREN; break;
            case ')': token.type = TOKEN_RPAREN; break;
            case '{': token.type = TOKEN_LBRACE; break;
            case '}': token.type = TOKEN_RBRACE; break;
            case ';': token.type = TOKEN_SEMICOLON; break;
            default: token.type = TOKEN_UNKNOWN; break;
        }

        ++(*input);
    }

    return token;
}

#endif