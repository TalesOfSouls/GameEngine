/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_CAMERA_H
#define COMS_CAMERA_H

#include "../stdlib/Types.h"
#include "../stdlib/GameMathTypes.h"
#include "../math/matrix/Matrix.h"
#include "../compiler/CompilerUtils.h"
#include "CameraMovement.h"
#include "../gpuapi/GpuApiType.h"
#include "../sort/Sort.h"

#define CAMERA_MAX_INPUTS 4

// @todo Please check out if we can switch to quaternions. We tried but failed.

/**
 * Gpu API coordinate information
 *
 *          Coord-Sys.  NDC-X   NDC-Y   NDC-Z   Clip-Space-Z    Y-Axis
 * DirectX  left        [-1, 1] [-1, 1] [0, 1]  [0, 1]          Up = positive
 * Opengl   right       [-1, 1] [-1, 1] [-1, 1] [-1, 1]         Up = positive
 * Vulkan   right       [-1, 1] [-1, 1] [0, 1]  [0, 1]          Down = positive
 * Metal    right       [-1, 1] [-1, 1] [0, 1]  [0, 1]          Up = positive
 *
 * The first value in Z always represents the near value and the second value the far value
 */

enum CameraStateChanges : byte {
    CAMERA_STATE_CHANGE_NONE = 0,
    CAMERA_STATE_CHANGE_WINDOW = 1 << 0,
    CAMERA_STATE_CHANGE_POSITION = 1 << 1,
    CAMERA_STATE_CHANGE_ORIENTATION = 1 << 2,
    CAMERA_STATE_CHANGE_OTHER = 1 << 3, // e.g. fov, aspect, ...
};

struct Frustum {
    // A frustum consists of 6 planes where every plane has the form ax + by + cz + d = 0
    // This means every plane requires 4 parameters
    union {
        f32 plane[6 * 4];
        v4_f32 eq[6];
    };
};

struct Camera {
    byte state_changes;
    GpuApiType gpu_api_type;

    v3_f32 location;
    v4_f32 orientation;

    v3_f32 front;
    v3_f32 right;
    v3_f32 up;
    v3_f32 world_up;

    f32 speed;
    f32 sensitivity;
    f32 zoom;

    // @question consider to use v2_f32 with width and height value
    // v2_f32 viewport;
    f32 viewport_width;
    f32 viewport_height;

    f32 fov;
    f32 znear;
    f32 zfar;
    f32 aspect;

    // @question Consider to replace with v16_f32 types
    // Careful, you cannot change the order of this
    // The reason is we copy all of the data to the gpu in one go in some cases
    // If we would change the order we would also change the order of the data on the gpu
    // This of course could brick the shaders
    // However, if we use v16_f32 someone might assume that the internal data can use simd
    // BUT that is never the case, the camera data is not using simd since we are rarely performing operations at the moment
    // If this changes in the future and we use this for cpu calculations it could make sense to change to v16_f32 and store only when uploading to gpu
    alignas(64) f32 projection[16];

    // @performance Couldn't we create an optimized orth matrix for hud elements
    //      Remember for hud elements we don't need to consider the z component when calculating x/y and width/height
    //      Of course we still need a normal orth matrix for non-hud elements
    alignas(64) f32 orth[16];
    alignas(64) f32 view[16];
    alignas(64) Frustum frustum;
};

