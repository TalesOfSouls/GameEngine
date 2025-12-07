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

// Keep includes to a minimum to avoid circular dependencies
// Log is used in many other header files
#include "../stdlib/Types.h"
#include "../compiler/CompilerUtils.h"
#include "../architecture/Intrinsics.h"
#include "../utils/StringUtils.h"
#include "DebugMemory.h"

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
    // WARNING: Must be multiple of 8
    #define MAX_LOG_LENGTH 256
#endif

#ifndef MAX_LOG_MESSAGES
    #define MAX_LOG_MESSAGES 256
#endif

// We need to make some platform specific defines here to avoid including other header files
// Other header files most likely would also use the log header creating circular dependencies
// As a result we are kinda implementing some things twice
#if _WIN32
    #include <stdio.h>
    #include <windows.h>
    #include <time.h>

    /**
     * Get the system time
     *
     * @return uint64
     */
    static inline HOT_CODE
    uint64 log_sys_time() NO_EXCEPT
    {
        SYSTEMTIME sys_time;
        FILETIME file_time;
        ULARGE_INTEGER large_int;

        GetLocalTime(&sys_time);
        SystemTimeToFileTime(&sys_time, &file_time);

        // Convert FILETIME to a 64-bit integer
        large_int.LowPart = file_time.dwLowDateTime;
        large_int.HighPart = file_time.dwHighDateTime;

        return ((uint64) (large_int.QuadPart / 10000000ULL)) - ((uint64) 11644473600ULL);
    }

    static HANDLE _log_fp;
    typedef volatile long log_spinlock32;

    /**
     * Initialize the log specific spinlock
     *
     * @param log_spinlock32* lock Spinlock variable
     *
     * @return void
     */
    FORCE_INLINE
    void log_spinlock_init(log_spinlock32* const lock) NO_EXCEPT
    {
        *lock = 0;
    }

    /**
     * Start the spinlock
     *
     * @param log_spinlock32*   lock    Spinlock variable
     * @param int32             delay   Minimum amount of time spend befor rechecking spinlock
     *
     * @return void
     */
    static FORCE_INLINE HOT_CODE
    void log_spinlock_start(log_spinlock32* const lock, int32 delay = 10) NO_EXCEPT
    {
        while (InterlockedExchange(lock, 1) != 0) {
            LARGE_INTEGER frequency, start, end;
            QueryPerformanceFrequency(&frequency);
            QueryPerformanceCounter(&start);

            const long long target = start.QuadPart + (delay * frequency.QuadPart) / 1000000;

            do {
                QueryPerformanceCounter(&end);
            } while (end.QuadPart < target);
        }
    }

    /**
     * End/unlock the spinlock
     *
     * @param log_spinlock32* lock Spinlock variable
     *
     * @return void
     */
    static FORCE_INLINE HOT_CODE
    void log_spinlock_end(log_spinlock32* const lock) NO_EXCEPT
    {
        InterlockedExchange(lock, 0);
    }
#elif __linux__
    #include <time.h>
    #include <sys/time.h>
    #include <unistd.h>

    /**
     * Get the system time
     *
     * @return uint64
     */
    static inline HOT_CODE
    uint64 log_sys_time() NO_EXCEPT
    {
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);

        return (uint64) ts.tv_sec * 1000000ULL + (uint64) ts.tv_nsec / 1000ULL;
    }

    static int32 _log_fp;
    typedef volatile int32 log_spinlock32;

    /**
     * Initialize the log specific spinlock
     *
     * @param log_spinlock32* lock Spinlock variable
     *
     * @return void
     */
    FORCE_INLINE
    void log_spinlock_init(log_spinlock32* const lock) NO_EXCEPT
    {
        *lock = 0;
    }

    /**
     * Start the spinlock
     *
     * @param log_spinlock32*   lock    Spinlock variable
     * @param int32             delay   Minimum amount of time spend befor rechecking spinlock
     *
     * @return void
     */
    static FORCE_INLINE HOT_CODE
    void log_spinlock_start(log_spinlock32* const lock, int32 delay = 10) NO_EXCEPT
    {
        while (__atomic_exchange_n(lock, 1, __ATOMIC_ACQUIRE) != 0) {
            struct timespec start, now;
            clock_gettime(CLOCK_MONOTONIC, &start);
            const uint64 target_ns = usec * 1000ULL;

            do {
                clock_gettime(CLOCK_MONOTONIC, &now);
                const uint64 elapsed = (now.tv_sec - start.tv_sec) * 1000000000ULL
                    + (now.tv_nsec - start.tv_nsec);

                if (elapsed >= target_ns) {
                    break;
                }
            } while (true);
        }
    }

    /**
     * End/unlock the spinlock
     *
     * @param log_spinlock32* lock Spinlock variable
     *
     * @return void
     */
    static FORCE_INLINE HOT_CODE
    void log_spinlock_end(log_spinlock32* const lock) NO_EXCEPT
{
        __atomic_store_n(lock, 0, __ATOMIC_RELEASE);
    }
