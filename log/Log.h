/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_LOG_H
#define COMS_LOG_H

#include "../stdlib/Types.h"
#include "../compiler/CompilerUtils.h"
#include "../architecture/Intrinsics.h"
#include "../thread/ThreadDefines.h"
#include "../utils/StringUtils.h"
#include "../utils/TimeUtils.h"

/**
 * The logging is both using file logging and in-memory logging.
 * Debug builds also log to the debug console, or alternative standard output if no dedicated debug console is available
 */

#ifndef LOG_LEVEL
    // 0 = no logging at all
    // 1 = release logging
    // 2 = internal logging
    // 3 = debug logging
    // 4 = most verbose (probably has significant performance impacts)
    #if DEBUG
        #if DEBUG_STRICT
            #define LOG_LEVEL 4
        #else
            #define LOG_LEVEL 3
        #endif
    #elif INTERNAL
        #define LOG_LEVEL 2
    #elif RELEASE
        #define LOG_LEVEL 1
    #else
        #define LOG_LEVEL 0
    #endif
#endif

#ifndef MAX_LOG_LENGTH
    #define MAX_LOG_LENGTH 256
#endif

#ifndef MAX_LOG_MESSAGES
    #define MAX_LOG_MESSAGES 256
#endif

#if _WIN32
    static HANDLE _log_fp;
#elif __linux__
    static int32 _log_fp;
#endif

struct LogMemory {
    byte* memory;

    uint64 size;
    uint64 pos;

    // @performance Should I use spinlock?
    mutex mtx;
};
static LogMemory* _log_memory = NULL;

enum LogDataType {
    LOG_DATA_NONE,
    LOG_DATA_VOID,
    LOG_DATA_BYTE,
    LOG_DATA_INT16,
    LOG_DATA_INT32,
    LOG_DATA_UINT16,
    LOG_DATA_UINT32,
    LOG_DATA_INT64,
    LOG_DATA_UINT64,
    LOG_DATA_CHAR,
    LOG_DATA_CHAR_STR,
    LOG_DATA_FLOAT32,
    LOG_DATA_FLOAT64
};

struct LogMessage {
    const char* file;
    const char* function;
    int32 line;
    uint64 time;
    char* message;

    // We use this element to force a new line when saving the log to the file
    // This is MUCH faster compared to iteratively export every log message with a new line
    // The new line makes it much easier to manually read the log file (especially during development)
    char newline;
};

struct LogData {
    LogDataType type;
    void* value;
};

#define LOG_DATA_ARRAY 5
struct LogDataArray{
    LogData data[LOG_DATA_ARRAY];
};

byte* log_get_memory() noexcept
{
    if (_log_memory->pos + MAX_LOG_LENGTH > _log_memory->size) {
        _log_memory->pos = 0;
    }

    byte* offset = (byte *) (_log_memory->memory + _log_memory->pos);
    memset((void *) offset, 0, MAX_LOG_LENGTH);

    _log_memory->pos += MAX_LOG_LENGTH;

    return offset;
}

// @performance This should only be called async to avoid blocking (e.g. render loop)
void log_to_file()
{
    // we don't log an empty log pool
    if (!_log_memory || _log_memory->pos == 0 || !_log_fp) {
        return;
    }

    #if _WIN32
        DWORD written;
        WriteFile(
            _log_fp,
            (char *) _log_memory->memory,
            (uint32) _log_memory->pos,
            &written,
            NULL
        );
    #else
        if (_log_fp < 0) {
            return;
        }

        write(
            _log_fp,
            (char *) _log_memory->memory,
            (uint32) _log_memory->pos
        );
    #endif
}

// Same as log_to_file with the exception that reset the log pos to avoid repeated output
inline
void log_flush()
{
    if (!_log_memory || _log_memory->pos == 0 || !_log_fp) {
        return;
    }

    mutex_lock(&_log_memory->mtx);
    log_to_file();
    _log_memory->pos = 0;
    mutex_unlock(&_log_memory->mtx);
}