void camera_frustum_update(Camera* const camera) NO_EXCEPT
{
    const v3_f32 pos = camera->location;
    const v3_f32 front = camera->front;
    const v3_f32 up = camera->up;
    const v3_f32 right = camera->right;

    // Near and far plane centers
    const v3_f32 near_center = {
        pos.x + front.x * camera->znear,
        pos.y + front.y * camera->znear,
        pos.z + front.z * camera->znear
    };
    const v3_f32 far_center = {
        pos.x + front.x * camera->zfar,
        pos.y + front.y * camera->zfar,
        pos.z + front.z * camera->zfar
    };

    // Precompute near/far plane half extents
    const f32 tan_fov = tanf(camera->fov * 0.5f);
    const f32 fh = camera->zfar * tan_fov;
    const f32 fw = fh * camera->aspect;

    // Precompute scaled basis vectors to avoid multiple v3_scale calls
    const v3_f32 up_fh = { up.x * fh, up.y * fh, up.z * fh };
    const v3_f32 up_negfh = { -up_fh.x, -up_fh.y, -up_fh.z };

    const v3_f32 right_fw = { right.x * fw, right.y * fw, right.z * fw };
    const v3_f32 right_negfw = { -right_fw.x, -right_fw.y, -right_fw.z };

    const v3_f32 fc_up = vec3_add(far_center, up_fh);
    const v3_f32 fc_down = vec3_add(far_center, up_negfh);

    // Compute far plane corners using the pre-shifted positions
    const v3_f32 ftl = vec3_add(fc_up, right_negfw);
    const v3_f32 ftr = vec3_add(fc_up, right_fw);
    const v3_f32 fbl = vec3_add(fc_down, right_negfw);
    const v3_f32 fbr = vec3_add(fc_down, right_fw);

    const v3_f32 ftl_r = vec3_sub(ftl, pos);
    const v3_f32 ftr_r = vec3_sub(ftr, pos);
    const v3_f32 fbl_r = vec3_sub(fbl, pos);
    const v3_f32 fbr_r = vec3_sub(fbr, pos);

    v3_f32 cross;

    // Left plane: cross( ftl - pos, fbl - pos )
    cross = vec3_cross(ftl_r, fbl_r);
    camera->frustum.eq[0] = { cross.x, cross.y, cross.z, -(cross.x * pos.x + cross.y * pos.y + cross.z * pos.z) };

    // Right plane: cross( fbr - pos, ftr - pos )
    cross = vec3_cross(fbr_r, ftr_r);
    camera->frustum.eq[1] = { cross.x, cross.y, cross.z, -(cross.x * pos.x + cross.y * pos.y + cross.z * pos.z) };

    // Bottom plane: cross( fbl - pos, fbr - pos )
    cross = vec3_cross(fbl_r, fbr_r);
    camera->frustum.eq[2] = { cross.x, cross.y, cross.z, -(cross.x * pos.x + cross.y * pos.y + cross.z * pos.z) };

    // Top plane: cross( ftr - pos, ftl - pos )
    cross = vec3_cross(ftr_r, ftl_r);
    camera->frustum.eq[3] = { cross.x, cross.y, cross.z, -(cross.x * pos.x + cross.y * pos.y + cross.z * pos.z) };

    // Near plane: normal = front
    camera->frustum.eq[4] = {
        front.x, front.y, front.z,
        -(front.x * near_center.x + front.y * near_center.y + front.z * near_center.z)
    };

    // Far plane: normal = -front
    camera->frustum.eq[5] = {
        -front.x, -front.y, -front.z,
        (front.x * far_center.x + front.y * far_center.y + front.z * far_center.z)
    };
}

// In some cases we only need to update the frustum based on position updates
// This allows us to simply make shifts to the original frustum positions
inline
void camera_frustum_pos_update(Camera* const camera, v3_f32 old_pos) NO_EXCEPT
{
    const v3_f32 delta = {
        camera->location.x - old_pos.x,
        camera->location.y - old_pos.y,
        camera->location.z - old_pos.z
    };

    // Update plane distances: d_new = d_old - n · delta
    for (size_t i = 0; i < 6; ++i) {
        camera->frustum.eq[i].w -= camera->frustum.eq[i].x * delta.x
            + camera->frustum.eq[i].y * delta.y
            + camera->frustum.eq[i].z * delta.z;
    }
}

static FORCE_INLINE
void camera_init_rh_opengl(Camera* const camera) NO_EXCEPT
{
    camera->orientation = {0.0f, -90.0f, 0.0f, 1.0f};
    camera->front = {0.0f, 0.0f, -1.0f};
    camera->right = {1.0f, 0.0f, 0.0f};
    camera->up = {0.0f, 1.0f, 0.0f};
    camera->world_up = {0.0f, 1.0f, 0.0f};
}

static FORCE_INLINE
void camera_init_rh_vulkan(Camera* const camera) NO_EXCEPT
{
    camera->orientation = {0.0f, -90.0f, 0.0f, 1.0f};
    camera->front = {0.0f, 0.0f, -1.0f};
    camera->right = {1.0f, 0.0f, 0.0f};
    camera->up = {0.0f, -1.0f, 0.0f};
    camera->world_up = {0.0f, -1.0f, 0.0f};
}

static FORCE_INLINE
void camera_init_lh(Camera* const camera) NO_EXCEPT
{
    camera->orientation = {0.0f, 90.0f, 0.0f, 1.0f};
    camera->front = {0.0f, 0.0f, 1.0f};
    camera->right = {1.0f, 0.0f, 0.0f};
    camera->up = {0.0f, 1.0f, 0.0f};
    camera->world_up = {0.0f, 1.0f, 0.0f};
}

