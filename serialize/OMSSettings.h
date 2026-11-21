/**
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_SERIALIZE_OMSSETTINGS_H
#define COMS_SERIALIZE_OMSSETTINGS_H

#include "../stdlib/Stdlib.h"

struct SettingsMatch {
    const char* name;
    DataType type;
    size_t offset;
    int32 count;
};
/**
 * Example for a struct because structs are a little bit more unintuitive
 * First you define the struct and then afterwards you define the struct members
 * The text file output is like an inline array e.g. "key_forward[] 12 13 14 1 2"
 * The first 3 elements describe the uint16 array and then we have 2 uint8 values
 *
 * {"key_forward", DATA_TYPE_STRUCT, offsetof(InputSettings, key_forward), 3},
 * {"key_forward.scan_codes", DATA_TYPE_UINT16_ARRAY, offsetof(InputSettings, key_forward) + offsetof(Hotkey, scan_codes), ARRAY_COUNT_MEMBER(Hotkey, scan_codes)},
 * {"key_forward.key_state", DATA_TYPE_UINT8, offsetof(InputSettings, key_forward) + offsetof(Hotkey, key_state), 1},
 * {"key_forward.context", DATA_TYPE_UINT8, offsetof(InputSettings, key_forward) + offsetof(Hotkey, context), 1},
 */

// Characters that end the name of a setting in a settings file ("name[] 1 2", "name{} 1 2 3", "name: 1")
#define SETTINGS_NAME_END "\r\n\t :[{"

/**
 * Length of the setting name at the beginning of a line (without "[]", "{}")
 */
static inline
int32 settings_name_length(const char* const line) NO_EXCEPT
{
    int32 length = 0;
    while (line[length] && !str_find(SETTINGS_NAME_END, line[length])) {
        ++length;
    }

    return length;
}

static inline
char* settings_save_name(
    const SettingsMatch* const __restrict match,
    char* __restrict out
) NO_EXCEPT
{
    const int32 name_length = (int32) str_length(match->name);
    str_copy(out, match->name, name_length);
    out += name_length;

    switch (match->type) {
        case DATA_TYPE_BOOL_ARRAY: FALLTHROUGH;
        case DATA_TYPE_INT8_ARRAY: FALLTHROUGH;
        case DATA_TYPE_INT16_ARRAY: FALLTHROUGH;
        case DATA_TYPE_INT32_ARRAY: FALLTHROUGH;
        case DATA_TYPE_INT64_ARRAY: FALLTHROUGH;
        case DATA_TYPE_UINT8_ARRAY: FALLTHROUGH;
        case DATA_TYPE_UINT16_ARRAY: FALLTHROUGH;
        case DATA_TYPE_UINT32_ARRAY: FALLTHROUGH;
        case DATA_TYPE_UINT64_ARRAY: FALLTHROUGH;
        case DATA_TYPE_F32_ARRAY: FALLTHROUGH;
        case DATA_TYPE_F64_ARRAY: FALLTHROUGH;
        case DATA_TYPE_V4_F32_ARRAY: {
            *out++ = '[';
            *out++ = ']';
        } break;
        case DATA_TYPE_STRUCT: {
            *out++ = '{';
            *out++ = '}';
        } break;
        default: break;
    }

    // There must be a whitespace after the settings name
    *out++ = ' ';

    return out;
}

// Decimals of floats in the settings file (str_to_float reads at most 6 decimals)
#define SETTINGS_FLOAT_PRECISION 6

/**
 * Writes a float without unnecessary trailing zeros (e.g. "0.5", "1.0", "0.785398")
 * A value written this way is read back as the same value (@see str_to_float)
 */
static inline
char* settings_save_float(f64 value, char* __restrict out) NO_EXCEPT
{
    if (value < 0.0) {
        *out++ = '-';
        value = -value;
    }

    // Rounded fixed point value (float_to_str truncates the decimals -> 0.785398 would become 0.785397)
    const uint64 scale = 1000000;
    const uint64 fixed = (uint64) (value * (f64) scale + 0.5);

    out += int_to_str((uint64) (fixed / scale), out);
    *out++ = '.';

    // Decimals with leading zeros, without unnecessary trailing zeros (at least one decimal)
    char decimals[SETTINGS_FLOAT_PRECISION];
    uint64 fraction = fixed % scale;
    for (int32 i = SETTINGS_FLOAT_PRECISION - 1; i >= 0; --i) {
        decimals[i] = (char) ('0' + fraction % 10);
        fraction /= 10;
    }

    int32 length = SETTINGS_FLOAT_PRECISION;
    while (length > 1 && decimals[length - 1] == '0') {
        --length;
    }

    memcpy(out, decimals, length);
    out += length;
    *out = '\0';

    return out;
}

