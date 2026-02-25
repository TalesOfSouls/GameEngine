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

#include "../stdlib/Stdlib.h"
#include "../stdlib/GameMathTypes.h"
#include "../gpuapi/GpuApiType.h"

#define CAMERA_MAX_INPUTS 4
#define CAMERA_MIN_FOV  25.0f
#define CAMERA_MAX_FOV 100.0f

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

#endif