static inline HOT_CODE
void camera_vectors_update(Camera* const camera) NO_EXCEPT
{
    /*
    f32 cos_ori_x = cosf(deg2rad(camera->orientation.x));
    camera->front.x = cos_ori_x * cosf(deg2rad(camera->orientation.y));
    camera->front.y = sinf(deg2rad(camera->orientation.x));
    camera->front.z = cos_ori_x * sinf(deg2rad(camera->orientation.y));
    */

    f32 cos_ori_x;
    SINCOSF(deg2rad(camera->orientation.x), camera->front.y, cos_ori_x);
    SINCOSF(deg2rad(camera->orientation.y), camera->front.z, camera->front.x);
    camera->front.x *= cos_ori_x;
    camera->front.z *= cos_ori_x;

    camera->right = vec3_cross(camera->front, camera->world_up);
    camera->up = vec3_cross(camera->right, camera->front);

    // We checked if combining these 3 into a single SIMD function, but it was slower
    vec3_normalize(&camera->right);
    vec3_normalize(&camera->front);
    vec3_normalize(&camera->up);
}

inline HOT_CODE
void camera_rotate(Camera* const camera, int32 dx, int32 dy) NO_EXCEPT
{
    camera->state_changes |= CAMERA_STATE_CHANGE_ORIENTATION;
    camera->orientation.x += dy * camera->sensitivity;
    camera->orientation.y -= dx * camera->sensitivity;

    if (camera->orientation.x > 89.0f) {
        camera->orientation.x = 89.0f;
    } else if (camera->orientation.x < -89.0f) {
        camera->orientation.x = -89.0f;
    }

    if (camera->orientation.y > 360.0f) {
        camera->orientation.y -= 360.0f;
    } else if (camera->orientation.y < -360.0f) {
        camera->orientation.y += 360.0f;
    }

    camera_vectors_update(camera);
}

inline
void camera_movement(
    Camera* const __restrict camera,
    CameraMovement movement,
    f32 dt,
    bool relative_to_world = true
) NO_EXCEPT
{
    camera->state_changes |= CAMERA_STATE_CHANGE_POSITION;
    const f32 velocity = camera->speed * dt;

    if (relative_to_world) {
        switch(movement) {
            case CAMERA_MOVEMENT_FORWARD: {
                    camera->location.z += velocity;
                } break;
            case CAMERA_MOVEMENT_BACK: {
                    camera->location.z -= velocity;
                } break;
            case CAMERA_MOVEMENT_LEFT: {
                    camera->location.x -= velocity;
                } break;
            case CAMERA_MOVEMENT_RIGHT: {
                    camera->location.x += velocity;
                } break;
            case CAMERA_MOVEMENT_UP: {
                    camera->location.y += velocity;
                } break;
            case CAMERA_MOVEMENT_DOWN: {
                    camera->location.y -= velocity;
                } break;
            case CAMERA_MOVEMENT_PITCH_UP: {
                    camera->orientation.x += velocity;
                } break;
            case CAMERA_MOVEMENT_PITCH_DOWN: {
                    camera->orientation.x -= velocity;
                } break;
            case CAMERA_MOVEMENT_ROLL_LEFT: {
                    camera->orientation.z += velocity;
                } break;
            case CAMERA_MOVEMENT_ROLL_RIGHT: {
                    camera->orientation.z -= velocity;
                } break;
            case CAMERA_MOVEMENT_YAW_LEFT: {
                    camera->orientation.y += velocity;
                } break;
            case CAMERA_MOVEMENT_YAW_RIGHT: {
                    camera->orientation.y -= velocity;
                } break;
            case CAMERA_MOVEMENT_ZOOM_IN: {
                    camera->zoom += velocity;
                } break;
            case CAMERA_MOVEMENT_ZOOM_OUT: {
                    camera->zoom -= velocity;
                } break;
            default: {
                UNREACHABLE();
            }
        }
    } else {
        camera->state_changes |= CAMERA_STATE_CHANGE_ORIENTATION;

        const v3_f32 forward = camera->front;

        v3_f32 right = vec3_cross(camera->world_up, forward);
        v3_f32 up = vec3_cross(right, forward);

        vec3_normalize(&right);
        vec3_normalize(&up);

        switch(movement) {
            case CAMERA_MOVEMENT_NONE: {
                return;
            };
            case CAMERA_MOVEMENT_FORWARD: {
                    camera->location.x += forward.x * velocity;
                    camera->location.y += forward.y * velocity;
                    camera->location.z += forward.z * velocity;
                } break;
            case CAMERA_MOVEMENT_BACK: {
                    camera->location.x -= forward.x * velocity;
                    camera->location.y -= forward.y * velocity;
                    camera->location.z -= forward.z * velocity;
                } break;
            case CAMERA_MOVEMENT_LEFT: {
                    camera->location.x -= right.x * velocity;
                    camera->location.y -= right.y * velocity;
                    camera->location.z -= right.z * velocity;
                } break;
            case CAMERA_MOVEMENT_RIGHT: {
                    camera->location.x += right.x * velocity;
                    camera->location.y += right.y * velocity;
                    camera->location.z += right.z * velocity;
                } break;
            case CAMERA_MOVEMENT_UP: {
                    camera->location.x += up.x * velocity;
                    camera->location.y += up.y * velocity;
                    camera->location.z += up.z * velocity;
                } break;
            case CAMERA_MOVEMENT_DOWN: {
                    camera->location.x -= up.x * velocity;
                    camera->location.y -= up.y * velocity;
                    camera->location.z -= up.z * velocity;
                } break;
            case CAMERA_MOVEMENT_PITCH_UP: {
                    camera->orientation.x += velocity;
                } break;
            case CAMERA_MOVEMENT_PITCH_DOWN: {
                    camera->orientation.x -= velocity;
                } break;
            case CAMERA_MOVEMENT_ROLL_LEFT: {
                    camera->orientation.z += velocity;
                } break;
            case CAMERA_MOVEMENT_ROLL_RIGHT: {
                    camera->orientation.z -= velocity;
                } break;
            case CAMERA_MOVEMENT_YAW_LEFT: {
                    camera->orientation.z += velocity;
                } break;
            case CAMERA_MOVEMENT_YAW_RIGHT: {
                    camera->orientation.z -= velocity;
                } break;
            case CAMERA_MOVEMENT_ZOOM_IN: {
                    camera->zoom += velocity;
                } break;
            case CAMERA_MOVEMENT_ZOOM_OUT: {
                    camera->zoom -= velocity;
                } break;
            default: {
                UNREACHABLE();
            }
        }
    }
}

