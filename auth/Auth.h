/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_AUTH_H
#define COMS_AUTH_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../EngineDependencies/curl/include/curl/curl.h"

#define MAX_AUTH_POST_LENGTH 1024
#define MAX_AUTH_RESPONSE_LENGTH 1024

/**
 * Parse response data
 *
 * @param char*  data  Response data
 * @param size_t size  Response size
 * @param count  count Response count
 * @param void*  arg   Parsed data
 *
 * @return size_t Parsed data size
 */
size_t write_function(char* data, size_t size, size_t count, void* arg)
{
    char* dst = (char *)arg;
    size_t length = str_length(dst);

    // Ensure we do not exceed the buffer length
    size_t available_space = OMS_MIN(MAX_AUTH_RESPONSE_LENGTH, str_length(dst));
    if (available_space > 0) {
        strncat(dst, data, available_space - 1);
    }

    dst[available_space - 1] = '\0';

    return length;
}

/**
 * Get access token from remote source
 *
 * @param const char* url            Access url
 * @param char*       access_token   Url response (hopefully access token)
 * @param const char* username       Username for authentication
 * @param const char* identity_token Identity token (e.g. password)
 *
 * @return int 0 for failure, > 0 for success
 */
int get_access_token(
    const char* url,
    char* access_token,
    const char* username,
    const char* identity_token
)
{
    CURL *curl = curl_easy_init();
    if (!curl) {
        return 0;
    }

    char post[MAX_AUTH_POST_LENGTH] = {};
    char response[MAX_AUTH_RESPONSE_LENGTH] = {};
    uint32 http_code = 0;

    snprintf(
        post,
        MAX_AUTH_POST_LENGTH,
        "username=%s&identity_token=%s",
        username,
        identity_token
    );

    #ifdef _WIN32
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
    #endif

    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_function);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, response);
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post);

    CURLcode code = curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    curl_easy_cleanup(curl);

    if (code == CURLE_OK && http_code == 200) {
        strncpy(access_token, response, str_length(response));

        return 1;
    }

    return 0;
}

#endif