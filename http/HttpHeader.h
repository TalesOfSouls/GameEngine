/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_JINGGA_HTTP_HEADER_H
#define COMS_JINGGA_HTTP_HEADER_H

#include "../stdlib/Types.h"
#include "header/HttpHeaderKey.h"

struct HttpHeaderElement {
    HttpHeaderKey key;

    uint16 value_offset;
    uint16 value_length;
};

#endif