// you can have up to 4 camera movement inputs at the same time
inline
void camera_movement(
    Camera* const __restrict camera,
    const CameraMovement* const __restrict movement,
    f32 dt,
    bool relative_to_world = true
) NO_EXCEPT
{
    camera->state_changes |= CAMERA_STATE_CHANGE_POSITION;
    const f32 velocity = camera->speed * dt;

    if (relative_to_world) {
        for (int32 i = 0; i < CAMERA_MAX_INPUTS; i++) {
            switch(movement[i]) {
                case CAMERA_MOVEMENT_FORWARD: {
                        camera->location.z += velocity;
                    } break;
                case CAMERA_MOVEMENT_BACK: {
                        camera->location.z -= velocity;
                    } break;
                case CAMERA_MOVEMENT_LEFT: {
                        camera->location.x -= velocity;
                    } break;
                case CAMERA_MOVEMENT_RIGHT: {
                        camera->location.x += velocity;
                    } break;
                case CAMERA_MOVEMENT_UP: {
                        camera->location.y += velocity;
                    } break;
                case CAMERA_MOVEMENT_DOWN: {
                        camera->location.y -= velocity;
                    } break;
                case CAMERA_MOVEMENT_PITCH_UP: {
                        camera->orientation.x += velocity;
                    } break;
                case CAMERA_MOVEMENT_PITCH_DOWN: {
                        camera->orientation.x -= velocity;
                    } break;
                case CAMERA_MOVEMENT_ROLL_LEFT: {
                        camera->orientation.z += velocity;
                    } break;
                case CAMERA_MOVEMENT_ROLL_RIGHT: {
                        camera->orientation.z -= velocity;
                    } break;
                case CAMERA_MOVEMENT_YAW_LEFT: {
                        camera->orientation.y += velocity;
                    } break;
                case CAMERA_MOVEMENT_YAW_RIGHT: {
                        camera->orientation.y -= velocity;
                    } break;
                case CAMERA_MOVEMENT_ZOOM_IN: {
                        camera->zoom += velocity;
                    } break;
                case CAMERA_MOVEMENT_ZOOM_OUT: {
                        camera->zoom -= velocity;
                    } break;
                default: {
                    UNREACHABLE();
                }
            }
        }
    } else {
        const v3_f32 forward = camera->front;

        v3_f32 right = vec3_cross(camera->world_up, forward);
        v3_f32 up = vec3_cross(right, forward);

        vec3_normalize(&right);
        vec3_normalize(&up);

        for (int32 i = 0; i < CAMERA_MAX_INPUTS; i++) {
            switch(movement[i]) {
                case CAMERA_MOVEMENT_NONE: {
                    return;
                };
                case CAMERA_MOVEMENT_FORWARD: {
                        camera->location.x += forward.x * velocity;
                        camera->location.y += forward.y * velocity;
                        camera->location.z += forward.z * velocity;
                    } break;
                case CAMERA_MOVEMENT_BACK: {
                        camera->location.x -= forward.x * velocity;
                        camera->location.y -= forward.y * velocity;
                        camera->location.z -= forward.z * velocity;
                    } break;
                case CAMERA_MOVEMENT_LEFT: {
                        camera->location.x -= right.x * velocity;
                        camera->location.y -= right.y * velocity;
                        camera->location.z -= right.z * velocity;
                    } break;
                case CAMERA_MOVEMENT_RIGHT: {
                        camera->location.x += right.x * velocity;
                        camera->location.y += right.y * velocity;
                        camera->location.z += right.z * velocity;
                    } break;
                case CAMERA_MOVEMENT_UP: {
                        camera->location.x += up.x * velocity;
                        camera->location.y += up.y * velocity;
                        camera->location.z += up.z * velocity;
                    } break;
                case CAMERA_MOVEMENT_DOWN: {
                        camera->location.x -= up.x * velocity;
                        camera->location.y -= up.y * velocity;
                        camera->location.z -= up.z * velocity;
                    } break;
                case CAMERA_MOVEMENT_PITCH_UP: {
                        camera->orientation.x += velocity;
                    } break;
                case CAMERA_MOVEMENT_PITCH_DOWN: {
                        camera->orientation.x -= velocity;
                    } break;
                case CAMERA_MOVEMENT_ROLL_LEFT: {
                        camera->orientation.z += velocity;
                    } break;
                case CAMERA_MOVEMENT_ROLL_RIGHT: {
                        camera->orientation.z -= velocity;
                    } break;
                case CAMERA_MOVEMENT_YAW_LEFT: {
                        camera->orientation.z += velocity;
                    } break;
                case CAMERA_MOVEMENT_YAW_RIGHT: {
                        camera->orientation.z -= velocity;
                    } break;
                case CAMERA_MOVEMENT_ZOOM_IN: {
                        camera->zoom += velocity;
                    } break;
                case CAMERA_MOVEMENT_ZOOM_OUT: {
                        camera->zoom -= velocity;
                    } break;
                default: {
                    UNREACHABLE();
                }
            }
        }
    }
}

