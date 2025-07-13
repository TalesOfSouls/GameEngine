/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_JINGGA_HTTP_STATUS_CODE_H
#define COMS_JINGGA_HTTP_STATUS_CODE_H

#include "../stdlib/Types.h"

enum HttpStatusCode : uint16 {
    HTTP_STATUS_CODE_100 = 100,
    HTTP_STATUS_CODE_101 = 101,
    HTTP_STATUS_CODE_102 = 102,
    HTTP_STATUS_CODE_200 = 200,
    HTTP_STATUS_CODE_201 = 201,
    HTTP_STATUS_CODE_202 = 202,
    HTTP_STATUS_CODE_203 = 203,
    HTTP_STATUS_CODE_204 = 204,
    HTTP_STATUS_CODE_205 = 205,
    HTTP_STATUS_CODE_206 = 206,
    HTTP_STATUS_CODE_207 = 207,
    HTTP_STATUS_CODE_300 = 300,
    HTTP_STATUS_CODE_301 = 301,
    HTTP_STATUS_CODE_302 = 302,
    HTTP_STATUS_CODE_303 = 303,
    HTTP_STATUS_CODE_304 = 304,
    HTTP_STATUS_CODE_305 = 305,
    HTTP_STATUS_CODE_306 = 306,
    HTTP_STATUS_CODE_307 = 307,
    HTTP_STATUS_CODE_308 = 308,
    HTTP_STATUS_CODE_400 = 400,
    HTTP_STATUS_CODE_401 = 401,
    HTTP_STATUS_CODE_402 = 402,
    HTTP_STATUS_CODE_403 = 403,
    HTTP_STATUS_CODE_404 = 404,
    HTTP_STATUS_CODE_405 = 405,
    HTTP_STATUS_CODE_406 = 406,
    HTTP_STATUS_CODE_407 = 407,
    HTTP_STATUS_CODE_408 = 408,
    HTTP_STATUS_CODE_409 = 409,
    HTTP_STATUS_CODE_410 = 410,
    HTTP_STATUS_CODE_411 = 411,
    HTTP_STATUS_CODE_412 = 412,
    HTTP_STATUS_CODE_413 = 413,
    HTTP_STATUS_CODE_414 = 414,
    HTTP_STATUS_CODE_415 = 415,
    HTTP_STATUS_CODE_416 = 416,
    HTTP_STATUS_CODE_417 = 417,
    HTTP_STATUS_CODE_421 = 421,
    HTTP_STATUS_CODE_422 = 422,
    HTTP_STATUS_CODE_423 = 423,
    HTTP_STATUS_CODE_424 = 424,
    HTTP_STATUS_CODE_425 = 425,
    HTTP_STATUS_CODE_426 = 426,
    HTTP_STATUS_CODE_428 = 428,
    HTTP_STATUS_CODE_429 = 429,
    HTTP_STATUS_CODE_431 = 431,
    HTTP_STATUS_CODE_451 = 451,
    HTTP_STATUS_CODE_500 = 500,
    HTTP_STATUS_CODE_501 = 501,
    HTTP_STATUS_CODE_502 = 502,
    HTTP_STATUS_CODE_503 = 503,
    HTTP_STATUS_CODE_504 = 504,
    HTTP_STATUS_CODE_505 = 505,
    HTTP_STATUS_CODE_506 = 506,
    HTTP_STATUS_CODE_507 = 507,
    HTTP_STATUS_CODE_508 = 508,
    HTTP_STATUS_CODE_509 = 509,
    HTTP_STATUS_CODE_510 = 510,
    HTTP_STATUS_CODE_511 = 511,
};