/**
 * Writes the value of one integer/float element
 *
 * @param DataType type Element type (e.g. DATA_TYPE_UINT16 for DATA_TYPE_UINT16_ARRAY)
 */
static inline
char* settings_save_number(const byte* const __restrict member, DataType type, char* __restrict out) NO_EXCEPT
{
    switch (type) {
        case DATA_TYPE_BOOL: out += int_to_str((int64) *((const bool *) member), out); break;
        case DATA_TYPE_INT8: out += int_to_str((int64) *((const int8 *) member), out); break;
        case DATA_TYPE_INT16: out += int_to_str((int64) *((const int16 *) member), out); break;
        case DATA_TYPE_INT32: out += int_to_str((int64) *((const int32 *) member), out); break;
        case DATA_TYPE_INT64: out += int_to_str((int64) *((const int64 *) member), out); break;
        case DATA_TYPE_UINT8: out += int_to_str((int64) *((const uint8 *) member), out); break;
        case DATA_TYPE_UINT16: out += int_to_str((int64) *((const uint16 *) member), out); break;
        case DATA_TYPE_UINT32: out += int_to_str((int64) *((const uint32 *) member), out); break;
        case DATA_TYPE_UINT64: out += int_to_str((uint64) *((const uint64 *) member), out); break;
        case DATA_TYPE_F32: out = settings_save_float((f64) *((const f32 *) member), out); break;
        case DATA_TYPE_F64: out = settings_save_float(*((const f64 *) member), out); break;
        default: UNREACHABLE();
    }

    return out;
}

/**
 * Writes "v0 v1 v2 ..." of an array
 */
static inline
char* settings_save_array(
    const byte* const __restrict member, DataType element_type, int32 element_size, int32 count,
    char* __restrict out
) NO_EXCEPT
{
    for (int32 j = 0; j < count; ++j) {
        if (j) {
            *out++ = ' ';
        }

        out = settings_save_number(member + element_size * j, element_type, out);
    }

    return out;
}

static inline
char* settings_save_value(
    const byte* const __restrict settings_data,
    const SettingsMatch* const __restrict match, int32 match_index,
    char* out
) NO_EXCEPT
{
    const byte* const member = settings_data + match[match_index].offset;
    const int32 count = match[match_index].count;

    switch (match[match_index].type) {
        case DATA_TYPE_BOOL: FALLTHROUGH;
        case DATA_TYPE_INT8: FALLTHROUGH;
        case DATA_TYPE_INT16: FALLTHROUGH;
        case DATA_TYPE_INT32: FALLTHROUGH;
        case DATA_TYPE_INT64: FALLTHROUGH;
        case DATA_TYPE_UINT8: FALLTHROUGH;
        case DATA_TYPE_UINT16: FALLTHROUGH;
        case DATA_TYPE_UINT32: FALLTHROUGH;
        case DATA_TYPE_UINT64: FALLTHROUGH;
        case DATA_TYPE_F32: FALLTHROUGH;
        case DATA_TYPE_F64: {
            out = settings_save_number(member, match[match_index].type, out);
        } break;

        // Array data (same format as settings_load_value: "name[] 1 2 3")
        case DATA_TYPE_BOOL_ARRAY: out = settings_save_array(member, DATA_TYPE_BOOL, (int32) sizeof(bool), count, out); break;
        case DATA_TYPE_INT8_ARRAY: out = settings_save_array(member, DATA_TYPE_INT8, (int32) sizeof(int8), count, out); break;
        case DATA_TYPE_INT16_ARRAY: out = settings_save_array(member, DATA_TYPE_INT16, (int32) sizeof(int16), count, out); break;
        case DATA_TYPE_INT32_ARRAY: out = settings_save_array(member, DATA_TYPE_INT32, (int32) sizeof(int32), count, out); break;
        case DATA_TYPE_INT64_ARRAY: out = settings_save_array(member, DATA_TYPE_INT64, (int32) sizeof(int64), count, out); break;
        case DATA_TYPE_UINT8_ARRAY: out = settings_save_array(member, DATA_TYPE_UINT8, (int32) sizeof(uint8), count, out); break;
        case DATA_TYPE_UINT16_ARRAY: out = settings_save_array(member, DATA_TYPE_UINT16, (int32) sizeof(uint16), count, out); break;
        case DATA_TYPE_UINT32_ARRAY: out = settings_save_array(member, DATA_TYPE_UINT32, (int32) sizeof(uint32), count, out); break;
        case DATA_TYPE_UINT64_ARRAY: out = settings_save_array(member, DATA_TYPE_UINT64, (int32) sizeof(uint64), count, out); break;
        case DATA_TYPE_F32_ARRAY: out = settings_save_array(member, DATA_TYPE_F32, (int32) sizeof(f32), count, out); break;
        case DATA_TYPE_F64_ARRAY: out = settings_save_array(member, DATA_TYPE_F64, (int32) sizeof(f64), count, out); break;

        // Vectors
        case DATA_TYPE_V4_F32: {
            out = settings_save_array(member, DATA_TYPE_F32, (int32) sizeof(f32), 4, out);
        } break;
        case DATA_TYPE_V4_F32_ARRAY: {
            out = settings_save_array(member, DATA_TYPE_F32, (int32) sizeof(f32), 4 * count, out);
        } break;

        // String data
        case DATA_TYPE_CHAR: {
            if (*member) {
                *out++ = (char) *member;
            }
        } break;
        case DATA_TYPE_CHAR_STR: {
            out += (count > 1)
                ? str_copy_to_eol((const char *) member, out, count)
                : str_copy_to_eol((const char *) member, out);
        } break;
        case DATA_TYPE_WCHAR_STR: {
            char temp[256];
            wchar_to_char(temp, (const wchar_t *) member);

            out += str_copy_to_eol((const char *) temp, out);
        } break;
        case DATA_TYPE_STRUCT: {
            // WARNING: This does not support recursive/nested structs
            // The .count represents how many members the struct has, the N following matches are the members
            for (int32 struct_count = 1; struct_count <= count; ++struct_count) {
                if (struct_count > 1) {
                    *out++ = ' ';
                }

                out = settings_save_value(settings_data, match, match_index + struct_count, out);
            }
        } break;
        default:
            UNREACHABLE();
    }

    return out;
}

