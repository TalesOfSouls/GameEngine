/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_JINGGA_HTTP_REQUEST_H
#define COMS_JINGGA_HTTP_REQUEST_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../stdlib/Types.h"
#include "../utils/StringUtils.h"
#include "../log/Log.h"

#include "HttpMethod.h"
#include "HttpProtocol.h"
#include "HttpUri.h"
#include "HttpHeader.h"
#include "header/HttpHeaderKey.h"
#include "../network/SocketConnection.h"
#include "../memory/ThreadedChunkMemory.h"

enum HttpRequestState : byte {
    HTTP_REQUEST_STATE_NONE = 1 << 0,
    HTTP_REQUEST_STATE_HEADER_SENT = 1 << 1,
    HTTP_REQUEST_STATE_HEADER_BODY_SENT = 1 << 2,
    HTTP_REQUEST_STATE_HEADER_FINALIZED = 1 << 3
};

#define MIN_HTTP_REQUEST_CONTENT 2048

/**
 * Data layout
 *  HttpRequest
 *      ...
 *  char* header
 *
 *  @todo allow to overwrite body for iterative large body handling
 *  Request body (excl. file data if available)
 *  HttpHeader[]
 *  char* body // chunked
 *
 */

 // @todo for large request overwrite existing data and just send that instead of allocating too much memory
// The order of the members and their types is carefully chosen to make them fit into 1 cache line
// The request content must come directly after the request
struct HttpRequest {
    // Chunk id
    int32 id;

    // Defines the amount of chunks this http request uses (incl. http header)
    uint16 size;

    // Flag to indicate if the request is already returned
    // Uses HttpRequestState
    byte state;

    HttpUri uri;

    HttpMethod method;
    HttpProtocol protocol;

    // Element information
    uint16 header_available_count;
    uint16 header_used_count;

    // Value information
    uint16 header_available_size;
    uint16 header_used_size;

    // Body information
    uint16 body_offset;
    // uint32 body_available_size; Comes from size * chunk_size - body_offset
    uint32 body_used_size;
};

inline
void http_request_grow(HttpRequest* __restrict* request, int32 count, ThreadedChunkMemory* mem)
{
    HttpRequest* req = *request;

    int32 id = thrd_chunk_resize(mem, req->id, req->size, count);
    req = (HttpRequest*) thrd_chunk_get_element(mem, id);
    req->id = id;
    req->size = count;

    *request = req;
}

