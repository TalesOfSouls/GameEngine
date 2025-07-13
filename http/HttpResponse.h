/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_JINGGA_HTTP_RESPONSE_H
#define COMS_JINGGA_HTTP_RESPONSE_H

#include "../stdlib/Types.h"
#include "HttpMethod.h"
#include "HttpHeader.h"
#include "HttpProtocol.h"
#include "HttpStatusCode.h"
#include "header/HttpHeaderKey.h"
#include "../network/SocketConnection.h"
#include "../memory/ThreadedChunkMemory.h"

enum HttpResponseState : byte {
    HTTP_RESPONSE_STATE_NONE = 1 << 0,
    HTTP_RESPONSE_STATE_HEADER_SENT = 1 << 1,
    HTTP_RESPONSE_STATE_HEADER_BODY_SENT = 1 << 2,
    HTTP_RESPONSE_STATE_HEADER_FINALIZED = 1 << 3
};

#define MIN_HTTP_RESPONSE_CONTENT 32768

/**
 * Data layout
 *  HttpResponse
 *      ...
 *  HttpHeaderElement elements[...]
 *  char header_values[...]
 *  char body[...]
 *
 * NOTE: that the memory area for header elements and header values is chunked
 * This means that we usually "allocate" multiple elements so we don't have to perform a growth too often
 */

// @todo for large responses overwrite existing data and just send that instead of allocating too much memory
struct HttpResponse {
    // Chunk id
    int32 id;

    // Defines the amount of chunks this http response uses (incl. http header and body)
    uint16 size;

    // Flag to indicate if the response is already returned
    // Uses HttpResponseState
    byte state;

