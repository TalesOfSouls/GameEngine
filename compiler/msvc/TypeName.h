/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_COMPILER_MSVC_TYPE_NAME_H
#define COMS_COMPILER_MSVC_TYPE_NAME_H

#include "CompilerUtils.h"
#include "../../utils/StringUtils.h"

template<typename T>
CONSTEXPR auto GetRawTypeName() {
    CONSTEXPR const char* fn = __FUNCSIG__;
    CONSTEXPR const char* prefix = "GetRawTypeName<";
    CONSTEXPR const char* suffix = ">(";
    CONSTEXPR const char* start = strstr(fn, prefix);
    CONSTEXPR const char* adjusted_start = start ? start + strlen(prefix) : fn;

    CONSTEXPR const char* end = strstr(adjusted_start, suffix);
    CONSTEXPR const char* final_start = end ? adjusted_start : fn;
    CONSTEXPR size_t length = end ? (end - adjusted_start) : strlen(adjusted_start);

    // Create a struct that holds the string in a CONSTEXPR-friendly way
    struct Result {
        char str[128] = {0};

        CONSTEXPR Result() {
            for (size_t i = 0; i < length && i < 127; ++i) {
                str[i] = final_start[i];
            }
            str[length < 127 ? length : 127] = '\0';
        }

        CONSTEXPR const char* Get() const { return str; }
    };

    // This will create a static storage duration object when used at runtime
    static CONSTEXPR Result result;
    return result.Get();
}

#endif