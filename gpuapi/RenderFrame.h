/**
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_GPUAPI_RENDER_FRAME_H
#define COMS_GPUAPI_RENDER_FRAME_H

#include "../stdlib/Types.h"
#include "../camera/Camera.h"
#include "../gpuapi/anti_aliasing/AntiAliasingType.h"

struct RenderSettings {
    uint32 fps_limit;
    uint32 ui_visibility_flags;
    AntiAliasingType anti_aliasing;
    int8 anti_aliasing_detail;
    byte color_deficiency;
    byte atlas_caching;

    // 3D scene resolution in percent of the window resolution (0/100 = native) @see render_scale_viewport()
    byte render_scale;

    // Swap interval (0 = off, 1 = on, -1 = adaptive if supported)
    int8 vsync;

    #if (defined(DEBUG) && DEBUG) || (defined(INTERNAL) && INTERNAL)
        uint32 debug_visibility_flags;
    #endif
};

struct RenderFrameUi {
    void* vertices;
    uint32* indices;

    uint32 vertex_capacity;
    uint32 index_capacity;
    uint32 vertex_count;
    uint32 index_count;
};

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

    byte data[3904];
};

/**
 * Resolution of the 3D scene (@see RenderSettings::render_scale)
 * 0 or >= 100 = native resolution, the scale is clamped to >= 25%
 */
FORCE_INLINE
bool render_scale_active(byte render_scale) NO_EXCEPT
{
    return render_scale && render_scale < 100;
}

FORCE_INLINE
v2_f32 render_scale_viewport(v2_f32 viewport, byte render_scale) NO_EXCEPT
{
    if (!render_scale_active(render_scale)) {
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