void log(const char* str, const char* file, const char* function, int32 line)
{
    if (!_log_memory) {
        return;
    }

    mutex_lock(&_log_memory->mtx);

    int32 len = str_length(str);
    while (len > 0) {
        LogMessage* msg = (LogMessage *) log_get_memory();

        // Dump to file
        msg->file = file;
        msg->function = function;
        msg->line = line;
        msg->message = (char *) (msg + 1);
        msg->time = system_time();
        msg->newline = '\n';

        int32 message_length = (int32) OMS_MIN((int32) (MAX_LOG_LENGTH - sizeof(LogMessage) - 1), len);

        memcpy(msg->message, str, message_length);
        msg->message[message_length] = '\0';
        str += message_length;
        len -= MAX_LOG_LENGTH - sizeof(LogMessage);

        #if DEBUG || VERBOSE
            // In debug mode we always output the log message to the debug console
            char time_str[13];
            format_time_hh_mm_ss_ms(time_str, msg->time / 1000ULL);
            compiler_debug_print(time_str);
            compiler_debug_print(" ");
            compiler_debug_print(msg->message);
            compiler_debug_print("\n");
        #endif

        if (_log_memory->size - _log_memory->pos < MAX_LOG_LENGTH) {
            log_to_file();
            _log_memory->pos = 0;
        }
    }

    mutex_unlock(&_log_memory->mtx);
}

void log(const char* format, LogDataArray data, const char* file, const char* function, int32 line)
{
    if (!_log_memory) {
        return;
    }

    if (data.data[0].type == LOG_DATA_VOID || data.data[0].type == LOG_DATA_NONE) {
        log(format, file, function, line);
        return;
    }

    ASSERT_SIMPLE(str_length(format) + str_length(file) + str_length(function) + 50 < MAX_LOG_LENGTH);

    mutex_lock(&_log_memory->mtx);

    LogMessage* msg = (LogMessage *) log_get_memory();
    msg->file = file;
    msg->function = function;
    msg->line = line;
    msg->message = (char *) (msg + 1);
    msg->time = system_time();
    msg->newline = '\n';

    char temp_format[MAX_LOG_LENGTH];
    str_copy_short(msg->message, format);

    for (int32 i = 0; i < LOG_DATA_ARRAY; ++i) {
        if (data.data[i].type == LOG_DATA_VOID || data.data[i].type == LOG_DATA_NONE) {
            break;
        }

        str_copy_short(temp_format, msg->message);

        switch (data.data[i].type) {
            case LOG_DATA_NONE: {
            }   break;
            case LOG_DATA_BYTE: {
                sprintf_fast_iter(msg->message, temp_format, (int32) *((byte *) data.data[i].value));
            } break;
            case LOG_DATA_INT16: {
                sprintf_fast_iter(msg->message, temp_format, (int32) *((int16 *) data.data[i].value));
            } break;
            case LOG_DATA_UINT16: {
                sprintf_fast_iter(msg->message, temp_format, (uint32) *((uint16 *) data.data[i].value));
            } break;
            case LOG_DATA_INT32: {
                sprintf_fast_iter(msg->message, temp_format, *((int32 *) data.data[i].value));
            } break;
            case LOG_DATA_UINT32: {
                sprintf_fast_iter(msg->message, temp_format, *((uint32 *) data.data[i].value));
            } break;
            case LOG_DATA_INT64: {
                sprintf_fast_iter(msg->message, temp_format, *((int64 *) data.data[i].value));
            } break;
            case LOG_DATA_UINT64: {
                sprintf_fast_iter(msg->message, temp_format, *((uint64 *) data.data[i].value));
            } break;
            case LOG_DATA_CHAR: {
                sprintf_fast_iter(msg->message, temp_format, *((char *) data.data[i].value));
            } break;
            case LOG_DATA_CHAR_STR: {
                sprintf_fast_iter(msg->message, temp_format, (const char *) data.data[i].value);
            } break;
            case LOG_DATA_FLOAT32: {
                sprintf_fast_iter(msg->message, temp_format, *((f32 *) data.data[i].value));
            } break;
            case LOG_DATA_FLOAT64: {
                sprintf_fast_iter(msg->message, temp_format, *((f64 *) data.data[i].value));
            } break;
            default: {
                UNREACHABLE();
            }
        }
    }

    #if DEBUG || VERBOSE
        // In debug mode we always output the log message to the debug console
        char time_str[13];
        format_time_hh_mm_ss_ms(time_str, msg->time / 1000ULL);
        compiler_debug_print(time_str);
        compiler_debug_print(" ");
        compiler_debug_print(msg->message);
        compiler_debug_print("\n");
    #endif

    if (_log_memory->size - _log_memory->pos < MAX_LOG_LENGTH) {
        log_to_file();
        _log_memory->pos = 0;
    }

    mutex_unlock(&_log_memory->mtx);
}

