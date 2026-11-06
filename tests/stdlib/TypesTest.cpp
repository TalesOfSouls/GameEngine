#include "../TestFramework.h"
#include "../../stdlib/Types.h"
#include "../../stdlib/Defines.h"
#include "../../stdlib/GameMathTypes.h"

/**
 * Type + define tests
 *
 * Types.h defines the fixed size integer/float types, the small vector structs and
 * some helper types. Defines.h defines the limits/constants and the feature macros.
 * math_internal.h/Platform.h (pulled in through Stdlib.h) provide the min/max/clamp
 * and rounding helpers which are tested here as well.
 */

static void test_types_fixed_sizes() {
    TEST_EQUALS(sizeof(int8), 1);
    TEST_EQUALS(sizeof(int16), 2);
    TEST_EQUALS(sizeof(int32), 4);
    TEST_EQUALS(sizeof(int64), 8);

    TEST_EQUALS(sizeof(uint8), 1);
    TEST_EQUALS(sizeof(uint16), 2);
    TEST_EQUALS(sizeof(uint32), 4);
    TEST_EQUALS(sizeof(uint64), 8);

    TEST_EQUALS(sizeof(f32), 4);
    TEST_EQUALS(sizeof(f64), 8);

    TEST_EQUALS(sizeof(byte), 1);
    TEST_EQUALS(sizeof(sbyte), 1);
    TEST_EQUALS(sizeof(umm), sizeof(uintptr_t));
    TEST_EQUALS(sizeof(smm), sizeof(intptr_t));
    TEST_EQUALS(sizeof(utf8), 4);

    // byte is unsigned, sbyte is signed
    byte unsigned_byte = (byte) -1;
    sbyte signed_byte = (sbyte) -1;
    TEST_EQUALS(unsigned_byte, 255);
    TEST_EQUALS(signed_byte, -1);
    TEST_TRUE((byte) 200 > (byte) 100);
    TEST_TRUE((sbyte) -1 < (sbyte) 0);

    // The Defines.h limits have to match the type sizes
    TEST_EQUALS(MAX_BYTE, 0xFF);
    TEST_EQUALS(MAX_UINT8, 0xFF);
    TEST_EQUALS(MAX_UINT16, 0xFFFF);
    TEST_EQUALS((long long) MAX_UINT32, (long long) 0xFFFFFFFF);
    TEST_EQUALS((long long) MAX_UINT64, (long long) 0xFFFFFFFFFFFFFFFFULL);
    TEST_EQUALS(MAX_INT8, 127);
    TEST_EQUALS(MAX_INT16, 32767);
    TEST_EQUALS(MAX_INT32, 2147483647);
    TEST_EQUALS((long long) MAX_INT64, (long long) 0x7FFFFFFFFFFFFFFFLL);
    TEST_EQUALS(KILOBYTE, 1024);
    TEST_EQUALS(MEGABYTE, 1024 * 1024);
    TEST_EQUALS((long long) GIGABYTE, (long long) 1073741824ULL);
    TEST_EQUALS(SEC_MILLI, 1000);
    TEST_EQUALS(MIN_MILLI, 60000);
    TEST_EQUALS(SEC_MICRO, 1000000);
    TEST_EQUALS(MIN_MICRO, 60000000);
}

static void test_types_bits_union() {
    f32_bits float_bits;
    float_bits.f = 1.0f;
    TEST_EQUALS(float_bits.u, 0x3F800000);
    float_bits.u = 0x40000000;
    TEST_EQUALS_WITH_DELTA(float_bits.f, 2.0f, 0.000001);

    float_bits.f = -1.0f;
    TEST_EQUALS(float_bits.u, 0xBF800000);
    TEST_EQUALS(float_bits.u & FLOAT32_SIGN_MASK, 0x80000000);

    f64_bits double_bits;
    double_bits.f = 1.0;
    TEST_EQUALS((long long) double_bits.u, (long long) 0x3FF0000000000000ULL);
    double_bits.u = 0x4000000000000000ULL;
    TEST_EQUALS_WITH_DELTA(double_bits.f, 2.0, 0.0000001);

    double_bits.f = -0.5;
    TEST_EQUALS((long long) (double_bits.u >> 63), 1);

    // The unions don't change their size
    TEST_EQUALS(sizeof(f32_bits), 4);
    TEST_EQUALS(sizeof(f64_bits), 8);
}

