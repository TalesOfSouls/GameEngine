/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_DATABASE_PARAM_H
#define COMS_DATABASE_PARAM_H

#include "../stdlib/Types.h"

enum DbParamType : byte {
    DB_PARAM_INT8,
    DB_PARAM_INT16,
    DB_PARAM_INT32,
    DB_PARAM_INT64,
    DB_PARAM_F32,
    DB_PARAM_F64,
    DB_PARAM_TEXT,
    DB_PARAM_NULL
};

struct DbParam {
    DbParamType type;
    union {
        int8 int8_val;
        int16 int16_val;
        int32 int32_val;
        int64 int64_val;
        f32 f32_val;
        f64 f64_val;
        const char* text_val;
    };
};

// Helper macros for cleaner calling syntax
#define DB_INT8(x) {DB_PARAM_INT8, .int8_val=(x)}
#define DB_INT16(x) {DB_PARAM_INT16, .int16_val=(x)}
#define DB_INT32(x) {DB_PARAM_INT32, .int32_val=(x)}
#define DB_INT64(x) {DB_PARAM_INT64, .int64_val=(x)}
#define DB_F32(x) {DB_PARAM_F32, .f32_val=(x)}
#define DB_F64(x) {DB_PARAM_F64, .f64_val=(x)}
#define DB_TEXT(x) {DB_PARAM_TEXT, .text_val=(x)}
#define DB_END {DB_PARAM_NULL}

#endif