#define LOG_TO_FILE() log_to_file()
#define LOG_FLUSH() log_flush()

#if LOG_LEVEL == 4
    #define LOG_1(format, ...) log((format), LogDataArray{__VA_ARGS__}, __FILE__, __func__, __LINE__)
    #define LOG_2(format, ...) log((format), LogDataArray{__VA_ARGS__}, __FILE__, __func__, __LINE__)
    #define LOG_3(format, ...) log((format), LogDataArray{__VA_ARGS__}, __FILE__, __func__, __LINE__)
    #define LOG_4(format, ...) log((format), LogDataArray{__VA_ARGS__}, __FILE__, __func__, __LINE__)

    #define LOG_TRUE_1(should_log, format, ...) if ((should_log)) log((format), LogDataArray{__VA_ARGS__}, __FILE__, __func__, __LINE__)
    #define LOG_TRUE_2(should_log, format, ...) if ((should_log)) log((format), LogDataArray{__VA_ARGS__}, __FILE__, __func__, __LINE__)
    #define LOG_TRUE_3(should_log, format, ...) if ((should_log)) log((format), LogDataArray{__VA_ARGS__}, __FILE__, __func__, __LINE__)
    #define LOG_TRUE_4(should_log, format, ...) if ((should_log)) log((format), LogDataArray{__VA_ARGS__}, __FILE__, __func__, __LINE__)

    #define LOG_FALSE_1(should_log, format, ...) if (!(should_log)) log((format), LogDataArray{__VA_ARGS__}, __FILE__, __func__, __LINE__)
    #define LOG_FALSE_2(should_log, format, ...) if (!(should_log)) log((format), LogDataArray{__VA_ARGS__}, __FILE__, __func__, __LINE__)
    #define LOG_FALSE_3(should_log, format, ...) if (!(should_log)) log((format), LogDataArray{__VA_ARGS__}, __FILE__, __func__, __LINE__)
    #define LOG_FALSE_4(should_log, format, ...) if (!(should_log)) log((format), LogDataArray{__VA_ARGS__}, __FILE__, __func__, __LINE__)

    #define LOG_IF_1(expr, str_succeeded, str_failed) if ((expr)) { log((str_succeeded), __FILE__, __func__, __LINE__); } else { log((str_succeeded), __FILE__, __func__, __LINE__); }
    #define LOG_IF_2(expr, str_succeeded, str_failed) if ((expr)) { log((str_succeeded), __FILE__, __func__, __LINE__); } else { log((str_succeeded), __FILE__, __func__, __LINE__); }
    #define LOG_IF_3(expr, str_succeeded, str_failed) if ((expr)) { log((str_succeeded), __FILE__, __func__, __LINE__); } else { log((str_succeeded), __FILE__, __func__, __LINE__); }
    #define LOG_IF_4(expr, str_succeeded, str_failed) if ((expr)) { log((str_succeeded), __FILE__, __func__, __LINE__); } else { log((str_succeeded), __FILE__, __func__, __LINE__); }

    #define LOG_CYCLE_START(var_name) uint64 var_name##_start_time = intrin_timestamp_counter()
    #define LOG_CYCLE_END(var_name, format) \
        uint64 var_name##_duration = (uint64) (intrin_timestamp_counter() - var_name##_start_time); \
        LOG_1((format), {{LOG_DATA_UINT64, &var_name##_duration}})

    // Only intended for manual debugging
    // Of course a developer could always use printf but by providing this option,
    // we hope to avoid the situation where someone forgets to remove the printf
    // By using this macro we at least ensure it gets removed from the release build
    #define DEBUG_VERBOSE(str) compiler_debug_print((str))
    #define DEBUG_FORMAT_VERBOSE(format, ...) \
    ({ \
        char debug_str[1024]; \
        sprintf_fast(&debug_str, 1024, format, __VA_ARGS__); \
        compiler_debug_print((debug_str)); \
    })
#elif LOG_LEVEL == 3
    #define LOG_1(format, ...) log((format), LogDataArray{__VA_ARGS__}, __FILE__, __func__, __LINE__)
    #define LOG_2(format, ...) log((format), LogDataArray{__VA_ARGS__}, __FILE__, __func__, __LINE__)
    #define LOG_3(format, ...) log((format), LogDataArray{__VA_ARGS__}, __FILE__, __func__, __LINE__)
    #define LOG_4(format, ...) ((void) 0)

    #define LOG_TRUE_1(should_log, format, ...) if ((should_log)) log((format), LogDataArray{__VA_ARGS__}, __FILE__, __func__, __LINE__)
    #define LOG_TRUE_2(should_log, format, ...) if ((should_log)) log((format), LogDataArray{__VA_ARGS__}, __FILE__, __func__, __LINE__)
    #define LOG_TRUE_3(should_log, format, ...) if ((should_log)) log((format), LogDataArray{__VA_ARGS__}, __FILE__, __func__, __LINE__)
    #define LOG_TRUE_4(should_log, format, ...) ((void) 0)

    #define LOG_FALSE_1(should_log, format, ...) if (!(should_log)) log((format), LogDataArray{__VA_ARGS__}, __FILE__, __func__, __LINE__)
    #define LOG_FALSE_2(should_log, format, ...) if (!(should_log)) log((format), LogDataArray{__VA_ARGS__}, __FILE__, __func__, __LINE__)
    #define LOG_FALSE_3(should_log, format, ...) if (!(should_log)) log((format), LogDataArray{__VA_ARGS__}, __FILE__, __func__, __LINE__)
    #define LOG_FALSE_4(should_log, format, ...) ((void) 0)

    #define LOG_IF_1(expr, str_succeeded, str_failed) if ((expr)) { log((str_succeeded), __FILE__, __func__, __LINE__); } else { log((str_succeeded), __FILE__, __func__, __LINE__); }
    #define LOG_IF_2(expr, str_succeeded, str_failed) if ((expr)) { log((str_succeeded), __FILE__, __func__, __LINE__); } else { log((str_succeeded), __FILE__, __func__, __LINE__); }
    #define LOG_IF_3(expr, str_succeeded, str_failed) if ((expr)) { log((str_succeeded), __FILE__, __func__, __LINE__); } else { log((str_succeeded), __FILE__, __func__, __LINE__); }
    // Only logs on failure
    #define LOG_IF_4(expr, str_succeeded, str_failed) if (!(expr)) log((str_succeeded), __FILE__, __func__, __LINE__)

    #define LOG_CYCLE_START(var_name) uint64 var_name##_start_time = intrin_timestamp_counter()
    #define LOG_CYCLE_END(var_name, format) \
        uint64 var_name##_duration = (uint64) (intrin_timestamp_counter() - var_name##_start_time); \
        LOG_1((format), {{LOG_DATA_UINT64, &var_name##_duration}})

    #define DEBUG_VERBOSE(str) ((void) 0)
    #define DEBUG_FORMAT_VERBOSE(str, ...) ((void) 0)
#elif LOG_LEVEL == 2
    #define LOG_1(format, ...) log((format), LogDataArray{__VA_ARGS__}, __FILE__, __func__, __LINE__)
    #define LOG_2(format, ...) log((format), LogDataArray{__VA_ARGS__}, __FILE__, __func__, __LINE__)
    #define LOG_3(format, ...) ((void) 0)
    #define LOG_4(format, ...) ((void) 0)

    #define LOG_TRUE_1(should_log, format, ...) if ((should_log)) log((format), LogDataArray{__VA_ARGS__}, __FILE__, __func__, __LINE__)
    #define LOG_TRUE_2(should_log, format, ...) if ((should_log)) log((format), LogDataArray{__VA_ARGS__}, __FILE__, __func__, __LINE__)
    #define LOG_TRUE_3(should_log, format, ...) ((void) 0)
    #define LOG_TRUE_4(should_log, format, ...) ((void) 0)

    #define LOG_FALSE_1(should_log, format, ...) if (!(should_log)) log((format), LogDataArray{__VA_ARGS__}, __FILE__, __func__, __LINE__)
    #define LOG_FALSE_2(should_log, format, ...) if (!(should_log)) log((format), LogDataArray{__VA_ARGS__}, __FILE__, __func__, __LINE__)
    #define LOG_FALSE_3(should_log, format, ...) ((void) 0)
    #define LOG_FALSE_4(should_log, format, ...) ((void) 0)

    #define LOG_IF_1(expr, str_succeeded, str_failed) if ((expr)) { log((str_succeeded), __FILE__, __func__, __LINE__); } else { log((str_succeeded), __FILE__, __func__, __LINE__); }
    #define LOG_IF_2(expr, str_succeeded, str_failed) if ((expr)) { log((str_succeeded), __FILE__, __func__, __LINE__); } else { log((str_succeeded), __FILE__, __func__, __LINE__); }
    // Only logs on failure
    #define LOG_IF_3(expr, str_succeeded, str_failed) if (!(expr)) log((str_succeeded), __FILE__, __func__, __LINE__)
    #define LOG_IF_4(expr, str_succeeded, str_failed) ((void) 0)

    #define LOG_CYCLE_START(var_name) uint64 var_name##_start_time = intrin_timestamp_counter()
    #define LOG_CYCLE_END(var_name, format) \
        uint64 var_name##_duration = (uint64) (intrin_timestamp_counter() - var_name##_start_time); \
        LOG_1((format), {{LOG_DATA_UINT64, &var_name##_duration}})

    #define DEBUG_VERBOSE(str) ((void) 0)
    #define DEBUG_FORMAT_VERBOSE(str, ...) ((void) 0)
#elif LOG_LEVEL == 1
    #define LOG_1(format, ...) log((format), LogDataArray{__VA_ARGS__}, __FILE__, __func__, __LINE__)
    #define LOG_2(format, ...) ((void) 0)
    #define LOG_3(format, ...) ((void) 0)
    #define LOG_4(format, ...) ((void) 0)

    #define LOG_TRUE_1(should_log, format, ...) if ((should_log)) log((format), LogDataArray{__VA_ARGS__}, __FILE__, __func__, __LINE__)
    #define LOG_TRUE_2(should_log, format, ...) ((void) 0)
    #define LOG_TRUE_3(should_log, format, ...) ((void) 0)
    #define LOG_TRUE_4(should_log, format, ...) ((void) 0)

    #define LOG_FALSE_1(should_log, format, ...) if (!(should_log)) log((format), LogDataArray{__VA_ARGS__}, __FILE__, __func__, __LINE__)
    #define LOG_FALSE_2(should_log, format, ...) ((void) 0)
    #define LOG_FALSE_3(should_log, format, ...) ((void) 0)
    #define LOG_FALSE_4(should_log, format, ...) ((void) 0)

    #define LOG_IF_1(expr, str_succeeded, str_failed) if ((expr)) { log((str_succeeded), __FILE__, __func__, __LINE__); } else { log((str_succeeded), __FILE__, __func__, __LINE__); }
    // Only logs on failure
    #define LOG_IF_2(expr, str_succeeded, str_failed) if (!(expr)) log((str_succeeded), __FILE__, __func__, __LINE__)
    #define LOG_IF_3(expr, str_succeeded, str_failed) ((void) 0)
    #define LOG_IF_4(expr, str_succeeded, str_failed) ((void) 0)

    #define LOG_CYCLE_START(var_name) ((void) 0)
    #define LOG_CYCLE_END(var_name, format) ((void) 0)

    #define DEBUG_VERBOSE(str) ((void) 0)
    #define DEBUG_FORMAT_VERBOSE(str, ...) ((void) 0)
#elif LOG_LEVEL == 0
    #define LOG_1(format, ...) ((void) 0)
    #define LOG_2(format, ...) ((void) 0)
    #define LOG_3(format, ...) ((void) 0)
    #define LOG_4(format, ...) ((void) 0)

    #define LOG_TRUE_1(should_log, format, ...) ((void) 0)
    #define LOG_TRUE_2(should_log, format, ...) ((void) 0)
    #define LOG_TRUE_3(should_log, format, ...) ((void) 0)
    #define LOG_TRUE_4(should_log, format, ...) ((void) 0)

    #define LOG_FALSE_1(should_log, format, ...) ((void) 0)
    #define LOG_FALSE_2(should_log, format, ...) ((void) 0)
    #define LOG_FALSE_3(should_log, format, ...) ((void) 0)
    #define LOG_FALSE_4(should_log, format, ...) ((void) 0)

    #define LOG_IF_1(expr, str_succeeded, str_failed) ((void) 0)
    #define LOG_IF_2(expr, str_succeeded, str_failed) ((void) 0)
    #define LOG_IF_3(expr, str_succeeded, str_failed) ((void) 0)
    #define LOG_IF_4(expr, str_succeeded, str_failed) ((void) 0)

    #define LOG_CYCLE_START(var_name) ((void) 0)
    #define LOG_CYCLE_END(var_name, format) ((void) 0)

    #define DEBUG_VERBOSE(str) ((void) 0)
    #define DEBUG_FORMAT_VERBOSE(str, ...) ((void) 0)
#endif

#endif