#endif

// By using this constructor/destructor pattern you can avoid deadlocks in case of exceptions
// Why? Well because if we go out of scope the destructor is automatically called and the lock is unlocked
struct LogSpinlockGuard {
    log_spinlock32* _lock = NULL;

    inline HOT_CODE
    LogSpinlockGuard(log_spinlock32* const lock, int32 delay = 10) {
        this->_lock = lock;

        log_spinlock_start(this->_lock, delay);
    }

    inline HOT_CODE
    void unlock() {
        if (this->_lock) {
            log_spinlock_end(this->_lock);
            this->_lock = NULL;
        }
    }

    inline HOT_CODE
    ~LogSpinlockGuard() {
        this->unlock();
    }
};

struct LogMemory {
    byte* memory;

    uint64 size;
    uint64 pos;

    log_spinlock32 lock;
};
static LogMemory* _log_memory = NULL;

struct LogMessage {
    const char* file;
    const char* function;
    uint64 time;
    char* message;
    int32 line;

    // We use this element to force a new line when saving the log to the file
    // This is MUCH faster compared to iteratively export every log message with a new line
    // The new line makes it much easier to manually read the log file (especially during development)
    char newline;
};

struct LogData {
    DataType type;
    const void* value;
};

#define LOG_DATA_ARRAY 5
struct LogDataArray{
    LogData data[LOG_DATA_ARRAY];
};

/**
 * Find memory buffer to write log to
 *
 * @return byte*
 */
static inline HOT_CODE
byte* log_get_memory() NO_EXCEPT
{
    if (_log_memory->pos + MAX_LOG_LENGTH > _log_memory->size) {
        _log_memory->pos = 0;
    }

    byte* const offset = (byte *) (_log_memory->memory + _log_memory->pos);
    memset((void *) offset, 0, MAX_LOG_LENGTH);

    _log_memory->pos += MAX_LOG_LENGTH;

    return offset;
}

/**
 * Log data to file
 *
 * WARNING: This is not thread safe by itself
 *
 * @param const void*   data    Data to log
 * @param size_t        size    Data size to log
 *
 * @performance This should only be called async to avoid blocking (e.g. render loop)
 *              Careful this this function is not thread safe
 *
 * @return void
 */
void log_to_file(const void* data, size_t size) NO_EXCEPT
{
    if (!_log_memory || _log_memory->pos == 0 || !_log_fp) {
        return;
    }

    #if _WIN32
        DWORD written;
        WriteFile(
            _log_fp,
            (char *) data,
            (uint32) size,
            &written,
            NULL
        );
    #else
        if (_log_fp < 0) {
            return;
        }

        write(
            _log_fp,
            (char *) data,
            (uint32) size
        );
    #endif
}

/**
 * Log data to file and reset log position
 *
 * @return void
 */
inline
void log_flush() NO_EXCEPT
{
    if (!_log_memory || _log_memory->pos == 0 || !_log_fp) {
        return;
    }

    LogSpinlockGuard _guard(&_log_memory->lock, 0);
    log_to_file(_log_memory->memory, _log_memory->pos);
    _log_memory->pos = 0;
}

/**
 * Log message to (debug-)terminal
 *
 * @param uint64        time    Timestamp of the log message
 * @param const char*   msg     Message to log
 *
 * @return void
 */