void http_header_value_set(
    HttpRequest* __restrict* request,
    HttpHeaderKey key,
    const char* __restrict value,
    ThreadedChunkMemory* mem,
    size_t value_length = 0
) {
    HttpRequest* req = *request;
    char* body_ptr = ((char *) (req + 1)) + req->body_offset;
    HttpHeaderElement* elements = (HttpHeaderElement *) (req + 1);

    HttpHeaderElement* element = NULL;
    for (int32 i = 0; i < req->header_used_count; ++i) {
        if (elements[i].key == key) {
            element = &elements[i];

            break;
        }
    }

    value_length = value_length == 0 ? str_length(value) : value_length;
    int32 header_content_offset = req->header_available_count * sizeof(HttpHeaderElement);

    if (element) {
        // Replace existing value
        if (value_length <= element->value_length) {
            // New value can use same memory since it is smaller
            // We don't size down since it is wasted performance for the hypothetical edge case where this is beneficial
            // The edge case if a size reduction would result in avoiding a memory expansion later on
            // @bug What if we first reduce the size and then increase it even though the original size would still be able to contain it?
            memcpy(((char *) (req + 1)) + element->value_offset, value, value_length);
        } else {
            // New value is larger than old value and requires memory moves

            uint32 grow_header_content = req->header_used_size + value_length >= req->header_available_size;
            uint32 header_value_growth = (uint32) OMS_MAX(grow_header_content * (1 * 256 * sizeof(char)), value_length);

            if (header_value_growth) {
                // The header content cannot hold the value

                // We calculate the body size and then subtract the used space to find the free body size
                if (header_value_growth > (req->size * mem->chunk_size - req->body_offset - sizeof(HttpRequest)) - req->body_used_size) {
                    // We need to grow the request object since we don't have enough free space in the body to grow into
                    http_request_grow(request, req->size + 1, mem);
                    req = *request;
                    body_ptr = ((char *) (req + 1)) + req->body_offset;
                }

                if (req->body_used_size) {
                    // Move body if we have body
                    memmove(
                        body_ptr + header_value_growth, // new body start position
                        body_ptr, // old body start position
                        req->body_used_size // data to move
                    );
                }

                // We now move the body start position
                req->body_offset += header_value_growth;

                // New element is positioned at the end of the existing header content
                // @bug We are wasting the original value memory e.g.
                //  old data: ... other element_value ... old_value ... other element_value
                //  new data: ... other element_value ... old_value ... other element_value .. new_value
                //  As you can see we still use memory for the old_value which is not even tracked any more
                //  Solution: shift header content completely and re-reference the value_offset of every other element
                //  Ideal: ..other element_value ... new_value ... other element_value
                //  This is ideal since we only need to memmove the data after new_value
                element->value_offset = header_content_offset + req->header_used_size;

                // The header content growth in size
                req->header_available_size += header_value_growth;

                // The used header growth in size
                req->header_used_size += value_length;
            } else {
                // The header content can hold the value
                // Add the value at the end of content (careful same bug as above)
                element->value_offset = req->body_offset - (req->header_available_size - req->header_used_size);
            }

            memcpy(((char *) (req + 1)) + element->value_offset, value, value_length);
        }

        element->value_length = value_length;
    } else {
        // Add new value
        uint32 grow_header_elements = req->header_used_count >= req->header_available_count;
        uint32 grow_header_content = req->header_used_size + value_length > req->header_available_size;

        uint32 header_element_addition = grow_header_elements * 4;
        uint32 header_element_growth = header_element_addition * sizeof(HttpHeaderElement);
        uint32 header_value_growth = (uint32) OMS_MAX((grow_header_content) * (1 * 256 * sizeof(char)), value_length);

        if (header_element_growth || header_value_growth) {
            if (header_element_growth + header_value_growth > (req->size * mem->chunk_size - req->body_offset - sizeof(HttpRequest)) - req->body_used_size) {
                // We need to grow the request object since we don't have enough free space in the body to grow into
                http_request_grow(request, req->size + 1, mem);
                req = *request;
                body_ptr = ((char *) (req + 1)) + req->body_offset;
                elements = (HttpHeaderElement *) (req + 1);
            }

            if (req->body_used_size) {
                // Move body if we have body
                memmove(
                    body_ptr + header_element_growth + header_value_growth, // New body start position
                    body_ptr, // Old body start
                    req->body_used_size // Data to move
                );
            }

            if (header_element_growth && req->header_used_size) {
                // If we are growing the element array, we need to move the content
                memmove(
                    ((char *) (req + 1)) + sizeof(HttpHeaderElement) * (req->header_available_count + header_element_addition), // New element value start position
                    ((char *) (req + 1)) + sizeof(HttpHeaderElement) * req->header_available_count, // Old element value start position
                    req->header_used_size
                );

                // We need to adjust the offset position because of the move
                for (int32 i = 0; i < req->header_used_count; ++i) {
                    elements[i].value_offset += header_element_growth;
                }
            }

            // We now move the body start position
            req->body_offset += header_element_growth + header_value_growth;
            req->header_available_count += header_element_growth / sizeof(HttpHeaderElement);
            req->header_available_size += header_value_growth;
        }

        // Set element
        element = &elements[req->header_used_count];
        element->key = key;
        // The value is added to the end of the values
        element->value_offset = req->header_available_count * sizeof(HttpHeaderElement) + req->header_used_size;
        element->value_length = (uint16) value_length;

        // Set value
        memcpy(((char *) (req + 1)) + element->value_offset, value, value_length);
        req->body_used_size += value_length;

        req->header_used_size += (uint16) value_length;
        ++req->header_used_count;
    }
}

