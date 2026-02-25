/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_PLATFORM_WIN32_FILE_UTILS_H
#define COMS_PLATFORM_WIN32_FILE_UTILS_H

typedef HANDLE FileHandle;
typedef HANDLE MMFHandle;
typedef OVERLAPPED file_overlapped;

struct FileBodyAsync {
    // doesn't include null termination (same as str_length)
    uint64 size;
    byte* content;
    OVERLAPPED ov;
};

#endif