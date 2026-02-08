/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
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

static inline
char* settings_save_name(
    const SettingsMatch* const __restrict match,
    char* __restrict out
) NO_EXCEPT
{
    out += str_copy(out, match->name);

    switch (match->type) {
        case DATA_TYPE_UINT16_ARRAY: FALLTHROUGH;
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

static inline
char* settings_save_value(
    const byte* const __restrict settings_data,
    const SettingsMatch* const __restrict match, int32 match_index,
    char* out
) NO_EXCEPT
{
    const byte* const member = settings_data + match[match_index].offset;

    switch (match[match_index].type) {
        case DATA_TYPE_BOOL: {
            out += int_to_str((int64) *member, out);
        } break;
        case DATA_TYPE_INT8: {
            out += int_to_str((int64) *((int8 *) member), out);
        } break;
        case DATA_TYPE_INT16: {
            out += int_to_str((int64) *((int16 *) member), out);
        } break;
        case DATA_TYPE_INT32: {
            out += int_to_str((int64) *((int32 *) member), out);
        } break;
        case DATA_TYPE_INT64: {
            out += int_to_str((int64) *((int64 *) member), out);
        } break;
        case DATA_TYPE_UINT8: {
            out += int_to_str((int64) *((uint8 *) member), out);
        } break;
        case DATA_TYPE_UINT16: {
            out += int_to_str((int64) *((uint16 *) member), out);
        } break;
        case DATA_TYPE_UINT32: {
            out += int_to_str((int64) *((uint32 *) member), out);
        } break;
        case DATA_TYPE_UINT64: {
            out += int_to_str((int64) *((uint64 *) member), out);
        } break;
        case DATA_TYPE_F32: {
            out += float_to_str((f64) *((f32 *) member), out);
        } break;
        case DATA_TYPE_F64: {
            out += float_to_str((f64) *((f64 *) member), out);
        } break;

        // Array data
        case DATA_TYPE_UINT16_ARRAY: {
            for (int32 j = 0; j < match[match_index].count; ++j) {
                out += sprintf_fast(
                    out, "%d\n",
                    (int32) *((uint16 *) member)
                );
            }
        } break;

        // Vectors
        case DATA_TYPE_V4_F32: {
            out += float_to_str((f32) *((f32 *) member), out);
            *out++ = ' ';
            out += float_to_str((f32) *((f32 *) (member + sizeof(f32) * 1)), out);
            *out++ = ' ';
            out += float_to_str((f32) *((f32 *) (member + sizeof(f32) * 2)), out);
            *out++ = ' ';
            out += float_to_str((f32) *((f32 *) (member + sizeof(f32) * 3)), out);
        } break;
        case DATA_TYPE_V4_F32_ARRAY: {
            for (int32 j = 0; j < match[match_index].count; ++j) {
                out += sprintf_fast(
                    out, "%f %f %f %f\n",
                    (f32) *((f32 *) member),
                    (f32) *((f32 *) (member + sizeof(f32) * 1)),
                    (f32) *((f32 *) (member + sizeof(f32) * 2)),
                    (f32) *((f32 *) (member + sizeof(f32) * 3))
                );
            }
        } break;

        // String data
        case DATA_TYPE_CHAR: {
            out += str_copy(out, (const char *) member);
        } break;
        case DATA_TYPE_CHAR_STR: {
            str_copy_to_eol((const char *) member, out);
        } break;
        case DATA_TYPE_WCHAR_STR: {
            char temp[256];
            wchar_to_char(out, (const wchar_t *) member);

            str_copy_to_eol((const char *) temp, out);
        } break;
        case DATA_TYPE_STRUCT: {
            --out;

            // WARNING: This does not support recursive/nested structs
            for (int struct_count = 1; struct_count <= match[match_index].count; ++struct_count) {
                ++out;
                *out = ' ';
                out = settings_save_value(settings_data, match, match_index + struct_count, out);
            }
        } break;
        default:
            UNREACHABLE();
    }

    return out;
}

void settings_save(
    const void* const __restrict settings_data,
    const char* __restrict in,
    char* __restrict out, size_t out_length,
    const SettingsMatch* const __restrict match,
    int32 match_count
)  NO_EXCEPT
{
    char line[2048];
    char setting[64];

    const byte* const settings = (const byte *) settings_data;
    const char* const start = out;

    int32 offset;
    while (*in && out_length - (out - start) > 128) {
        if (in[0] == '/' && in[1] == '/') {
            offset = str_copy_to_eol(in, out);
            in += offset;
            out += offset;

            *out = '\n';
            ++out;

            continue;
        }

        in += str_copy_to_eol(in, line, sizeof(line) - 1);
        sscanf_fast(line, "%s ", setting, 63);

        bool found = false;

        for (int32 i = 0; i < match_count; ++i) {
            if (strcmp(line, match[i].name) == 0) {
                out = settings_save_name(&match[i], out);
                out = settings_save_value(settings, match, i, out);

                // Newline after settings line output
                *out++ = '\n';

                found = true;
                break;
            }
        }

        if (found) {
            continue;
        }

        // Couldn't find any match -> just copy old values
        out += str_copy_to_eol(line, out);
        *out++ = '\n';
    }
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
            // move past [
            ++pos;

            int32 index = -1;
            if (isdigit(*pos)) {
                index = (int32) str_to_int(pos, &pos); ++pos;
            }

            str_move_to(&pos, ' ');
            ++pos;

            if (index >= 0) {
                // handles name[0] 1
                ((bool *) member)[index] = (bool) str_to_int(pos);
            } else {
                // handles name[] 1 1 0 1 0
                for (int match_count = 0; match_count < match[match_index].count; ++match_count) {
                    str_move_past(&pos, ' ');
                    ((bool *) member)[++index] = (bool) str_to_int(pos);
                }
            }
        } break;
        case DATA_TYPE_INT8_ARRAY: {
            // move past [
            ++pos;

            int32 index = -1;
            if (isdigit(*pos)) {
                index = (int32) str_to_int(pos, &pos); ++pos;
            }

            str_move_to(&pos, ' ');
            ++pos;

            if (index >= 0) {
                // handles name[0] 1
                ((int8 *) member)[index] = (int8) str_to_int(pos);
            } else {
                // handles name[] 1 1 0 1 0
                for (int match_count = 0; match_count < match[match_index].count; ++match_count) {
                    str_move_past(&pos, ' ');
                    ((int8 *) member)[++index] = (int8) str_to_int(pos);
                }
            }
        } break;
        case DATA_TYPE_INT16_ARRAY: {
            // move past [
            ++pos;

            int32 index = -1;
            if (isdigit(*pos)) {
                index = (int32) str_to_int(pos, &pos); ++pos;
            }

            str_move_to(&pos, ' ');
            ++pos;

            if (index >= 0) {
                // handles name[0] 1
                ((int16 *) member)[index] = (int16) str_to_int(pos);
            } else {
                // handles name[] 1 1 0 1 0
                for (int match_count = 0; match_count < match[match_index].count; ++match_count) {
                    str_move_past(&pos, ' ');
                    ((int16 *) member)[++index] = (int16) str_to_int(pos);
                }
            }
        } break;
        case DATA_TYPE_INT32_ARRAY: {
            // move past [
            ++pos;

            int32 index = -1;
            if (isdigit(*pos)) {
                index = (int32) str_to_int(pos, &pos); ++pos;
            }

            str_move_to(&pos, ' ');
            ++pos;

            if (index >= 0) {
                // handles name[0] 1
                ((int32 *) member)[index] = (int32) str_to_int(pos);
            } else {
                // handles name[] 1 1 0 1 0
                for (int match_count = 0; match_count < match[match_index].count; ++match_count) {
                    str_move_past(&pos, ' ');
                    ((int32 *) member)[++index] = (int32) str_to_int(pos);
                }
            }
        } break;
        case DATA_TYPE_INT64_ARRAY: {
            // move past [
            ++pos;

            int32 index = -1;
            if (isdigit(*pos)) {
                index = (int32) str_to_int(pos, &pos); ++pos;
            }

            str_move_to(&pos, ' ');
            ++pos;

            if (index >= 0) {
                // handles name[0] 1
                ((int64 *) member)[index] = (int64) str_to_int(pos);
            } else {
                // handles name[] 1 1 0 1 0
                for (int match_count = 0; match_count < match[match_index].count; ++match_count) {
                    str_move_past(&pos, ' ');
                    ((int64 *) member)[++index] = (int64) str_to_int(pos);
                }
            }
        } break;
        case DATA_TYPE_UINT8_ARRAY: {
            // move past [
            ++pos;

            int32 index = -1;
            if (isdigit(*pos)) {
                index = (int32) str_to_int(pos, &pos); ++pos;
            }

            str_move_to(&pos, ' ');
            ++pos;

            if (index >= 0) {
                // handles name[0] 1
                ((uint8 *) member)[index] = (uint8) str_to_int(pos);
            } else {
                // handles name[] 1 1 0 1 0
                for (int match_count = 0; match_count < match[match_index].count; ++match_count) {
                    str_move_past(&pos, ' ');
                    ((uint8 *) member)[++index] = (uint8) str_to_int(pos);
                }
            }
        } break;
        case DATA_TYPE_UINT16_ARRAY: {
            // move past [
            ++pos;

            int32 index = -1;
            if (isdigit(*pos)) {
                index = (int32) str_to_int(pos, &pos); ++pos;
            }

            str_move_to(&pos, ' ');
            ++pos;

            if (index >= 0) {
                // handles name[0] 1
                ((uint16 *) member)[index] = (uint16) str_to_int(pos);
            } else {
                // handles name[] 1 1 0 1 0
                for (int match_count = 0; match_count < match[match_index].count; ++match_count) {
                    str_move_past(&pos, ' ');
                    ((uint16 *) member)[++index] = (uint16) str_to_int(pos);
                }
            }
        } break;
        case DATA_TYPE_UINT32_ARRAY: {
            // move past [
            ++pos;

            int32 index = -1;
            if (isdigit(*pos)) {
                index = (int32) str_to_int(pos, &pos); ++pos;
            }

            str_move_to(&pos, ' ');
            ++pos;

            if (index >= 0) {
                // handles name[0] 1
                ((uint32 *) member)[index] = (uint32) str_to_int(pos);
            } else {
                // handles name[] 1 1 0 1 0
                for (int match_count = 0; match_count < match[match_index].count; ++match_count) {
                    str_move_past(&pos, ' ');
                    ((uint32 *) member)[++index] = (uint32) str_to_int(pos);
                }
            }
        } break;
        case DATA_TYPE_UINT64_ARRAY: {
            // move past [
            ++pos;

            int32 index = -1;
            if (isdigit(*pos)) {
                index = (int32) str_to_int(pos, &pos); ++pos;
            }

            str_move_to(&pos, ' ');
            ++pos;

            if (index >= 0) {
                // handles name[0] 1
                ((uint64 *) member)[index] = (uint64) str_to_int(pos);
            } else {
                // handles name[] 1 1 0 1 0
                for (int match_count = 0; match_count < match[match_index].count; ++match_count) {
                    str_move_past(&pos, ' ');
                    ((uint64 *) member)[++index] = (uint64) str_to_int(pos);
                }
            }
        } break;
        case DATA_TYPE_F32_ARRAY: {
            // move past [
            ++pos;

            int32 index = -1;
            if (isdigit(*pos)) {
                index = (int32) str_to_int(pos, &pos); ++pos;
            }

            str_move_to(&pos, ' ');
            ++pos;

            if (index >= 0) {
                // handles name[0] 1
                ((f32 *) member)[index] = (f32) str_to_float(pos);
            } else {
                // handles name[] 1 1 0 1 0
                for (int match_count = 0; match_count < match[match_index].count; ++match_count) {
                    str_move_past(&pos, ' ');
                    ((f32 *) member)[++index] = (f32) str_to_float(pos);
                }
            }
        } break;
        case DATA_TYPE_F64_ARRAY: {
            // move past [
            ++pos;

            int32 index = -1;
            if (isdigit(*pos)) {
                index = (int32) str_to_int(pos, &pos); ++pos;
            }

            str_move_to(&pos, ' ');
            ++pos;

            if (index >= 0) {
                // handles name[0] 1
                ((f64 *) member)[index] = (f64) str_to_float(pos);
            } else {
                // handles name[] 1 1 0 1 0
                for (int match_count = 0; match_count < match[match_index].count; ++match_count) {
                    str_move_past(&pos, ' ');
                    ((f64 *) member)[++index] = (f64) str_to_float(pos);
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
            // move past [
            ++pos;

            int32 index = -1;
            if (isdigit(*pos)) {
                index = (int32) str_to_int(pos, &pos); ++pos;
            }

            str_move_to(&pos, ' ');
            ++pos;

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
            str_copy_to_eol(pos, (char *) member, match[match_index].count);
        } break;
        case DATA_TYPE_WCHAR_STR: {
            char temp[256];
            str_copy_to_eol(pos, (char *) temp);
            char_to_wchar((wchar_t *) member, temp, match[match_index].count);
        } break;
        case DATA_TYPE_STRUCT: {
            // Move past {}
            str_move_to(&pos, ' ');

            // Structs are a little bit nasty
            // The count represents how many members the struct has
            // This means the N following elements are the struct members

            // Iterate through all struct members
            for (int struct_count = 1; struct_count <= match[match_index].count; ++struct_count) {
                str_move_past(&pos, ' ');
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
        str_move_to(&data, "\r\n\t :[");

        // Move to value
        str_skip_whitespace(&data);

        // Is there a value?
        if (is_eol(data) || *data == '\0') {
            continue;
        }

        for (int32 i = 0; i < match_count; ++i) {
            // @performance calculating strlen every time is stupid
            if (strncmp(name, match[i].name, strlen(match[i].name)) == 0) {
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

#endif