HttpRequest* http_request_create(ThreadedChunkMemory* mem)
{
    int32 request_buffer_count = CEIL_DIV(sizeof(HttpRequest) + MIN_HTTP_REQUEST_CONTENT, mem->chunk_size);
    int32 request_buffer_id = thrd_chunk_reserve(mem, request_buffer_count);
    HttpRequest* request = (HttpRequest *) thrd_chunk_get_element(mem, request_buffer_id);

    request->id = request_buffer_id;
    request->size = request_buffer_count;
    request->protocol = HTTP_PROTOCOL_1_1;

    // Create content length placehoder, this header element is always required
    http_header_value_set(&request, HTTP_HEADER_KEY_CONTENT_LENGTH, "           ", mem);

    // Prepare the chunked sub-regions
    request->header_available_count = 16;
    request->header_available_size = 4 * 256 * sizeof(char);
    request->body_offset = request->header_available_count * sizeof(HttpHeaderElement) + request->header_available_size;

    /*
    request->body_available_size = request_buffer_count * mem->chunk_size
        - request->header_available_count * sizeof(HttpHeaderElement)
        - request->header_available_size;
    */

    return request;
}

inline
const char* http_request_body(const HttpRequest* request) {
    return ((const char *) (request + 1)) + request->body_offset;
}

FORCE_INLINE
bool http_header_is_complete(const char* request, size_t length) {
    return str_contains(request, "\r\n\r\n", length);
}

inline
bool http_header_is_complete(const HttpRequest* request) {
    const char* body = (const char *) (request + 1) + request->body_offset;
    return str_contains(body, "\r\n\r\n", request->body_used_size);
}

inline
const HttpHeaderElement* http_header_element_get(const HttpRequest* request, HttpHeaderKey key)
{
    const HttpHeaderElement* elements = (HttpHeaderElement *) (request + 1);
    for (int32 i = 0; i < request->header_used_count; ++i) {
        if (elements[i].key == key) {
            return &elements[i];
        }
    }

    return NULL;
}

inline
const char* http_header_value_get(const HttpRequest* request, const HttpHeaderElement* header_element)
{
    return ((const char *) (request + 1)) + header_element->value_offset;
}

bool http_request_has_file_upload(const HttpRequest* request) {
    const HttpHeaderElement* header = http_header_element_get(request, HTTP_HEADER_KEY_CONTENT_TYPE);
    if (!header) {
        return false;
    }

    const char* header_value = http_header_value_get(request, header);
    if ((str_compare_caseless(header_value, "application/", OMS_MIN(header->value_length, sizeof("application/") - 1)) == 0
            && str_compare_caseless(header_value, "application/json", OMS_MIN(header->value_length, sizeof("application/json") - 1)) != 0)
        || str_compare_caseless(header_value, "image/", OMS_MIN(header->value_length, sizeof("image/") - 1)) == 0
        || str_compare_caseless(header_value, "audio/", OMS_MIN(header->value_length, sizeof("audio/") - 1)) == 0
        || str_compare_caseless(header_value, "video/", OMS_MIN(header->value_length, sizeof("video/") - 1)) == 0
        || str_compare_caseless(header_value, "text/csv", OMS_MIN(header->value_length, sizeof("text/csv") - 1)) == 0
    ) {
        return true;
    }

    if (str_compare_caseless(header_value, "multipart/form-data", OMS_MIN(header->value_length, sizeof("multipart/form-data") - 1)) != 0) {
        return false;
    }

    // @todo not every form-data is a file upload but it is at least possible

    return false;
}