const char* http_status_text(HttpStatusCode code) {
    switch (code) {
        case HTTP_STATUS_CODE_100:
            return "Continue";
        case HTTP_STATUS_CODE_101:
            return "Switching Protocols";
        case HTTP_STATUS_CODE_102:
            return "Processing";
        case HTTP_STATUS_CODE_200:
            return "OK";
        case HTTP_STATUS_CODE_201:
            return "Created";
        case HTTP_STATUS_CODE_202:
            return "Accepted";
        case HTTP_STATUS_CODE_203:
            return "Non-Authoritative Information";
        case HTTP_STATUS_CODE_204:
            return "No Content";
        case HTTP_STATUS_CODE_205:
            return "Reset Content";
        case HTTP_STATUS_CODE_206:
            return "Partial Content";
        case HTTP_STATUS_CODE_207:
            return "Multi-Status";
        case HTTP_STATUS_CODE_300:
            return "Multiple Choices";
        case HTTP_STATUS_CODE_301:
            return "Moved Permanently";
        case HTTP_STATUS_CODE_302:
            return "Found";
        case HTTP_STATUS_CODE_303:
            return "See Other";
        case HTTP_STATUS_CODE_304:
            return "Not Modified";
        case HTTP_STATUS_CODE_305:
            return "Use Proxy";
        case HTTP_STATUS_CODE_306:
            return "Switch Proxy";
        case HTTP_STATUS_CODE_307:
            return "Temporary Redirect";
        case HTTP_STATUS_CODE_400:
            return "Bad Request";
        case HTTP_STATUS_CODE_401:
            return "Unauthorized";
        case HTTP_STATUS_CODE_402:
            return "Payment Required";
        case HTTP_STATUS_CODE_403:
            return "Forbidden";
        case HTTP_STATUS_CODE_404:
            return "Not Found";
        case HTTP_STATUS_CODE_405:
            return "Method Not Allowed";
        case HTTP_STATUS_CODE_406:
            return "Not Acceptable";
        case HTTP_STATUS_CODE_407:
            return "Proxy Authentication Required";
        case HTTP_STATUS_CODE_408:
            return "Request Timeout";
        case HTTP_STATUS_CODE_409:
            return "Conflict";
        case HTTP_STATUS_CODE_410:
            return "Gone";
        case HTTP_STATUS_CODE_411:
            return "Length Required";
        case HTTP_STATUS_CODE_412:
            return "Precondition Failed";
        case HTTP_STATUS_CODE_413:
            return "Request Entity Too Large";
        case HTTP_STATUS_CODE_414:
            return "Request-URI Too Long";
        case HTTP_STATUS_CODE_415:
            return "Unsupported Media Type";
        case HTTP_STATUS_CODE_416:
            return "Requested Range Not Satisfiable";
        case HTTP_STATUS_CODE_417:
            return "Expectation Failed";
        case HTTP_STATUS_CODE_421:
            return "Misdirected Request";
        case HTTP_STATUS_CODE_422:
            return "Unprocessable Entity";
        case HTTP_STATUS_CODE_423:
            return "Locked";
        case HTTP_STATUS_CODE_424:
            return "Failed Dependency";
        case HTTP_STATUS_CODE_425:
            return "Unordered Collection";
        case HTTP_STATUS_CODE_426:
            return "Upgrade Required";
        case HTTP_STATUS_CODE_431:
            return "Request Header Fields Too Large";
        case HTTP_STATUS_CODE_500:
            return "Internal Server Error";
        case HTTP_STATUS_CODE_501:
            return "Not Implemented";
        case HTTP_STATUS_CODE_502:
            return "Bad Gateway";
        case HTTP_STATUS_CODE_503:
            return "Service Unavailable";
        case HTTP_STATUS_CODE_504:
            return "Gateway Timeout";
        case HTTP_STATUS_CODE_505:
            return "HTTP Version Not Supported";
        case HTTP_STATUS_CODE_506:
            return "Variant Also Negotiates";
        case HTTP_STATUS_CODE_507:
            return "Insufficient Storage";
        case HTTP_STATUS_CODE_509:
            return "Bandwidth Limit Exceeded";
        case HTTP_STATUS_CODE_510:
            return "Not Extended";
        case HTTP_STATUS_CODE_511:
            return "Network Authentication Required";
        default:
            UNREACHABLE();
    }
}

#endif