/**
 * Writes one complete setting line without the line break ("name{} 1 2 3", "name 5")
 *
 * @param int32 match_index Index of the match (for structs the index of the struct match)
 */
inline
char* settings_save_line(
    const void* const __restrict settings_data,
    const SettingsMatch* const __restrict match, int32 match_index,
    char* __restrict out
) NO_EXCEPT
{
    out = settings_save_name(&match[match_index], out);

    return settings_save_value((const byte *) settings_data, match, match_index, out);
}

/**
 * Element type + size of an array type (e.g. DATA_TYPE_UINT8 for DATA_TYPE_UINT8_ARRAY)
 *
 * @return bool false if the type is no number array
 */
static inline
bool settings_array_element(DataType type, DataType* const __restrict element_type, int32* const __restrict element_size) NO_EXCEPT
{
    switch (type) {
        case DATA_TYPE_BOOL_ARRAY: *element_type = DATA_TYPE_BOOL; *element_size = (int32) sizeof(bool); return true;
        case DATA_TYPE_INT8_ARRAY: *element_type = DATA_TYPE_INT8; *element_size = (int32) sizeof(int8); return true;
        case DATA_TYPE_INT16_ARRAY: *element_type = DATA_TYPE_INT16; *element_size = (int32) sizeof(int16); return true;
        case DATA_TYPE_INT32_ARRAY: *element_type = DATA_TYPE_INT32; *element_size = (int32) sizeof(int32); return true;
        case DATA_TYPE_INT64_ARRAY: *element_type = DATA_TYPE_INT64; *element_size = (int32) sizeof(int64); return true;
        case DATA_TYPE_UINT8_ARRAY: *element_type = DATA_TYPE_UINT8; *element_size = (int32) sizeof(uint8); return true;
        case DATA_TYPE_UINT16_ARRAY: *element_type = DATA_TYPE_UINT16; *element_size = (int32) sizeof(uint16); return true;
        case DATA_TYPE_UINT32_ARRAY: *element_type = DATA_TYPE_UINT32; *element_size = (int32) sizeof(uint32); return true;
        case DATA_TYPE_UINT64_ARRAY: *element_type = DATA_TYPE_UINT64; *element_size = (int32) sizeof(uint64); return true;
        case DATA_TYPE_F32_ARRAY: *element_type = DATA_TYPE_F32; *element_size = (int32) sizeof(f32); return true;
        case DATA_TYPE_F64_ARRAY: *element_type = DATA_TYPE_F64; *element_size = (int32) sizeof(f64); return true;
        default: return false;
    }
}