// @performance we could probably significantly improve this by handling this directly instead of calling the helper functions
// In the case below we know exactly if additional header elements will follow or not
void http_header_parse(HttpRequest** http_request, const char* request, ThreadedChunkMemory* mem) {
    const char* request_start = request;
    HttpRequest* http_req = *http_request;

    //////////////////////////////////////////////////
    // Parsing HTTP request line
    //////////////////////////////////////////////////

    str_skip_empty(&request);

    // Parse request type
    if (str_compare(request, "GET") == 0) {
        http_req->method = HTTP_METHOD_GET;
    } else if (str_compare(request, "POST") == 0) {
        http_req->method = HTTP_METHOD_POST;
    } else if (str_compare(request, "PUT") == 0) {
        http_req->method = HTTP_METHOD_PUT;
    } else if (str_compare(request, "DELETE") == 0) {
        http_req->method = HTTP_METHOD_DELETE;
    } else {
        // Additional request types are possible BUT we don't support them in our internal framework
        // If this would be a public framework we would've to support additional request types
        http_req->method = HTTP_METHOD_UNKNOWN;
    }

    // Parse reuqest path
    str_move_past(&request, ' ');
    http_req->uri.path_offset = request - request_start;

    str_skip_until_list(&request, ":?# ");
    http_req->uri.path_length = (request - request_start) - http_req->uri.path_offset;

    // Parse port
    if (*request == ':') {
        http_req->uri.port = (uint16) str_to_int(request, &request);
    }

    // Parse query parameters
    if (*request == '?') {
        http_req->uri.parameter_offset = request - request_start;
        str_skip_until_list(&request, "# ");
        //http_req->uri.parameter_length = (request - request_start) - http_req->uri.parameter_offset;
    }

    // Parse fragment
    if (*request == '#') {
        http_req->uri.fragment_offset = request - request_start;
        str_move_to(&request, ' ');
        http_req->uri.fragment_length = (request - request_start) - http_req->uri.fragment_offset;
    }

    // Parse protocol
    str_move_past(&request, ' ');
    if (str_compare(request, "HTTP/", sizeof("HTTP/") - 1) != 0) {
        LOG_1("[ERROR] Invalid HTTP header, no protocol defined");
        ASSERT_SIMPLE(false);

        return;
    }

    request += sizeof("HTTP/") - 1;
    if (*request == '1') {
        http_req->protocol = HTTP_PROTOCOL_1_1;
    } else if (*request == '2') {
        http_req->protocol = HTTP_PROTOCOL_2;
    } else if (*request == '3') {
        http_req->protocol = HTTP_PROTOCOL_3;
    } else {
        http_req->protocol = HTTP_PROTOCOL_UNKNOWN;
    }

    //////////////////////////////////////////////////
    // Parsing HTTP headers
    //////////////////////////////////////////////////
    // The HTTP headers end with \r\n\r\n (= one empty line/element)
    while (request[0] != '\r' && request[1] != '\n' && request[2] != '\r' && request[3] != '\n') {
        str_move_past(&request, '\n');
        const char* key = request;

        str_move_past(&request, ':');
        str_skip_empty(&request);
        const char* value = request;
        str_move_to(&request, '\r');

        http_header_value_set(http_request, http_header_key_text(key), value, mem, request - value);
    }
}

void parse_multipart_data(const char *body, const char *boundary) {
    char *buffer = strdup(body);
    if (!buffer) {
        perror("Failed to allocate memory for buffer");
        exit(EXIT_FAILURE);
    }

    char *part = strtok(buffer, boundary);
    while (part) {
        // Skip leading and trailing newlines
        while (*part == '\r' || *part == '\n') part++;

        // Parse part headers and content
        char* headers_end = strstr(part, "\r\n\r\n");
        if (headers_end) {
            *headers_end = '\0'; // Terminate headers
            char *content = headers_end + 4; // Skip "\r\n\r\n"

            printf("Part Headers:\n%s\n", part);
            printf("Part Content:\n%s\n", content);
        }

        part = strtok(NULL, boundary);
    }

    free(buffer);
}

/*
int main() {
    const char *http_request =
        "POST /submit HTTP/1.1\n"
        "Host: www.example.com\n"
        "Content-Type: multipart/form-data; boundary=----WebKitFormBoundary7MA4YWxkTrZu0gW\n"
        "Content-Length: 348\n"
        "\n"
        "------WebKitFormBoundary7MA4YWxkTrZu0gW\n"
        "Content-Disposition: form-data; name=\"username\"\n"
        "\n"
        "john_doe\n"
        "------WebKitFormBoundary7MA4YWxkTrZu0gW\n"
        "Content-Disposition: form-data; name=\"file\"; filename=\"example.txt\"\n"
        "Content-Type: text/plain\n"
        "\n"
        "This is a sample file.\n"
        "------WebKitFormBoundary7MA4YWxkTrZu0gW--\n";

    HttpRequest request;
    http_request_parse(http_request, &request);
    print_http_request(&request);

    // Parse multipart data if Content-Type is multipart/form-data
    for (int i = 0; i < request.header_count; i++) {
        if (strstr(request.headers[i], "Content-Type: multipart/form-data")) {
            const char *boundary_start = strstr(request.headers[i], "boundary=");
            if (boundary_start) {
                char boundary[128];
                sscanf(boundary_start, "boundary=%127s", boundary);
                printf("\nParsing multipart data with boundary: %s\n", boundary);
                parse_multipart_data(request.body, boundary);
            }
            break;
        }
    }

    return 0;
}
    */

#endif