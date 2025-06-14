/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_JINGGA_HTTP_PROTOCOL_H
#define COMS_JINGGA_HTTP_PROTOCOL_H

#include "../stdlib/Types.h"

#define HTTP_PROTOCOL_1_1_STR "1.1"
#define HTTP_PROTOCOL_2_STR "2"
#define HTTP_PROTOCOL_3_STR "3"

enum HttpProtocol : byte {
    HTTP_PROTOCOL_UNKNOWN,
    HTTP_PROTOCOL_1_1,
    HTTP_PROTOCOL_2,
    HTTP_PROTOCOL_3,
};

const char* http_protocol_text(HttpProtocol protocol) {
    switch (protocol) {
        case HTTP_PROTOCOL_1_1:
            return "HTTP/1.1";
        case HTTP_PROTOCOL_2:
            return "HTTP/2";
        case HTTP_PROTOCOL_3:
            return "HTTP/3";
        default:
            UNREACHABLE();
    }
}

#endif