static void test_types_v2() {
    v2_int8 v2i8 = {0};
    v2i8.x = 1;
    v2i8.y = 2;
    TEST_EQUALS(v2i8.x, 1);
    TEST_EQUALS(v2i8.y, 2);
    TEST_EQUALS(v2i8.vec[0], 1);
    TEST_EQUALS(v2i8.vec[1], 2);
    TEST_EQUALS(sizeof(v2_int8), 2);

    // The union aliases have to map to the same memory
    v2i8.width = 3;
    v2i8.height = 4;
    TEST_EQUALS(v2i8.x, 3);
    TEST_EQUALS(v2i8.y, 4);

    #if OMS_LITTLE_ENDIAN
        TEST_EQUALS(v2i8.val, (4 << 8) + 3);
    #else
        TEST_EQUALS(v2i8.val, (3 << 8) + 4);
    #endif

    v2_int16 v2i16 = {0};
    v2i16.width = 1000;
    v2i16.height = 2000;
    TEST_EQUALS(v2i16.x, 1000);
    TEST_EQUALS(v2i16.y, 2000);
    TEST_EQUALS(sizeof(v2_int16), 4);

    v2_uint16 v2u16 = {0};
    v2u16.x = 65535;
    v2u16.y = 1;
    TEST_EQUALS(v2u16.vec[0], 65535);
    TEST_EQUALS(v2u16.vec[1], 1);
    TEST_EQUALS(v2u16.val, (int64) (65535 + (1 << 16)));

    v2_int32 v2i32 = {0};
    v2i32.min = -5;
    v2i32.max = 5;
    v2i32.x = 7;
    TEST_EQUALS(v2i32.min, 7);
    TEST_EQUALS(v2i32.vec[0], 7);
    TEST_EQUALS(v2i32.vec[1], 5);
    TEST_EQUALS(sizeof(v2_int32), 8);

    v2_uint32 v2u32 = {0};
    v2u32.x = 42;
    v2u32.y = 43;
    TEST_EQUALS(v2u32.width, 42);
    TEST_EQUALS(v2u32.height, 43);

    v2_f32 v2f = {0};
    v2f.x = 1.5f;
    v2f.y = -2.5f;
    TEST_EQUALS_WITH_DELTA(v2f.width, 1.5f, 0.000001);
    TEST_EQUALS_WITH_DELTA(v2f.height, -2.5f, 0.000001);
    TEST_EQUALS(sizeof(v2_f32), 8);

    v2_f64 v2d = {0};
    v2d.x = 1.5;
    v2d.y = -2.5;
    TEST_EQUALS_WITH_DELTA(v2d.vec[0], 1.5, 0.0000001);
    TEST_EQUALS_WITH_DELTA(v2d.vec[1], -2.5, 0.0000001);
    TEST_EQUALS(sizeof(v2_f64), 16);
}

