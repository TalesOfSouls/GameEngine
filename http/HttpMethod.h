/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_JINGGA_HTTP_METHOD_H
#define COMS_JINGGA_HTTP_METHOD_H

#include "../stdlib/Types.h"

enum HttpMethod : byte {
    HTTP_METHOD_UNKNOWN = 0,
    HTTP_METHOD_GET = 1 << 0,
    HTTP_METHOD_POST = 1 << 1,
    HTTP_METHOD_PUT = 1 << 2,
    HTTP_METHOD_DELETE = 1 << 3,
    HTTP_METHOD_ANY = (1 << 4) - 1,
};

typedef HttpMethod HttpVerb;

const char* http_method_text(HttpMethod method) {
    switch (method) {
        case HTTP_METHOD_GET:
            return "GET";
        case HTTP_METHOD_POST:
            return "POST";
        case HTTP_METHOD_PUT:
            return "PUT";
        case HTTP_METHOD_DELETE:
            return "DELETE";
        default:
            UNREACHABLE();
    }
}

#endif