FORCE_INLINE HOT_CODE
void camera_orth_matrix_lh(Camera* const camera) NO_EXCEPT
{
    mat4_ortho_sparse_lh(
        camera->orth,
        0.0f, camera->viewport_width,
        0.0f, camera->viewport_height,
        camera->znear,
        camera->zfar
    );
}

FORCE_INLINE HOT_CODE
void camera_ui_matrix_lh(Camera* const camera) NO_EXCEPT
{
    mat4_ortho_sparse_lh(
        camera->orth,
        0.0f, camera->viewport_width,
        0.0f, camera->viewport_height,
        camera->znear,
        camera->zfar
    );
}

FORCE_INLINE HOT_CODE
void camera_orth_matrix_rh_opengl(Camera* const camera) NO_EXCEPT
{
    mat4_ortho_sparse_rh_opengl(
        camera->orth,
        0.0f, camera->viewport_width,
        0.0f, camera->viewport_height,
        camera->znear,
        camera->zfar
    );
}

FORCE_INLINE HOT_CODE
void camera_orth_matrix_rh_software(Camera* const camera) NO_EXCEPT
{
    mat4_ortho_sparse_rh_software(
        camera->orth,
        0.0f, camera->viewport_width,
        0.0f, camera->viewport_height,
        camera->znear,
        camera->zfar
    );
}

FORCE_INLINE HOT_CODE
void camera_ui_matrix_rh_software(Camera* const camera) NO_EXCEPT
{
    mat4_ortho_sparse_rh_software(
        camera->orth,
        0.0f, camera->viewport_width,
        0.0f, camera->viewport_height,
        camera->znear,
        camera->zfar
    );
}

