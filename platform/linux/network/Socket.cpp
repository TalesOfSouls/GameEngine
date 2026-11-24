/**
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_PLATFORM_LINUX_NETWORK_SOCKET_C
#define COMS_PLATFORM_LINUX_NETWORK_SOCKET_C

#include "../../../stdlib/Stdlib.h"
#include "../../../log/Log.h"
#include "Socket.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/sendfile.h>
#include <fcntl.h>

// Linux doesn't need a socket library initialization
FORCE_INLINE
bool socket_init() NO_EXCEPT
{
    return true;
}

FORCE_INLINE
void socket_cleanup() NO_EXCEPT
{
}

inline
void socket_close(socketid sd) NO_EXCEPT
{
    shutdown(sd, SHUT_RDWR);
    close(sd);
}

/**
 * Max. time a blocking receive waits for data
 *
 * @param socketid  sd          Socket
 * @param uint32    timeout_ms  Timeout in milliseconds
 *
 * @return bool True on success
 */
inline
bool socket_receive_timeout_set(socketid sd, uint32 timeout_ms) NO_EXCEPT
{
    timeval timeout;
    timeout.tv_sec = timeout_ms / 1000;
    timeout.tv_usec = (timeout_ms % 1000) * 1000;

    return setsockopt(sd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) == 0;
}

// Only needed on Windows (@see platform/win32/network/Socket.cpp)
FORCE_INLINE
void socket_udp_connreset_ignore(socketid) NO_EXCEPT
{
}

#define SOCKET_FILE_CHUNK_SIZE 4096

// @todo implement a version that supports compression (gz)
int32 socket_http_file_send(int32 client_sock, const char* __restrict file_path, const char* __restrict content_type = NULL) {
    int32 file_fd;
    struct stat file_stat;
    int32 rc;
    char header[SOCKET_FILE_CHUNK_SIZE];
    int32 header_len;

    file_fd = open(file_path, O_RDONLY);
    if (file_fd == -1) {
        LOG_1("[ERROR] Opening file");

        return -1;
    }

    if (fstat(file_fd, &file_stat) < 0) {
        LOG_1("[ERROR] Getting file stats");
        close(file_fd);

        return -1;
    }

    // Prepare HTTP headers
    if (content_type == NULL) {
        content_type = "application/octet-stream";
    }

    header_len = sprintf_fast(header, sizeof(header),
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %l\r\n"
        "Connection: close\r\n" // @todo recosnider better handling
        "\r\n",
        content_type, (int64) file_stat.st_size);

    // Send HTTP headers
    rc = send(client_sock, header, header_len, 0);
    if (rc != header_len) {
        LOG_1("[ERROR] Sending HTTP headers");
        close(file_fd);

        return -1;
    }

    off_t offset = 0;
    ssize_t sent_bytes = sendfile(client_sock, file_fd, &offset, file_stat.st_size);
    if (sent_bytes != file_stat.st_size) {
        LOG_1("[ERROR] Sending file content");
        close(file_fd);

        return -1;
    }

    close(file_fd);

    LOG_4("[INFO] sending file");

    return 0;
}

#endif