static inline HOT_CODE
void log_to_terminal(uint64 time, const char* const msg) NO_EXCEPT
{
    char time_str[13];
    format_time_hh_mm_ss_ms(time_str, time / 1000ULL);
    compiler_debug_print(time_str);
    compiler_debug_print(" ");
    compiler_debug_print(msg);
    compiler_debug_print("\n");
}

/**
 * Log message
 *
 * @param const char*   str         String to log
 * @param const char*   file        Origin file where the log message comes from
 * @param const char*   function    Origin function of the log message
 * @param int32         line        Origin line
 *
 * @return void
 */
HOT_CODE
void log(const char* str, const char* const file, const char* function, int32 line) NO_EXCEPT
{
    if (!_log_memory) {
        return;
    }

    LogSpinlockGuard _guard(&_log_memory->lock, 0);

    int32 len = (int32) str_length(str);

    // Ensure that we have enough space in our log memory, otherwise log to file
    const int32 total_len = (len + (MAX_LOG_LENGTH - 1) / MAX_LOG_LENGTH) * sizeof(LogMessage) + len;
    if (_log_memory->size - (_log_memory->pos + total_len) < MAX_LOG_LENGTH) {
        log_to_file(_log_memory->memory, _log_memory->pos);
        _log_memory->pos = 0;
    }

    while (len > 0) {
        LogMessage* const msg = (LogMessage *) log_get_memory();

        // Dump to file
        msg->file = file;
        msg->function = function;
        msg->line = line;
        msg->message = (char *) (msg + 1);
        msg->time = log_sys_time();
        msg->newline = '\n';

        const int32 message_length = (int32) OMS_MIN((int32) (MAX_LOG_LENGTH - sizeof(LogMessage) - 1), len);

        memcpy(msg->message, str, message_length);
        msg->message[message_length] = '\0';
        str += message_length;
        len -= MAX_LOG_LENGTH - sizeof(LogMessage);

        #if DEBUG || VERBOSE
            // In debug mode we always output the log message to the debug console
            log_to_terminal(msg->time, msg->message);
        #endif

        DEBUG_MEMORY_WRITE((uintptr_t) msg, sizeof(*msg) + message_length);
    }
}

/**
 * Log message
 *
 * @param const char*   format      String format to log
 * @param LogDataArray  data        Data to put into the format string
 * @param const char*   file        Origin file where the log message comes from
 * @param const char*   function    Origin function of the log message
 * @param int32         line        Origin line
 *
 * @return void
 */
