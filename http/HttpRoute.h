/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_JINGGA_HTTP_ROUTE_H
#define COMS_JINGGA_HTTP_ROUTE_H

#include "../stdlib/Types.h"
#include "HttpMethod.h"
#include "../account/PermissionType.h"

#define MAX_HTTP_ROUTE_LENGTH 127

enum HttpRouteFlags {
    HTTP_ROUTE_FLAG_ACTUVE = 1 << 0,
    HTTP_ROUTE_FLAG_CSRF_REQUIRED = 1 << 1,
};

struct HttpRoutePermission {
    uint16 unit;
    uint16 app;
    uint16 module;
    PermissionType type;
    uint16 category;
};

// A route can have different end points depending on the method
struct HttpRouteDetails {
    uint32 func_id;

    // bit field for HttpMethod
    byte method;

    // bit field for HttpRouteFlags
    byte flags;

    HttpRoutePermission permission;
};

struct HttpRoute {
    char route[MAX_HTTP_ROUTE_LENGTH];

    byte details_count;
    HttpRouteDetails* details;
};

#endif