    HttpProtocol protocol;
    HttpStatusCode status_code;

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
void http_response_grow(HttpResponse* __restrict* response, int32 count, ThreadedChunkMemory* mem)
{
    HttpResponse* resp = *response;

    int32 id = thrd_chunk_resize(mem, resp->id, resp->size, count);
    resp = (HttpResponse*) thrd_chunk_get_element(mem, id);
    resp->id = id;
    resp->size = count;

    *response = resp;
}

void http_header_value_set(
    HttpResponse* __restrict* response,
    HttpHeaderKey key,
    const char* __restrict value,
    ThreadedChunkMemory* mem
) {
    HttpResponse* resp = *response;
    char* body_ptr = ((char *) (resp + 1)) + resp->body_offset;
    HttpHeaderElement* elements = (HttpHeaderElement *) (resp + 1);

    HttpHeaderElement* element = NULL;
    for (int32 i = 0; i < resp->header_used_count; ++i) {
        if (elements[i].key == key) {
            element = &elements[i];

            break;
        }
    }

    size_t value_length = str_length(value);
    int32 header_content_offset = resp->header_available_count * sizeof(HttpHeaderElement);

    if (element) {
        // Replace existing value
        if (value_length <= element->value_length) {
            // New value can use same memory since it is smaller
            // We don't size down since it is wasted performance for the hypothetical edge case where this is beneficial
            // The edge case if a size reduction would result in avoiding a memory expansion later on
            // @bug What if we first reduce the size and then increase it even though the original size would still be able to contain it?
            memcpy(((char *) (resp + 1)) + element->value_offset, value, value_length);
        } else {
            // New value is larger than old value and requires memory moves

            uint32 grow_header_content = resp->header_used_size + value_length >= resp->header_available_size;
            uint32 header_value_growth = (uint32) OMS_MAX(grow_header_content * (1 * 256 * sizeof(char)), value_length);

            if (header_value_growth) {
                // The header content cannot hold the value

                // We calculate the body size and then subtract the used space to find the free body size
                if (header_value_growth > (resp->size * mem->chunk_size - resp->body_offset - sizeof(HttpResponse)) - resp->body_used_size) {
                    // We need to grow the response object since we don't have enough free space in the body to grow into
                    http_response_grow(response, resp->size + 1, mem);
                    resp = *response;
                    body_ptr = ((char *) (resp + 1)) + resp->body_offset;
                }

                if (resp->body_used_size) {
                    // Move body if we have body
                    memmove(
                        body_ptr + header_value_growth, // new body start position
                        body_ptr, // old body start position
                        resp->body_used_size // data to move
                    );
                }

                // We now move the body start position
                resp->body_offset += header_value_growth;

                // New element is positioned at the end of the existing header content
                // @bug We are wasting the original value memory e.g.
                //  old data: ... other element_value ... old_value ... other element_value
                //  new data: ... other element_value ... old_value ... other element_value .. new_value
                //  As you can see we still use memory for the old_value which is not even tracked any more
                //  Solution: shift header content completely and re-reference the value_offset of every other element
                //  Ideal: ..other element_value ... new_value ... other element_value
                //  This is ideal since we only need to memmove the data after new_value
                element->value_offset = header_content_offset + resp->header_used_size;

                // The header content growth in size
                resp->header_available_size += header_value_growth;

                // The used header growth in size
                resp->header_used_size += value_length;
            } else {
                // The header content can hold the value
                // Add the value at the end of content (careful same bug as above)
                element->value_offset = resp->body_offset - (resp->header_available_size - resp->header_used_size);
            }

            memcpy(((char *) (resp + 1)) + element->value_offset, value, value_length);
        }

        element->value_length = value_length;
    } else {
        // Add new value
        uint32 grow_header_elements = resp->header_used_count >= resp->header_available_count;
        uint32 grow_header_content = resp->header_used_size + value_length > resp->header_available_size;

        uint32 header_element_addition = grow_header_elements * 4;
        uint32 header_element_growth = header_element_addition * sizeof(HttpHeaderElement);
        uint32 header_value_growth = (uint32) OMS_MAX((grow_header_content) * (1 * 256 * sizeof(char)), value_length);

        if (header_element_growth || header_value_growth) {
            if (header_element_growth + header_value_growth > (resp->size * mem->chunk_size - resp->body_offset - sizeof(HttpResponse)) - resp->body_used_size) {
                // We need to grow the response object since we don't have enough free space in the body to grow into
                http_response_grow(response, resp->size + 1, mem);
                resp = *response;
                body_ptr = ((char *) (resp + 1)) + resp->body_offset;
                elements = (HttpHeaderElement *) (resp + 1);
            }

            if (resp->body_used_size) {
                // Move body if we have body
                memmove(
                    body_ptr + header_element_growth + header_value_growth, // New body start position
                    body_ptr, // Old body start
                    resp->body_used_size // Data to move
                );
            }

            if (header_element_growth && resp->header_used_size) {
                // If we are growing the element array, we need to move the content
                memmove(
                    ((char *) (resp + 1)) + sizeof(HttpHeaderElement) * (resp->header_available_count + header_element_addition), // New element value start position
                    ((char *) (resp + 1)) + sizeof(HttpHeaderElement) * resp->header_available_count, // Old element value start position
                    resp->header_used_size
                );

                // We need to adjust the offset position because of the move
                for (int32 i = 0; i < resp->header_used_count; ++i) {
                    elements[i].value_offset += header_element_growth;
                }
            }

            // We now move the body start position
            resp->body_offset += header_element_growth + header_value_growth;
            resp->header_available_count += header_element_growth / sizeof(HttpHeaderElement);
            resp->header_available_size += header_value_growth;
        }

        // Set element
        element = &elements[resp->header_used_count];
        element->key = key;
        // The value is added to the end of the values
        element->value_offset = resp->header_available_count * sizeof(HttpHeaderElement) + resp->header_used_size;
        element->value_length = (uint16) value_length;

        // Set value
        memcpy(((char *) (resp + 1)) + element->value_offset, value, value_length);
        resp->body_used_size += value_length;

        resp->header_used_size += (uint16) value_length;
        ++resp->header_used_count;
    }
}

HttpResponse* http_response_create(ThreadedChunkMemory* mem)
{
    int32 response_buffer_count = CEIL_DIV(sizeof(HttpResponse) + MIN_HTTP_RESPONSE_CONTENT, mem->chunk_size);
    int32 response_buffer_id = thrd_chunk_reserve(mem, response_buffer_count);
    HttpResponse* response = (HttpResponse *) thrd_chunk_get_element(mem, response_buffer_id);

    response->id = response_buffer_id;
    response->size = response_buffer_count;
    response->protocol = HTTP_PROTOCOL_1_1;
    response->status_code = HTTP_STATUS_CODE_200;

    // Create content length placehoder, this header element is always required
    http_header_value_set(&response, HTTP_HEADER_KEY_CONTENT_LENGTH, "           ", mem);

    // Prepare the chunked sub-regions
    response->header_available_count = 16;
    response->header_available_size = 4 * 256 * sizeof(char);
    response->body_offset = response->header_available_count * sizeof(HttpHeaderElement) + response->header_available_size;

    /*
    response->body_available_size = response_buffer_count * mem->chunk_size
        - response->header_available_count * sizeof(HttpHeaderElement)
        - response->header_available_size;
    */

    return response;
}

inline
const char* http_response_body(const HttpResponse* response) {
    return ((const char *) (response + 1)) + response->body_offset;
}

inline
const HttpHeaderElement* http_header_element_get(const HttpResponse* response, HttpHeaderKey key)
{
    const HttpHeaderElement* elements = (HttpHeaderElement *) (response + 1);
    for (int32 i = 0; i < response->header_used_count; ++i) {
        if (elements[i].key == key) {
            return &elements[i];
        }
    }

    return NULL;
}

inline
const char* http_header_value_get(const HttpResponse* response, const HttpHeaderElement* header_element)
{
    return ((const char *) (response + 1)) + header_element->value_offset;
}

// @todo we need a streamed response version http_response_stream()
// WARNING: We expect response to already contain a header element called content-length
void http_response_send(const SocketConnection* __restrict socket, HttpResponse* __restrict response)
{
    char header[4096];
    char* header_ref;

    header_ref = header;

    // First line
    header_ref += str_copy(header_ref, http_protocol_text(response->protocol));
    *header_ref++ = ' ';

    header_ref += str_copy(header_ref, http_status_text(response->status_code));
    *header_ref++ = '\r';
    *header_ref++ = '\n';

    char content_length[12];
    int_to_str(response->body_used_size, content_length);
    http_header_value_set(&response, HTTP_HEADER_KEY_CONTENT_LENGTH, content_length, NULL);

    const HttpHeaderElement* elements = (HttpHeaderElement *) (response + 1);

    // Headers
    for (int32 i = 0; i < response->header_used_count; ++i) {
        const HttpHeaderElement* element = &elements[i];

        header_ref += str_copy(header_ref, http_header_key_text(element->key));
        *header_ref++ = ' ';

        memcpy(header_ref, (const char *) elements + element->value_offset, element->value_length);
        header_ref += element->value_length;

        *header_ref++ = '\r';
        *header_ref++ = '\n';
    }

    *header_ref++ = '\r';
    *header_ref++ = '\n';

    // Use entire header array for first send
    // This also has a direct impact for the time to first byte (ttfb)
    // You may also have heard that crical css should be in head of the html (this is one of the reasons)
    int32 body_size_to_add = OMS_CLAMP((int32) (sizeof(header) - (header_ref - header)), 0, (int32) response->body_used_size);
    if (body_size_to_add && response->body_offset) {
        memcpy(
            header_ref,
            ((const char *) (response + 1)) + response->body_offset,
            body_size_to_add
        );

        header_ref += body_size_to_add;
    }

    // Send headers & potentially some content
    send(socket->sd, header, header_ref - header, 0);

    // Do we have data remaining to be sent?
    if (response->body_offset && response->body_used_size - body_size_to_add > 0) {
        // @question Do we need chunked sends?
        send(
            socket->sd,
            ((const char *) (response + 1)) + response->body_offset + body_size_to_add,
            response->body_used_size - body_size_to_add,
            0
        );
    }
}

void http_response_body_add(HttpResponse** response, const char* __restrict body, size_t length, ThreadedChunkMemory* mem)
{
    HttpResponse* resp = *response;
    char* response_body = (char *) (resp + 1);

    length = (length == 0) ? str_length(body) : length;

    // Resize if needed
    if (resp->body_used_size + length > resp->size * mem->chunk_size - sizeof(HttpResponse)) {
        int32 response_buffer_count = CEIL_DIV(sizeof(HttpResponse) + resp->body_used_size + length, mem->chunk_size);
        http_response_grow(&resp, response_buffer_count, mem);

        *response = resp;
        response_body = (char*) (resp + 1);
    }

    memcpy(response_body + resp->body_used_size, body, length);
    resp->body_used_size += length;
}

#endif