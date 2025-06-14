/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 * @link      https://danceswithcode.net/engineeringnotes/quaternions/quaternions.html
 */
#ifndef COMS_MATH_MATRIX_QUATERNION_FLOAT32_H
#define COMS_MATH_MATRIX_QUATERNION_FLOAT32_H

#include "../../utils/TestUtils.h"
#include "MatrixFloat32.h"

#if ARM
    #include "../../stdlib/IntrinsicsArm.h"
#else
    #include "../../stdlib/Intrinsics.h"
#endif

// @todo Remove unused functions there are a lot (AFTER you implemented quaternion handling in the camera)

inline
void quaternion_unit(v4_f32* quat)
{
    f32 length = oms_rsqrt(quat->w * quat->w + quat->x * quat->x + quat->y * quat->y + quat->z * quat->z);

    quat->w *= length;
    quat->x *= length;
    quat->y *= length;
    quat->z *= length;
}

inline
void quaternion_from_euler(v4_f32* quat, f32 pitch, f32 yaw, f32 roll)
{
    f32 y = OMS_DEG2RAD(yaw);
    f32 cy = cosf(y / 2);
    f32 sy = sinf(y / 2);

    f32 p = OMS_DEG2RAD(pitch);
    f32 cp = cosf(p / 2);
    f32 sp = sinf(p / 2);

    f32 r = OMS_DEG2RAD(roll);
    f32 cr = cosf(r / 2);
    f32 sr = sinf(r / 2);

    quat->w = cr * cp * cy + sr * sp * sy;
    quat->x = sr * cp * cy - cr * sp * sy;
    quat->y = cr * sp * cy + sr * cp * sy;
    quat->z = cr * cp * sy - sr * sp * cy;

    quaternion_unit(quat);
}

inline
void quaternion_from_euler(v4_f32* __restrict quat, const v3_f32* __restrict v)
{
    f32 y = OMS_RAD2DEG(v->v / 2);
    f32 cy = cosf(y);
    f32 sy = sinf(y);

    f32 p = OMS_RAD2DEG(v->u * 0.5f);
    f32 cp = cosf(p);
    f32 sp = sinf(p);

    f32 r = OMS_RAD2DEG(v->w * 0.5f);
    f32 cr = cosf(r);
    f32 sr = sinf(r);

    quat->w = cr * cp * cy + sr * sp * sy;
    quat->x = sr * cp * cy - cr * sp * sy;
    quat->y = cr * sp * cy + sr * cp * sy;
    quat->z = cr * cp * sy - sr * sp * cy;
}

inline
void quaternion_from_axis_angle(v4_f32* quat, const v3_f32* __restrict axis, f32 rad) {
    f32 half_angle = rad / 2.0f;
    f32 s = sinf(half_angle);

    quat->x = axis->x * s;
    quat->y = axis->y * s;
    quat->z = axis->z * s;
    quat->w = cosf(half_angle);

    quaternion_unit(quat);
}

void quaternion_to_euler(const v4_f32* __restrict quat, v3_f32* __restrict v) {
    // Pitch
    f32 sinp = 2.0f * (quat->w * quat->x + quat->y * quat->z);
    f32 cosp = 1.0f - 2.0f * (quat->x * quat->x + quat->y * quat->y);
    v->pitch = atan2f(sinp, cosp);

    // Check for gimbal lock
    if (OMS_ABS_F32(sinp) >= 0.9999f) {
        v->yaw = atan2f(quat->x * quat->z - quat->w * quat->y, quat->w * quat->x + quat->y * quat->z);
        v->roll = 0.0f;
    } else {
        // Yaw
        f32 siny = 2.0f * (quat->w * quat->y - quat->z * quat->x);
        v->yaw = asinf(siny);

        // Roll
        f32 sinr = 2.0f * (quat->w * quat->z + quat->x * quat->y);
        f32 cosr = 1.0f - 2.0f * (quat->y * quat->y + quat->z * quat->z);
        v->roll = atan2f(sinr, cosr);
    }
}

