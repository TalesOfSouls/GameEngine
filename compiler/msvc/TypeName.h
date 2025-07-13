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
constexpr auto GetRawTypeName() {
    constexpr const char* fn = __FUNCSIG__;
    constexpr const char* prefix = "GetRawTypeName<";
    constexpr const char* suffix = ">(";
    constexpr const char* start = str_find_constexpr(fn, prefix);
    constexpr const char* adjusted_start = start ? start + str_length_constexpr(prefix) : fn;

    constexpr const char* end = str_find_constexpr(adjusted_start, suffix);
    constexpr const char* final_start = end ? adjusted_start : fn;
    constexpr size_t length = end ? (end - adjusted_start) : str_length_constexpr(adjusted_start);

    // Create a struct that holds the string in a constexpr-friendly way
    struct Result {
        char str[128] = {};

        constexpr Result() {
            for (size_t i = 0; i < length && i < 127; ++i) {
                str[i] = final_start[i];
            }
            str[length < 127 ? length : 127] = '\0';
        }

        constexpr const char* Get() const { return str; }
    };

    // This will create a static storage duration object when used at runtime
    static constexpr Result result;
    return result.Get();
}

#endif