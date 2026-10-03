#pragma once
#ifndef COMS_SCENE_INFO_H
#define COMS_SCENE_INFO_H

#include "../stdlib/Stdlib.h"
#include "../ui/UILayout.h"
#include "../ui/UITheme.h"

struct SceneInfo {
    // This variable is only written to and read from by the update loop and potentially some helper worker threads
    // This helper worker threads however are "owned" by the update loop
    atomic<int32> scene_update_setup_state;

    // This variable is only written by the render loop and potentially some helper worker threads
    // This helper worker threads however are "owned" by the update loop
    // The reading however also happens from the update loop because the update loop needs to know if it can send
    // Scene specific data for the new scene
    atomic<int32> scene_render_setup_state;

    // Scene specific state
    int32 scene_state;

    // Keeps track of the setup iterations until it got finally setup
    uint32 scene_setup_counter;

    // This represents the actual Scene data e.g. Scene0, Scene1, ...
    // @todo Find a better way to handle this, the size of 8K is probably also not enough
    byte data[8192];
    UILayout ui_layout;

    // This is scene specific theme data
    // We also have global theme data, which is not defined in here
    UITheme ui_theme;
};

#endif