static void test_types_v3_v4() {
    v3_int8 v3i8 = {0};
    v3i8.x = 1;
    v3i8.y = 2;
    v3i8.z = 3;
    TEST_EQUALS(v3i8.r, 1);
    TEST_EQUALS(v3i8.g, 2);
    TEST_EQUALS(v3i8.b, 3);
    TEST_EQUALS(v3i8.vec[2], 3);
    TEST_EQUALS(sizeof(v3_int8), 3);

    v3_byte v3b = {0};
    v3b.x = 10;
    v3b.y = 20;
    v3b.z = 30;
    TEST_EQUALS(v3b.vec[0], 10);
    TEST_EQUALS(v3b.vec[1], 20);
    TEST_EQUALS(v3b.vec[2], 30);
    TEST_EQUALS(sizeof(v3_byte), 3);

    v4_int8 v4i8 = {0};
    v4i8.x = 1;
    v4i8.y = 2;
    v4i8.z = 3;
    v4i8.w = 4;
    TEST_EQUALS(v4i8.a, 4);
    TEST_EQUALS(sizeof(v4_int8), 4);
    #if OMS_LITTLE_ENDIAN
        TEST_EQUALS(v4i8.val, 0x04030201);
    #else
        TEST_EQUALS(v4i8.val, 0x01020304);
    #endif

    v4_byte v4b = {0};
    v4b.r = 0xA1;
    v4b.g = 0xA2;
    v4b.b = 0xA3;
    v4b.a = 0xA4;
    TEST_EQUALS(v4b.vec[3], 0xA4);
    TEST_EQUALS(sizeof(v4_uint8), 4);

    v4_int16 v4i16 = {0};
    v4i16.x = 1;
    v4i16.y = 2;
    v4i16.z = 3;
    v4i16.w = 4;
    TEST_EQUALS(v4i16.width, 3);
    TEST_EQUALS(v4i16.height, 4);
    TEST_EQUALS(sizeof(v4_int16), 8);

    v4_uint16 v4u16 = {0};
    v4u16.x = 65535;
    v4u16.y = 1;
    v4u16.width = 2;
    v4u16.height = 3;
    TEST_EQUALS(v4u16.vec[0], 65535);
    TEST_EQUALS(v4u16.vec[2], 2);
    TEST_EQUALS(sizeof(v4_uint16), 8);

    v3_int32 v3i32 = {0};
    v3i32.r = 11;
    v3i32.g = 22;
    v3i32.b = 33;
    TEST_EQUALS(v3i32.x, 11);
    TEST_EQUALS(v3i32.y, 22);
    TEST_EQUALS(v3i32.vec[2], 33);
    TEST_EQUALS(sizeof(v3_int32), 12);

    v3_uint32 v3u32 = {0};
    v3u32.x = 1;
    v3u32.y = 2;
    v3u32.z = 3;
    TEST_EQUALS(v3u32.vec[2], 3);

    v4_int32 v4i32 = {0};
    v4i32.x = 1;
    v4i32.y = 2;
    v4i32.z = 3;
    v4i32.w = 4;
    TEST_EQUALS(v4i32.width, 3);
    TEST_EQUALS(v4i32.height, 4);
    TEST_EQUALS(v4i32.vec[0], 1);
    TEST_EQUALS(v4i32.vec[3], 4);
    TEST_EQUALS(sizeof(v4_int32), 16);

    v4_uint32 v4u32 = {0};
    v4u32.x = 0xFFFFFFFF;
    v4u32.w = 1;
    TEST_EQUALS(v4u32.vec[0], 0xFFFFFFFF);
    TEST_EQUALS(v4u32.height, 1);

    v3_f32 v3f = {0};
    v3f.pitch = 1.0f;
    v3f.yaw = 2.0f;
    v3f.roll = 3.0f;
    TEST_EQUALS_WITH_DELTA(v3f.x, 1.0f, 0.000001);
    TEST_EQUALS_WITH_DELTA(v3f.y, 2.0f, 0.000001);
    TEST_EQUALS_WITH_DELTA(v3f.z, 3.0f, 0.000001);
    TEST_EQUALS(sizeof(v3_f32), 12);

    v3_f64 v3d = {0};
    v3d.u = 1.0;
    v3d.v = 2.0;
    v3d.w = 3.0;
    TEST_EQUALS_WITH_DELTA(v3d.x, 1.0, 0.0000001);
    TEST_EQUALS_WITH_DELTA(v3d.vec[2], 3.0, 0.0000001);
    TEST_EQUALS(sizeof(v3_f64), 24);

    v4_f32 v4f = {0};
    v4f.x = 1.0f;
    v4f.y = 2.0f;
    v4f.z = 3.0f;
    v4f.w = 4.0f;
    // v4_f32 has multiple aliases for the same four values
    TEST_EQUALS_WITH_DELTA(v4f.x1, 1.0f, 0.000001);
    TEST_EQUALS_WITH_DELTA(v4f.y1, 2.0f, 0.000001);
    TEST_EQUALS_WITH_DELTA(v4f.width, 3.0f, 0.000001);
    TEST_EQUALS_WITH_DELTA(v4f.height, 4.0f, 0.000001);
    TEST_EQUALS_WITH_DELTA(v4f.a, 4.0f, 0.000001);
    TEST_EQUALS(sizeof(v4_f32), 16);
    TEST_EQUALS(sizeof(quaternion), 16);

    v4_f64 v4d = {0};
    v4d.x = 1.0;
    v4d.y = 2.0;
    v4d.z = 3.0;
    v4d.w = 4.0;
    TEST_EQUALS_WITH_DELTA(v4d.x2, 3.0, 0.0000001);
    TEST_EQUALS_WITH_DELTA(v4d.y2, 4.0, 0.0000001);
    TEST_EQUALS(sizeof(v4_f64), 32);

    v2_int64 v2i64 = {0};
    v2i64.x = -1;
    v2i64.y = 1;
    TEST_EQUALS(sizeof(v2_int64), 16);
    TEST_EQUALS(v2i64.vec[0], -1);

    v3_int64 v3i64 = {0};
    v3i64.z = 5;
    TEST_EQUALS(v3i64.vec[2], 5);
    TEST_EQUALS(sizeof(v3_int64), 24);

    v4_int64 v4i64 = {0};
    v4i64.w = 7;
    TEST_EQUALS(v4i64.vec[3], 7);
    TEST_EQUALS(sizeof(v4_int64), 32);
}