void quaternion_multiply(v4_f32* __restrict quat, const v4_f32* __restrict quat1, const v4_f32* __restrict quat2)
{
    quat->x = quat1->w * quat2->x + quat1->x * quat2->w + quat1->y * quat2->z - quat1->z * quat2->y;
    quat->y = quat1->w * quat2->y - quat1->x * quat2->z + quat1->y * quat2->w + quat1->z * quat2->x;
    quat->z = quat1->w * quat2->z + quat1->x * quat2->y - quat1->y * quat2->x + quat1->z * quat2->w;
    quat->w = quat1->w * quat2->w - quat1->x * quat2->x - quat1->y * quat2->y - quat1->z * quat2->z;
}

void quaternion_inverse(v4_f32* __restrict quat, const v4_f32* __restrict quat_origin) {
    f32 norm = quat_origin->w * quat_origin->w
        + quat_origin->x * quat_origin->x
        + quat_origin->y * quat_origin->y
        + quat_origin->z * quat_origin->z;

    quat->w = quat_origin->w / norm;
    quat->x = -quat_origin->x / norm;
    quat->y = -quat_origin->y / norm;
    quat->z = -quat_origin->z / norm;
}

inline
void quaternion_to_rotation(f32* __restrict matrix, const v4_f32* __restrict quat)
{
    matrix[0] = 1.0f - 2.0f * (quat->y * quat->y + quat->z * quat->z);
    matrix[1] = 2.0f * (quat->x * quat->y - quat->z * quat->w);
    matrix[2] = 2.0f * (quat->x * quat->z + quat->y * quat->w);
    matrix[3] = 0.0f;

    matrix[4] = 2.0f * (quat->x * quat->y + quat->z * quat->w);
    matrix[5] = 1.0f - 2.0f * (quat->x * quat->x + quat->z * quat->z);
    matrix[6] = 2.0f * (quat->y * quat->z - quat->x * quat->w);
    matrix[7] = 0.0f;

    matrix[8] = 2.0f * (quat->x * quat->z - quat->y * quat->w);
    matrix[9] = 2.0f * (quat->y * quat->z + quat->x * quat->w);
    matrix[10] = 1.0f - 2.0f * (quat->x * quat->x + quat->y * quat->y);
    matrix[11] = 0.0f;

    matrix[12] = 0.0f;
    matrix[13] = 0.0f;
    matrix[14] = 0.0f;
    matrix[15] = 1.0f;
}

inline
void quaternion_to_rotation_sparse(f32* __restrict matrix, const v4_f32* __restrict quat)
{
    matrix[0] = 1.0f - 2.0f * (quat->y * quat->y + quat->z * quat->z);
    matrix[1] = 2.0f * (quat->x * quat->y - quat->z * quat->w);
    matrix[2] = 2.0f * (quat->x * quat->z + quat->y * quat->w);

    matrix[3] = 2.0f * (quat->x * quat->y + quat->z * quat->w);
    matrix[4] = 1.0f - 2.0f * (quat->x * quat->x + quat->z * quat->z);
    matrix[5] = 2.0f * (quat->y * quat->z - quat->x * quat->w);

    matrix[6] = 2.0f * (quat->x * quat->z - quat->y * quat->w);
    matrix[7] = 2.0f * (quat->y * quat->z + quat->x * quat->w);
    matrix[8] = 1.0f - 2.0f * (quat->x * quat->x + quat->y * quat->y);
}

inline
void quaternion_from_vec(v4_f32* __restrict quat, const v4_f32* __restrict vec)
{
    quat->x = vec->x;
    quat->y = vec->y;
    quat->z = vec->z;
    quat->w = 0;
}

inline
void quaternion_from_vec(v4_f32* __restrict quat, const v3_f32* __restrict vec)
{
    quat->x = vec->x;
    quat->y = vec->y;
    quat->z = vec->z;
    quat->w = 0;
}

inline
void quaternion_to_vec(v4_f32* __restrict vec, const v4_f32* __restrict quat)
{
    vec->x = quat->x;
    vec->y = quat->y;
    vec->z = quat->z;
    vec->w = 0.0f;
}