/**
 * Writes the new version of an existing line of a settings file
 *      "name[2] 5"         -> only this array element is written
 *      "name 5 // comment" -> the comment is kept
 *      "name"              -> stays without value as long as the value is empty/0 (e.g. "not set" settings)
 *
 * @param const char*   line        Existing line (without line break)
 * @param int32         name_length Length of the name at the beginning of the line (@see settings_name_length)
 */
static inline
char* settings_save_existing_line(
    const void* const __restrict settings_data,
    const SettingsMatch* const __restrict match, int32 match_index,
    const char* const __restrict line, int32 name_length,
    char* __restrict out
) NO_EXCEPT
{
    const SettingsMatch* const setting = &match[match_index];
    const char* pos = line + name_length;

    // "name[N] value" = one array element
    int32 index = -1;
    if (pos[0] == '[' && pos[1] >= '0' && pos[1] <= '9') {
        index = (int32) str_to_int(pos + 1);
    }

    // Skip the name suffix ("[]", "[N]", "{}", ":") + whitespace
    while (*pos && *pos != ' ' && *pos != '\t') {
        ++pos;
    }

    while (*pos == ' ' || *pos == '\t') {
        ++pos;
    }

    // Trailing comment ("//" after a whitespace), not for strings (e.g. urls)
    const char* comment = NULL;
    if (setting->type != DATA_TYPE_CHAR && setting->type != DATA_TYPE_CHAR_STR && setting->type != DATA_TYPE_WCHAR_STR) {
        comment = (pos[0] == '/' && pos[1] == '/') ? pos : str_find(pos, " //");
    }

    const bool has_value = *pos && pos != comment;

    char* const start = out;

    DataType element_type;
    int32 element_size;
    if (index >= 0 && index < setting->count && settings_array_element(setting->type, &element_type, &element_size)) {
        memcpy(out, setting->name, name_length);
        out += name_length;
        *out++ = '[';
        out += int_to_str((int64) index, out);
        *out++ = ']';
        *out++ = ' ';

        const char* const member = (const char *) settings_data + setting->offset;
        out = settings_save_number((const byte *) member + element_size * index, element_type, out);
    } else {
        out = settings_save_line(settings_data, match, match_index, out);
    }

    if (!has_value) {
        // Only zeros (or nothing) -> keep the line without value
        const char* value = start + name_length;
        while (*value && *value != ' ') {
            ++value;
        }

        bool is_zero = true;
        for (const char* c = value; c < out; ++c) {
            if (*c != ' ' && *c != '0' && *c != '.' && *c != '-') {
                is_zero = false;
                break;
            }
        }

        if (is_zero) {
            out = start;
            out += str_copy_to_eol(line, out);

            return out;
        }
    }

    if (comment) {
        if (comment[0] != ' ') {
            *out++ = ' ';
        }

        out += str_copy_to_eol(comment, out);
    }

    *out = '\0';

    return out;
}

/**
 * Does a settings file content contain a setting (exact name, with or without "[]"/"{}")
 */
inline
bool settings_file_contains(const char* __restrict in, const char* __restrict name) NO_EXCEPT
{
    const int32 name_length = (int32) str_length(name);

    while (*in) {
        while (*in == ' ' || *in == '\t') {
            ++in;
        }

        if (settings_name_length(in) == name_length && str_compare(in, name, name_length) == 0) {
            return true;
        }

        // Next line
        while (*in && *in != '\n') {
            ++in;
        }

        if (*in) {
            ++in;
        }
    }

    return false;
}

/**
 * Creates a new settings file content based on an existing file
 * Comments, empty lines and unknown settings are kept, the values of known settings are replaced
 * Settings that are not in the old file are NOT added (@see settings_save_line)
 *
 * @param const char* in Content of the existing file (NULL or "" = empty file)
 *
 * @return size_t Length of the output
 */