static void test_types_alignment() {
    // The SIMD types have to be aligned so they can be loaded with aligned loads
    TEST_EQUALS(alignof(v4_int32), 16);
    TEST_EQUALS(alignof(v4_uint32), 16);
    TEST_EQUALS(alignof(v4_f32), 16);
    TEST_EQUALS(alignof(v16_f32), 64);

    TEST_EQUALS(sizeof(v16_f32), 64);
    TEST_EQUALS(alignof(v16_f32) % alignof(v4_f32), 0);

    // Plain types have no over alignment
    TEST_EQUALS(alignof(v2_f32), alignof(f32));
    TEST_EQUALS(alignof(v3_f32), alignof(f32));
    TEST_EQUALS(alignof(v4_f64), alignof(f64));

    // The matrix can be used as 4x4 float array or as 4 rows of 4 floats
    v16_f32 matrix = {0};
    matrix.vec[0] = 1.0f;
    matrix.vec[5] = 2.0f;
    matrix.vec[10] = 3.0f;
    matrix.vec[15] = 4.0f;
    TEST_EQUALS_WITH_DELTA(matrix.rows[0].x, 1.0f, 0.000001);
    TEST_EQUALS_WITH_DELTA(matrix.rows[1].y, 2.0f, 0.000001);
    TEST_EQUALS_WITH_DELTA(matrix.rows[2].z, 3.0f, 0.000001);
    TEST_EQUALS_WITH_DELTA(matrix.rows[3].w, 4.0f, 0.000001);
    TEST_EQUALS_WITH_DELTA(matrix.mat[0][0], 1.0f, 0.000001);
    TEST_EQUALS_WITH_DELTA(matrix.mat[3][3], 4.0f, 0.000001);
}

