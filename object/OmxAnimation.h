/**
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_OBJECT_OMX_ANIMATION_H
#define COMS_OBJECT_OMX_ANIMATION_H

#include "../stdlib/Stdlib.h"
#include "../animation/Animation.h"
#include "Omx.h"

// Animations target bones by name hash. Before an animation can be played on a skeleton it needs to be bound once:
//      uint16 channel_to_bone[anim->channel_count];
//      omx_animation_bind(anim_file, anim, model_file, skeleton, channel_to_bone);
// The binding can be cached per (animation, skeleton) pair. Afterwards the animation can be sampled every frame.
//
// Retargeting rule (animation authored on a different skeleton):
//      rotation channels are always applied
//      translation channels are only applied to root bones and scaled by translation_scale (e.g. model height / rig height)
//      all other translations keep the bind pose of the target skeleton (= proportions of the target model are kept)
//      OMX_ANIMATION_FULL_TRANSLATION disables this rule

struct OmxTransform {
    v3_f32 position;
    v3_f32 scale;
    quaternion rotation;
};

// Per instance animation layer (e.g. layer 0 = walk, layer 1 = wave arm)
struct OmxAnimationLayer {
    uint32 animation;  // animation id, OMX_INVALID_ID = inactive
    f32 time;
    f32 speed;
    f32 weight;
};

// Returns the number of channels that found a matching bone
uint32 omx_animation_bind(
    const OmxFile* anim_file,
    const OmxAnimation* anim,
    const OmxFile* skel_file,
    const OmxSkeleton* skeleton,
    uint16* channel_to_bone
) NO_EXCEPT
{
    const OmxChannel* channels = omx_animation_channels(anim_file, anim);

    uint32 matched = 0;
    for (uint32 i = 0; i < anim->channel_count; ++i) {
        channel_to_bone[i] = omx_skeleton_find_bone(skel_file, skeleton, channels[i].bone);
        matched += channel_to_bone[i] != OMX_INVALID_BONE;
    }

    return matched;
}

inline
void omx_pose_from_bind(const OmxFile* skel_file, const OmxSkeleton* skeleton, OmxTransform* pose) NO_EXCEPT
{
    const OmxBone* bones = omx_skeleton_bones(skel_file, skeleton);
    for (uint32 i = 0; i < skeleton->bone_count; ++i) {
        pose[i].position = bones[i].position;
        pose[i].rotation = bones[i].rotation;
        pose[i].scale = bones[i].scale;
    }
}

static inline
void omx_quaternion_nlerp(const f32* a, const f32* b, f32 t, f32* out) NO_EXCEPT
{
    // shortest path
    const f32 dot = a[0] * b[0] + a[1] * b[1] + a[2] * b[2] + a[3] * b[3];
    const f32 sign = dot < 0.0f ? -1.0f : 1.0f;

    f32 length_sq = 0.0f;
    for (int32 i = 0; i < 4; ++i) {
        out[i] = a[i] + (b[i] * sign - a[i]) * t;
        length_sq += out[i] * out[i];
    }

    const f32 inv_length = 1.0f / sqrtf(length_sq);
    for (int32 i = 0; i < 4; ++i) {
        out[i] *= inv_length;
    }
}

// Samples a single channel at time (time is already wrapped/clamped)
static
void omx_channel_sample(const OmxFile* file, const OmxChannel* ch, f32 time, f32* out) NO_EXCEPT
{
    const OmxKeyframe* keys = file->keys + ch->key_offset;
    const int32 values = ch->path == OMX_CHANNEL_ROTATION ? 4 : 3;

    if (time <= keys[0].time || ch->key_count == 1) {
        memcpy(out, keys[0].value, sizeof(f32) * values);

        return;
    }

    if (time >= keys[ch->key_count - 1].time) {
        memcpy(out, keys[ch->key_count - 1].value, sizeof(f32) * values);

        return;
    }

    // Binary search for the key in front of time
    uint32 low = 0;
    uint32 high = ch->key_count - 1;
    while (high - low > 1) {
        const uint32 mid = (low + high) / 2;
        if (keys[mid].time <= time) {
            low = mid;
        } else {
            high = mid;
        }
    }

    const OmxKeyframe* k0 = &keys[low];
    const OmxKeyframe* k1 = &keys[high];

    const f32 t = anim_ease((time - k0->time) / (k1->time - k0->time), k0->ease);

    if (ch->path == OMX_CHANNEL_ROTATION) {
        omx_quaternion_nlerp(k0->value, k1->value, t, out);
    } else {
        for (int32 i = 0; i < 3; ++i) {
            out[i] = k0->value[i] + (k1->value[i] - k0->value[i]) * t;
        }
    }
}

FORCE_INLINE
f32 omx_animation_wrap_time(const OmxAnimation* anim, f32 time) NO_EXCEPT
{
    if (anim->duration <= 0.0f) {
        return 0.0f;
    }

    if (anim->flags & OMX_ANIMATION_LOOP) {
        time = fmodf(time, anim->duration);

        return time < 0.0f ? time + anim->duration : time;
    }

    return time < 0.0f ? 0.0f : (time > anim->duration ? anim->duration : time);
}

// Writes the sampled animation into pose (pose must contain a valid pose, e.g. omx_pose_from_bind())
// Bones without channel keep their current transform
void omx_animation_sample(
    const OmxFile* anim_file,
    const OmxAnimation* anim,
    const OmxFile* skel_file,
    const OmxSkeleton* skeleton,
    const uint16* channel_to_bone,
    f32 time,
    f32 translation_scale,
    OmxTransform* pose
) NO_EXCEPT
{
    time = omx_animation_wrap_time(anim, time);

    const OmxChannel* channels = omx_animation_channels(anim_file, anim);
    const OmxBone* bones = omx_skeleton_bones(skel_file, skeleton);
    const bool full_translation = anim->flags & OMX_ANIMATION_FULL_TRANSLATION;

    for (uint32 i = 0; i < anim->channel_count; ++i) {
        const uint16 bone = channel_to_bone[i];
        const OmxChannel* ch = &channels[i];
        if (bone == OMX_INVALID_BONE || ch->key_count == 0) {
            continue;
        }

        f32 value[4];
        omx_channel_sample(anim_file, ch, time, value);

        switch (ch->path) {
            case OMX_CHANNEL_POSITION: {
                    if (bones[bone].parent == OMX_INVALID_BONE) {
                        pose[bone].position = { value[0] * translation_scale, value[1] * translation_scale, value[2] * translation_scale };
                    } else if (full_translation) {
                        pose[bone].position = { value[0], value[1], value[2] };
                    }
                } break;
            case OMX_CHANNEL_ROTATION: {
                    memcpy(pose[bone].rotation.vec, value, sizeof(f32) * 4);
                } break;
            case OMX_CHANNEL_SCALE: {
                    pose[bone].scale = { value[0], value[1], value[2] };
                } break;
            default:
                UNREACHABLE();
        }
    }
}

// out = a * (1 - t) + b * t, e.g. walk -> run transition
void omx_pose_blend(const OmxTransform* a, const OmxTransform* b, f32 t, uint32 count, OmxTransform* out) NO_EXCEPT
{
    for (uint32 i = 0; i < count; ++i) {
        for (int32 j = 0; j < 3; ++j) {
            out[i].position.vec[j] = a[i].position.vec[j] + (b[i].position.vec[j] - a[i].position.vec[j]) * t;
            out[i].scale.vec[j] = a[i].scale.vec[j] + (b[i].scale.vec[j] - a[i].scale.vec[j]) * t;
        }

        omx_quaternion_nlerp(a[i].rotation.vec, b[i].rotation.vec, t, out[i].rotation.vec);
    }
}

// Collects the events in the time range (from, to]. Handles loop wrap around.
// Returns the number of events written to out (max. out_size)
uint32 omx_animation_collect_events(
    const OmxFile* anim_file,
    const OmxAnimation* anim,
    f32 from,
    f32 to,
    const OmxEvent** out,
    uint32 out_size
) NO_EXCEPT
{
    const OmxEvent* events = omx_animation_events(anim_file, anim);
    uint32 count = 0;

    const f32 wrapped_from = omx_animation_wrap_time(anim, from);
    const f32 wrapped_to = omx_animation_wrap_time(anim, to);

    // Looped over the end -> (from, duration] + [0, to]
    const bool wrapped = (anim->flags & OMX_ANIMATION_LOOP) && to - from > 0.0f && wrapped_to < wrapped_from;

    for (uint32 i = 0; i < anim->event_count && count < out_size; ++i) {
        const f32 t = events[i].time;

        const bool hit = wrapped
            ? (t > wrapped_from || t <= wrapped_to)
            : (t > wrapped_from && t <= wrapped_to);

        if (hit) {
            out[count++] = &events[i];
        }
    }

    return count;
}

/////////////////////////////////////////////////////////////
// Matrices (column major 4x4, f32[16])
/////////////////////////////////////////////////////////////

static inline
void omx_mat4_from_transform(const OmxTransform* t, f32* m) NO_EXCEPT
{
    const f32 x = t->rotation.x;
    const f32 y = t->rotation.y;
    const f32 z = t->rotation.z;
    const f32 w = t->rotation.w;

    m[0] = (1.0f - 2.0f * (y * y + z * z)) * t->scale.x;
    m[1] = (2.0f * (x * y + z * w)) * t->scale.x;
    m[2] = (2.0f * (x * z - y * w)) * t->scale.x;
    m[3] = 0.0f;

    m[4] = (2.0f * (x * y - z * w)) * t->scale.y;
    m[5] = (1.0f - 2.0f * (x * x + z * z)) * t->scale.y;
    m[6] = (2.0f * (y * z + x * w)) * t->scale.y;
    m[7] = 0.0f;

    m[8] = (2.0f * (x * z + y * w)) * t->scale.z;
    m[9] = (2.0f * (y * z - x * w)) * t->scale.z;
    m[10] = (1.0f - 2.0f * (x * x + y * y)) * t->scale.z;
    m[11] = 0.0f;

    m[12] = t->position.x;
    m[13] = t->position.y;
    m[14] = t->position.z;
    m[15] = 1.0f;
}

// out = a * b (out must not alias a or b)
static inline
void omx_mat4_mul(const f32* a, const f32* b, f32* out) NO_EXCEPT
{
    for (int32 col = 0; col < 4; ++col) {
        for (int32 row = 0; row < 4; ++row) {
            out[col * 4 + row] = a[row] * b[col * 4]
                + a[4 + row] * b[col * 4 + 1]
                + a[8 + row] * b[col * 4 + 2]
                + a[12 + row] * b[col * 4 + 3];
        }
    }
}

// Inverse of an affine matrix (rotation * scale + translation)
static inline
void omx_mat4_inverse_affine(const f32* m, f32* out) NO_EXCEPT
{
    const f32 a = m[0], b = m[4], c = m[8];
    const f32 d = m[1], e = m[5], f = m[9];
    const f32 g = m[2], h = m[6], i = m[10];

    const f32 det = a * (e * i - f * h) - b * (d * i - f * g) + c * (d * h - e * g);
    const f32 inv_det = det != 0.0f ? 1.0f / det : 0.0f;

    out[0] = (e * i - f * h) * inv_det;
    out[4] = (c * h - b * i) * inv_det;
    out[8] = (b * f - c * e) * inv_det;
    out[1] = (f * g - d * i) * inv_det;
    out[5] = (a * i - c * g) * inv_det;
    out[9] = (c * d - a * f) * inv_det;
    out[2] = (d * h - e * g) * inv_det;
    out[6] = (b * g - a * h) * inv_det;
    out[10] = (a * e - b * d) * inv_det;

    out[12] = -(out[0] * m[12] + out[4] * m[13] + out[8] * m[14]);
    out[13] = -(out[1] * m[12] + out[5] * m[13] + out[9] * m[14]);
    out[14] = -(out[2] * m[12] + out[6] * m[13] + out[10] * m[14]);

    out[3] = 0.0f;
    out[7] = 0.0f;
    out[11] = 0.0f;
    out[15] = 1.0f;
}

// Local pose -> model space matrices (out = bone_count * 16 floats)
// Works in a single loop since parents are always defined before their children
void omx_pose_to_model_space(const OmxFile* skel_file, const OmxSkeleton* skeleton, const OmxTransform* pose, f32* out) NO_EXCEPT
{
    const OmxBone* bones = omx_skeleton_bones(skel_file, skeleton);

    f32 local[16];
    for (uint32 i = 0; i < skeleton->bone_count; ++i) {
        if (bones[i].parent == OMX_INVALID_BONE) {
            omx_mat4_from_transform(&pose[i], out + i * 16);
        } else {
            omx_mat4_from_transform(&pose[i], local);
            omx_mat4_mul(out + bones[i].parent * 16, local, out + i * 16);
        }
    }
}

// Inverse bind matrices (out = bone_count * 16 floats), only needs to be calculated once per skeleton
void omx_skeleton_inverse_bind(const OmxFile* skel_file, const OmxSkeleton* skeleton, f32* out, f32* temp) NO_EXCEPT
{
    const OmxBone* bones = omx_skeleton_bones(skel_file, skeleton);

    f32 local[16];
    for (uint32 i = 0; i < skeleton->bone_count; ++i) {
        const OmxTransform t = { bones[i].position, bones[i].scale, bones[i].rotation };

        if (bones[i].parent == OMX_INVALID_BONE) {
            omx_mat4_from_transform(&t, temp + i * 16);
        } else {
            omx_mat4_from_transform(&t, local);
            omx_mat4_mul(temp + bones[i].parent * 16, local, temp + i * 16);
        }

        omx_mat4_inverse_affine(temp + i * 16, out + i * 16);
    }
}

// Final skinning matrices for the gpu: model_space * inverse_bind
// model_space is overwritten with the result
void omx_skin_matrices(const OmxSkeleton* skeleton, const f32* inverse_bind, f32* model_space) NO_EXCEPT
{
    f32 temp[16];
    for (uint32 i = 0; i < skeleton->bone_count; ++i) {
        omx_mat4_mul(model_space + i * 16, inverse_bind + i * 16, temp);
        memcpy(model_space + i * 16, temp, sizeof(temp));
    }
}

#endif
