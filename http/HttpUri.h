/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_JINGGA_HTTP_URI_H
#define COMS_JINGGA_HTTP_URI_H

#include "../stdlib/Types.h"

// WARNING: Be careful when changing order, members and types
// The current configuration is carefully chosen (see below)
struct HttpUri {
    byte path_offset;
    byte path_length;

    uint16 parameter_offset;
    uint16 parementers_length;

    // A parameter consists of 2 values: n-th value = length; n+1-th value = offset
    uint16 parementer_array_offset;
    byte parameter_array_count;
    byte fragment_length;
    uint16 fragment_offset;

    uint16 port;
};

FORCE_INLINE
const char* http_uri_path_get(const HttpUri* uri, const char* base) {
    return base + uri->path_offset;
}

#endif