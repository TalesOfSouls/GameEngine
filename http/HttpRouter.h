/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_JINGGA_HTTP_ROUTER_H
#define COMS_JINGGA_HTTP_ROUTER_H

#include "../stdlib/Types.h"
#include "../memory/BufferMemory.h"
#include "../utils/RegexSimplified.h"
#include "HttpRoute.h"
#include "HttpMethod.h"

#define HTTP_ROUTE_SEGMENT_LENGTH 32

struct HttpRouteNode {
    char segment[HTTP_ROUTE_SEGMENT_LENGTH];

    // Route information
    // This is empty relatively often since only the last node(s) in a path usually have an endpoint defined
    // However, replacing this with another uint16 route_id for example only saves us 4 bytes,
    // BUT costs us another indirection once we arrive at a matched route/endpoint
    // The current implemenation allows us to directly jump into the detail definitions and iterate them
    uint32 detail_offset;
    byte detail_count;

    // Do this node require regex matching?
    bool is_regex;

    // How many child nodes does this node have
    uint16 children_count;

    // Defines the offset into the nodes array where the children can be found
    uint32 children_offset;
};

struct HttpRouter {
    HttpRouteNode* nodes;
    HttpRouteDetails* route_details;

    uint16 node_count;
    uint16 node_capacity;

    uint32 route_detail_count;
    uint32 route_detail_capacity;
};

void http_router_init(HttpRouter* router, uint32 route_count, BufferMemory* buf, int32 alignment = 64) {
    // We expect 3 path components per route
    // If more are required, we will increase the memory later
    router->nodes = (HttpRouteNode *) buffer_get_memory(buf, route_count * 3 * sizeof(HttpRouteNode), alignment, true);
    router->node_capacity = route_count * 3;
    router->node_count = 0;

    // We expect at least one route detail per route
    // On average it is probably more like 1.x but if we need more we will increase as required later
    router->route_details = (HttpRouteDetails *) buffer_get_memory(buf, route_count  * sizeof(HttpRouteDetails), alignment, true);
    router->route_detail_capacity = route_count;
    router->route_detail_count = 0;
}

/**
 * Optimizes the memory layout of the router by making all nodes with the same level consecutive in memory
 * This improves the caching since an element doesn't match we need to compare our current search with the other elements of the same level
 * If these elements are consecutive, there is a bigger chance that they are already loaded into L1 or L2 cache
 */
void http_router_optimize() {

}

// Add a new route
void http_router_add(
    HttpRouter* router,
    const HttpRoute* route
) {

}

void http_router_find_iter(
    const HttpRouter* router,
    const char* uri_segments,
    int32 uri_segment_index,
    int32 uri_segment_count,
    HttpRouteDetails** matches,
    int32* match_count,
    HttpRouteNode* node = NULL
) {
    for (uint32 i = 0; i < node->children_count; ++i) {
        HttpRouteNode* test_node = &router->nodes[node->children_offset + i];
        if ((!test_node->is_regex && str_compare(test_node->segment, uri_segments) == 0)
            || (test_node->is_regex && regex_simplified_validate(test_node->segment, uri_segments))
        ) {
            if (uri_segment_index < uri_segment_count && test_node->children_count) {
                // We have more in our uri path AND more child nodes
                // -> We need to continue pattern matching
                http_router_find_iter(
                    router,
                    uri_segments + str_length(uri_segments) + 1,
                    uri_segment_index + 1,
                    uri_segment_count,
                    matches,
                    match_count,
                    test_node
                );
            } else if (uri_segment_index == uri_segment_count && !test_node->children_count) {
                // We reached the end of the uri path and the end of the node chain
                // -> We found a possible match
                matches[(*match_count)++] = &router->route_details[test_node->detail_offset + i];
            } else if (uri_segment_index >= uri_segment_count && test_node->children_count) {
                // We reached the end of the uri path BUT still have child nodes
                // -> This can only be a match if any of the child chains from here on are optional/wildcard matches
                http_router_find_iter(
                    router,
                    "",
                    uri_segment_index + 1,
                    uri_segment_count,
                    matches,
                    match_count,
                    test_node
                );
            } else if (uri_segment_index < uri_segment_count && !test_node->children_count) {
                // We have more in our uri path BUT no more child nodes
                // -> This can only be a match if the test_node is a regex node that also matches all other path segments
                if (test_node->is_regex) {
                    bool is_valid = true;
                    for (int32 j = uri_segment_index + 1; j < uri_segment_count; ++j) {
                        if (!regex_simplified_validate(test_node->segment, uri_segments)) {
                            is_valid = false;
                            break;
                        }
                    }

                    if (is_valid) {
                        matches[(*match_count)++] = &router->route_details[test_node->detail_offset + i];
                    }
                }
            }
        }
    }
}

void http_router_route(
    const HttpRouter* router,
    const char* uri,
    bool has_csrf,
    HttpMethod method,
    HttpRouteDetails** matches,
    int32* match_count
) {
    char uri_segments[MAX_HTTP_ROUTE_LENGTH];
    char* segments_temp = uri_segments;

    int32 uri_segment_count = 0;
    int32 i = 0;

    while (*uri != '\0' && i < MAX_HTTP_ROUTE_LENGTH) {
        if (*uri == '/') {
            *segments_temp++ = '\0';
            ++uri;
            ++uri_segment_count;
        } else {
            *segments_temp++ = *uri++;
        }

        ++i;
    }

    *segments_temp = '\0';

    // Find potential matches based on the route
    int32 temp_match_count = 0;
    http_router_find_iter(
        router,
        uri_segments,
        0,
        uri_segment_count,
        matches,
        &temp_match_count,
        router->nodes
    );

    // Remove matches that don't fit the additional criteria
    // The reason why we don't do this in the route iteration is that we don't want to pass this information in every step
    // We need to remember that often only the last 1/2 path entries have actually a route attached
    *match_count = 0;
    for (i = 0; i < temp_match_count; ++i) {
        if ((matches[i]->method & method) // matches method/verb
            && (matches[i]->flags & HTTP_ROUTE_FLAG_ACTUVE) // route is active
            && (!(matches[i]->flags & HTTP_ROUTE_FLAG_CSRF_REQUIRED) // doesn't require csrf
                || ((matches[i]->flags & HTTP_ROUTE_FLAG_CSRF_REQUIRED) && has_csrf) // requires csrf & person has csrf
            )
        ) {
            // We only have to re-assign if the temp result has different elements than the final result
            // aka if a route has additional conditions like method, activity, ...
            if (*match_count != i) {
                matches[*match_count] = matches[i];
            }

            ++(*match_count);
        }
    }
}

#endif