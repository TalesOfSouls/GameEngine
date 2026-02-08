/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_SYSTEM_FILE_UTILS_C
#define COMS_SYSTEM_FILE_UTILS_C

#if _WIN32
    #include "../platform/win32/FileUtils.cpp"
#elif __linux__
    #include "../platform/linux/FileUtils.cpp"
#endif

// @question Consider to create a FileUtils.h file that also defines struct FileBody,
//          which is currently defined in Utils.h
typedef void (*FileToLoadCallback)(FileBody* file, void* data);

struct FileToLoad {
    FileToLoadCallback callback;
    char file_path[64 - sizeof(void*)];
};

#endif