FORCE_INLINE HOT_CODE
void camera_orth_matrix_rh_vulkan(Camera* const camera) NO_EXCEPT
{
    mat4_ortho_sparse_rh_vulkan(
        camera->orth,
        0.0f, camera->viewport_width,
        0.0f, camera->viewport_height,
        camera->znear,
        camera->zfar
    );
}

FORCE_INLINE HOT_CODE
void camera_ui_matrix_rh_vulkan(Camera* const camera) NO_EXCEPT
{
    mat4_ortho_sparse_rh_vulkan(
        camera->orth,
        0.0f, camera->viewport_width,
        0.0f, camera->viewport_height,
        camera->znear,
        camera->zfar
    );
}

inline HOT_CODE
void camera_projection_matrix_lh(Camera* const camera) NO_EXCEPT
{
    //mat4_identity(camera->projection);
    camera->projection[15] = 1.0f;
    mat4_perspective_sparse_lh(
        camera->projection,
        camera->fov,
        camera->aspect,
        camera->znear,
        camera->zfar
    );
}

inline HOT_CODE
void camera_projection_matrix_rh_opengl(Camera* const camera) NO_EXCEPT
{
    //mat4_identity(camera->projection);
    camera->projection[15] = 1.0f;
    mat4_perspective_sparse_rh(
        camera->projection,
        camera->fov,
        camera->aspect,
        camera->znear,
        camera->zfar
    );
}

inline HOT_CODE
void camera_projection_matrix_rh_vulkan(Camera* const camera) NO_EXCEPT
{
    //mat4_identity(camera->projection);
    camera->projection[15] = 1.0f;
    // @bug Fix
    mat4_perspective_sparse_rh(
        camera->projection,
        camera->fov,
        camera->aspect,
        camera->znear,
        camera->zfar
    );
}

// This is usually not used, since it is included in the view matrix
// expects the identity matrix
FORCE_INLINE
void camera_translation_matrix_sparse_rh(const Camera* const __restrict camera, f32* translation) NO_EXCEPT
{
    translation[12] = camera->location.x;
    translation[13] = camera->location.y;
    translation[14] = camera->location.z;
}

FORCE_INLINE
void camera_translation_matrix_sparse_lh(const Camera* const __restrict camera, f32* translation) NO_EXCEPT
{
    translation[3] = camera->location.x;
    translation[7] = camera->location.y;
    translation[11] = camera->location.z;
}

void
camera_view_matrix_lh(Camera* const camera) NO_EXCEPT
{
    const v3_f32 zaxis = camera->front;

    v3_f32 xaxis = vec3_cross(camera->world_up, zaxis);
    vec3_normalize(&xaxis);

    const v3_f32 yaxis = vec3_cross(zaxis, xaxis);

    // We tested if it would make sense to create a vec3_dot_sse version for the 3 dot products
    // The result was that it is not faster, only if we would do 4 dot products would we see an improvement
    camera->view[0] = xaxis.x;
    camera->view[1] = yaxis.x;
    camera->view[2] = zaxis.x;
    //camera->view[3] = 0.0f;
    camera->view[4] = xaxis.y;
    camera->view[5] = yaxis.y;
    camera->view[6] = zaxis.y;
    //camera->view[7] = 0.0f;
    camera->view[8] = xaxis.z;
    camera->view[9] = yaxis.z;
    camera->view[10] = zaxis.z;
    //camera->view[11] = 0.0f;
    camera->view[12] = -vec3_dot(xaxis, camera->location);
    camera->view[13] = -vec3_dot(yaxis, camera->location);
    camera->view[14] = -vec3_dot(zaxis, camera->location);
    camera->view[15] = 1.0f;
}

void
camera_view_matrix_rh_opengl(Camera* const camera) NO_EXCEPT
{
    const v3_f32 zaxis = { -camera->front.x, -camera->front.y, -camera->front.z };

    v3_f32 xaxis = vec3_cross(zaxis, camera->world_up);
    vec3_normalize(&xaxis);

    const v3_f32 yaxis = vec3_cross(zaxis, xaxis);

   // We tested if it would make sense to create a vec3_dot_sse version for the 3 dot products
    // The result was that it is not faster, only if we would do 4 dot products would we see an improvement
    camera->view[0] = xaxis.x;
    camera->view[1] = yaxis.x;
    camera->view[2] = zaxis.x;
    //camera->view[3] = 0.0f;
    camera->view[4] = xaxis.y;
    camera->view[5] = yaxis.y;
    camera->view[6] = zaxis.y;
    //camera->view[7] = 0.0f;
    camera->view[8] = xaxis.z;
    camera->view[9] = yaxis.z;
    camera->view[10] = zaxis.z;
    //camera->view[11] = 0.0f;
    camera->view[12] = -vec3_dot(xaxis, camera->location);
    camera->view[13] = -vec3_dot(yaxis, camera->location);
    camera->view[14] = -vec3_dot(zaxis, camera->location);
    camera->view[15] = 1.0f;
}