size_t settings_save(
    const void* const __restrict settings_data,
    const char* __restrict in,
    char* __restrict out, size_t out_length,
    const SettingsMatch* const __restrict match,
    int32 match_count
)  NO_EXCEPT
{
    char line[2048];

    const char* const start = out;

    if (!in) {
        in = "";
    }

    int32 offset;
    while (*in && out_length - (out - start) > sizeof(line) + 128) {
        // The line breaks are kept (\n or \r\n)
        while (str_is_eol(*in)) {
            const int32 eol = is_eol(in) ? is_eol(in) : 1;
            memcpy(out, in, eol);
            out += eol;
            in += eol;
        }

        if (!*in) {
            break;
        }

        if (in[0] == '/' && in[1] == '/') {
            offset = str_copy_to_eol(in, out);
            in += offset;
            out += offset;

            continue;
        }

        in += str_copy_to_eol(in, line, sizeof(line) - 1);

        // The name in the file may have a suffix ("name[]", "name{}")
        const int32 name_length = settings_name_length(line);

        bool found = false;
        for (int32 i = 0; i < match_count; ++i) {
            if (name_length > 0
                && str_compare(line, match[i].name, name_length) == 0
                && match[i].name[name_length] == '\0'
            ) {
                out = settings_save_existing_line(settings_data, match, i, line, name_length, out);

                found = true;
                break;
            }
        }

        if (found) {
            continue;
        }

        // Couldn't find any match -> just copy old values
        out += str_copy_to_eol(line, out);
    }

    *out = '\0';

    return (size_t) (out - start);
}