static void test_types_conversions() {
    v2_int32 i2 = {-1, 2};
    v2_f32 f2 = to_v2_f32(i2);
    TEST_EQUALS_WITH_DELTA(f2.x, -1.0f, 0.000001);
    TEST_EQUALS_WITH_DELTA(f2.y, 2.0f, 0.000001);

    v3_int32 i3 = {1, -2, 3};
    v3_f32 f3 = to_v3_f32(i3);
    TEST_EQUALS_WITH_DELTA(f3.x, 1.0f, 0.000001);
    TEST_EQUALS_WITH_DELTA(f3.y, -2.0f, 0.000001);
    TEST_EQUALS_WITH_DELTA(f3.z, 3.0f, 0.000001);

    v4_int32 i4 = {1, 2, 3, 4};
    v4_f32 f4 = to_v4_f32(i4);
    TEST_EQUALS_WITH_DELTA(f4.x, 1.0f, 0.000001);
    TEST_EQUALS_WITH_DELTA(f4.w, 4.0f, 0.000001);

    v2_int32 roundtrip2 = to_v2_int32(f2);
    TEST_EQUALS(roundtrip2.x, -1);
    TEST_EQUALS(roundtrip2.y, 2);

    v3_int32 roundtrip3 = to_v3_int32(f3);
    TEST_EQUALS(roundtrip3.x, 1);
    TEST_EQUALS(roundtrip3.y, -2);
    TEST_EQUALS(roundtrip3.z, 3);

    v4_int32 roundtrip4 = to_v4_int32(f4);
    TEST_EQUALS(roundtrip4.x, 1);
    TEST_EQUALS(roundtrip4.y, 2);
    TEST_EQUALS(roundtrip4.z, 3);
    TEST_EQUALS(roundtrip4.w, 4);

    // Truncation (not rounding) is the documented behaviour of the C style cast
    const v2_int32 truncation_source = {1, 2};
    const v2_f32 truncated = to_v2_f32(truncation_source);
    const v2_f32 negative_source = {1.9f, -1.9f};
    const v2_int32 negative_truncated = to_v2_int32(negative_source);
    TEST_EQUALS(negative_truncated.x, 1);
    TEST_EQUALS(negative_truncated.y, -1);
    TEST_EQUALS_WITH_DELTA(truncated.y, 2.0f, 0.000001);
}

static void test_types_array() {
    Array<int32, 4> array = {{1, 2, 3, 4}};
    TEST_EQUALS(array[0], 1);
    TEST_EQUALS(array[3], 4);
    TEST_EQUALS(sizeof(array), 16);

    array[2] = 30;
    TEST_EQUALS(array.data[2], 30);
    TEST_EQUALS(array.elements()[0], 1);

    int32* raw = array.elements();
    raw[1] = 20;
    TEST_EQUALS(array[1], 20);

    const Array<int32, 4>& const_array = array;
    TEST_EQUALS(const_array[2], 30);
    TEST_EQUALS(const_array.elements()[3], 4);

    Array<v2_f32, 2> vector_array = {{{1.0f, 2.0f}, {3.0f, 4.0f}}};
    TEST_EQUALS_WITH_DELTA(vector_array[1].x, 3.0f, 0.000001);
    TEST_EQUALS_WITH_DELTA(vector_array[1].y, 4.0f, 0.000001);
}

static void test_types_datatype_enum() {
    TEST_EQUALS(DATA_TYPE_VOID, 0);
    TEST_EQUALS(DATA_TYPE_BOOL, 1);
    TEST_EQUALS(DATA_TYPE_INT8, 2);
    TEST_EQUALS(DATA_TYPE_INT16, 3);
    TEST_EQUALS(DATA_TYPE_INT32, 4);
    TEST_EQUALS(DATA_TYPE_INT64, 5);
    TEST_EQUALS(DATA_TYPE_UINT8, 6);
    TEST_EQUALS(DATA_TYPE_F32, 10);
    TEST_EQUALS(DATA_TYPE_F64, 11);

    // The enum is stored as a single byte in binary files
    TEST_EQUALS(sizeof(DataType), 1);

    // All values are unique
    TEST_TRUE(DATA_TYPE_VOID != DATA_TYPE_BOOL);
    TEST_TRUE(DATA_TYPE_V2_F32_ARRAY < DATA_TYPE_V4_F32_ARRAY);
    TEST_TRUE(DATA_TYPE_STRUCT > DATA_TYPE_V4_F32_ARRAY);
}