void
camera_view_matrix_rh_vulkan(Camera* const camera) NO_EXCEPT
{
    const v3_f32 zaxis = { -camera->front.x, -camera->front.y, -camera->front.z };

    v3_f32 xaxis = vec3_cross(zaxis, camera->world_up);
    vec3_normalize(&xaxis);

    const v3_f32 yaxis = vec3_cross(zaxis, xaxis);

   // We tested if it would make sense to create a vec3_dot_sse version for the 3 dot products
    // The result was that it is not faster, only if we would do 4 dot products would we see an improvement
    camera->view[0] = xaxis.x;
    camera->view[1] = yaxis.x;
    camera->view[2] = zaxis.x;
    //camera->view[3] = 0.0f;
    camera->view[4] = xaxis.y;
    camera->view[5] = yaxis.y;
    camera->view[6] = zaxis.y;
    //camera->view[7] = 0.0f;
    camera->view[8] = xaxis.z;
    camera->view[9] = yaxis.z;
    camera->view[10] = zaxis.z;
    //camera->view[11] = 0.0f;
    camera->view[12] = -vec3_dot(xaxis, camera->location);
    camera->view[13] = -vec3_dot(yaxis, camera->location);
    camera->view[14] = -vec3_dot(zaxis, camera->location);
    camera->view[15] = 1.0f;
}

inline
f32 camera_step_closer(GpuApiType type, f32 value) NO_EXCEPT
{
    // WARNING: The value depends on the near and far plane.
    // The reason for this is they will get smaller and smaller with increasing zfar values
    // until the difference effectively becomes 0 -> vertices occupy the same zindex -> zfighting
    // For safety reasons we calculate a rather generous offset.
    // @performance Maybe it makes sense in the future to just pick a small CONST epsilon value
    switch (type) {
        case GPU_API_TYPE_SOFTWARE:
        case GPU_API_TYPE_OPENGL:
            return value + (nextafterf(value, -INFINITY) - value) * 1000;
        case GPU_API_TYPE_VULKAN:
            return value + (nextafterf(value, -INFINITY) - value) * 1000;
        case GPU_API_TYPE_DIRECTX:
            return value + (nextafterf(value, -INFINITY) - value) * 1000;
        default:
            UNREACHABLE();
    }
}

inline
f32 camera_step_away(GpuApiType type, f32 value) NO_EXCEPT
{
    // WARNING: The value depends on the near and far plane.
    // The reason for this is they will get smaller and smaller with increasing zfar values
    // until the difference effectively becomes 0 -> vertices occupy the same zindex -> zfighting
    // For safety reasons we calculate a rather generous offset.
    // @performance Maybe it makes sense in the future to just pick a small CONST epsilon value
    switch (type) {
        case GPU_API_TYPE_SOFTWARE:
        case GPU_API_TYPE_OPENGL:
            return value + (nextafterf(value, INFINITY) - value) * 1000;
        case GPU_API_TYPE_VULKAN:
            return value + (nextafterf(value, INFINITY) - value) * 1000;
        case GPU_API_TYPE_DIRECTX:
            return value + (nextafterf(value, INFINITY) - value) * 1000;
        default:
            UNREACHABLE();
    }
}

inline
void camera_init(Camera* const camera) NO_EXCEPT
{
    camera->znear = 0.1f;
    camera->zfar = 10000.0f;

    switch (camera->gpu_api_type) {
        case GPU_API_TYPE_SOFTWARE: {
            camera_init_rh_opengl(camera);
            camera_projection_matrix_rh_opengl(camera);
            camera_orth_matrix_rh_software(camera);
            camera_view_matrix_rh_opengl(camera);
        } break;
        case GPU_API_TYPE_OPENGL: {
            camera_init_rh_opengl(camera);
            camera_projection_matrix_rh_opengl(camera);
            camera_orth_matrix_rh_opengl(camera);
            camera_view_matrix_rh_opengl(camera);
        } break;
        case GPU_API_TYPE_VULKAN: {
            camera_init_rh_vulkan(camera);
            camera_projection_matrix_rh_vulkan(camera);
            camera_orth_matrix_rh_vulkan(camera);
            camera_view_matrix_rh_vulkan(camera);
        } break;
        case GPU_API_TYPE_DIRECTX: {
            camera_init_lh(camera);
            camera_projection_matrix_lh(camera);
            camera_orth_matrix_lh(camera);
            camera_view_matrix_lh(camera);
        } break;
        default:
            UNREACHABLE();
    }
}