static inline
const char* settings_load_value(
    byte* const __restrict settings_data,
    const char* __restrict pos,
    const SettingsMatch* const __restrict match,
    int32 match_index
) NO_EXCEPT
{
    void* const member = (void *) (settings_data + match[match_index].offset);

    str_skip_whitespace(&pos);

    // I am doing direct assignments to 'member' instead of memcpy
    // Normally this would be UB, but I think it should be fine here
    // because 'member' should be correctly aligned. Afterall it's just a pointer into a struct member
    switch (match[match_index].type) {
        case DATA_TYPE_BOOL: {
            *((bool *) member) = (bool) str_to_int(pos);
        } break;
        case DATA_TYPE_INT8: {
            *((int8 *) member) = (int8) str_to_int(pos);
        } break;
        case DATA_TYPE_INT16: {
            *((int16 *) member) = (int16) str_to_int(pos);
        } break;
        case DATA_TYPE_INT32: {
            *((int32 *) member) = (int32) str_to_int(pos);
        } break;
        case DATA_TYPE_INT64: {
            *((int64 *) member) = (int64) str_to_int(pos);
        } break;
        case DATA_TYPE_UINT8: {
            *((uint8 *) member) = (uint8) str_to_int(pos);
        } break;
        case DATA_TYPE_UINT16: {
            *((uint16 *) member) = (uint16) str_to_int(pos);
        } break;
        case DATA_TYPE_UINT32: {
            *((uint32 *) member) = (uint32) str_to_int(pos);
        } break;
        case DATA_TYPE_UINT64: {
            *((uint64 *) member) = (uint64) str_to_int(pos);
        } break;
        case DATA_TYPE_F32: {
            *((f32 *) member) = (f32) str_to_float(pos, &pos);
        } break;
        case DATA_TYPE_F64: {
            *((f64 *) member) = (f64) str_to_float(pos, &pos);
        } break;

        // Array data
        case DATA_TYPE_BOOL_ARRAY: {
            int32 index = -1;
            if (*pos == '[') {
                // move past [
                ++pos;

                if (isdigit(*pos)) {
                    index = (int32) str_to_int(pos, &pos); ++pos;
                }

                str_move_past(&pos, ' ');
            }

            if (index >= 0) {
                // handles name[0] 1
                ((bool *) member)[index] = (bool) str_to_int(pos);
            } else {
                // handles name[] 1 1 0 1 0
                for (int match_count = 0; match_count < match[match_index].count; ++match_count) {
                    str_skip_whitespace(&pos);
                    ((bool *) member)[++index] = (bool) str_to_int(pos);
                    str_move_to(&pos, " \n");
                }
            }
        } break;
        case DATA_TYPE_INT8_ARRAY: {
            int32 index = -1;
            if (*pos == '[') {
                // move past [
                ++pos;

                if (isdigit(*pos)) {
                    index = (int32) str_to_int(pos, &pos); ++pos;
                }

                str_move_past(&pos, ' ');
            }

            if (index >= 0) {
                // handles name[0] 1
                ((int8 *) member)[index] = (int8) str_to_int(pos);
            } else {
                // handles name[] 1 1 0 1 0
                for (int match_count = 0; match_count < match[match_index].count; ++match_count) {
                    str_skip_whitespace(&pos);
                    ((int8 *) member)[++index] = (int8) str_to_int(pos);
                    str_move_to(&pos, " \n");
                }
            }
        } break;
        case DATA_TYPE_INT16_ARRAY: {
            int32 index = -1;
            if (*pos == '[') {
                // move past [
                ++pos;

                if (isdigit(*pos)) {
                    index = (int32) str_to_int(pos, &pos); ++pos;
                }

                str_move_past(&pos, ' ');
            }

            if (index >= 0) {
                // handles name[0] 1
                ((int16 *) member)[index] = (int16) str_to_int(pos);
            } else {
                // handles name[] 1 1 0 1 0
                for (int match_count = 0; match_count < match[match_index].count; ++match_count) {
                    str_skip_whitespace(&pos);
                    ((int16 *) member)[++index] = (int16) str_to_int(pos);
                    str_move_to(&pos, " \n");
                }
            }
        } break;
        case DATA_TYPE_INT32_ARRAY: {
            int32 index = -1;
            if (*pos == '[') {
                // move past [
                ++pos;

                if (isdigit(*pos)) {
                    index = (int32) str_to_int(pos, &pos); ++pos;
                }

                str_move_past(&pos, ' ');
            }

            if (index >= 0) {
                // handles name[0] 1
                ((int32 *) member)[index] = (int32) str_to_int(pos);
            } else {
                // handles name[] 1 1 0 1 0
                for (int match_count = 0; match_count < match[match_index].count; ++match_count) {
                    str_skip_whitespace(&pos);
                    ((int32 *) member)[++index] = (int32) str_to_int(pos);
                    str_move_to(&pos, " \n");
                }
            }
        } break;
        case DATA_TYPE_INT64_ARRAY: {
            int32 index = -1;
            if (*pos == '[') {
                // move past [
                ++pos;

                if (isdigit(*pos)) {
                    index = (int32) str_to_int(pos, &pos); ++pos;
                }

                str_move_past(&pos, ' ');
            }

            if (index >= 0) {
                // handles name[0] 1
                ((int64 *) member)[index] = (int64) str_to_int(pos);
            } else {
                // handles name[] 1 1 0 1 0
                for (int match_count = 0; match_count < match[match_index].count; ++match_count) {
                    str_skip_whitespace(&pos);
                    ((int64 *) member)[++index] = (int64) str_to_int(pos);
                    str_move_to(&pos, " \n");
                }
            }
        } break;
        case DATA_TYPE_UINT8_ARRAY: {
            int32 index = -1;
            if (*pos == '[') {
                // move past [
                ++pos;

                if (isdigit(*pos)) {
                    index = (int32) str_to_int(pos, &pos); ++pos;
                }

                str_move_past(&pos, ' ');
            }

            if (index >= 0) {
                // handles name[0] 1
                ((uint8 *) member)[index] = (uint8) str_to_int(pos);
            } else {
                // handles name[] 1 1 0 1 0
                for (int match_count = 0; match_count < match[match_index].count; ++match_count) {
                    str_skip_whitespace(&pos);
                    ((uint8 *) member)[++index] = (uint8) str_to_int(pos);
                    str_move_to(&pos, " \n");
                }
            }
        } break;
        case DATA_TYPE_UINT16_ARRAY: {
            int32 index = -1;
            if (*pos == '[') {
                // move past [
                ++pos;

                if (isdigit(*pos)) {
                    index = (int32) str_to_int(pos, &pos); ++pos;
                }

                str_move_past(&pos, ' ');
            }

            if (index >= 0) {
                // handles name[0] 1
                ((uint16 *) member)[index] = (uint16) str_to_int(pos);
            } else {
                // handles name[] 1 1 0 1 0
                for (int match_count = 0; match_count < match[match_index].count; ++match_count) {
                    str_skip_whitespace(&pos);
                    ((uint16 *) member)[++index] = (uint16) str_to_int(pos);
                    str_move_to(&pos, " \n");
                }
            }
        } break;
        case DATA_TYPE_UINT32_ARRAY: {
            int32 index = -1;
            if (*pos == '[') {
                // move past [
                ++pos;

                if (isdigit(*pos)) {
                    index = (int32) str_to_int(pos, &pos); ++pos;
                }

                str_move_past(&pos, ' ');
            }

            if (index >= 0) {
                // handles name[0] 1
                ((uint32 *) member)[index] = (uint32) str_to_int(pos);
            } else {
                // handles name[] 1 1 0 1 0
                for (int match_count = 0; match_count < match[match_index].count; ++match_count) {
                    str_skip_whitespace(&pos);
                    ((uint32 *) member)[++index] = (uint32) str_to_int(pos);
                    str_move_to(&pos, " \n");
                }
            }
        } break;
        case DATA_TYPE_UINT64_ARRAY: {
            int32 index = -1;
            if (*pos == '[') {
                // move past [
                ++pos;

                if (isdigit(*pos)) {
                    index = (int32) str_to_int(pos, &pos); ++pos;
                }

                str_move_past(&pos, ' ');
            }

            if (index >= 0) {
                // handles name[0] 1
                ((uint64 *) member)[index] = (uint64) str_to_int(pos);
            } else {
                // handles name[] 1 1 0 1 0
                for (int match_count = 0; match_count < match[match_index].count; ++match_count) {
                    str_skip_whitespace(&pos);
                    ((uint64 *) member)[++index] = (uint64) str_to_int(pos);
                    str_move_to(&pos, " \n");
                }
            }
        } break;
        case DATA_TYPE_F32_ARRAY: {
            int32 index = -1;
            if (*pos == '[') {
                // move past [
                ++pos;

                if (isdigit(*pos)) {
                    index = (int32) str_to_int(pos, &pos); ++pos;
                }

                str_move_past(&pos, ' ');
            }

            if (index >= 0) {
                // handles name[0] 1
                ((f32 *) member)[index] = (f32) str_to_float(pos);
            } else {
                // handles name[] 1 1 0 1 0
                for (int match_count = 0; match_count < match[match_index].count; ++match_count) {
                    str_skip_whitespace(&pos);
                    ((f32 *) member)[++index] = (f32) str_to_float(pos);
                    str_move_to(&pos, " \n");
                }
            }
        } break;
        case DATA_TYPE_F64_ARRAY: {
            int32 index = -1;
            if (*pos == '[') {
                // move past [
                ++pos;

                if (isdigit(*pos)) {
                    index = (int32) str_to_int(pos, &pos); ++pos;
                }

                str_move_past(&pos, ' ');
            }

            if (index >= 0) {
                // handles name[0] 1
                ((f64 *) member)[index] = (f64) str_to_float(pos);
            } else {
                // handles name[] 1 1 0 1 0
                for (int match_count = 0; match_count < match[match_index].count; ++match_count) {
                    str_skip_whitespace(&pos);
                    ((f64 *) member)[++index] = (f64) str_to_float(pos);
                    str_move_to(&pos, " \n");
                }
            }
        } break;

        // Vector data
        case DATA_TYPE_V4_F32: {
            *((f32 *) member) = (f32) str_to_float(pos, &pos);
            str_move_past(&pos, ' ');
            *(((f32 *) member) + 1) = (f32) str_to_float(pos, &pos);
            str_move_past(&pos, ' ');
            *(((f32 *) member) + 2) = (f32) str_to_float(pos, &pos);
            str_move_past(&pos, ' ');
            *(((f32 *) member) + 3) = (f32) str_to_float(pos, &pos);
        } break;
        case DATA_TYPE_V4_F32_ARRAY: {
            int32 index = -1;
            if (*pos == '[') {
                // move past [
                ++pos;

                if (isdigit(*pos)) {
                    index = (int32) str_to_int(pos, &pos); ++pos;
                }

                str_move_past(&pos, ' ');
            }

            ASSERT_TRUE(index >= 0);

            // handles name[0] 1.0 2.0 1.0 0.0
            // handles name[1] = 1.0 2.0 1.0 0.0
            ((v4_f32 *) member)[index].vec[0] = (f32) str_to_float(pos);
            str_move_past(&pos, ' ');
            ((v4_f32 *) member)[index].vec[1] = (f32) str_to_float(pos);
            str_move_past(&pos, ' ');
            ((v4_f32 *) member)[index].vec[2] = (f32) str_to_float(pos);
            str_move_past(&pos, ' ');
            ((v4_f32 *) member)[index].vec[3] = (f32) str_to_float(pos);
        } break;

        // String data
        case DATA_TYPE_CHAR: {
            *((char *) member) = *pos;
        } break;
        case DATA_TYPE_CHAR_STR: {
            if (match[match_index].count > 1) {
                str_copy_to_eol(pos, (char *) member, match[match_index].count);
            } else {
                str_copy_to_eol(pos, (char *) member);
            }
        } break;
        case DATA_TYPE_WCHAR_STR: {
            char temp[256];
            str_copy_to_eol(pos, (char *) temp);
            char_to_wchar((wchar_t *) member, temp, match[match_index].count);
        } break;
        case DATA_TYPE_STRUCT: {
            // Structs are a little bit nasty
            // The .count represents how many members the struct has
            // This means the N following elements are the struct members

            // Iterate through all struct members
            for (int struct_count = 1; struct_count <= match[match_index].count; ++struct_count) {
                str_move_to(&pos, ' ');
                pos = settings_load_value(settings_data, pos, match, match_index + struct_count);
            }
        } break;
        default:
            UNREACHABLE();
    }

    return pos;
}