HOT_CODE
void log(const char* const format, LogDataArray data, const char* const file, const char* const function, int32 line) NO_EXCEPT
{
    if (!_log_memory) {
        return;
    }

    if (data.data[0].type == DATA_TYPE_VOID) {
        log(format, file, function, line);
        return;
    }

    ASSERT_TRUE(str_length(format) + str_length(file) + str_length(function) + 50 < MAX_LOG_LENGTH);

    LogSpinlockGuard _guard(&_log_memory->lock, 0);

    // Is data raw output?
    if (data.data[0].type == DATA_TYPE_BYTE_ARRAY) {
        // If length is larger than buffer directly log to file
        const int32 total_len = *((int32 *) data.data[0].value);
        #if DEBUG || VERBOSE
            // In debug mode we always output the log message to the debug console
            log_to_terminal(log_sys_time(), format);
        #endif

        // NOTE: We are not storing raw data in memory
        //      This is because we need \0 terminated text in memory and only allow MAX_LOG_LENGTH
        //      The raw data doesn't fulfill \0 and probably also doesn't fulfill MAX_LOG_LENGTH

        log_to_file(format, total_len);

        return;
    }

    LogMessage* const msg = (LogMessage *) log_get_memory();
    msg->file = file;
    msg->function = function;
    msg->line = line;
    msg->message = (char *) (msg + 1);
    msg->time = log_sys_time();
    msg->newline = '\n';

    char temp_format[MAX_LOG_LENGTH];
    str_copy(msg->message, format);

    for (int32 i = 0; i < LOG_DATA_ARRAY; ++i) {
        if (data.data[i].type == DATA_TYPE_VOID) {
            break;
        }

        str_copy(temp_format, msg->message);

        switch (data.data[i].type) {
            case DATA_TYPE_VOID: {
            }   break;
            case DATA_TYPE_UINT8: {
                sprintf_fast_iter(msg->message, temp_format, (int32) *((byte *) data.data[i].value));
            } break;
            case DATA_TYPE_INT16: {
                sprintf_fast_iter(msg->message, temp_format, (int32) *((int16 *) data.data[i].value));
            } break;
            case DATA_TYPE_UINT16: {
                sprintf_fast_iter(msg->message, temp_format, (uint32) *((uint16 *) data.data[i].value));
            } break;
            case DATA_TYPE_INT32: {
                sprintf_fast_iter(msg->message, temp_format, *((int32 *) data.data[i].value));
            } break;
            case DATA_TYPE_UINT32: {
                sprintf_fast_iter(msg->message, temp_format, *((uint32 *) data.data[i].value));
            } break;
            case DATA_TYPE_INT64: {
                sprintf_fast_iter(msg->message, temp_format, *((int64 *) data.data[i].value));
            } break;
            case DATA_TYPE_UINT64: {
                sprintf_fast_iter(msg->message, temp_format, *((uint64 *) data.data[i].value));
            } break;
            case DATA_TYPE_CHAR: {
                sprintf_fast_iter(msg->message, temp_format, *((char *) data.data[i].value));
            } break;
            case DATA_TYPE_CHAR_STR: {
                sprintf_fast_iter(msg->message, temp_format, (const char *) data.data[i].value);
            } break;
            case DATA_TYPE_F32: {
                sprintf_fast_iter(msg->message, temp_format, *((f32 *) data.data[i].value));
            } break;
            case DATA_TYPE_F64: {
                sprintf_fast_iter(msg->message, temp_format, *((f64 *) data.data[i].value));
            } break;
            default: {
                UNREACHABLE();
            }
        }
    }

    #if DEBUG || VERBOSE
        // In debug mode we always output the log message to the debug console
        log_to_terminal(msg->time, msg->message);
    #endif

    DEBUG_MEMORY_WRITE((uintptr_t) msg, sizeof(*msg) + MAX_LOG_LENGTH);

    if (_log_memory->size - _log_memory->pos < MAX_LOG_LENGTH) {
        log_to_file(_log_memory->memory, _log_memory->pos);
        _log_memory->pos = 0;
    }
}

#define LOG_TO_FILE() log_to_file(_log_memory->memory, _log_memory->pos)
#define LOG_FLUSH() log_flush()

/**
 * This is just an annoying helper function required for MSVC
 * which doesn't allow (LogData) {type, value} as a return/type case
 *
 * @param DataType  type    Data type
 * @param void*     value   Pointer to value
 *
 * @return LogData
 */
inline HOT_CODE
LogData makeLogData(DataType type, const void* value) NO_EXCEPT
{
    LogData d = {type, value};
    return d;
}
#define LOG_ENTRY(type, value) makeLogData(type, (const void*)(value))

// WARNING: Unfortunately this helper function is required since post c++20 nested-brace initialize no longer support array initialization
#include <initializer_list>
inline HOT_CODE
LogDataArray makeLogDataArray(std::initializer_list<LogData> list) NO_EXCEPT
{
    LogDataArray arr = {};
    int32 i = 0;

    for (std::initializer_list<LogData>::const_iterator it = list.begin(); it != list.end(); ++it) {
        if (i >= LOG_DATA_ARRAY) {
            break;
        }

        const LogData& item = *it;
        arr.data[i++] = item;
    }

    return arr;
}

/**
 * LOG_1 = Release build (optimized)
 * LOG_2 = Internal build (optimized)
 * LOG_3 = Debug Build (unoptimized = slow)
 * LOG_4 = Strict/Intensive logging (very slow)
 *
 * This results in ZERO costs for LOG_4 messages when in release builds
 */

