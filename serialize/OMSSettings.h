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

void settings_save(
    const void* const __restrict settings_data,
    const char* in,
    char* out, size_t out_length,
    const SettingsMatch* match,
    int32 match_count
)  NO_EXCEPT
{
    char line[2048];
    char setting[64];

    const byte* const settings = (byte *) settings_data;
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

        size_t name_length;
        for (int32 i = 0; i < match_count; ++i) {
            name_length = str_length(match[i].name);
            if (str_compare(line, match[i].name, name_length) == 0) {
                out += str_copy(out, match[i].name);

                // There must be a whitespace after the settings name
                *out++ = ' ';

                const byte* const member = settings + match[i].offset;

                switch (match[i].type) {
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
                        // We need to go back to ensure the correct formatting for arrays
                        out -= name_length + 1;

                        for (int32 j = 0; j < match[i].count; ++j) {
                            out += sprintf_fast(
                                out, "%s[%d] %d\n",
                                match[i].name, j, (int32) *((uint16 *) member)
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
                        // We need to go back to ensure the correct formatting for arrays
                        out -= name_length + 1;

                        for (int32 j = 0; j < match[i].count; ++j) {
                            out += sprintf_fast(
                                out, "%s[%d] %f %f %f %f\n",
                                match[i].name, j,
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
                    default:
                        UNREACHABLE();
                }

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

void settings_load(
    void* const __restrict settings_data,
    const char* const data,
    const SettingsMatch* const match,
    int32 math_count
)
{
    const char* pos = data;
    const char* name;
    const byte* const settings = (byte *) settings_data;

    while (*pos != '\0') {
        // Skip all whitespaces and new lines
        str_skip_empty(&pos);

        // Skip comment
        if (*pos == '/' && pos[1] == '/') {
            str_move_to(&pos, '\n');

            continue;
        }

        // Get name
        name = pos;
        str_skip_until_list(&pos, "\r\n\t :[");

        // Move to value
        str_skip_whitespace(&pos);

        // Is there a value?
        if (is_eol(pos) || *pos == '\0') {
            continue;
        }

        //const char* name_prev;

        // index in case of array data
        //int32 index;

        for (int32 i = 0; i < math_count; ++i) {
            if (str_compare(name, match[i].name, str_length(match[i].name)) == 0) {
                /*
                // If the name is != prev name -> we need to reset the index
                if (str_compare(name, name_prev) != 0) {
                    index = -1;
                }
                */

                void* const member = (void *) (settings + match[i].offset);

                switch (match[i].type) {
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
                        if (str_is_num(*pos)) {
                            index = (int32) str_to_int(pos, &pos); ++pos;
                        }

                        str_skip_until_list(&pos, " ");
                        ++pos;

                        if (index >= 0) {
                            // handles name[0] = 1
                            ((bool *) member)[index] = (bool) str_to_int(pos);
                        } else {
                            // handles name[] = 1 1 0 1 0
                            do {
                                str_skip_whitespace(&pos);
                                ((bool *) member)[++index] = (bool) str_to_int(pos);
                                str_skip_until_list(&pos, " \n");
                            } while (*pos == ' ' && str_is_num(*(pos + 1)));
                        }
                    } break;
                    case DATA_TYPE_INT8_ARRAY: {
                        // move past [
                        ++pos;

                        int32 index = -1;
                        if (str_is_num(*pos)) {
                            index = (int32) str_to_int(pos, &pos); ++pos;
                        }

                        str_skip_until_list(&pos, " ");
                        ++pos;

                        if (index >= 0) {
                            // handles name[0] = 1
                            ((int8 *) member)[index] = (int8) str_to_int(pos);
                        } else {
                            // handles name[] = 1 1 0 1 0
                            do {
                                str_skip_whitespace(&pos);
                                ((int8 *) member)[++index] = (int8) str_to_int(pos);
                                str_skip_until_list(&pos, " \n");
                            } while (*pos == ' ' && str_is_num(*(pos + 1)));
                        }
                    } break;
                    case DATA_TYPE_INT16_ARRAY: {
                        // move past [
                        ++pos;

                        int32 index = -1;
                        if (str_is_num(*pos)) {
                            index = (int32) str_to_int(pos, &pos); ++pos;
                        }

                        str_skip_until_list(&pos, " ");
                        ++pos;

                        if (index >= 0) {
                            // handles name[0] = 1
                            ((int16 *) member)[index] = (int16) str_to_int(pos);
                        } else {
                            // handles name[] = 1 1 0 1 0
                            do {
                                str_skip_whitespace(&pos);
                                ((int16 *) member)[++index] = (int16) str_to_int(pos);
                                str_skip_until_list(&pos, " \n");
                            } while (*pos == ' ' && str_is_num(*(pos + 1)));
                        }
                    } break;
                    case DATA_TYPE_INT32_ARRAY: {
                        // move past [
                        ++pos;

                        int32 index = -1;
                        if (str_is_num(*pos)) {
                            index = (int32) str_to_int(pos, &pos); ++pos;
                        }

                        str_skip_until_list(&pos, " ");
                        ++pos;

                        if (index >= 0) {
                            // handles name[0] = 1
                            ((int32 *) member)[index] = (int32) str_to_int(pos);
                        } else {
                            // handles name[] = 1 1 0 1 0
                            do {
                                str_skip_whitespace(&pos);
                                ((int32 *) member)[++index] = (int32) str_to_int(pos);
                                str_skip_until_list(&pos, " \n");
                            } while (*pos == ' ' && str_is_num(*(pos + 1)));
                        }
                    } break;
                    case DATA_TYPE_INT64_ARRAY: {
                        // move past [
                        ++pos;

                        int32 index = -1;
                        if (str_is_num(*pos)) {
                            index = (int32) str_to_int(pos, &pos); ++pos;
                        }

                        str_skip_until_list(&pos, " ");
                        ++pos;

                        if (index >= 0) {
                            // handles name[0] = 1
                            ((int64 *) member)[index] = (int64) str_to_int(pos);
                        } else {
                            // handles name[] = 1 1 0 1 0
                            do {
                                str_skip_whitespace(&pos);
                                ((int64 *) member)[++index] = (int64) str_to_int(pos);
                                str_skip_until_list(&pos, " \n");
                            } while (*pos == ' ' && str_is_num(*(pos + 1)));
                        }
                    } break;
                    case DATA_TYPE_UINT8_ARRAY: {
                        // move past [
                        ++pos;

                        int32 index = -1;
                        if (str_is_num(*pos)) {
                            index = (int32) str_to_int(pos, &pos); ++pos;
                        }

                        str_skip_until_list(&pos, " ");
                        ++pos;

                        if (index >= 0) {
                            // handles name[0] = 1
                            ((uint8 *) member)[index] = (uint8) str_to_int(pos);
                        } else {
                            // handles name[] = 1 1 0 1 0
                            do {
                                str_skip_whitespace(&pos);
                                ((uint8 *) member)[++index] = (uint8) str_to_int(pos);
                                str_skip_until_list(&pos, " \n");
                            } while (*pos == ' ' && str_is_num(*(pos + 1)));
                        }
                    } break;
                    case DATA_TYPE_UINT16_ARRAY: {
                        // move past [
                        ++pos;

                        int32 index = -1;
                        if (str_is_num(*pos)) {
                            index = (int32) str_to_int(pos, &pos); ++pos;
                        }

                        str_skip_until_list(&pos, " ");
                        ++pos;

                        if (index >= 0) {
                            // handles name[0] = 1
                            ((uint16 *) member)[index] = (uint16) str_to_int(pos);
                        } else {
                            // handles name[] = 1 1 0 1 0
                            do {
                                str_skip_whitespace(&pos);
                                ((uint16 *) member)[++index] = (uint16) str_to_int(pos);
                                str_skip_until_list(&pos, " \n");
                            } while (*pos == ' ' && str_is_num(*(pos + 1)));
                        }
                    } break;
                    case DATA_TYPE_UINT32_ARRAY: {
                        // move past [
                        ++pos;

                        int32 index = -1;
                        if (str_is_num(*pos)) {
                            index = (int32) str_to_int(pos, &pos); ++pos;
                        }

                        str_skip_until_list(&pos, " ");
                        ++pos;

                        if (index >= 0) {
                            // handles name[0] = 1
                            ((uint32 *) member)[index] = (uint32) str_to_int(pos);
                        } else {
                            // handles name[] = 1 1 0 1 0
                            do {
                                str_skip_whitespace(&pos);
                                ((uint32 *) member)[++index] = (uint32) str_to_int(pos);
                                str_skip_until_list(&pos, " \n");
                            } while (*pos == ' ' && str_is_num(*(pos + 1)));
                        }
                    } break;
                    case DATA_TYPE_UINT64_ARRAY: {
                        // move past [
                        ++pos;

                        int32 index = -1;
                        if (str_is_num(*pos)) {
                            index = (int32) str_to_int(pos, &pos); ++pos;
                        }

                        str_skip_until_list(&pos, " ");
                        ++pos;

                        if (index >= 0) {
                            // handles name[0] = 1
                            ((uint64 *) member)[index] = (uint64) str_to_int(pos);
                        } else {
                            // handles name[] = 1 1 0 1 0
                            do {
                                str_skip_whitespace(&pos);
                                ((uint64 *) member)[++index] = (uint64) str_to_int(pos);
                                str_skip_until_list(&pos, " \n");
                            } while (*pos == ' ' && str_is_num(*(pos + 1)));
                        }
                    } break;
                    case DATA_TYPE_F32_ARRAY: {
                        // move past [
                        ++pos;

                        int32 index = -1;
                        if (str_is_num(*pos)) {
                            index = (int32) str_to_int(pos, &pos); ++pos;
                        }

                        str_skip_until_list(&pos, " ");
                        ++pos;

                        if (index >= 0) {
                            // handles name[0] = 1
                            ((f32 *) member)[index] = (f32) str_to_float(pos);
                        } else {
                            // handles name[] = 1 1 0 1 0
                            do {
                                str_skip_whitespace(&pos);
                                ((f32 *) member)[++index] = (f32) str_to_float(pos);
                                str_skip_until_list(&pos, " \n");
                            } while (*pos == ' ' && str_is_num(*(pos + 1)));
                        }
                    } break;
                    case DATA_TYPE_F64_ARRAY: {
                        // move past [
                        ++pos;

                        int32 index = -1;
                        if (str_is_num(*pos)) {
                            index = (int32) str_to_int(pos, &pos); ++pos;
                        }

                        str_skip_until_list(&pos, " ");
                        ++pos;

                        if (index >= 0) {
                            // handles name[0] = 1
                            ((f64 *) member)[index] = (f64) str_to_float(pos);
                        } else {
                            // handles name[] = 1 1 0 1 0
                            do {
                                str_skip_whitespace(&pos);
                                ((f64 *) member)[++index] = (f64) str_to_float(pos);
                                str_skip_until_list(&pos, " \n");
                            } while (*pos == ' ' && str_is_num(*(pos + 1)));
                        }
                    } break;

                    // Vector data
                    case DATA_TYPE_V4_F32: {
                        *((f32 *) member) = (f32) str_to_float(pos, &pos); ++pos;
                        *(((f32 *) member) + 1) = (f32) str_to_float(pos, &pos); ++pos;
                        *(((f32 *) member) + 2) = (f32) str_to_float(pos, &pos); ++pos;
                        *(((f32 *) member) + 3) = (f32) str_to_float(pos, &pos);
                    } break;
                    case DATA_TYPE_V4_F32_ARRAY: {
                        // move past [
                        ++pos;

                        int32 index = -1;
                        if (str_is_num(*pos)) {
                            index = (int32) str_to_int(pos, &pos); ++pos;
                        }

                        str_skip_until_list(&pos, " ");
                        ++pos;

                        ASSERT_TRUE(index >= 0);

                        // handles name[0] = 1.0 2.0 1.0 0.0
                        // handles name[1] = 1.0 2.0 1.0 0.0
                        ((v4_f32 *) member)[index].vec[0] = (f32) str_to_float(pos); ++pos;
                        ((v4_f32 *) member)[index].vec[1] = (f32) str_to_float(pos); ++pos;
                        ((v4_f32 *) member)[index].vec[2] = (f32) str_to_float(pos); ++pos;
                        ((v4_f32 *) member)[index].vec[3] = (f32) str_to_float(pos);
                    } break;

                    // String data
                    case DATA_TYPE_CHAR: {
                        *((char *) member) = *pos;
                    } break;
                    case DATA_TYPE_CHAR_STR: {
                        str_copy_to_eol(pos, (char *) member, match[i].count);
                    } break;
                    case DATA_TYPE_WCHAR_STR: {
                        char temp[256];
                        str_copy_to_eol(pos, (char *) temp);
                        char_to_wchar((wchar_t *) member, temp, match[i].count);
                    } break;
                    default:
                        UNREACHABLE();
                }

                break;
            }
        }

        str_move_to(&pos, '\n');
    }
}

#endif