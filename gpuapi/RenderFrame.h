/**
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_GPUAPI_RENDER_FRAME_H
#define COMS_GPUAPI_RENDER_FRAME_H

#include "../stdlib/Types.h"
#include "../stdlib/GameMathTypes.h"

struct RenderFrame {
    // Flags which content type actually changed
    // @type SceneState
    uint64 change_flags;

    // When did we start to create this render frame
    uint64 started_at;

    // When did we finish creating this render frame and hand it off to the rendering loop
    uint64 handoff_at;

    byte scene_id;

    // Tells the render loop to setup a new scene
    byte scene_id_new;

    Camera camera;
};

#endif