inline
void quaternion_to_vec(v3_f32* __restrict vec, const v4_f32* __restrict quat)
{
    vec->x = quat->x;
    vec->y = quat->y;
    vec->z = quat->z;
}

inline
void quaternion_rotate_vector(v3_f32* __restrict vec, const v4_f32* __restrict quat, v3_f32* __restrict a)
{
    // @todo consider to not create this variable and cast quat to a v3_f32 pointer in cross! (the order is correct)
    v3_f32 q2 = {quat->x, quat->y, quat->z};

    v3_f32 cross;
    vec3_cross(&cross, &q2, a);

    vec->x = a->x + 2.0f * cross.x * quat->w + q2.y * cross.z - q2.z * cross.y;
    vec->y = a->y + 2.0f * cross.y * quat->w + q2.z * cross.x - q2.x * cross.z;
    vec->z = a->z + 2.0f * cross.z * quat->w + q2.x * cross.y - q2.y * cross.x;
}

// active = point rotated respective to coordinate system
inline
void quaternion_rotate_active(v4_f32* __restrict p, const v4_f32* __restrict quat, const v4_f32* __restrict quat_inv)
{
    //ASSERT_SIMPLE(OMS_ABS_F32(x * x + y * y + z * z + w * z - 1.0f) < 0.01);

    v4_f32 p_tmp;
    quaternion_multiply(&p_tmp, quat_inv, p);
    quaternion_multiply(p, &p_tmp, quat);
}

// passive = coordinate system is rotated
inline
void quaternion_rotate_passive(v4_f32* __restrict p, const v4_f32* __restrict quat, const v4_f32* __restrict quat_inv)
{
    //ASSERT_SIMPLE(OMS_ABS_F32(x * x + y * y + z * z + w * w - 1.0f) < 0.01);

    v4_f32 p_tmp;
    quaternion_multiply(&p_tmp, quat, p);
    quaternion_multiply(p, &p_tmp, quat_inv);
}

// Rotation algorithm
// 1. create quat from pitch/yaw/roll
// 2. convert the quat to unit length
// 3. create quat_inv
// 4. create quat from vec
// 5. call quat_rotate_*
// 6. convert quat to vec
// @todo Since this is usually done on multiple vecs, we should probably accept an array of vecs and then use simd
void quaternion_rotate_active(v3_f32* vec, f32 pitch, f32 yaw, f32 roll)
{
    v4_f32 q;
    quaternion_from_euler(&q, pitch, yaw, roll); // q is already in unit length

    v4_f32 q_inv;
    quaternion_inverse(&q_inv, &q);

    v4_f32 p = { vec->x, vec->y, vec->z, 0.0f };

    quaternion_rotate_active(&p, &q, &q_inv);

    vec->x = p.x;
    vec->y = p.y;
    vec->z = p.z;
}

void quaternion_rotate_active(v4_f32* quat, f32 pitch, f32 yaw, f32 roll)
{
    v4_f32 q;
    quaternion_from_euler(&q, pitch, yaw, roll); // q is already in unit length

    v4_f32 q_inv;
    quaternion_inverse(&q_inv, &q);

    quaternion_rotate_active(quat, &q, &q_inv);
}

void quaternion_rotate_passive(v3_f32* vec, f32 pitch, f32 yaw, f32 roll)
{
    v4_f32 q;
    quaternion_from_euler(&q, pitch, yaw, roll); // q is already in unit length

    v4_f32 q_inv;
    quaternion_inverse(&q_inv, &q);

    v4_f32 p = { vec->x, vec->y, vec->z, 0.0f };

    quaternion_rotate_passive(&p, &q, &q_inv);

    vec->x = p.x;
    vec->y = p.y;
    vec->z = p.z;
}

void quaternion_rotate_passive(v4_f32* quat, f32 pitch, f32 yaw, f32 roll)
{
    v4_f32 q;
    quaternion_from_euler(&q, pitch, yaw, roll); // q is already in unit length

    v4_f32 q_inv;
    quaternion_inverse(&q_inv, &q);

    quaternion_rotate_passive(quat, &q, &q_inv);
}

#endif