#if LOG_LEVEL == 4
    #define LOG_1(format, ...) log((format), makeLogDataArray({__VA_ARGS__}), __FILE__, __func__, __LINE__)
    #define LOG_2(format, ...) log((format), makeLogDataArray({__VA_ARGS__}), __FILE__, __func__, __LINE__)
    #define LOG_3(format, ...) log((format), makeLogDataArray({__VA_ARGS__}), __FILE__, __func__, __LINE__)
    #define LOG_4(format, ...) log((format), makeLogDataArray({__VA_ARGS__}), __FILE__, __func__, __LINE__)

    #define LOG_TRUE_1(should_log, format, ...) if ((should_log)) log((format), makeLogDataArray({__VA_ARGS__}), __FILE__, __func__, __LINE__)
    #define LOG_TRUE_2(should_log, format, ...) if ((should_log)) log((format), makeLogDataArray({__VA_ARGS__}), __FILE__, __func__, __LINE__)
    #define LOG_TRUE_3(should_log, format, ...) if ((should_log)) log((format), makeLogDataArray({__VA_ARGS__}), __FILE__, __func__, __LINE__)
    #define LOG_TRUE_4(should_log, format, ...) if ((should_log)) log((format), makeLogDataArray({__VA_ARGS__}), __FILE__, __func__, __LINE__)

    #define LOG_FALSE_1(should_log, format, ...) if (!(should_log)) log((format), makeLogDataArray({__VA_ARGS__}), __FILE__, __func__, __LINE__)
    #define LOG_FALSE_2(should_log, format, ...) if (!(should_log)) log((format), makeLogDataArray({__VA_ARGS__}), __FILE__, __func__, __LINE__)
    #define LOG_FALSE_3(should_log, format, ...) if (!(should_log)) log((format), makeLogDataArray({__VA_ARGS__}), __FILE__, __func__, __LINE__)
    #define LOG_FALSE_4(should_log, format, ...) if (!(should_log)) log((format), makeLogDataArray({__VA_ARGS__}), __FILE__, __func__, __LINE__)

    #define LOG_IF_1(expr, str_succeeded, str_failed) if ((expr)) { log((str_succeeded), __FILE__, __func__, __LINE__); } else { log((str_succeeded), __FILE__, __func__, __LINE__); }
    #define LOG_IF_2(expr, str_succeeded, str_failed) if ((expr)) { log((str_succeeded), __FILE__, __func__, __LINE__); } else { log((str_succeeded), __FILE__, __func__, __LINE__); }
    #define LOG_IF_3(expr, str_succeeded, str_failed) if ((expr)) { log((str_succeeded), __FILE__, __func__, __LINE__); } else { log((str_succeeded), __FILE__, __func__, __LINE__); }
    #define LOG_IF_4(expr, str_succeeded, str_failed) if ((expr)) { log((str_succeeded), __FILE__, __func__, __LINE__); } else { log((str_succeeded), __FILE__, __func__, __LINE__); }

    #define LOG_CYCLE_START(var_name) uint64 var_name##_start_time = intrin_timestamp_counter()
    #define LOG_CYCLE_END(var_name, format) \
        uint64 var_name##_duration = (uint64) (intrin_timestamp_counter() - var_name##_start_time); \
        LOG_1((format), {DATA_TYPE_UINT64, &var_name##_duration})

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
    #define LOG_1(format, ...) log((format), makeLogDataArray({__VA_ARGS__}), __FILE__, __func__, __LINE__)
    #define LOG_2(format, ...) log((format), makeLogDataArray({__VA_ARGS__}), __FILE__, __func__, __LINE__)
    #define LOG_3(format, ...) log((format), makeLogDataArray({__VA_ARGS__}), __FILE__, __func__, __LINE__)
    #define LOG_4(format, ...) ((void) 0)

    #define LOG_TRUE_1(should_log, format, ...) if ((should_log)) log((format), makeLogDataArray({__VA_ARGS__}), __FILE__, __func__, __LINE__)
    #define LOG_TRUE_2(should_log, format, ...) if ((should_log)) log((format), makeLogDataArray({__VA_ARGS__}), __FILE__, __func__, __LINE__)
    #define LOG_TRUE_3(should_log, format, ...) if ((should_log)) log((format), makeLogDataArray({__VA_ARGS__}), __FILE__, __func__, __LINE__)
    #define LOG_TRUE_4(should_log, format, ...) ((void) 0)

    #define LOG_FALSE_1(should_log, format, ...) if (!(should_log)) log((format), makeLogDataArray({__VA_ARGS__}), __FILE__, __func__, __LINE__)
    #define LOG_FALSE_2(should_log, format, ...) if (!(should_log)) log((format), makeLogDataArray({__VA_ARGS__}), __FILE__, __func__, __LINE__)
    #define LOG_FALSE_3(should_log, format, ...) if (!(should_log)) log((format), makeLogDataArray({__VA_ARGS__}), __FILE__, __func__, __LINE__)
    #define LOG_FALSE_4(should_log, format, ...) ((void) 0)

    #define LOG_IF_1(expr, str_succeeded, str_failed) if ((expr)) { log((str_succeeded), __FILE__, __func__, __LINE__); } else { log((str_succeeded), __FILE__, __func__, __LINE__); }
    #define LOG_IF_2(expr, str_succeeded, str_failed) if ((expr)) { log((str_succeeded), __FILE__, __func__, __LINE__); } else { log((str_succeeded), __FILE__, __func__, __LINE__); }
    #define LOG_IF_3(expr, str_succeeded, str_failed) if ((expr)) { log((str_succeeded), __FILE__, __func__, __LINE__); } else { log((str_succeeded), __FILE__, __func__, __LINE__); }
    // Only logs on failure
    #define LOG_IF_4(expr, str_succeeded, str_failed) if (!(expr)) log((str_succeeded), __FILE__, __func__, __LINE__)

    #define LOG_CYCLE_START(var_name) uint64 var_name##_start_time = intrin_timestamp_counter()
    #define LOG_CYCLE_END(var_name, format) \
        uint64 var_name##_duration = (uint64) (intrin_timestamp_counter() - var_name##_start_time); \
        LOG_1((format), {DATA_TYPE_UINT64, &var_name##_duration})

    #define DEBUG_VERBOSE(str) ((void) 0)
    #define DEBUG_FORMAT_VERBOSE(str, ...) ((void) 0)
#elif LOG_LEVEL == 2
    #define LOG_1(format, ...) log((format), makeLogDataArray({__VA_ARGS__}), __FILE__, __func__, __LINE__)
    #define LOG_2(format, ...) log((format), makeLogDataArray({__VA_ARGS__}), __FILE__, __func__, __LINE__)
    #define LOG_3(format, ...) ((void) 0)
    #define LOG_4(format, ...) ((void) 0)

    #define LOG_TRUE_1(should_log, format, ...) if ((should_log)) log((format), makeLogDataArray({__VA_ARGS__}), __FILE__, __func__, __LINE__)
    #define LOG_TRUE_2(should_log, format, ...) if ((should_log)) log((format), makeLogDataArray({__VA_ARGS__}), __FILE__, __func__, __LINE__)
    #define LOG_TRUE_3(should_log, format, ...) ((void) 0)
    #define LOG_TRUE_4(should_log, format, ...) ((void) 0)

    #define LOG_FALSE_1(should_log, format, ...) if (!(should_log)) log((format), makeLogDataArray({__VA_ARGS__}), __FILE__, __func__, __LINE__)
    #define LOG_FALSE_2(should_log, format, ...) if (!(should_log)) log((format), makeLogDataArray({__VA_ARGS__}), __FILE__, __func__, __LINE__)
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
        LOG_1((format), {DATA_TYPE_UINT64, &var_name##_duration})

    #define DEBUG_VERBOSE(str) ((void) 0)
    #define DEBUG_FORMAT_VERBOSE(str, ...) ((void) 0)
#elif LOG_LEVEL == 1
    #define LOG_1(format, ...) log((format), makeLogDataArray({__VA_ARGS__}), __FILE__, __func__, __LINE__)
    #define LOG_2(format, ...) ((void) 0)
    #define LOG_3(format, ...) ((void) 0)
    #define LOG_4(format, ...) ((void) 0)

    #define LOG_TRUE_1(should_log, format, ...) if ((should_log)) log((format), makeLogDataArray({__VA_ARGS__}), __FILE__, __func__, __LINE__)
    #define LOG_TRUE_2(should_log, format, ...) ((void) 0)
    #define LOG_TRUE_3(should_log, format, ...) ((void) 0)
    #define LOG_TRUE_4(should_log, format, ...) ((void) 0)

    #define LOG_FALSE_1(should_log, format, ...) if (!(should_log)) log((format), makeLogDataArray({__VA_ARGS__}), __FILE__, __func__, __LINE__)
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