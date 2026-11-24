/**
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_GPUAPI_RENDER_FRAME_H
#define COMS_GPUAPI_RENDER_FRAME_H

#include "../stdlib/Types.h"
#include "../stdlib/Defines.h"
#include "../camera/Camera.h"
#include "../gpuapi/anti_aliasing/AntiAliasingType.h"

struct RenderSettings {
    uint32 fps_limit;
    uint32 ui_visibility_flags;
    AntiAliasingType anti_aliasing;
    int8 anti_aliasing_detail;
    byte color_deficiency;
    byte atlas_caching;

    // 3D scene resolution in percent of the window resolution (0/100 = native)
    // render_scale_viewport()
    byte render_scale;

    // Swap interval (0 = off, 1 = on, -1 = adaptive if supported)
    int8 vsync;

    // SIMD step size of the cpu (runtime check)
    uint8 simd_step_size;

    #if (defined(DEBUG) && DEBUG) || (defined(INTERNAL) && INTERNAL)
        uint32 debug_visibility_flags;
    #endif
};

// Changed vertex/index range of one ui version
// The render frames + gpu buffers can be several versions behind -> the last versions are kept
// and a copy only covers the ranges that changed since its version
#define RENDER_FRAME_UI_PATCH_HISTORY 8

struct RenderFrameUiPatch {
    // The ui version this patch created (index in the history = version % RENDER_FRAME_UI_PATCH_HISTORY)
    uint16 version;

    // true = everything changed (e.g. full ui re-build, other layout) -> full copy
    bool full;

    // Changed range [start, end), start >= end = nothing changed
    uint32 vertex_start;
    uint32 vertex_end;
    uint32 index_start;
    uint32 index_end;
};

struct RenderFrameUi {
    void* vertices;
    uint32* indices;

    uint32 vertex_capacity;
    uint32 index_capacity;
    uint32 vertex_count;
    uint32 index_count;

    // Patch history at the time of the copy, the render thread uses it for the gpu buffers
    RenderFrameUiPatch patches[RENDER_FRAME_UI_PATCH_HISTORY];
};

/**
 * Combines the patches of the versions (from, to] into one changed range
 *
 * @param const RenderFrameUiPatch* patches Patch history (RENDER_FRAME_UI_PATCH_HISTORY entries)
 * @param uint16                    from    Version of the data that is updated
 * @param uint16                    to      Version to update to
 *
 * @return bool false = a full copy is needed (too old, unknown version or a full change in between)
 */
inline
bool render_frame_ui_patch_range(
    const RenderFrameUiPatch* const __restrict patches,
    uint16 from, uint16 to,
    uint32* const __restrict vertex_start, uint32* const __restrict vertex_end,
    uint32* const __restrict index_start, uint32* const __restrict index_end
) NO_EXCEPT
{
    *vertex_start = 0xFFFFFFFF;
    *vertex_end = 0;
    *index_start = 0xFFFFFFFF;
    *index_end = 0;

    const uint16 count = (uint16) (to - from);
    if (count > RENDER_FRAME_UI_PATCH_HISTORY) {
        return false;
    }

    for (uint16 i = 1; i <= count; ++i) {
        const uint16 version = (uint16) (from + i);
        const RenderFrameUiPatch* const patch = &patches[version % RENDER_FRAME_UI_PATCH_HISTORY];
        if (patch->version != version || patch->full) {
            return false;
        }

        if (patch->vertex_start < patch->vertex_end) {
            *vertex_start = patch->vertex_start < *vertex_start ? patch->vertex_start : *vertex_start;
            *vertex_end = patch->vertex_end > *vertex_end ? patch->vertex_end : *vertex_end;
        }

        if (patch->index_start < patch->index_end) {
            *index_start = patch->index_start < *index_start ? patch->index_start : *index_start;
            *index_end = patch->index_end > *index_end ? patch->index_end : *index_end;
        }
    }

    if (*vertex_start >= *vertex_end) {
        *vertex_start = 0;
        *vertex_end = 0;
    }

    if (*index_start >= *index_end) {
        *index_start = 0;
        *index_end = 0;
    }

    return true;
}

// @todo Instead of passing these things as fixed part of the RenderFrame,
//      we might want to pass them as part of a queue element
union RenderFrameMessage {
    RenderFrameUi ui;
    RenderSettings settings;
};

/**
 * Created by the update loop and read by the render loop
 */
struct RenderFrame {
    // When did we start to create this render frame
    uint64 started_at;

    // When did we finish creating this render frame and hand it off to the rendering loop
    DEBUG_VAR_DEF(uint64, handoff_at);

    byte scene_id;

    // Tells the render loop to setup a new scene
    byte scene_id_new;

    uint16 settings_version;
    uint16 camera_version;
    uint16 ui_version;
    RenderSettings settings;
    Camera camera;
    RenderFrameUi ui;

    // Scene specific data to be passed from update thread to render thread
    // @todo consider to change to a queue or similar
    byte data[3904];
};

CONSTEXPR FORCE_INLINE
v2_f32 render_scale_viewport(v2_f32 viewport, byte render_scale) NO_EXCEPT
{
    if (!render_scale) {
        return viewport;
    }

    const f32 scale = (f32) (render_scale < 25 ? 25 : render_scale) / 100.0f;
    const int32 width = (int32) (viewport.width * scale);
    const int32 height = (int32) (viewport.height * scale);

    return {
        (f32) (width > 0 ? width : 1),
        (f32) (height > 0 ? height : 1)
    };
}

#endif