static void test_types_min_max() {
    TEST_EQUALS(max_branched(3, 7), 7);
    TEST_EQUALS(max_branched(9, 7), 9);
    TEST_EQUALS(max_branched(-3, -7), -3);
    TEST_EQUALS(min_branched(3, 7), 3);
    TEST_EQUALS(min_branched(9, 7), 7);
    TEST_EQUALS(min_branched(-3, -7), -7);
    TEST_EQUALS(clamp_branched(5, 0, 10), 5);
    TEST_EQUALS(clamp_branched(-5, 0, 10), 0);
    TEST_EQUALS(clamp_branched(15, 0, 10), 10);

    TEST_EQUALS(max_branchless(3, 7), 7);
    TEST_EQUALS(max_branchless(9, 7), 9);
    TEST_EQUALS(min_branchless(3, 7), 3);
    TEST_EQUALS(min_branchless(9, 7), 7);
    TEST_EQUALS(clamp_branchless(5, 0, 10), 5);
    TEST_EQUALS(clamp_branchless(-5, 0, 10), 0);
    TEST_EQUALS(clamp_branchless(15, 0, 10), 10);

    // The platform macros wrap the (branchless) implementation
    TEST_EQUALS(OMS_MAX(3, 7), 7);
    TEST_EQUALS(OMS_MIN(3, 7), 3);
    TEST_EQUALS(OMS_CLAMP(15, 0, 10), 10);
    TEST_EQUALS(OMS_CLAMP(-15, 0, 10), 0);

    TEST_EQUALS_WITH_DELTA(oms_max(1.5f, 2.5f), 2.5f, 0.000001);
    TEST_EQUALS_WITH_DELTA(oms_min(1.5f, 2.5f), 1.5f, 0.000001);
    TEST_EQUALS_WITH_DELTA(oms_clamp(5.0f, 0.0f, 10.0f), 5.0f, 0.000001);
    TEST_EQUALS_WITH_DELTA(oms_clamp(-5.0f, 0.0f, 10.0f), 0.0f, 0.000001);
    TEST_EQUALS_WITH_DELTA(oms_clamp(15.0, 0.0, 10.0), 10.0, 0.0000001);
}

static void test_types_rounding_and_div() {
    TEST_EQUALS(CEIL_DIV(10, 3), 4);
    TEST_EQUALS(CEIL_DIV(9, 3), 3);
    TEST_EQUALS(CEIL_DIV(0, 5), 0);
    TEST_EQUALS(CEIL_DIV(1, 8), 1);

    TEST_EQUALS(ceil_div_pow2<8>(0), 0);
    TEST_EQUALS(ceil_div_pow2<8>(1), 1);
    TEST_EQUALS(ceil_div_pow2<8>(8), 1);
    TEST_EQUALS(ceil_div_pow2<8>(9), 2);
    TEST_EQUALS(ceil_div_pow2<64>(65), 2);
    TEST_EQUALS(ceil_div_pow2<1024>(1025), 2);

    TEST_EQUALS(__internal_log2_pow2(1), 0);
    TEST_EQUALS(__internal_log2_pow2(2), 1);
    TEST_EQUALS(__internal_log2_pow2(1024), 10);

    TEST_EQUALS(__internal_abs((int32) -5), 5);
    TEST_EQUALS(__internal_abs((int32) 5), 5);
    TEST_EQUALS(__internal_abs((int64) -5), 5);
    TEST_EQUALS_WITH_DELTA(__internal_abs(-2.5f), 2.5f, 0.000001);
    TEST_EQUALS_WITH_DELTA(__internal_abs(-2.5), 2.5, 0.0000001);

    TEST_EQUALS(__internal_floor(2.7f), 2);
    TEST_EQUALS(__internal_floor(-2.7f), -3);
    TEST_EQUALS(__internal_floor(2.0f), 2);
    TEST_EQUALS(__internal_ceil(2.1f), 3);
    TEST_EQUALS(__internal_ceil(-2.1f), -2);
    TEST_EQUALS(__internal_ceil(2.0f), 2);

    TEST_EQUALS(__internal_round(2.4f), 2);
    TEST_EQUALS(__internal_round(2.6f), 3);
    TEST_EQUALS(__internal_round(-2.6f), -3);
    TEST_EQUALS(__internal_round(-2.4f), -2);

    TEST_EQUALS(oms_round_positive(2.4f), 2);
    TEST_EQUALS(oms_round_positive(2.6f), 3);
    TEST_EQUALS(oms_round_positive(2.6), 3);

    // floor_div has to round towards negative infinity
    TEST_EQUALS(floor_div(7, 3), 2);
    TEST_EQUALS(floor_div(-7, 3), -3);
    TEST_EQUALS(floor_div(7, -3), -2);
    TEST_EQUALS(floor_div(-6, 3), -2);
}

