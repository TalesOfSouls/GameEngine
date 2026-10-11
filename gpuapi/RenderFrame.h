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
    uint32 debug_visibility_flags;
    uint32 ui_visibility_flags;
    AntiAliasingType anti_aliasing;
    int8 anti_aliasing_detail;
    byte color_deficiency;
    byte atlas_caching;
};

struct RenderFrameUi {
    void* vertices;
    uint32* indices;

    uint32 vertex_capacity;
    uint32 index_capacity;
    uint32 vertex_count;
    uint32 index_count;
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
    Camera camera;
    RenderSettings settings;
    RenderFrameUi ui;

    byte data[3904];
};

#endif