void settings_load(
    void* const __restrict settings_data,
    const char* __restrict data,
    const SettingsMatch* const __restrict match,
    int32 match_count
) NO_EXCEPT
{
    const char* name;

    byte* settings = (byte *) settings_data;

    while (*data != '\0') {
        // Skip all whitespaces and new lines
        str_skip_empty(&data);

        // Skip comment
        if (*data == '/' && data[1] == '/') {
            str_move_to(&data, '\n');

            continue;
        }

        // Get name
        name = data;
        str_move_to(&data, SETTINGS_NAME_END);
        const size_t name_length = (size_t) (data - name);

        // Move to value
        str_skip_whitespace(&data);

        // Is there a value?
        if (is_eol(data) || *data == '\0') {
            continue;
        }

        for (int32 i = 0; i < match_count; ++i) {
            // Exact match (a name may be the prefix of another name)
            if (str_compare(name, match[i].name, name_length) == 0 && match[i].name[name_length] == '\0') {
                data = settings_load_value(settings, data, match, i);

                break;
            }
        }

        // After parsing a line we move to the next line
        // Careful the parsing above does not necessarily move the data pointer.
        // That's why we have to do it here
        str_move_to(&data, '\n');
    }
}

struct SettingsValidation {
    const char* name;

    DataType type;
    size_t offset;
    int32 count;