static void test_types_game_math() {
    AABB_int32 box = {{0, 0, 0}, {10, 20, 30}};
    const v3_int32 center = aabb_center(box);
    TEST_EQUALS(center.x, 5);
    TEST_EQUALS(center.y, 10);
    TEST_EQUALS(center.z, 15);

    const AABB_f32 box_float = {{0.0f, 0.0f, 0.0f}, {10.0f, 20.0f, 30.0f}};
    const v3_f32 float_center = aabb_center(box_float);
    TEST_EQUALS_WITH_DELTA(float_center.x, 5.0f, 0.000001);
    TEST_EQUALS_WITH_DELTA(float_center.y, 10.0f, 0.000001);
    TEST_EQUALS_WITH_DELTA(float_center.z, 15.0f, 0.000001);

    // AABB cast keeps the values
    const AABB_f32 cast_result = aabb_cast(box);
    TEST_EQUALS_WITH_DELTA(cast_result.min.x, 0.0f, 0.000001);
    TEST_EQUALS_WITH_DELTA(cast_result.max.z, 30.0f, 0.000001);

    // Loosening by the same factor is the identity for an int box
    const AABB_int32 loosened = aabb_loosen(box, 1.0f);
    TEST_EQUALS(loosened.min.x, 0);
    TEST_EQUALS(loosened.min.y, 0);
    TEST_EQUALS(loosened.max.x, 10);
    TEST_EQUALS(loosened.max.z, 30);

    // Loosening by 2 doubles the extents around the center
    const AABB_int32 doubled = aabb_loosen(box, 2.0f);
    TEST_EQUALS(doubled.min.x, -5);
    TEST_EQUALS(doubled.max.x, 15);
    TEST_EQUALS(doubled.min.y, -10);
    TEST_EQUALS(doubled.max.y, 30);

    // Overlap checks
    const AABB_int32 other = {{5, 5, 5}, {15, 15, 15}};
    const AABB_int32 outside = {{100, 100, 100}, {110, 110, 110}};
    const AABB_int32 touching = {{10, 20, 30}, {20, 30, 40}};
    TEST_TRUE(aabb_overlap(box, other));
    TEST_FALSE(aabb_overlap(box, outside));
    TEST_TRUE(aabb_overlap(box, touching));
    TEST_TRUE(aabb_overlap(box, box));

    // Lerp / remap
    TEST_EQUALS_WITH_DELTA(OMS_LERP(0.0f, 10.0f, 0.25f), 2.5f, 0.000001);
    TEST_EQUALS_WITH_DELTA(OMS_INVERSE_LERP(0.0f, 10.0f, 2.5f), 0.25f, 0.000001);
    TEST_EQUALS_WITH_DELTA(OMS_REMAP(5.0f, 0.0f, 10.0f, 100.0f, 200.0f), 150.0f, 0.000001);
}

#ifdef UBER_TEST
    #ifdef main
        #undef main
    #endif
    #define main StdlibTypesTest
#endif

int main() {
    TEST_INIT(128);

    TEST_RUN(test_types_fixed_sizes);
    TEST_RUN(test_types_bits_union);
    TEST_RUN(test_types_v2);
    TEST_RUN(test_types_v3_v4);
    TEST_RUN(test_types_alignment);
    TEST_RUN(test_types_conversions);
    TEST_RUN(test_types_array);
    TEST_RUN(test_types_datatype_enum);
    TEST_RUN(test_types_min_max);
    TEST_RUN(test_types_rounding_and_div);
    TEST_RUN(test_types_game_math);

    TEST_FINALIZE();

    return 0;
}