inline
bool aabb_intersects_frustum(const AABB_f32* const __restrict box, const Frustum* const __restrict frustum) NO_EXCEPT
{
    for(int32 i = 0; i < 6; ++i) {
        const v4_f32 eq = frustum->eq[i];
        const v3_f32 positive = {
            eq.x >= 0.0f ? box->max.x : box->min.x,
            eq.y >= 0.0f ? box->max.y : box->min.y,
            eq.z >= 0.0f ? box->max.z : box->min.z
        };

        if(eq.x * positive.x + eq.y * positive.y + eq.z * positive.z + eq.w < 0) {
            return false;
        }
    }

    return true;
}

inline
bool aabb_intersects_frustum(const AABB_int32* const __restrict box, const Frustum* const __restrict frustum) NO_EXCEPT
{
    for(int32 i = 0; i < 6; ++i) {
        const v4_f32 eq = frustum->eq[i];
        const v3_f32 positive = {
            (f32) (eq.x >= 0.0f ? box->max.x : box->min.x),
            (f32) (eq.y >= 0.0f ? box->max.y : box->min.y),
            (f32) (eq.z >= 0.0f ? box->max.z : box->min.z)
        };

        if(eq.x * positive.x + eq.y * positive.y + eq.z * positive.z + eq.w < 0) {
            return false;
        }
    }

    return true;
}

#if defined(OPENGL)
    #define camera_projection_matrix(camera) camera_projection_matrix_rh_opengl((camera))
    #define camera_orth_matrix(camera) camera_orth_matrix_rh_opengl((camera))
    #define camera_ui_matrix(camera) camera_ui_matrix_rh_opengl((camera))
    #define camera_view_matrix(camera) camera_view_matrix_rh_opengl((camera))
    #define camera_translation_matrix_sparse(camera, translation) camera_translation_matrix_sparse_rh((camera), (translation))
#elif defined(VULKAN)
    #define camera_projection_matrix(camera) camera_projection_matrix_rh_vulkan((camera))
    #define camera_orth_matrix(camera) camera_orth_matrix_rh_vulkan((camera))
    #define camera_ui_matrix(camera) camera_ui_matrix_rh_vulkan((camera))
    #define camera_view_matrix(camera) camera_view_matrix_rh_vulkan((camera))
    #define camera_translation_matrix_sparse(camera, translation) camera_translation_matrix_sparse_rh((camera), (translation))
#elif defined(DIRECTX)
    #define camera_projection_matrix(camera) camera_projection_matrix_lh((camera))
    #define camera_orth_matrix(camera) camera_orth_matrix_lh((camera))
    #define camera_ui_matrix(camera) camera_ui_matrix_lh((camera))
    #define camera_view_matrix(camera) camera_view_matrix_lh((camera))
    #define camera_translation_matrix_sparse(camera, translation) camera_translation_matrix_sparse_lh((camera), (translation))
#elif defined(SOFTWARE)
    #if _WIN32
        #define camera_projection_matrix(camera) camera_projection_matrix_rh_opengl((camera))
        #define camera_orth_matrix(camera) camera_orth_matrix_rh_software((camera))
        #define camera_ui_matrix(camera) camera_ui_matrix_rh_software((camera))
        #define camera_view_matrix(camera) camera_view_matrix_rh_opengl((camera))
        #define camera_translation_matrix_sparse(camera, translation) camera_translation_matrix_sparse_rh((camera), (translation))
    #else
        #define camera_projection_matrix(camera) camera_projection_matrix_rh_opengl((camera))
        #define camera_orth_matrix(camera) camera_orth_matrix_rh_software((camera))
        #define camera_ui_matrix(camera) camera_ui_matrix_rh_software((camera))
        #define camera_view_matrix(camera) camera_view_matrix_rh_opengl((camera))
        #define camera_translation_matrix_sparse(camera, translation) camera_translation_matrix_sparse_rh((camera), (translation))
    #endif
#endif

#endif