    union {
        int64 min;
        f64 fmin;
    };

    union {
        int64 max;
        f64 fmax;
    };
};

/**
 * Basic settings validation to spot gross data corruption/invalid values
 *
 * @param const void*               settings_data   Settings struct cast to a void pointer
 * @param const SettingsValidation* match           Array of validation data
 * @param int                       match_count     Validation array size
 *
 * @return int Returns > 0 (1 indexed) for the first invalid value, otherwise 0 on success
 */
int settings_validate(
    const void* const __restrict settings_data,
    const SettingsValidation* const __restrict match,
    int match_count
) {
    const byte* settings = (const byte *) settings_data;

    for (int i = 0; i < match_count; ++i) {
        const void* const member = (const void *) (settings + match[i].offset);

        switch (match[i].type) {
            case DATA_TYPE_INT8: {
                if (*((int8 *) member) < match[i].min || *((int8 *) member) > match[i].max) {
                    return i + 1;
                }
            } break;
            case DATA_TYPE_INT16: {
                if (*((int16 *) member) < match[i].min || *((int16 *) member) > match[i].max) {
                    return i + 1;
                }
            } break;
            case DATA_TYPE_INT32: {
                if (*((int32 *) member) < match[i].min || *((int32 *) member) > match[i].max) {
                    return i + 1;
                }
            } break;
            case DATA_TYPE_INT64:  {
                if (*((int64 *) member) < match[i].min || *((int64 *) member) > match[i].max) {
                    return i + 1;
                }
            } break;
            case DATA_TYPE_UINT8: {
                if (*((uint8 *) member) < match[i].min || *((uint8 *) member) > match[i].max) {
                    return i + 1;
                }
            } break;
            case DATA_TYPE_UINT16: {
                if (*((uint16 *) member) < match[i].min || *((uint16 *) member) > match[i].max) {
                    return i + 1;
                }
            } break;
            case DATA_TYPE_UINT32: {
                if (*((uint32 *) member) < match[i].min || *((uint32 *) member) > match[i].max) {
                    return i + 1;
                }
            } break;
            case DATA_TYPE_UINT64: {
                if (*((uint64 *) member) < (uint64) match[i].min || *((uint64 *) member) > (uint64) match[i].max) {
                    return i + 1;
                }
            } break;
            case DATA_TYPE_F32: {
                if (*((f32 *) member) < match[i].fmin || *((f32 *) member) > match[i].fmax) {
                    return i + 1;
                }
            } break;
            case DATA_TYPE_F64: {
                if (*((f64 *) member) < match[i].fmin || *((f64 *) member) > match[i].fmax) {
                    return i + 1;
                }
            } break;
            default: UNREACHABLE();
        }
    }

    return 0;
}

#endif